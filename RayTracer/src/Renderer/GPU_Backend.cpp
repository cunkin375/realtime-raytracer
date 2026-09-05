#include "GPU_Backend.hpp"
#include "Scene.hpp"

#include <atomic>
#include <backends/imgui_impl_vulkan.h>
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
    OutputImageBuffer
};
constexpr auto operator*(Binding b) noexcept { return std::to_underlying(b); }

using Begin = bool;

} // namespace

// --- Public Methods ---------------------------------------------------------------------------------------

GPU_Backend::GPU_Backend() : valid_state_{ false }
{
    device_ = Walnut::Application::GetDevice();

    /* Create descriptor set layout */
    auto layout = std::array<::VkDescriptorSetLayoutBinding, 5>{};

    layout[*Binding::CameraSettingsUBO] =
        ::VkDescriptorSetLayoutBinding{ .binding         = 0,
                                        .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::SpheresSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding         = 1,
                                        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::MaterialsSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding         = 2,
                                        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::AccumulationDataSSBO] =
        ::VkDescriptorSetLayoutBinding{ .binding         = 3,
                                        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                        .descriptorCount = 1,
                                        .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT };

    layout[*Binding::OutputImageBuffer] =
        ::VkDescriptorSetLayoutBinding{ .binding         = 4,
                                        .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                                        .descriptorCount = 1,
                                        .stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT };

    auto layout_info =
        ::VkDescriptorSetLayoutCreateInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                           .bindingCount = static_cast<u32>(layout.size()),
                                           .pBindings    = layout.data() };

    Check(::vkCreateDescriptorSetLayout(device_, &layout_info, nullptr, &descriptor_set_layout_));

    // Create pipeline layout
    auto pipeline_layout_info = ::VkPipelineLayoutCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = 1,
        .pSetLayouts            = &descriptor_set_layout_,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };

    Check(::vkCreatePipelineLayout(device_, &pipeline_layout_info, nullptr, &pipeline_layout_));

    /* Compile Shaders */
    if (false == CompileShaders("RayTracer/assets/shaders/raytracer.hlsl"))
    {
        Log::Error("Shaders failed to compile.");
        return;
    }

    /* Create Compute Pipeline */
    auto shader_stage_info = ::VkPipelineShaderStageCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader_module_,
        .pName  = "main" // must match '-E main' in DXC
    };

    auto pipeline_info = ::VkComputePipelineCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage  = shader_stage_info,
        .layout = pipeline_layout_,
    };

    Check(
        ::vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &compute_pipeline_));

    auto pool_sizes = std::array{
        ::VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1 },
        ::VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3 },
        ::VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1 },
    };

    auto pool_info = ::VkDescriptorPoolCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets       = 1,
        .poolSizeCount = static_cast<u32>(pool_sizes.size()),
        .pPoolSizes    = pool_sizes.data(),
    };
    Check(::vkCreateDescriptorPool(device_, &pool_info, nullptr, &descriptor_pool_));

    auto allocation_info = ::VkDescriptorSetAllocateInfo{
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptor_pool_,
        .descriptorSetCount = 1,
        .pSetLayouts        = &descriptor_set_layout_,
    };
    Check(::vkAllocateDescriptorSets(device_, &allocation_info, &descriptor_set_));

    // NOTE: keep this at the bottom
    shader_watcher_ = std::make_unique<DirectoryWatcher>(
        Util::ResolvePath("RayTracer/assets/shaders/"),
        [this](const DirectoryWatcher::FileEvent &event) -> void { pending_reload_ = true; });
    if (shader_watcher_ != nullptr)
    {
        valid_state_ = true;
        shader_watcher_->SetEnabled(true);
        Log::Info("Shader watcher intialized.");
    }
}

GPU_Backend::~GPU_Backend()
{
    // wait for descriptor set to be done before destroying buffers and image
    ::vkDeviceWaitIdle(device_);

    DestroyBuffer(ubo_meta_);
    DestroyBuffer(ssbo_spheres_);
    DestroyBuffer(ssbo_materials_);
    DestroyBuffer(ssbo_accumulation_);

    if (shared_sampler_)
        ::vkDestroySampler(device_, shared_sampler_, nullptr);
    if (shared_image_view_)
        ::vkDestroyImageView(device_, shared_image_view_, nullptr);
    if (shared_image_)
        ::vkDestroyImage(device_, shared_image_, nullptr);
    if (shared_image_memory_)
        ::vkFreeMemory(device_, shared_image_memory_, nullptr);

    if (descriptor_pool_)
        ::vkDestroyDescriptorPool(device_, descriptor_pool_, nullptr);
    if (descriptor_set_layout_)
        ::vkDestroyDescriptorSetLayout(device_, descriptor_set_layout_, nullptr);
    if (pipeline_layout_)
        ::vkDestroyPipelineLayout(device_, pipeline_layout_, nullptr);
    if (compute_pipeline_)
        ::vkDestroyPipeline(device_, compute_pipeline_, nullptr);
    if (compute_shader_module_)
        ::vkDestroyShaderModule(device_, compute_shader_module_, nullptr);
}

