#include "GPU_Backend.hpp"
#include "Scene.hpp"

#include <memory>
#include <vulkan/vulkan.h>

// clang-format off
/**
 * Getting HLSL to compile across Windows and Linux is really annoying, so everything here MUST stay in the
 * exact order that it is in. INITGUID is needed for Windows to properly used the Windows Runtiem Template
 * Library, which provides smart pointers for COM used by the DirectX Compiler (DXC). Using C++ smart pointers
 * will not work and return errors when ID_PPV_ARGS expands to __uuidof(**(ppType)), ID_PPV_ARGS_Helper(ppvType).
*/
#define INITGUID
#ifdef _WIN32
#include <initguid.h>
#include <wrl/client.h>
#endif

/**
 * GUID definitions for DXC interfaces. MinGW does not define these.
*/
#ifdef __MINGW32__
namespace MinGWHelper
{
constexpr uint8_t Nybble(char c)
{
    return (c >= '0' && c <= '9')   ? static_cast<uint8_t>(c - '0')
           : (c >= 'a' && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10)
           : (c >= 'A' && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10)
                                    : 0;
}
constexpr uint8_t Byte(char c1, char c2) { return static_cast<uint8_t>((Nybble(c1) << 4) | Nybble(c2)); }
} // namespace MinGWHelper

#define CROSS_PLATFORM_UUIDOF(interface, spec)                                                               \
    struct interface;                                                                                        \
    template <>                                                                                              \
    inline const GUID &__mingw_uuidof<interface>()                                                           \
    {                                                                                                        \
        static constexpr GUID guid = {                                                                       \
            static_cast<uint32_t>(MinGWHelper::Byte(spec[0], spec[1])) << 24                                 \
                | static_cast<uint32_t>(MinGWHelper::Byte(spec[2], spec[3])) << 16                           \
                | static_cast<uint32_t>(MinGWHelper::Byte(spec[4], spec[5])) << 8                            \
                | MinGWHelper::Byte(spec[6], spec[7]),                                                       \
            static_cast<uint16_t>(static_cast<uint16_t>(MinGWHelper::Byte(spec[9], spec[10])) << 8           \
                                  | MinGWHelper::Byte(spec[11], spec[12])),                                  \
            static_cast<uint16_t>(static_cast<uint16_t>(MinGWHelper::Byte(spec[14], spec[15])) << 8          \
                                  | MinGWHelper::Byte(spec[16], spec[17])),                                  \
            { MinGWHelper::Byte(spec[19], spec[20]), MinGWHelper::Byte(spec[21], spec[22]),                  \
              MinGWHelper::Byte(spec[24], spec[25]), MinGWHelper::Byte(spec[26], spec[27]),                  \
              MinGWHelper::Byte(spec[28], spec[29]), MinGWHelper::Byte(spec[30], spec[31]),                  \
              MinGWHelper::Byte(spec[32], spec[33]), MinGWHelper::Byte(spec[34], spec[35]) }                 \
        };                                                                                                   \
        return guid;                                                                                         \
    }
#endif

#include <dxc/dxcapi.h>
#undef INITGUID
// clang-format on

#include "Util/Aliases.hpp"
#include "Util/Log.hpp"
#include "Util/Path.hpp"
#include "Walnut/Application.h"

namespace
{

// These are COM smart pointers that work across Windows and Linux
#ifdef _WIN32
template <typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
#else
template <typename T>
using ComPtr = CComPtr<T>;
#endif

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

using Begin = bool;

} // namespace

GPU_Backend::GPU_Backend() : valid_state_{ false }
{
    device_ = Walnut::Application::GetDevice();

    /* Create descriptor set layout */
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

    /* Compile Shaders */
    if (false == CompileShaders("RayTracer/assets/shaders/raytracer.hlsl"))
    {
        Log::Error("Shaders failed to compile, using CPU renderer as fallback.");
        return;
    }

    /* Create Compute Pipeline */
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

    auto pool_sizes = std::array<::VkDescriptorPoolSize, 2>{
        ::VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        ::VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4 },
    };

    auto pool_info = ::VkDescriptorPoolCreateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    Check(::vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_));

    auto allocation_info = ::VkDescriptorSetAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptor_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts = &descriptor_set_layout_,
    };
    Check(::vkAllocateDescriptorSets(device_, &allocation_info, &descriptor_set_));

    // NOTE: keep this at the bottom
    shader_watcher_ = std::make_unique<DirectoryWatcher>(
        Util::ResolvePath("RayTracer/assets/shaders/"),
        [this](const DirectoryWatcher::FileEvent &event) -> void { HotReloadShader(); });

    if (shader_watcher_ != nullptr)
        valid_state_ = true;
}

