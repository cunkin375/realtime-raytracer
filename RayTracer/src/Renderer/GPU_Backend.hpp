#pragma once

#include <vulkan/vulkan.h>

#include "Camera.hpp"
#include "Scene.hpp"

#include "Math/Vector.hpp"
#include "RayTracing/Ray.hpp"
#include "Util/Aliases.hpp"
#include "Util/DirectoryWatcher.hpp"

class GPU_Backend
{
public:
    GPU_Backend();

    void Render(const Camera &camera, const Scene &scene, u32 *image_data, fVector4 *accumulation_data);

    void SetImageParameters(u32 width, u32 height, u32 frame_index_);

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
        ::VkBuffer handle{ VK_NULL_HANDLE };
        ::VkDeviceMemory memory{ VK_NULL_HANDLE };
        ::VkDeviceSize size{ 0 };
    };

    // std140 UBO float3 members must be padded to 16-byte boundaries
    struct GPU_MetaData
    {
        f32 camera_position[3];
        f32 _pad0;
        f32 ray_direction[3];
        f32 _pad1;
        f32 background[3];
        f32 image_width;
        f32 frame_index;
        u32 num_spheres;
    };
    static_assert(sizeof(GPU_MetaData) == 56);
    static_assert(sizeof(Sphere) == 20);
    static_assert(sizeof(Material) == 36);

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

    u32 FindMemoryType(u32 type_filter, ::VkMemoryPropertyFlags properties);

    GPU_Buffer AllocateBuffer(::VkDeviceSize size, ::VkBufferUsageFlags usage_flags,
                              ::VkMemoryPropertyFlags memory_properties);

private:
    Settings config_;

    std::unique_ptr<DirectoryWatcher> shader_watcher_{ nullptr };

    const Scene *active_scene_{ nullptr };
    const Camera *active_camera_{ nullptr };

    bool valid_state_{ false };

    u32 current_pixel_count_;

    GPU_Buffer ubo_camera_;
    GPU_Buffer ssbo_spheres_;
    GPU_Buffer ssbo_materials_;
    GPU_Buffer ssbo_accumulation_;
    GPU_Buffer ssbo_image_;

    ::VkDevice device_{ VK_NULL_HANDLE };
    ::VkShaderModule compute_shader_module_{ VK_NULL_HANDLE };
    ::VkDescriptorSetLayout descriptor_set_layout_{ VK_NULL_HANDLE };
    ::VkDescriptorSet descriptor_set_{ VK_NULL_HANDLE };
    ::VkDescriptorPool descriptor_pool_{ VK_NULL_HANDLE };
    ::VkPipelineLayout pipeline_layout_{ VK_NULL_HANDLE };
    ::VkPipeline compute_pipeline_{ VK_NULL_HANDLE };
};
