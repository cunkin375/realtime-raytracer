#include "GPU_Backend.hpp"

#include <memory>
#include <vulkan/vulkan.h>

#include <dxc/dxcapi.h>

#include "Util/Aliases.hpp"
#include "Util/Log.hpp"
#include "Util/Path.hpp"
#include "Walnut/Application.h"

namespace
{

struct DxcDeleter
{
    void operator()(IUnknown *pointer) const
    {
        if (pointer != nullptr)
            pointer->Release();
    }
};

void Check(VkResult result, std::source_location location = std::source_location::current())
{
    using namespace Log;
    if (result != VK_SUCCESS)
        Log::PrintAt<Level::Error>(location, "{} returned error ({})", location.function_name(),
                                   static_cast<i32>(result));
}
void Check(HRESULT result, std::source_location location = std::source_location::current())
{
    using namespace Log;
    if (result != S_OK)
        Log::PrintAt<Level::Error>(location, "{} returned error ({})", location.function_name(),
                                   static_cast<i32>(result));
}

enum class Binding : u32
{
    CameraSettingsUBO = 0,
    SpheresSSBO,
    MaterialsSSBO,
    AccumulationDataSSBO,
    OutputImageSSBO,
};

constexpr auto operator*(Binding b) noexcept { return std::to_underlying(b); }

} // namespace

GPU_Backend::GPU_Backend() : valid_state_{ false }
{
    device_ = Walnut::Application::GetDevice();

    // === Create descriptor set layout ===
    auto layout = std::array<::VkDescriptorSetLayoutBinding, 5>{};

    layout[*Binding::CameraSettingsUBO] =
        ::VkDescriptorSetLayoutBinding{ .binding = 0,
                                        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::SpheresSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding = 1,
                                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::MaterialsSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding = 2,
                                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::AccumulationDataSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding = 3,
                                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::OutputImageSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding = 4,
                                        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT };

    auto layout_info =
        ::VkDescriptorSetLayoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                           .bindingCount = static_cast<u32>(layout.size()),
                                           .pBindings = layout.data() };

    Check(::vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_));

    // === Create Pipeline Layout ===
    auto pipeline_layout_info = ::VkPipelineLayoutCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr,
    };

    Check(::vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_));

    // === Compile Shaders ===
    if (false == CompileShaders("RayTracer/assets/shaders/raytracer.hlsl"))
    {
        Log::Error("Shaders failed to compile, using CPU renderer as fallback.");
        return;
    }

    // === Create Compute Pipeline ===
    auto shader_stage_info = ::VkPipelineShaderStageCreateInfo{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader_module_,
        .pName = "main" // must match '-E main' in DXC
    };

    auto pipeline_info = ::VkComputePipelineCreateInfo{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shader_stage_info,
        .layout = pipeline_layout_,
    };

    Check(
        ::vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &compute_pipeline_));

    // NOTE: keep this at the bottom
    shader_watcher_ = std::make_unique<DirectoryWatcher>(
        Util::ResolvePath("RayTracer/assets/shaders/"),
        [this](const DirectoryWatcher::FileEvent &event) -> void { HotReloadShader(); });

    if (shader_watcher_ != nullptr)
        valid_state_ = true;
}

void Render() {}

bool GPU_Backend::CompileShaders(std::string_view shader_path)
{

    std::unique_ptr<IDxcUtils, DxcDeleter> utils;
    Check(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));

    IDxcCompiler3 *compiler;
    Check(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    auto file_data = Util::LoadAsBinary(shader_path);
    if (file_data.empty())
        return false;

    // Create a blob with encoding from the pinned memory
    IDxcBlobEncoding *source_blob;
    Check(utils->CreateBlobFromPinned(file_data.data(), static_cast<UINT32>(file_data.size()), CP_UTF8,
                                      &source_blob));

    // Fill the DxcBuffer safely from the blob
    auto source_buffer = ::DxcBuffer{
        .Ptr = source_blob->GetBufferPointer(),
        .Size = source_blob->GetBufferSize(),
        .Encoding = DXC_CP_UTF8,
    };

    std::vector<LPCWSTR> arguments = { L"-spirv", L"-T",   L"cs_6_5",
                                       L"-E",     L"main", L"-fspv-target-env=vulkan1.3" };

    std::unique_ptr<IDxcResult, DxcDeleter> result;
    compiler->Compile(&source_buffer, arguments.data(), static_cast<u32>(arguments.size()), nullptr,
                      IID_PPV_ARGS(&result));

    std::unique_ptr<IDxcBlobUtf8, DxcDeleter> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors != nullptr && errors->GetStringLength() != 0zu)
    {
        Log::Error("Compilation error in: {}", shader_path);
        std::cerr << errors->GetStringPointer() << "\n";
        return false;
    }

    std::unique_ptr<IDxcBlob, DxcDeleter> spriv_blob;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spriv_blob), nullptr);

    // NOTE: this might change as the GPU pipeline is optimized
    auto create_info =
        ::VkShaderModuleCreateInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = spriv_blob->GetBufferSize(),
                                    .pCode = reinterpret_cast<const u32 *>(spriv_blob->GetBufferPointer()) };
    Check(::vkCreateShaderModule(device_, &create_info, nullptr, &compute_shader_module_));
    return true;
}

void GPU_Backend::HotReloadShader() {}