void GPU_Backend::Render(const Camera &camera, const Scene &scene, u32 *image_data,
                         fVector4 *accumulation_data)
{
    ResizeBuffersIfNeeded(config_.image_width, config_.image_height, scene);

    {
        const auto &position = camera.GetPosition();
        const auto &direction = camera.GetDirection();
        const auto &background = scene.background;

        GPU_MetaData meta{};
        meta.camera_position[0] = position.x;
        meta.camera_position[1] = position.y;
        meta.camera_position[2] = position.z;
        meta.ray_direction[0] = direction.x;
        meta.ray_direction[1] = direction.y;
        meta.ray_direction[2] = direction.z;
        meta.background[0] = background.x;
        meta.background[1] = background.y;
        meta.background[2] = background.z;
        meta.image_width = static_cast<f32>(config_.image_width);
        meta.frame_index = static_cast<f32>(config_.frame_index);
        meta.num_spheres = static_cast<u32>(scene.spheres.size());

        void *pointer;
        ::vkMapMemory(device_, ubo_camera_.memory, 0, sizeof(GPU_MetaData), 0, &pointer);
        std::memcpy(pointer, &meta, sizeof(GPU_MetaData));
        ::vkUnmapMemory(device_, ubo_camera_.memory);
    }

    /* Upload Sphere Data */
    {
        void *pointer;
        ::vkMapMemory(device_, ssbo_spheres_.memory, 0, ssbo_spheres_.size, 0, &pointer);
        std::memcpy(pointer, scene.spheres.data(), ssbo_spheres_.size);
        ::vkUnmapMemory(device_, ssbo_spheres_.memory);
    }

    /* Upload Material Data */
    {
        void *pointer;
        ::vkMapMemory(device_, ssbo_materials_.memory, 0, ssbo_materials_.size, 0, &pointer);
        std::memcpy(pointer, scene.materials.data(), ssbo_spheres_.size);
        ::vkUnmapMemory(device_, ssbo_materials_.memory);
    }

    /* Upload Accumulation Buffer Data */
    {
        // synch host copy and GPU before dispatch
        const VkDeviceSize accumulation_bytes = sizeof(fVector4) * config_.image_width * config_.image_height;
        void *pointer;
        ::vkMapMemory(device_, ssbo_accumulation_.memory, 0, accumulation_bytes, 0, &pointer);
        std::memcpy(pointer, accumulation_data, accumulation_bytes);
        ::vkUnmapMemory(device_, ssbo_accumulation_.memory);
    }

    /* Record and submit compuote command buffer */

    ::VkCommandBuffer command = Walnut::Application::GetCommandBuffer(Begin{ true });

    ::vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);
    ::vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1,
                              &descriptor_set_, 0, nullptr);

    // must match
    const u32 thread_groups_x = (config_.image_width + 7) / 8;
    const u32 thread_groups_y = (config_.image_height + 7) / 8;
    ::vkCmdDispatch(command, thread_groups_x, thread_groups_y, 1);

    // end, submit, and sit on fence
    Walnut::Application::FlushCommandBuffer(command);

    // TODO: Code below should be removed once the GPU pipeline is better established

    // read image_data back for SetData()
    {
        const VkDeviceSize image_bytes = sizeof(u32) * config_.image_width * config_.image_height;
        void *pointer;

        ::vkMapMemory(device_, ssbo_image_.memory, 0, image_bytes, 0, &pointer);
        std::memcpy(image_data, pointer, image_bytes);
        ::vkUnmapMemory(device_, ssbo_image_.memory);
    }

    // read accumulation_data back to reset when frame_index = 1
    {
        const VkDeviceSize accumulation_bytes =
            sizeof(fVector4) * config_.image_width * config_.image_height;
        void *pointer;

        ::vkMapMemory(device_, ssbo_accumulation_.memory, 0, accumulation_bytes, 0, &pointer);
        std::memcpy(accumulation_data, pointer, accumulation_bytes);
        ::vkUnmapMemory(device_, ssbo_accumulation_.memory);
    }
}

// Private methods

bool GPU_Backend::CompileShaders(std::string_view shader_path)
{
    ComPtr<IDxcUtils> utils;
    Check(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));

    ComPtr<IDxcCompiler3> compiler;
    Check(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    auto file_data = Util::LoadAsBinary(shader_path);
    if (file_data.empty())
        return false;

    // Create a blob with encoding from the pinned memory
    ComPtr<IDxcBlobEncoding> source_blob;
    Check(utils->CreateBlobFromPinned(file_data.data(), static_cast<UINT32>(file_data.size()), DXC_CP_UTF8,
                                      &source_blob));

    // Fill the DxcBuffer safely from the blob
    auto source_buffer = ::DxcBuffer{
        .Ptr = source_blob->GetBufferPointer(),
        .Size = source_blob->GetBufferSize(),
        .Encoding = DXC_CP_UTF8,
    };

    // Ccalar layout resolves a lot of alignment issues
    // WARN: I am not dealing with manual memory alignment. It is stupid. If it causes problems migrate to
    // Slang.
    std::vector<LPCWSTR> arguments = {
        L"-spirv", L"-T", L"cs_6_5", L"-E", L"main", L"-fspv-target-env=vulkan1.3", L"-fvk-use-scalar-layout"
    };

    ComPtr<IDxcResult> result;
    compiler->Compile(&source_buffer, arguments.data(), static_cast<u32>(arguments.size()), nullptr,
                      IID_PPV_ARGS(&result));

    ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors != nullptr && errors->GetStringLength() != 0zu)
    {
        Log::Error("Compilation error in: {}", shader_path);
        std::cerr << errors->GetStringPointer() << "\n";
        return false;
    }

    ComPtr<IDxcBlob> spriv_blob;
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

