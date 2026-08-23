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
private:
    struct Settings
    {
        u32 image_width;
        u32 image_height;
        u32 frame_index;
        bool fast_random;
    };

private:
    Settings config_;

    std::unique_ptr<DirectoryWatcher> shader_watcher_{ nullptr };

    const Scene *active_scene_{ nullptr };
    const Camera *active_camera_{ nullptr };

    bool valid_state_{ false };

private:
    ::VkDevice device_{ VK_NULL_HANDLE };
    ::VkShaderModule compute_shader_module_{ VK_NULL_HANDLE };
    ::VkDescriptorSetLayout descriptor_set_layout_{ VK_NULL_HANDLE };
    ::VkPipelineLayout pipeline_layout_{ VK_NULL_HANDLE };
    ::VkPipeline compute_pipeline_{ VK_NULL_HANDLE };

public:
    GPU_Backend();

    void Render(const Camera &camera, const Scene &scene, u32 *image_data, fVector4 *accumulation_data);

    void SetImageParameters(u32 width, u32 height, u32 frame_index_, bool is_fast_random_enabled);

    bool ValidState() { return valid_state_; }

private:
    bool CompileShaders(std::string_view shader_path);
    void HotReloadShader();
};
