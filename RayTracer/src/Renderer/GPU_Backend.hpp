#pragma once

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

    DirectoryWatcher shader_watcher_;

    const Scene *active_scene_{ nullptr };
    const Camera *active_camera_{ nullptr };

public:
    GPU_Backend();

    void Render(const Camera &camera, const Scene &scene, u32 *image_data, fVector4 *accumulation_data);

    void SetImageParameters(u32 width, u32 height, u32 frame_index_, bool is_fast_random_enabled);

private:
    void CompileShaders();
    void HotReloadShader();
};