u32 GPU_Backend::FindMemoryType(u32 type_filter, ::VkMemoryPropertyFlags properties)
{
    ::VkPhysicalDeviceMemoryProperties memory_properties;
    ::vkGetPhysicalDeviceMemoryProperties(Walnut::Application::GetPhysicalDevice(), &memory_properties);

    for (auto i{ 0u }; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1u << i))
            && (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    Log::Error("FindMemoryType: No suitable memory type found");
    return 0;
}

// allocates memory for a specific buffer
GPU_Backend::GPU_Buffer GPU_Backend::AllocateBuffer(::VkDeviceSize size, ::VkBufferUsageFlags usage_flags,
                                                    ::VkMemoryPropertyFlags memory_properties)
{
    GPU_Buffer buffer;
    buffer.size = size;
    auto buffer_info = ::VkBufferCreateInfo{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    Check(::vkCreateBuffer(device_, &buffer_info, nullptr, &buffer.handle));

    ::VkMemoryRequirements requirements;
    ::vkGetBufferMemoryRequirements(device_, buffer.handle, &requirements);

    auto allocate_info = ::VkMemoryAllocateInfo{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = requirements.size,
        .memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, memory_properties),
    };
    Check(::vkAllocateMemory(device_, &allocate_info, nullptr, &buffer.memory));
    Check(::vkBindBufferMemory(device_, buffer.handle, buffer.memory, 0));

    return buffer;
}

// Reallocates memory for buffers
// - currently written to handle small buffers
void GPU_Backend::ResizeBuffersIfNeeded(u32 width, u32 height, const Scene &scene)
{
    const u32 pixel_count = width * height;
    if (pixel_count == current_pixel_count_)
        return;

    auto destroy = [&](GPU_Buffer &buffer)
    {
        if (buffer.handle)
            ::vkDestroyBuffer(device_, buffer.handle, nullptr);
        if (buffer.memory)
            ::vkFreeMemory(device_, buffer.memory, nullptr);
        buffer = {};
    };
    destroy(ubo_camera_);
    destroy(ssbo_spheres_);
    destroy(ssbo_materials_);
    destroy(ssbo_accumulation_);
    destroy(ssbo_image_);

    // Map memory without manual cache flushing
    // NOTE: this might cause performance issues on hardware without shared RAM or BAR window, and if used
    // with very large buffers
    constexpr auto HOST_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    ubo_camera_ = AllocateBuffer(sizeof(GPU_MetaData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HOST_VISIBLE);
    ssbo_spheres_ = AllocateBuffer(sizeof(Sphere) * scene.spheres.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   HOST_VISIBLE);
    ssbo_materials_ = AllocateBuffer(sizeof(Material), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HOST_VISIBLE);
    ssbo_accumulation_ = AllocateBuffer(sizeof(fVector4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HOST_VISIBLE);
    ssbo_image_ = AllocateBuffer(sizeof(Material), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HOST_VISIBLE);

    current_pixel_count_ = pixel_count;

    // rebind new VkBufferse
    WriteDescriptorSet();
}

// Rebinds buffers to descriptor set
void GPU_Backend::WriteDescriptorSet()
{
    auto ubo_info = ::VkDescriptorBufferInfo{ ubo_camera_.handle, 0, ubo_camera_.size };
    auto spheres_info = ::VkDescriptorBufferInfo{ ssbo_spheres_.handle, 0, ssbo_spheres_.size };
    auto materials_info = ::VkDescriptorBufferInfo{ ssbo_materials_.handle, 0, ssbo_materials_.size };
    auto accumulation_info = ::VkDescriptorBufferInfo{ ssbo_accumulation_.handle, 0, ssbo_materials_.size };
    auto image_info = ::VkDescriptorBufferInfo{ ssbo_image_.handle, 0, ssbo_image_.size };

    auto writes = std::array<::VkWriteDescriptorSet, 5>{
        ::VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = descriptor_set_,
                                .dstBinding = 0,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                .pBufferInfo = &ubo_info },
        ::VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = descriptor_set_,
                                .dstBinding = 1,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                .pBufferInfo = &spheres_info },
        ::VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = descriptor_set_,
                                .dstBinding = 2,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                .pBufferInfo = &materials_info },
        ::VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = descriptor_set_,
                                .dstBinding = 3,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                .pBufferInfo = &accumulation_info },
        ::VkWriteDescriptorSet{ .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                                .dstSet = descriptor_set_,
                                .dstBinding = 4,
                                .descriptorCount = 1,
                                .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                .pBufferInfo = &image_info },
    };
    ::vkUpdateDescriptorSets(device_, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
}