void GPU_Backend::SetImageParameters(u32 width, u32 height, u32 frame_index)
{
    config_ = Settings{
        .image_width  = width,
        .image_height = height,
        .frame_index  = frame_index,
    };
}

void GPU_Backend::Render(const Camera &camera, const Scene &scene)
{
    PollShaderChanges();
    if (!valid_state_)
        return;
    ResizeImageBuffersIfNeeded(config_.image_width, config_.image_height);
    ResizeObjectBuffersIfNeeded(scene);

    /* Upload Metadata */
    {
        const auto &position                  = camera.GetPosition();
        const auto &background                = scene.background;
        const auto &camera_inverse_view       = camera.GetInverseView();
        const auto &camera_inverse_projection = camera.GetInverseProjection();

        GPU_MetaData meta{};
        meta.camera_position[0]        = position.x;
        meta.camera_position[1]        = position.y;
        meta.camera_position[2]        = position.z;
        meta._pad0                     = 0.f;
        meta.camera_inverse_view       = camera_inverse_view;
        meta.camera_inverse_projection = camera_inverse_projection;
        meta.background[0]             = background.x;
        meta.background[1]             = background.y;
        meta.background[2]             = background.z;
        meta.image_width               = static_cast<f32>(config_.image_width);
        meta.image_height              = static_cast<f32>(config_.image_height);
        meta.frame_index               = static_cast<f32>(config_.frame_index);
        meta.num_spheres               = static_cast<u32>(scene.spheres.size());

        void *pointer;
        ::vkMapMemory(device_, ubo_meta_.memory, 0, sizeof(GPU_MetaData), 0, &pointer);
        std::memcpy(pointer, &meta, sizeof(GPU_MetaData));
        ::vkUnmapMemory(device_, ubo_meta_.memory);
    }

    /* Upload Sphere Data */
    {
        void              *pointer;
        const VkDeviceSize copy_size = sizeof(Sphere) * scene.spheres.size();
        ::vkMapMemory(device_, ssbo_spheres_.memory, 0, ssbo_spheres_.size, 0, &pointer);
        std::memcpy(pointer, scene.spheres.data(), copy_size);
        ::vkUnmapMemory(device_, ssbo_spheres_.memory);
    }

    /* Upload Material Data */
    {
        void *pointer;
        ::vkMapMemory(device_, ssbo_materials_.memory, 0, ssbo_materials_.size, 0, &pointer);
        std::memcpy(pointer, scene.materials.data(), ssbo_materials_.size);
        ::vkUnmapMemory(device_, ssbo_materials_.memory);
    }

    /* Record and submit compute command buffer */

    ::VkCommandBuffer command = Walnut::Application::GetCommandBuffer(Begin{ true });

    // transition shared image for compute shader write
    auto pre_image_barrier = ::VkImageMemoryBarrier2{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
        .srcStageMask     = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
        .srcAccessMask    = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
        .dstStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
        .dstAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
        .oldLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .newLayout        = VK_IMAGE_LAYOUT_GENERAL,
        .image            = shared_image_,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };

    auto buffer_barriers = std::array{
        // SpheresSSBO
        ::VkBufferMemoryBarrier2{
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_HOST_BIT,
            .srcAccessMask       = VK_ACCESS_2_HOST_WRITE_BIT,
            .dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = ssbo_spheres_.handle,
            .offset              = 0,
            .size                = ssbo_spheres_.size,
        },
        // MaterialsSSBO
        ::VkBufferMemoryBarrier2{
            .sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask        = VK_PIPELINE_STAGE_2_HOST_BIT,
            .srcAccessMask       = VK_ACCESS_2_HOST_WRITE_BIT,
            .dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            .dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .buffer              = ssbo_materials_.handle,
            .offset              = 0,
            .size                = ssbo_materials_.size,
        },
    };

    auto pre_dependency = ::VkDependencyInfo{
        .sType                    = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = static_cast<u32>(buffer_barriers.size()),
        .pBufferMemoryBarriers    = buffer_barriers.data(),
        .imageMemoryBarrierCount  = 1,
        .pImageMemoryBarriers     = &pre_image_barrier,
    };
    ::vkCmdPipelineBarrier2(command, &pre_dependency);

    if (config_.frame_index == 1)
        ::vkCmdFillBuffer(command, ssbo_accumulation_.handle, 0, ssbo_accumulation_.size, 0);

    ::vkCmdBindPipeline(command, VK_PIPELINE_BIND_POINT_COMPUTE, compute_pipeline_);
    ::vkCmdBindDescriptorSets(command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout_, 0, 1,
                              &descriptor_set_, 0, nullptr);

    // must match 8x8 dispatch
    const u32 thread_groups_x = (config_.image_width + 7) / 8;
    const u32 thread_groups_y = (config_.image_height + 7) / 8;
    ::vkCmdDispatch(command, thread_groups_x, thread_groups_y, 1);

    // The image has been written to, transition shared image for ImGui read
    auto post_barrier =
        ::VkImageMemoryBarrier2{ .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
                                 .srcStageMask     = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
                                 .srcAccessMask    = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
                                 .dstStageMask     = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                                 .dstAccessMask    = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
                                 .oldLayout        = VK_IMAGE_LAYOUT_GENERAL,
                                 .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 .image            = shared_image_,
                                 .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 } };

    auto post_dependency = ::VkDependencyInfo{
        .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers    = &post_barrier,
    };
    ::vkCmdPipelineBarrier2(command, &post_dependency);

    // end, submit, and sit on fence
    Walnut::Application::FlushCommandBuffer(command);
}

