#include "GPU_Backend.hpp"

#include <iostream>

#include <vulkan/vulkan.h>
// clang-format off
#include <wrl/client.h>
#include <dxc/dxcapi.h>
// clang-format on

#include "Util/Aliases.hpp"
#include "Util/Log.hpp"
#include "Walnut/Application.h"

namespace
{
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
} // namespace

GPU_Backend::GPU_Backend()
    : shader_watcher_{ std::filesystem::path("./Raytracer/assets/shaders/"),
                       [this](const DirectoryWatcher::FileEvent &event) { HotReloadShader(); } }
{
    ::VkInstance app = Walnut::Application::GetInstance();
    ::VkPhysicalDevice device = Walnut::Application::GetPhysicalDevice();
}

void GPU_Backend::CompileShaders()
{
    Microsoft::WRL::ComPtr<IDxcUtils> utils;
    Check(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));

    Microsoft::WRL::ComPtr<IDxcCompiler3> compiler;
    Check(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));

    DxcBuffer source_buffer;

    std::vector<LPCWSTR> arguments = { L"spriv", L"-T",   L"cs_6_5",
                                       L"-E",    L"main", L"-fspv-target-env='vulkan1.3'" };

    Microsoft::WRL::ComPtr<IDxcResult> result;
    compiler->Compile(&source_buffer, arguments.data(), static_cast<u32>(arguments.size()), nullptr,
                      IID_PPV_ARGS(&result));

    Microsoft::WRL::ComPtr<IDxcBlobUtf8> errors;
    result->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr);
    if (errors != nullptr && errors->GetStringLength() != 0zu)
    {
        std::cerr << errors->GetStringPointer() << "\n";
        return;
    }

    Microsoft::WRL::ComPtr<IDxcBlob> spriv_blob;
    result->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&spriv_blob), nullptr);

    // NOTE: this might change as the GPU pipeline is optimized
    auto create_info =
        ::VkShaderModuleCreateInfo{ .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                    .codeSize = spriv_blob->GetBufferSize(),
                                    .pCode = reinterpret_cast<const u32 *>(spriv_blob->GetBufferPointer()) };
    // ::vkCreateShaderModule(device_, &create_info, nullptr, &shader_module);
}

void GPU_Backend::HotReloadShader() {}
