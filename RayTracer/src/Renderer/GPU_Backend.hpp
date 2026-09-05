#pragma once

#include <atomic>

#include <vulkan/vulkan.h>

#include <glm/glm.hpp>

#include "Camera.hpp"
#include "Scene.hpp"

#include "Ray.hpp"
#include "Util/Aliases.hpp"
#include "Util/DirectoryWatcher.hpp"

class GPU_Backend
{
public:
    GPU_Backend();
    ~GPU_Backend();

    void Render(const Camera &camera, const Scene &scene);

    void SetImageParameters(u32 width, u32 height, u32 frame_index_);

    VkDescriptorSet GetDescriptorSet() { return imgui_descriptor_; }

    bool ValidState() { return valid_state_; }

private:
    struct Settings
    {
        u32 image_width;
        u32 image_height;
        u32 frame_index;
    };

    struct GPU_Buffer
    {
        ::VkBuffer       handle{ VK_NULL_HANDLE };
        ::VkDeviceMemory memory{ VK_NULL_HANDLE };
        ::VkDeviceSize   size{ 0 };
    };

    // std140 UBO float3 members must be padded to 16-byte boundaries
    struct alignas(16) GPU_MetaData
    {
        f32         camera_position[3];
        f32         _pad0;
        glm::mat4x4 camera_inverse_view;
        glm::mat4x4 camera_inverse_projection;
        f32         background[3];
        f32         image_width;
        f32         image_height;
        f32         frame_index;
        u32         num_spheres;
        f32         _pad2[2];
    };

    static_assert(sizeof(GPU_MetaData) == 192);
    static_assert(sizeof(glm::mat4x4) == 64);
    static_assert(sizeof(Sphere) == 32);
    static_assert(sizeof(Material) == 48);

    // sick nasty tricks
    // template<std::size_t> struct ShowSize;
    // ShowSize<sizeof(GPU_MetaData)> gpu_metadata_size;
    // ShowSize<sizeof(fVector4)> vec4_size;
    // ShowSize<sizeof(fVector3)> vec3_size;

private:
    bool CompileShaders(std::string_view shader_path);

    void HotReloadShader();

    void ResizeBuffersIfNeeded(u32 width, u32 height, const Scene &scene);

    void WriteDescriptorSet();

    void PollShaderChanges();

    u32 FindMemoryType(u32 type_filter, ::VkMemoryPropertyFlags properties);

    GPU_Buffer AllocateBuffer(::VkDeviceSize size, ::VkBufferUsageFlags usage_flags,
                              ::VkMemoryPropertyFlags memory_properties);

private:
    Settings config_;

    std::unique_ptr<DirectoryWatcher> shader_watcher_{ nullptr };
    std::atomic<bool>                 pending_reload_{ false };

    bool valid_state_{ false };

    u32 current_pixel_count_;

    GPU_Buffer ubo_meta_;
    GPU_Buffer ssbo_spheres_;
    GPU_Buffer ssbo_materials_;
    GPU_Buffer ssbo_accumulation_;

    // Image storage buffer
    ::VkImage         shared_image_{ VK_NULL_HANDLE };
    ::VkImageView     shared_image_view_{ VK_NULL_HANDLE };
    ::VkDeviceMemory  shared_image_memory_{ VK_NULL_HANDLE };
    ::VkSampler       shared_sampler_{ VK_NULL_HANDLE };
    ::VkDescriptorSet imgui_descriptor_{ VK_NULL_HANDLE };

    ::VkDevice              device_{ VK_NULL_HANDLE };
    ::VkShaderModule        compute_shader_module_{ VK_NULL_HANDLE };
    ::VkDescriptorSetLayout descriptor_set_layout_{ VK_NULL_HANDLE };
    ::VkDescriptorSet       descriptor_set_{ VK_NULL_HANDLE };
    ::VkDescriptorPool      descriptor_pool_{ VK_NULL_HANDLE };
    ::VkPipelineLayout      pipeline_layout_{ VK_NULL_HANDLE };
    ::VkPipeline            compute_pipeline_{ VK_NULL_HANDLE };
};