// --- Private Methods --------------------------------------------------------------------------------------

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
        .Ptr      = source_blob->GetBufferPointer(),
        .Size     = source_blob->GetBufferSize(),
        .Encoding = DXC_CP_UTF8,
    };

    std::vector<LPCWSTR> arguments = {
        L"-spirv", L"-T", L"cs_6_5", L"-E", L"main", L"-fspv-target-env=vulkan1.3",
        /* debug flags */
        // L"-fspv-debug=vulkan-with-source",
        // L"-Zi",
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
        ::VkShaderModuleCreateInfo{ .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = spriv_blob->GetBufferSize(),
                                    .pCode = reinterpret_cast<const u32 *>(spriv_blob->GetBufferPointer()) };

    Check(::vkCreateShaderModule(device_, &create_info, nullptr, &compute_shader_module_));
    return true;
}

void GPU_Backend::HotReloadShader()
{
    Log::Info("Hot-reloading shader.");

    // wait for GPU to finish work using old pipeline before replacing
    ::vkDeviceWaitIdle(device_);

    VkShaderModule old_module   = compute_shader_module_;
    VkPipeline     old_pipeline = compute_pipeline_;

    compute_shader_module_ = VK_NULL_HANDLE;

    if (false == CompileShaders("RayTracer/assets/shaders/raytracer.hlsl"))
    {
        Log::Error("Shader hot-reload failed, reverting to old pipeline.");
        compute_shader_module_ = old_module;
    }

    // create pipeline with new module
    auto shader_stage_info = ::VkPipelineShaderStageCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage  = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader_module_,
        .pName  = "main",
    };

    auto pipeline_info = ::VkComputePipelineCreateInfo{
        .sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage  = shader_stage_info,
        .layout = pipeline_layout_,
    };

    VkPipeline new_pipeline{ VK_NULL_HANDLE };
    VkResult   result =
        ::vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &new_pipeline);

    if (result != VK_SUCCESS)
    {
        Log::Error("Pipeline creation failed during hot reload, reverting to old pipeline.");
        ::vkDestroyShaderModule(device_, compute_shader_module_, nullptr);
        compute_shader_module_ = old_module;
        return;
    }

    // swap new pipeline and destroy old resources
    compute_pipeline_ = new_pipeline;

    ::vkDestroyPipeline(device_, old_pipeline, nullptr);
    ::vkDestroyShaderModule(device_, old_module, nullptr);

    Log::Info("Hot-reloaded shader.");
}

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

// Allocates memory for a specific buffer
GPU_Backend::GPU_Buffer GPU_Backend::AllocateBuffer(::VkDeviceSize size, ::VkBufferUsageFlags usage_flags,
                                                    ::VkMemoryPropertyFlags memory_properties)
{
    auto buffer      = GPU_Buffer{ .size = size };
    auto buffer_info = ::VkBufferCreateInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,
        .usage       = usage_flags,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    Check(::vkCreateBuffer(device_, &buffer_info, nullptr, &buffer.handle));

    ::VkMemoryRequirements requirements;
    ::vkGetBufferMemoryRequirements(device_, buffer.handle, &requirements);

    auto allocate_info = ::VkMemoryAllocateInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = requirements.size,
        .memoryTypeIndex = FindMemoryType(requirements.memoryTypeBits, memory_properties),
    };
    Check(::vkAllocateMemory(device_, &allocate_info, nullptr, &buffer.memory));
    Check(::vkBindBufferMemory(device_, buffer.handle, buffer.memory, 0));

    return buffer;
}

// Reallocates memory for buffers
void GPU_Backend::ResizeImageBuffersIfNeeded(u32 width, u32 height)
{
    const u32 pixel_count = width * height;
    if (pixel_count == current_pixel_count_)
        return;

    ::vkDeviceWaitIdle(device_);

    // Destroy old shared image resources
    // this must happen before destroying buffer information
    if (shared_image_view_)
        ::vkDestroyImageView(device_, shared_image_view_, nullptr);
    if (shared_image_)
        ::vkDestroyImage(device_, shared_image_, nullptr);
    if (shared_image_memory_)
        ::vkFreeMemory(device_, shared_image_memory_, nullptr);
    if (shared_sampler_)
        ::vkDestroySampler(device_, shared_sampler_, nullptr);
    shared_image_        = VK_NULL_HANDLE;
    shared_image_view_   = VK_NULL_HANDLE;
    shared_image_memory_ = VK_NULL_HANDLE;
    shared_sampler_      = VK_NULL_HANDLE;

    DestroyBuffer(ssbo_accumulation_);

    // Create image (STORAGE for compute write, SAMPLED for ImGui fragment read)
    auto image_info = ::VkImageCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .format        = VK_FORMAT_R32G32B32A32_SFLOAT,
        .extent        = { width, height, 1 },
        .mipLevels     = 1,
        .arrayLayers   = 1,
        .samples       = VK_SAMPLE_COUNT_1_BIT,
        .tiling        = VK_IMAGE_TILING_OPTIMAL,
        .usage         = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    };
    Check(::vkCreateImage(device_, &image_info, nullptr, &shared_image_));

    // Allocate and bind DEVICE_LOCAL memory for the image
    ::VkMemoryRequirements image_mem_requirements;
    ::vkGetImageMemoryRequirements(device_, shared_image_, &image_mem_requirements);
    auto image_alloc_info = ::VkMemoryAllocateInfo{
        .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = image_mem_requirements.size,
        .memoryTypeIndex =
            FindMemoryType(image_mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
    };
    Check(::vkAllocateMemory(device_, &image_alloc_info, nullptr, &shared_image_memory_));
    Check(::vkBindImageMemory(device_, shared_image_, shared_image_memory_, 0));

    // Create ImageView
    auto view_info = ::VkImageViewCreateInfo{
        .sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image            = shared_image_,
        .viewType         = VK_IMAGE_VIEW_TYPE_2D,
        .format           = VK_FORMAT_R32G32B32A32_SFLOAT,
        .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
    };
    Check(::vkCreateImageView(device_, &view_info, nullptr, &shared_image_view_));

    // Create Sampler
    auto sampler_info = ::VkSamplerCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter     = VK_FILTER_LINEAR,
        .minFilter     = VK_FILTER_LINEAR,
        .mipmapMode    = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeV  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .addressModeW  = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
        .maxAnisotropy = 1.0f,
    };
    Check(::vkCreateSampler(device_, &sampler_info, nullptr, &shared_sampler_));

    // Transition image from UNDEFINED to SHADER_READ_ONLY_OPTIMAL
    // This is the initial state for ImGui sampling
    // The compute pre-barrier will transition it to GENERAL before each dispatch
    {
        auto command      = Walnut::Application::GetCommandBuffer(Begin{ true });
        auto init_barrier = ::VkImageMemoryBarrier2{
            .sType            = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask     = VK_PIPELINE_STAGE_2_NONE,
            .srcAccessMask    = VK_ACCESS_2_NONE,
            .dstStageMask     = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            .dstAccessMask    = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            .oldLayout        = VK_IMAGE_LAYOUT_UNDEFINED,
            .newLayout        = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .image            = shared_image_,
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 },
        };
        auto init_dep = ::VkDependencyInfo{
            .sType                   = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers    = &init_barrier,
        };
        ::vkCmdPipelineBarrier2(command, &init_dep);
        Walnut::Application::FlushCommandBuffer(command);
    }

    // Register with ImGui for sampling
    imgui_descriptor_ = ImGui_ImplVulkan_AddTexture(shared_sampler_, shared_image_view_,
                                                    VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Allocate device local buffers
    ssbo_accumulation_ = AllocateBuffer(sizeof(fVector4) * pixel_count,
                                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    current_pixel_count_ = pixel_count;

    // rebind new VkBuffers
    WriteDescriptorSet();
}

void GPU_Backend::ResizeObjectBuffersIfNeeded(const Scene &scene)
{
    ::vkDeviceWaitIdle(device_);

    const VkDeviceSize new_spheres_size   = sizeof(Sphere) * scene.spheres.size();
    const VkDeviceSize new_materials_size = sizeof(Material) * scene.materials.size();

    if (new_spheres_size <= ssbo_spheres_.size || new_materials_size <= ssbo_materials_.size)
        return;

    DestroyBuffer(ubo_meta_);
    DestroyBuffer(ssbo_spheres_);
    DestroyBuffer(ssbo_materials_);

    // Map memory without manual cache flushing
    // NOTE: this might cause performance issues on hardware without shared RAM or BAR window, and if used
    // with very large buffers
    constexpr auto HOST_VISIBLE = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    // Allocate host visible buffers
    ubo_meta_     = AllocateBuffer(sizeof(GPU_MetaData), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, HOST_VISIBLE);
    ssbo_spheres_ = AllocateBuffer(sizeof(Sphere) * scene.spheres.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                                   HOST_VISIBLE);
    ssbo_materials_ = AllocateBuffer(sizeof(Material) * scene.materials.size(),
                                     VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, HOST_VISIBLE);
}

// Rebinds buffers to descriptor set
void GPU_Backend::WriteDescriptorSet()
{
    // clang-format off
    auto ubo_info          = VkDescriptorBufferInfo{ .buffer = ubo_meta_.handle         , .offset = 0, .range = ubo_meta_.size };
    auto spheres_info      = VkDescriptorBufferInfo{ .buffer = ssbo_spheres_.handle     , .offset = 0, .range = ssbo_spheres_.size };
    auto materials_info    = VkDescriptorBufferInfo{ .buffer = ssbo_materials_.handle   , .offset = 0, .range = ssbo_materials_.size };
    auto accumulation_info = VkDescriptorBufferInfo{ .buffer = ssbo_accumulation_.handle, .offset = 0, .range = ssbo_accumulation_.size };
    // clang-format on

    auto output_image_info = VkDescriptorImageInfo{ .sampler     = VK_NULL_HANDLE,
                                                    .imageView   = shared_image_view_,
                                                    .imageLayout = VK_IMAGE_LAYOUT_GENERAL };

    auto writes = std::array{
        VkWriteDescriptorSet{ .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                              .dstSet          = descriptor_set_,
                              .dstBinding      = 0,
                              .descriptorCount = 1,
                              .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                              .pBufferInfo     = &ubo_info },
        VkWriteDescriptorSet{ .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                              .dstSet          = descriptor_set_,
                              .dstBinding      = 1,
                              .descriptorCount = 1,
                              .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                              .pBufferInfo     = &spheres_info },
        VkWriteDescriptorSet{ .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                              .dstSet          = descriptor_set_,
                              .dstBinding      = 2,
                              .descriptorCount = 1,
                              .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                              .pBufferInfo     = &materials_info },
        VkWriteDescriptorSet{ .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                              .dstSet          = descriptor_set_,
                              .dstBinding      = 3,
                              .descriptorCount = 1,
                              .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                              .pBufferInfo     = &accumulation_info },
        VkWriteDescriptorSet{ .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                              .dstSet          = descriptor_set_,
                              .dstBinding      = 4,
                              .descriptorCount = 1,
                              .descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
                              .pImageInfo      = &output_image_info },
    };
    ::vkUpdateDescriptorSets(device_, static_cast<u32>(writes.size()), writes.data(), 0, nullptr);
}

void GPU_Backend::PollShaderChanges()
{
    shader_watcher_->PollEvents();
    if (pending_reload_.exchange(false))
        HotReloadShader();
}

void GPU_Backend::DestroyBuffer(GPU_Buffer &buffer)
{
    if (buffer.handle)
        ::vkDestroyBuffer(device_, buffer.handle, nullptr);
    if (buffer.memory)
        ::vkFreeMemory(device_, buffer.memory, nullptr);
    buffer = {};
}
