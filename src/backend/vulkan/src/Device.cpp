#include "snap/rhi/backend/vulkan/Device.h"
#include "snap/rhi/backend/common/Logging.hpp"
#include "snap/rhi/backend/common/PlatformSyncHandle.h"
#include "snap/rhi/backend/vulkan/Buffer.h"
#include "snap/rhi/backend/vulkan/CommandBuffer.h"
#include "snap/rhi/backend/vulkan/CommandQueue.h"
#include "snap/rhi/backend/vulkan/ComputePipeline.h"
#include "snap/rhi/backend/vulkan/DescriptorPool.h"
#include "snap/rhi/backend/vulkan/DescriptorSet.h"
#include "snap/rhi/backend/vulkan/DescriptorSetLayout.h"
#include "snap/rhi/backend/vulkan/Fence.h"
#include "snap/rhi/common/Scope.h"

#if SNAP_RHI_OS_ANDROID()
#include "snap/rhi/backend/common/platform/android/SyncHandle.h"
#include "snap/rhi/backend/vulkan/platform/android/Semaphore.h"
#endif
#include "snap/rhi/backend/vulkan/Framebuffer.h"
#include "snap/rhi/backend/vulkan/PipelineCache.h"
#include "snap/rhi/backend/vulkan/PipelineLayout.h"
#include "snap/rhi/backend/vulkan/QueryPool.h"
#include "snap/rhi/backend/vulkan/RenderPass.h"
#include "snap/rhi/backend/vulkan/RenderPipeline.h"
#include "snap/rhi/backend/vulkan/Sampler.h"
#include "snap/rhi/backend/vulkan/Semaphore.h"
#include "snap/rhi/backend/vulkan/ShaderLibrary.h"
#include "snap/rhi/backend/vulkan/ShaderModule.h"
#include "snap/rhi/backend/vulkan/Texture.h"
#include "snap/rhi/backend/vulkan/Utils.hpp"
#include <algorithm>
#include <set>
#include <snap/rhi/common/Throw.h>
#include <string>
#include <string_view>

#include <cstdlib>

namespace {
using namespace std::literals;

// =============================================================================
// MARK: - Validation Layer Configuration
// =============================================================================

/*
 * https://github.com/KhronosGroup/Vulkan-ValidationLayers/blob/main/docs/khronos_validation_layer.md
 *
 * SnapRHI will provide these Instance Validation Layers by default:
 *
 * VK_LAYER_KHRONOS_validation - Vulkan API validation and error checking.
 * https://vulkan.lunarg.com/doc/view/1.3.236.0/mac/khronos_validation_layer.html
 */
constexpr std::array InstanceValidationLayerNames = {"VK_LAYER_KHRONOS_validation"sv};
constexpr std::array InstanceLayerNames = InstanceValidationLayerNames;
constexpr std::array<std::string_view, 0> deviceLayersName = {};

// =============================================================================
// MARK: - Platform Flags
// =============================================================================

/**
 * @brief Bitmask flags for platform-specific extension requirements.
 *
 * Use bitwise OR to combine multiple platforms.
 * Example: Platform::Windows | Platform::Linux
 */
enum class Platform : uint8_t {
    None = 0,
    Windows = 1 << 0,
    Linux = 1 << 1,
    Android = 1 << 2,
    Apple = 1 << 3,

    // Common combinations
    Desktop = Windows | Linux | Apple,
    Mobile = Android | Apple,
    Posix = Linux | Android | Apple,
    All = Windows | Linux | Android | Apple,
};

// Enable bitwise operations for Platform enum
constexpr Platform operator|(Platform a, Platform b) {
    return static_cast<Platform>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr bool hasFlag(Platform flags, Platform flag) {
    return (static_cast<uint8_t>(flags) & static_cast<uint8_t>(flag)) != 0;
}

/**
 * @brief Returns the platform flag for the current build target.
 */
constexpr Platform getCurrentPlatform() {
#if SNAP_RHI_OS_WINDOWS()
    return Platform::Windows;
#elif SNAP_RHI_OS_ANDROID()
    return Platform::Android;
#elif SNAP_RHI_OS_APPLE()
    return Platform::Apple;
#elif SNAP_RHI_OS_LINUX_BASED()
    return Platform::Linux;
#else
    return Platform::None;
#endif
}

// =============================================================================
// MARK: - Extension Descriptor
// =============================================================================

/**
 * @brief Describes a Vulkan extension with its requirements and dependencies.
 *
 * This structure captures all the metadata needed to properly manage extension
 * enabling, including:
 * - The extension name
 * - The Vulkan version where it was promoted to core (if any)
 * - Platform requirements (as bitmask)
 * - Dependencies on other extensions
 */
struct ExtensionDescriptor {
    std::string_view name;
    uint32_t promotedToCore; // VK_API_VERSION where extension became core, 0 if not promoted
    bool isRequired;         // If true, device creation fails if not available
    Platform platforms;      // Bitmask of platforms that require this extension

    // Dependencies - other extensions that must be enabled first
    std::array<std::string_view, 4> dependencies;
    size_t dependencyCount;
};

// =============================================================================
// MARK: - Instance Extension Registry
// =============================================================================

/**
 * @brief Registry of all known instance extensions with their metadata.
 *
 * Each extension is documented with:
 * - Core promotion version (if applicable)
 * - Platform requirements (as bitmask)
 * - Dependencies
 *
 * Reference:
 * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_KHR_get_physical_device_properties2.html
 */
constexpr std::array<ExtensionDescriptor, 8> kInstanceExtensionRegistry = {{
    // VK_EXT_debug_utils - Debug markers, labels, and validation message callback
    {
        .name = VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::All,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_get_physical_device_properties2 - Extended physical device property queries
    // Promoted to core in Vulkan 1.1
    {
        .name = VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = true,
        .platforms = Platform::Windows | Platform::Linux | Platform::Apple,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_external_memory_capabilities - Query external memory handle capabilities
    // Promoted to core in Vulkan 1.1
    {
        .name = VK_KHR_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::Windows | Platform::Linux,
        .dependencies = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_external_semaphore_capabilities - Query external semaphore handle capabilities
    // Promoted to core in Vulkan 1.1
    {
        .name = VK_KHR_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::Android,
        .dependencies = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_external_fence_capabilities - Query external fence handle capabilities
    // Promoted to core in Vulkan 1.1
    {
        .name = VK_KHR_EXTERNAL_FENCE_CAPABILITIES_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::Android,
        .dependencies = {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_portability_enumeration - Required on macOS/iOS for MoltenVK
    {
        .name = VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = SNAP_RHI_VULKAN_DYNAMIC_LOADING(),
        .platforms = Platform::Apple,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_surface - Window system integration base extension (optional)
    {
        .name = VK_KHR_SURFACE_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::None, // Optional on all platforms
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_EXT_swapchain_colorspace - Extended swapchain color spaces (optional)
    {
        .name = VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::None,
        .dependencies = {},
        .dependencyCount = 0,
    },
}};

// =============================================================================
// MARK: - Device Extension Registry
// =============================================================================

/**
 * @brief Registry of all known device extensions with their metadata.
 *
 * Device extensions provide GPU-specific functionality and may have
 * dependencies on both instance and other device extensions.
 *
 * Extension dependency chain for VK_KHR_dynamic_rendering:
 *   VK_KHR_dynamic_rendering
 *     └─ VK_KHR_depth_stencil_resolve (promoted to 1.2)
 *         └─ VK_KHR_create_renderpass2 (promoted to 1.2)
 *             ├─ VK_KHR_multiview (promoted to 1.1)
 *             └─ VK_KHR_maintenance2 (promoted to 1.1)
 */
constexpr std::array kDeviceExtensionRegistry = {
    // VK_KHR_portability_subset - Required on macOS/iOS (MoltenVK)
    ExtensionDescriptor{
        .name = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = true,
        .platforms = Platform::Apple,
        .dependencies = {},
        .dependencyCount = 0,
    },

#if defined(VK_USE_PLATFORM_METAL_EXT)
    // VK_EXT_metal_objects - Metal interop on Apple platforms
    ExtensionDescriptor{
        .name = VK_EXT_METAL_OBJECTS_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::Apple,
        .dependencies = {},
        .dependencyCount = 0,
    },
#endif

    // VK_KHR_maintenance1 - Misc improvements (optional)
    ExtensionDescriptor{
        .name = VK_KHR_MAINTENANCE1_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::None,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_maintenance2 - Required by VK_KHR_create_renderpass2
    ExtensionDescriptor{
        .name = VK_KHR_MAINTENANCE_2_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::All, // Required for dynamic rendering chain
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_maintenance3 (optional)
    ExtensionDescriptor{
        .name = VK_KHR_MAINTENANCE_3_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::None,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_multiview - Required by VK_KHR_create_renderpass2
    ExtensionDescriptor{
        .name = "VK_KHR_multiview"sv,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::All, // Required for dynamic rendering chain
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_external_memory - External memory handle support
    ExtensionDescriptor{
        .name = VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::Windows | Platform::Linux,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_external_memory_win32 - Windows external memory handles
    ExtensionDescriptor{
        .name = "VK_KHR_external_memory_win32"sv,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::Windows,
        .dependencies = {VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_external_memory_fd - POSIX file descriptor external memory
    ExtensionDescriptor{
        .name = VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::Linux,
        .dependencies = {VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_external_semaphore - External semaphore handle support
    ExtensionDescriptor{
        .name = VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::Android,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_external_semaphore_fd - POSIX file descriptor semaphores
    ExtensionDescriptor{
        .name = VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::Linux | Platform::Android,
        .dependencies = {VK_KHR_EXTERNAL_SEMAPHORE_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_external_fence - External fence handle support
    ExtensionDescriptor{
        .name = VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_1,
        .isRequired = false,
        .platforms = Platform::Android,
        .dependencies = {},
        .dependencyCount = 0,
    },

    // VK_KHR_external_fence_fd - POSIX file descriptor fences
    ExtensionDescriptor{
        .name = VK_KHR_EXTERNAL_FENCE_FD_EXTENSION_NAME,
        .promotedToCore = 0,
        .isRequired = false,
        .platforms = Platform::Android,
        .dependencies = {VK_KHR_EXTERNAL_FENCE_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_create_renderpass2 - Required by VK_KHR_depth_stencil_resolve
    ExtensionDescriptor{
        .name = VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_2,
        .isRequired = false,
        .platforms = Platform::All,
        .dependencies = {"VK_KHR_multiview"sv, VK_KHR_MAINTENANCE_2_EXTENSION_NAME},
        .dependencyCount = 2,
    },

    // VK_KHR_depth_stencil_resolve - Required by VK_KHR_dynamic_rendering
    ExtensionDescriptor{
        .name = VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_2,
        .isRequired = false,
        .platforms = Platform::All,
        .dependencies = {VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_dynamic_rendering - Renderpass-less rendering
    ExtensionDescriptor{
        .name = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_3,
        .isRequired = false,
        .platforms = Platform::All,
        .dependencies = {VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME},
        .dependencyCount = 1,
    },

    // VK_KHR_synchronization2 - Improved synchronization primitives (optional)
    ExtensionDescriptor{
        .name = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME,
        .promotedToCore = VK_API_VERSION_1_3,
        .isRequired = false,
        .platforms = Platform::None,
        .dependencies = {},
        .dependencyCount = 0,
    },
};

// =============================================================================
// MARK: - Extension Manager
// =============================================================================

/**
 * @brief Manages Vulkan extension discovery, dependency resolution, and enabling.
 *
 * This class provides a clean interface for:
 * - Querying available extensions
 * - Resolving extension dependencies
 * - Handling core promotion (skipping extensions that are in core)
 * - Platform-specific extension requirements
 */
class ExtensionManager {
public:
    /**
     * @brief Checks if an extension is required for the current platform.
     */
    [[nodiscard]] static bool isRequiredForCurrentPlatform(const ExtensionDescriptor& ext) {
        return hasFlag(ext.platforms, getCurrentPlatform());
    }

    /**
     * @brief Checks if an extension is promoted to core for the given API version.
     */
    [[nodiscard]] static bool isPromotedToCore(const ExtensionDescriptor& ext, uint32_t apiVersion) {
        return ext.promotedToCore != 0 && apiVersion >= ext.promotedToCore;
    }

    /**
     * @brief Checks if an extension is available in the supported extensions list.
     */
    [[nodiscard]] static bool isExtensionSupported(std::string_view extensionName,
                                                   const std::vector<VkExtensionProperties>& supportedExtensions) {
        return std::any_of(
            supportedExtensions.begin(), supportedExtensions.end(), [extensionName](const VkExtensionProperties& prop) {
                return extensionName == prop.extensionName;
            });
    }

    /**
     * @brief Builds a list of instance extensions to enable.
     *
     * @param supportedExtensions List of extensions supported by the instance
     * @param apiVersion The target Vulkan API version
     * @param validationLayer Validation layer for error reporting
     * @return Vector of extension names to enable
     */
    [[nodiscard]] static std::vector<const char*> buildInstanceExtensions(
        const std::vector<VkExtensionProperties>& supportedExtensions,
        uint32_t apiVersion,
        snap::rhi::backend::common::ValidationLayer& validationLayer) {
        std::vector<const char*> result;
        std::set<std::string_view> enabledSet; // Track what's enabled for dependency resolution

        // First pass: collect all required extensions and their dependencies
        std::vector<const ExtensionDescriptor*> candidateExtensions;
        for (const auto& ext : kInstanceExtensionRegistry) {
            if (isRequiredForCurrentPlatform(ext)) {
                candidateExtensions.push_back(&ext);
            }
        }

        // Sort by dependency order (extensions with no dependencies first)
        std::sort(candidateExtensions.begin(),
                  candidateExtensions.end(),
                  [](const ExtensionDescriptor* a, const ExtensionDescriptor* b) {
                      return a->dependencyCount < b->dependencyCount;
                  });

        // Second pass: enable extensions with dependency resolution
        for (const ExtensionDescriptor* ext : candidateExtensions) {
            // Skip if already promoted to core
            if (isPromotedToCore(*ext, apiVersion)) {
                SNAP_RHI_LOGI("[Vulkan][Extensions] Skipping %s - promoted to core in Vulkan %u.%u",
                              ext->name.data(),
                              VK_API_VERSION_MAJOR(ext->promotedToCore),
                              VK_API_VERSION_MINOR(ext->promotedToCore));
                enabledSet.insert(ext->name); // Mark as "enabled" for dependency purposes
                continue;
            }

            // Check if extension is supported
            if (!isExtensionSupported(ext->name, supportedExtensions)) {
                if (ext->isRequired) {
                    SNAP_RHI_VALIDATE(validationLayer,
                                      false,
                                      snap::rhi::ReportLevel::CriticalError,
                                      snap::rhi::ValidationTag::CreateOp,
                                      "[Vulkan::Device] Required instance extension %s is not supported",
                                      ext->name.data());
                } else {
                    SNAP_RHI_LOGW("[Vulkan][Extensions] Optional instance extension %s is not supported",
                                  ext->name.data());
                }
                continue;
            }

            // Check dependencies
            bool dependenciesMet = true;
            for (size_t i = 0; i < ext->dependencyCount; ++i) {
                if (enabledSet.find(ext->dependencies[i]) == enabledSet.end()) {
                    // Dependency not enabled - try to enable it first
                    const ExtensionDescriptor* depExt = findExtension(kInstanceExtensionRegistry, ext->dependencies[i]);
                    if (depExt && isExtensionSupported(depExt->name, supportedExtensions) &&
                        !isPromotedToCore(*depExt, apiVersion)) {
                        result.push_back(depExt->name.data());
                        enabledSet.insert(depExt->name);
                        SNAP_RHI_LOGI("[Vulkan][Extensions] Enabling instance extension %s (dependency of %s)",
                                      depExt->name.data(),
                                      ext->name.data());
                    } else if (depExt && isPromotedToCore(*depExt, apiVersion)) {
                        enabledSet.insert(depExt->name); // Dependency satisfied by core
                    } else {
                        dependenciesMet = false;
                        SNAP_RHI_LOGW("[Vulkan][Extensions] Cannot enable %s - dependency %s not available",
                                      ext->name.data(),
                                      ext->dependencies[i].data());
                        break;
                    }
                }
            }

            if (dependenciesMet) {
                result.push_back(ext->name.data());
                enabledSet.insert(ext->name);
                SNAP_RHI_LOGI("[Vulkan][Extensions] Enabling instance extension %s", ext->name.data());
            }
        }

        return result;
    }

    /**
     * @brief Builds a list of device extensions to enable.
     *
     * @param supportedExtensions List of extensions supported by the device
     * @param apiVersion The device's supported Vulkan API version
     * @param validationLayer Validation layer for error reporting
     * @return Vector of extension names to enable
     */
    [[nodiscard]] static std::vector<const char*> buildDeviceExtensions(
        const std::vector<VkExtensionProperties>& supportedExtensions,
        uint32_t apiVersion,
        snap::rhi::backend::common::ValidationLayer& validationLayer) {
        std::vector<const char*> result;
        std::set<std::string_view> enabledSet;

        auto enableExtension = [&](const ExtensionDescriptor& ext, auto&& self) -> bool {
            if (enabledSet.count(ext.name))
                return true;

            if (isPromotedToCore(ext, apiVersion)) {
                enabledSet.insert(ext.name);
                return true;
            }

            if (!isExtensionSupported(ext.name, supportedExtensions)) {
                if (ext.isRequired) {
                    SNAP_RHI_VALIDATE(validationLayer,
                                      false,
                                      snap::rhi::ReportLevel::CriticalError,
                                      snap::rhi::ValidationTag::CreateOp,
                                      "[Vulkan::Device] Required device extension %s is not supported",
                                      ext.name.data());
                } else {
                    SNAP_RHI_LOGW("[Vulkan][Extensions] Optional device extension %s is not supported",
                                  ext.name.data());
                }
                return false;
            }

            for (size_t i = 0; i < ext.dependencyCount; ++i) {
                const ExtensionDescriptor* depExt = findExtension(kDeviceExtensionRegistry, ext.dependencies[i]);
                if (!depExt || !self(*depExt, self)) {
                    SNAP_RHI_LOGW("[Vulkan][Extensions] Cannot enable %s - dependency %s failed",
                                  ext.name.data(),
                                  ext.dependencies[i].data());
                    return false;
                }
            }

            result.push_back(ext.name.data());
            enabledSet.insert(ext.name);
            SNAP_RHI_LOGI("[Vulkan][Extensions] Enabling device extension %s", ext.name.data());
            return true;
        };

        for (const auto& ext : kDeviceExtensionRegistry) {
            if (isRequiredForCurrentPlatform(ext)) {
                enableExtension(ext, enableExtension);
            }
        }

        return result;
    }

    /**
     * @brief Checks if a specific extension is enabled or promoted to core.
     */
    [[nodiscard]] static bool isExtensionEnabled(std::string_view extensionName,
                                                 const std::vector<const char*>& enabledExtensions,
                                                 uint32_t apiVersion) {
        // Check if promoted to core
        for (const auto& ext : kDeviceExtensionRegistry) {
            if (ext.name == extensionName && isPromotedToCore(ext, apiVersion)) {
                return true;
            }
        }
        for (const auto& ext : kInstanceExtensionRegistry) {
            if (ext.name == extensionName && isPromotedToCore(ext, apiVersion)) {
                return true;
            }
        }

        // Check if explicitly enabled
        return std::any_of(enabledExtensions.begin(), enabledExtensions.end(), [extensionName](const char* enabled) {
            return extensionName == enabled;
        });
    }

private:
    /**
     * @brief Finds an extension descriptor by name in the registry.
     */
    template<size_t N>
    [[nodiscard]] static const ExtensionDescriptor* findExtension(const std::array<ExtensionDescriptor, N>& registry,
                                                                  std::string_view name) {
        for (const auto& ext : registry) {
            if (ext.name == name) {
                return &ext;
            }
        }
        return nullptr;
    }
};

snap::rhi::ReportLevel toReportLevel(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity) {
    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
        return snap::rhi::ReportLevel::Error;
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        return snap::rhi::ReportLevel::Warning;
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        return snap::rhi::ReportLevel::Info;
    }

    if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
        return snap::rhi::ReportLevel::Debug;
    }

    return snap::rhi::ReportLevel::Debug;
}

snap::rhi::ValidationTag toValidationTag(const VkDebugUtilsMessageTypeFlagsEXT /*messageTypes*/) {
    // Vulkan validation messages can cover many areas; treat them as general command/driver validation.
    return snap::rhi::ValidationTag::CommandBufferOp;
}

VKAPI_ATTR VkBool32 VKAPI_CALL mainDebugMessageCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                        void* pUserData) {
    auto validationLayer = static_cast<snap::rhi::backend::common::ValidationLayer*>(pUserData);
    validationLayer->report(toReportLevel(messageSeverity),
                            toValidationTag(messageTypes),
                            "[Vulkan][ValidationLayer] %s",
                            pCallbackData->pMessage);
    /*
     * https://registry.khronos.org/vulkan/specs/latest/man/html/PFN_vkDebugUtilsMessengerCallbackEXT.html
     *
     * The application should always return VK_FALSE.
     * */
    return VK_FALSE;
}

std::vector<VkLayerProperties> getInstanceLayerProperties(
    snap::rhi::backend::common::ValidationLayer& validationLayer) {
    uint32_t layersCount = 0;

    VkResult result = vkEnumerateInstanceLayerProperties(&layersCount, nullptr);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] Failed to get all Vulkan Instance Layer Properties");

    std::vector<VkLayerProperties> layersProperties(layersCount);

    result = vkEnumerateInstanceLayerProperties(&layersCount, layersProperties.data());
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] Failed to get all Vulkan Instance Layer Properties");

    return layersProperties;
}

std::vector<VkLayerProperties> getDeviceLayerProperties(VkPhysicalDevice physicalDevice,
                                                        snap::rhi::backend::common::ValidationLayer& validationLayer) {
    uint32_t layersCount = 0;

    VkResult result = vkEnumerateDeviceLayerProperties(physicalDevice, &layersCount, nullptr);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] Failed to get all Vulkan Device Layer Properties");

    std::vector<VkLayerProperties> layersProperties(layersCount);
    result = vkEnumerateDeviceLayerProperties(physicalDevice, &layersCount, layersProperties.data());
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] Failed to get all Vulkan Device Layer Properties");

    return layersProperties;
}

std::vector<VkExtensionProperties> getSupportedInstanceExtensions(const char* layerName) {
    uint32_t count = 0;
    std::vector<VkExtensionProperties> extensions;

    VkResult vkResult = vkEnumerateInstanceExtensionProperties(layerName, &count, nullptr);
    if (vkResult != VK_SUCCESS && vkResult != VK_INCOMPLETE) {
        return {};
    }

    extensions.resize(count);
    vkResult = vkEnumerateInstanceExtensionProperties(layerName, &count, extensions.data());
    if (vkResult != VK_SUCCESS) {
        return {};
    }

    return extensions;
}

std::vector<VkExtensionProperties> getSupportedDeviceExtensions(VkPhysicalDevice physicalDevice,
                                                                const char* layerName) {
    uint32_t count = 0;

    VkResult vkResult = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &count, nullptr);
    if (vkResult != VK_SUCCESS && vkResult != VK_INCOMPLETE) {
        return {};
    }

    std::vector<VkExtensionProperties> extensions(count);
    vkResult = vkEnumerateDeviceExtensionProperties(physicalDevice, layerName, &count, extensions.data());
    if (vkResult != VK_SUCCESS) {
        return {};
    }

    return extensions;
}

std::vector<VkExtensionProperties> getSupportedInstanceExtensions(const std::vector<VkLayerProperties>& properties) {
    std::vector<VkExtensionProperties> result = getSupportedInstanceExtensions(nullptr);

    for (const auto& info : properties) {
        std::vector<VkExtensionProperties> exts = getSupportedInstanceExtensions(info.layerName);

        result.insert(result.end(), exts.begin(), exts.end());
    }

    return result;
}

std::vector<VkExtensionProperties> getSupportedDeviceExtensions(VkPhysicalDevice physicalDevice,
                                                                const std::vector<VkLayerProperties>& properties) {
    std::vector<VkExtensionProperties> result = getSupportedDeviceExtensions(physicalDevice, nullptr);

    for (const auto& info : properties) {
        std::vector<VkExtensionProperties> exts = getSupportedDeviceExtensions(physicalDevice, info.layerName);

        result.insert(result.end(), exts.begin(), exts.end());
    }

    return result;
}

template<size_t Size>
std::vector<const char*> prepareLayerPropertiesList(const std::vector<VkLayerProperties>& properties,
                                                    const std::array<std::string_view, Size>& layersName) {
    std::vector<const char*> result;
    for (const auto& info : properties) {
        for (const auto& name : layersName) {
            if (!strcmp(info.layerName, name.data())) {
                result.push_back(name.data());
            }
        }
    }

    return result;
}

std::vector<VkPhysicalDevice> getPhysicalDevices(VkInstance instance,
                                                 snap::rhi::backend::common::ValidationLayer& validationLayer) {
    uint32_t count = 0;

    VkResult result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] Failed to enumerate physical devices");

    std::vector<VkPhysicalDevice> devices;
    devices.resize(count);
    result = vkEnumeratePhysicalDevices(instance, &count, devices.data());
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] Failed to enumerate physical devices");
    return devices;
}

std::vector<VkQueueFamilyProperties> getQueueFamilyProperties(const VkPhysicalDevice device) {
    uint32_t count = 0;
    std::vector<VkQueueFamilyProperties> queueFamilies;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, nullptr);

    queueFamilies.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &count, queueFamilies.data());

    return queueFamilies;
}

VkPhysicalDevice choosePhysicalDeviceByType(std::vector<VkPhysicalDevice> devices,
                                            const VkPhysicalDeviceType deviceType,
                                            const uint32_t queueFlags) {
    VkPhysicalDeviceProperties physicalDeviceProperties{};

    for (const auto& device : devices) {
        vkGetPhysicalDeviceProperties(device, &physicalDeviceProperties);
        std::vector<VkQueueFamilyProperties> queueFamilyProperties = getQueueFamilyProperties(device);

        for (const auto& info : queueFamilyProperties) {
            if ((info.queueFlags & queueFlags) && (physicalDeviceProperties.deviceType == deviceType)) {
                return device;
            }
        }
    }

    return VK_NULL_HANDLE;
}

inline VkBool32 setFeatureEnabled(VkBool32 featureSupported, VkBool32 forceToEnable) {
    const VkBool32 result = featureSupported & forceToEnable;
    return result;
}

VkPhysicalDeviceVulkan13Features preparePhysicalDeviceFeatures_1_3(
    const VkPhysicalDeviceVulkan13Features& srcPhysicalDeviceFeatures) {
    VkPhysicalDeviceVulkan13Features dstPhysicalDeviceFeatures{};
    dstPhysicalDeviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    dstPhysicalDeviceFeatures.pNext = nullptr;

    dstPhysicalDeviceFeatures.dynamicRendering = setFeatureEnabled(srcPhysicalDeviceFeatures.dynamicRendering, VK_TRUE);

    return dstPhysicalDeviceFeatures;
}

VkPhysicalDeviceFeatures preparePhysicalDeviceFeatures(const VkPhysicalDeviceFeatures& srcPhysicalDeviceFeatures) {
    VkPhysicalDeviceFeatures dstPhysicalDeviceFeatures{};

    dstPhysicalDeviceFeatures.robustBufferAccess =
        setFeatureEnabled(srcPhysicalDeviceFeatures.robustBufferAccess, VK_FALSE);
    dstPhysicalDeviceFeatures.geometryShader = setFeatureEnabled(srcPhysicalDeviceFeatures.geometryShader, VK_FALSE);
    dstPhysicalDeviceFeatures.tessellationShader =
        setFeatureEnabled(srcPhysicalDeviceFeatures.tessellationShader, VK_FALSE);
    dstPhysicalDeviceFeatures.sampleRateShading =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sampleRateShading, VK_FALSE);
    dstPhysicalDeviceFeatures.dualSrcBlend = setFeatureEnabled(srcPhysicalDeviceFeatures.dualSrcBlend, VK_FALSE);
    dstPhysicalDeviceFeatures.logicOp = setFeatureEnabled(srcPhysicalDeviceFeatures.logicOp, VK_FALSE);
    dstPhysicalDeviceFeatures.multiDrawIndirect =
        setFeatureEnabled(srcPhysicalDeviceFeatures.multiDrawIndirect, VK_FALSE);
    dstPhysicalDeviceFeatures.drawIndirectFirstInstance =
        setFeatureEnabled(srcPhysicalDeviceFeatures.drawIndirectFirstInstance, VK_FALSE);
    dstPhysicalDeviceFeatures.fillModeNonSolid =
        setFeatureEnabled(srcPhysicalDeviceFeatures.fillModeNonSolid, VK_FALSE);
    dstPhysicalDeviceFeatures.wideLines = setFeatureEnabled(srcPhysicalDeviceFeatures.wideLines, VK_FALSE);
    dstPhysicalDeviceFeatures.largePoints = setFeatureEnabled(srcPhysicalDeviceFeatures.largePoints, VK_FALSE);
    dstPhysicalDeviceFeatures.multiViewport = setFeatureEnabled(srcPhysicalDeviceFeatures.multiViewport, VK_FALSE);
    dstPhysicalDeviceFeatures.textureCompressionASTC_LDR =
        setFeatureEnabled(srcPhysicalDeviceFeatures.textureCompressionASTC_LDR, VK_FALSE);
    dstPhysicalDeviceFeatures.textureCompressionBC =
        setFeatureEnabled(srcPhysicalDeviceFeatures.textureCompressionBC, VK_FALSE);
    dstPhysicalDeviceFeatures.occlusionQueryPrecise =
        setFeatureEnabled(srcPhysicalDeviceFeatures.occlusionQueryPrecise, VK_FALSE);
    dstPhysicalDeviceFeatures.pipelineStatisticsQuery =
        setFeatureEnabled(srcPhysicalDeviceFeatures.pipelineStatisticsQuery, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderTessellationAndGeometryPointSize =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderTessellationAndGeometryPointSize, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderImageGatherExtended =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderImageGatherExtended, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderStorageImageExtendedFormats =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderStorageImageExtendedFormats, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderStorageImageMultisample =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderStorageImageMultisample, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderStorageImageReadWithoutFormat =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderStorageImageReadWithoutFormat, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderStorageImageWriteWithoutFormat =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderStorageImageWriteWithoutFormat, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderFloat64 = setFeatureEnabled(srcPhysicalDeviceFeatures.shaderFloat64, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderInt64 = setFeatureEnabled(srcPhysicalDeviceFeatures.shaderInt64, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderInt16 = setFeatureEnabled(srcPhysicalDeviceFeatures.shaderInt16, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderResourceResidency =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderResourceResidency, VK_FALSE);
    dstPhysicalDeviceFeatures.shaderResourceMinLod =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderResourceMinLod, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseBinding = setFeatureEnabled(srcPhysicalDeviceFeatures.sparseBinding, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidencyBuffer =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidencyBuffer, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidencyImage2D =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidencyImage2D, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidencyImage3D =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidencyImage3D, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidency2Samples =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidency2Samples, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidency4Samples =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidency4Samples, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidency8Samples =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidency8Samples, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidency16Samples =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidency16Samples, VK_FALSE);
    dstPhysicalDeviceFeatures.sparseResidencyAliased =
        setFeatureEnabled(srcPhysicalDeviceFeatures.sparseResidencyAliased, VK_FALSE);
    dstPhysicalDeviceFeatures.variableMultisampleRate =
        setFeatureEnabled(srcPhysicalDeviceFeatures.variableMultisampleRate, VK_FALSE);
    dstPhysicalDeviceFeatures.inheritedQueries =
        setFeatureEnabled(srcPhysicalDeviceFeatures.inheritedQueries, VK_FALSE);
    dstPhysicalDeviceFeatures.fragmentStoresAndAtomics =
        setFeatureEnabled(srcPhysicalDeviceFeatures.fragmentStoresAndAtomics, VK_FALSE);

    // enabled features
    dstPhysicalDeviceFeatures.depthBounds = setFeatureEnabled(srcPhysicalDeviceFeatures.depthBounds, VK_TRUE);
    dstPhysicalDeviceFeatures.shaderClipDistance =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderClipDistance, VK_TRUE);
    dstPhysicalDeviceFeatures.shaderCullDistance =
        setFeatureEnabled(srcPhysicalDeviceFeatures.shaderCullDistance, VK_TRUE);
    dstPhysicalDeviceFeatures.alphaToOne = setFeatureEnabled(srcPhysicalDeviceFeatures.alphaToOne, VK_TRUE);
    dstPhysicalDeviceFeatures.fullDrawIndexUint32 =
        setFeatureEnabled(srcPhysicalDeviceFeatures.fullDrawIndexUint32, VK_TRUE);
    dstPhysicalDeviceFeatures.depthBiasClamp = setFeatureEnabled(srcPhysicalDeviceFeatures.depthBiasClamp, VK_TRUE);
    dstPhysicalDeviceFeatures.imageCubeArray = setFeatureEnabled(srcPhysicalDeviceFeatures.imageCubeArray, VK_TRUE);
    dstPhysicalDeviceFeatures.independentBlend = setFeatureEnabled(srcPhysicalDeviceFeatures.independentBlend, VK_TRUE);
    dstPhysicalDeviceFeatures.samplerAnisotropy =
        setFeatureEnabled(srcPhysicalDeviceFeatures.samplerAnisotropy, VK_TRUE);
    dstPhysicalDeviceFeatures.depthClamp = setFeatureEnabled(srcPhysicalDeviceFeatures.depthClamp, VK_TRUE);
    dstPhysicalDeviceFeatures.textureCompressionETC2 =
        setFeatureEnabled(srcPhysicalDeviceFeatures.textureCompressionETC2, VK_TRUE);
    dstPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics =
        setFeatureEnabled(srcPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics, VK_TRUE);

    return dstPhysicalDeviceFeatures;
}

std::vector<VkDeviceQueueCreateInfo> prepareDeviceQueueCreateInfo(
    const std::vector<VkQueueFamilyProperties>& queueFamilyProperties, const uint32_t queueFlags) {
    constexpr uint32_t maxQueueCount = 8;
    static std::array<float, maxQueueCount> queuePriorities;

    std::vector<VkDeviceQueueCreateInfo> result{};

    for (size_t i = 0; i < queueFamilyProperties.size(); ++i) {
        if (queueFamilyProperties[i].queueFlags & queueFlags) {
            VkDeviceQueueCreateInfo createInfo{};

            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            createInfo.pNext = nullptr;
            createInfo.flags = 0;
            createInfo.queueFamilyIndex = static_cast<uint32_t>(i);
            createInfo.queueCount = std::min(maxQueueCount, queueFamilyProperties[i].queueCount);
            createInfo.pQueuePriorities = queuePriorities.data();

            queuePriorities.fill(1.0f / float(createInfo.queueCount));
            result.push_back(createInfo);
            break; // Currently we have to support only 1 command queue family
        }
    }

    return result;
}

void buildPixelFormatProperties(VkPhysicalDevice physicalDevice,
                                VkPhysicalDeviceProperties physicalDeviceProperties,
                                snap::rhi::Capabilities& caps) {
    VkFormatProperties properties{};

    for (uint32_t i = 0; i < static_cast<uint32_t>(snap::rhi::PixelFormat::Count); ++i) {
        auto& info = caps.formatProperties[i];

        snap::rhi::PixelFormat rhiFormat = static_cast<snap::rhi::PixelFormat>(i);
        VkFormat format = snap::rhi::backend::vulkan::toVkFormat(rhiFormat);
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &properties);

        VkSampleCountFlags counts = 0;
        if (!snap::rhi::hasStencilAspect(rhiFormat) && !snap::rhi::hasDepthAspect(rhiFormat)) {
            counts = physicalDeviceProperties.limits.framebufferColorSampleCounts;
        } else {
            counts = physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        }

        if (snap::rhi::isIntFormat(rhiFormat)) {
            counts &= physicalDeviceProperties.limits.sampledImageIntegerSampleCounts;
        }

        if (snap::rhi::hasDepthAspect(rhiFormat)) {
            counts &= physicalDeviceProperties.limits.sampledImageDepthSampleCounts;
        }

        if (snap::rhi::hasStencilAspect(rhiFormat)) {
            counts &= physicalDeviceProperties.limits.sampledImageStencilSampleCounts;
        }

        if (!snap::rhi::hasStencilAspect(rhiFormat) && !snap::rhi::hasDepthAspect(rhiFormat)) {
            counts &= physicalDeviceProperties.limits.sampledImageColorSampleCounts;
        }

        snap::rhi::SampleCount sampleCount = snap::rhi::SampleCount::Count1;
        while (static_cast<uint32_t>(sampleCount) <= counts) {
            sampleCount = static_cast<snap::rhi::SampleCount>(static_cast<uint32_t>(sampleCount) << 1);
        }
        sampleCount = static_cast<snap::rhi::SampleCount>(static_cast<uint32_t>(sampleCount) >> 1);

        info.framebufferSampleCounts = sampleCount;
        info.sampledTexture2DColorSampleCounts = sampleCount;
        info.sampledTexture2DArrayColorSampleCounts = sampleCount;

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) {
            info.textureFeatures =
                static_cast<snap::rhi::FormatFeatures>(info.textureFeatures | snap::rhi::FormatFeatures::Storage);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT) {
            info.textureFeatures =
                static_cast<snap::rhi::FormatFeatures>(info.textureFeatures | snap::rhi::FormatFeatures::Sampled);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) {
            info.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
                info.textureFeatures | snap::rhi::FormatFeatures::SampledFilterLinear);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) {
            info.textureFeatures = static_cast<snap::rhi::FormatFeatures>(info.textureFeatures |
                                                                          snap::rhi::FormatFeatures::ColorRenderable);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT) {
            info.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
                info.textureFeatures | snap::rhi::FormatFeatures::ColorRenderableBlend);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            info.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
                info.textureFeatures | snap::rhi::FormatFeatures::DepthStencilRenderable);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) {
            info.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
                info.textureFeatures | snap::rhi::FormatFeatures::BlitSrc | snap::rhi::FormatFeatures::TransferSrc);
        }

        if (properties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT) {
            info.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
                info.textureFeatures | snap::rhi::FormatFeatures::BlitDst | snap::rhi::FormatFeatures::TransferDst);
        }

        if (info.textureFeatures != snap::rhi::FormatFeatures::None && sampleCount != snap::rhi::SampleCount::Count1) {
            info.textureFeatures =
                static_cast<snap::rhi::FormatFeatures>(info.textureFeatures | snap::rhi::FormatFeatures::Resolve);
        }
    }

    {
        auto& grayscale = caps.formatProperties[static_cast<uint8_t>(snap::rhi::PixelFormat::Grayscale)];
        grayscale.textureFeatures = static_cast<snap::rhi::FormatFeatures>(
            snap::rhi::FormatFeatures::BlitDst | snap::rhi::FormatFeatures::SampledFilterLinear);
        grayscale.framebufferSampleCounts = snap::rhi::SampleCount::Count1;
        grayscale.sampledTexture2DColorSampleCounts = snap::rhi::SampleCount::Count1;
        grayscale.sampledTexture2DArrayColorSampleCounts = snap::rhi::SampleCount::Count1;
    }
}

snap::rhi::CommandQueueFeatures convertTo(const VkDeviceQueueCreateFlags flags) {
    snap::rhi::CommandQueueFeatures result = snap::rhi::CommandQueueFeatures::None;

    if (flags & VK_QUEUE_GRAPHICS_BIT) {
        result = static_cast<snap::rhi::CommandQueueFeatures>(result | snap::rhi::CommandQueueFeatures::Graphics);
    }

    if (flags & VK_QUEUE_COMPUTE_BIT) {
        result = static_cast<snap::rhi::CommandQueueFeatures>(result | snap::rhi::CommandQueueFeatures::Compute);
    }

    if (flags & VK_QUEUE_TRANSFER_BIT) {
        result = static_cast<snap::rhi::CommandQueueFeatures>(result | snap::rhi::CommandQueueFeatures::Transfer);
    }

    return result;
}

bool getTimestampQuerySupported(const VkPhysicalDeviceProperties& deviceProperties,
                                const VkQueueFamilyProperties& queueFamilyProperties) {
    if (deviceProperties.limits.timestampPeriod == 0) {
        return false;
    }
    if (deviceProperties.limits.timestampComputeAndGraphics) {
        return true;
    }
    return queueFamilyProperties.timestampValidBits != 0;
}
// ---------------------------------------------------------------------------
// Auto-configure ICD and VVL layer discovery for the Vulkan Loader.
//
// When Vulkan libraries are not installed in a standard SDK location,
// the Vulkan Loader cannot find the corresponding JSON manifests and
// vkCreateInstance returns VK_ERROR_INCOMPATIBLE_DRIVER (Apple) or
// fails to enable validation layers (all platforms).
//
// At CMake configure time we generate minimal JSON manifests that contain
// absolute library paths (via $<TARGET_FILE:...> generator expressions).
// The compile definitions SNAP_RHI_VK_ICD_MANIFEST_PATH and
// SNAP_RHI_VK_LAYER_MANIFEST_DIR carry those paths into the binary.
// This helper simply points the Vulkan Loader at them through environment
// variables.  It is a no-op when the env vars are already set by the caller
// or when the definitions are absent (e.g. standalone SDK builds).
// ---------------------------------------------------------------------------

// Platform-portable setenv that honours existing values.
// Defined as a macro so it doesn't trigger -Wunused-function when the
// SNAP_RHI_VK_* compile definitions are absent.
#if SNAP_RHI_OS_WINDOWS()
#define SNAP_RHI_SETENV_IF_ABSENT(name, value)                                                                         \
    do {                                                                                                               \
        if (!std::getenv(name))                                                                                        \
            _putenv_s(name, value);                                                                                    \
    } while (0)
#else
#define SNAP_RHI_SETENV_IF_ABSENT(name, value)                                                                         \
    do {                                                                                                               \
        if (!std::getenv(name))                                                                                        \
            setenv(name, value, 0);                                                                                    \
    } while (0)
#endif

static void ensureVulkanManifests() {
#if SNAP_RHI_OS_ANDROID()
    // Android's Vulkan loader ignores VK_LAYER_PATH and JSON manifests.
    // Layers are discovered from the app's native lib directory (lib/<abi>/)
    // which is only searched for debuggable APKs (android:debuggable=true).
    // Nothing to set here — the VVL .so must be bundled into the APK at
    // build time (see GLAD/Src/Vulkan/CMakeLists.txt POST_BUILD copy).
    SNAP_RHI_LOGI("[Vulkan][Device] Android: layer discovery via app native lib dir (VK_LAYER_PATH not used)");
#elif SNAP_RHI_OS_IOS()
    // iOS uses either statically-linked MoltenVK or embedded .framework
    // bundles inside the app.  The Vulkan Loader's env-var-based ICD/layer
    // discovery (VK_ICD_FILENAMES, VK_LAYER_PATH) is not applicable here —
    // those paths would reference the build machine, not the device.
    SNAP_RHI_LOGI("[Vulkan][Device] iOS: MoltenVK via static link or embedded framework (env vars not used)");
#else
    // Desktop platforms (macOS, Linux, Windows): point the Vulkan Loader at
    // build-time generated manifests via environment variables.
#ifdef SNAP_RHI_VK_ICD_MANIFEST_PATH
    if (!std::getenv("VK_ICD_FILENAMES") && !std::getenv("VK_DRIVER_FILES")) {
        SNAP_RHI_SETENV_IF_ABSENT("VK_ICD_FILENAMES", SNAP_RHI_VK_ICD_MANIFEST_PATH);
        SNAP_RHI_SETENV_IF_ABSENT("VK_DRIVER_FILES", SNAP_RHI_VK_ICD_MANIFEST_PATH);
        SNAP_RHI_LOGI("[Vulkan][Device] VK_ICD_FILENAMES -> %s", SNAP_RHI_VK_ICD_MANIFEST_PATH);
    }
#endif
#ifdef SNAP_RHI_VK_LAYER_MANIFEST_DIR
    if (!std::getenv("VK_LAYER_PATH")) {
        SNAP_RHI_SETENV_IF_ABSENT("VK_LAYER_PATH", SNAP_RHI_VK_LAYER_MANIFEST_DIR);
        SNAP_RHI_LOGI("[Vulkan][Device] VK_LAYER_PATH -> %s", SNAP_RHI_VK_LAYER_MANIFEST_DIR);
    }
#endif
#endif // platform selection
}

#undef SNAP_RHI_SETENV_IF_ABSENT

} // unnamed namespace

namespace snap::rhi::backend::vulkan {
Device::Device(const snap::rhi::backend::vulkan::DeviceCreateInfo& info)
    : snap::rhi::backend::common::DeviceContextless(info), vkDeviceCreateinfo(info) {
    ensureVulkanManifests();
#if SNAP_RHI_VULKAN_DYNAMIC_LOADING()
    SNAP_RHI_LOGI("[Vulkan][Device] Loading Vulkan entry points via GLAD...");
    int gladVersion = gladLoaderInitSafe();
    if (gladVersion == 0) {
        snap::rhi::common::throwException(
            "[Vulkan::Device] Failed to load Vulkan entry points (gladLoaderInitSafe returned 0). "
            "Ensure a Vulkan driver (e.g. MoltenVK on Apple platforms) is available.");
    }
    SNAP_RHI_LOGI(
        "[Vulkan][Device] GLAD loaded Vulkan %d.%d", GLAD_VERSION_MAJOR(gladVersion), GLAD_VERSION_MINOR(gladVersion));
#endif // SNAP_RHI_VULKAN_DYNAMIC_LOADING()

    initInstance(info.enabledInstanceExtensions);
    choosePhysicalDevice();
    initLogicalDevice(info.enabledDeviceExtensions);

#if SNAP_RHI_VULKAN_DYNAMIC_LOADING()
    SNAP_RHI_LOGI("[Vulkan][Device] Calling gladLoaderLoadVulkan...");
    gladLoaderLoadVulkan(instance, physicalDevice, device);
    SNAP_RHI_LOGI("[Vulkan][Device] gladLoaderLoadVulkan completed");
    initValidationLayerMessages();
    SNAP_RHI_LOGI("[Vulkan][Device] initValidationLayerMessages completed");
#endif // SNAP_RHI_VULKAN_DYNAMIC_LOADING()

    SNAP_RHI_LOGI("[Vulkan][Device] Calling initMemoryProperties...");
    initMemoryProperties();
    SNAP_RHI_LOGI("[Vulkan][Device] Calling initCapabilities...");
    initCapabilities();
    SNAP_RHI_LOGI("[Vulkan][Device] Calling initCommandQueues...");
    initCommandQueues();

    SNAP_RHI_LOGI("[Vulkan][Device] Calling reportCapabilities...");
    snap::rhi::backend::common::reportCapabilities(caps, validationLayer);
    SNAP_RHI_LOGI("[Vulkan][Device] Creating fencePool...");
    fencePool = std::make_unique<common::FencePool>(
        [this](const snap::rhi::FenceCreateInfo& info) {
            return this->createResourceNoDeviceRetain<snap::rhi::Fence, snap::rhi::backend::vulkan::Fence>(this, info);
        },
        validationLayer);
    SNAP_RHI_LOGI("[Vulkan][Device] Creating submissionTracker...");
    submissionTracker =
        std::make_unique<common::SubmissionTracker>(this, [this](const snap::rhi::DeviceContextCreateInfo& info) {
            return this->createResourceNoDeviceRetain<snap::rhi::DeviceContext,
                                                      snap::rhi::backend::common::DeviceContextless::DeviceContextNoOp>(
                this, info);
        });
    SNAP_RHI_LOGI("[Vulkan][Device] Device initialization complete!");
}

Device::~Device() noexcept(false) {
    commandQueues.clear();
    submissionTracker.reset();
    fencePool.reset();
    mainMessenger.reset();

    vkDestroyDevice(device, nullptr);
    physicalDevice = VK_NULL_HANDLE;
    vkDestroyInstance(instance, nullptr);
}

void Device::initInstance(std::span<const char* const> additionalInstanceExtensions) {
    // ==========================================================================
    // MARK: - Application Info Setup
    // ==========================================================================

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Snapchat";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "SnapRHI";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Query the maximum supported instance version
    uint32_t vulkanAPIVersion = VK_MAKE_VERSION(1, 0, 0);
    VkResult result = vkEnumerateInstanceVersion(&vulkanAPIVersion);
    if (result == VK_SUCCESS && vulkanAPIVersion > 0) {
        appInfo.apiVersion = VK_MAKE_VERSION(VK_VERSION_MAJOR(vulkanAPIVersion), VK_VERSION_MINOR(vulkanAPIVersion), 0);
    }

    // ==========================================================================
    // MARK: - API Version Selection
    // ==========================================================================

    preferredAPIVersion = 0;
    switch (vkDeviceCreateinfo.apiVersion) {
        case snap::rhi::backend::vulkan::APIVersion::Vulkan_1_0:
            preferredAPIVersion = VK_API_VERSION_1_0;
            break;
        case snap::rhi::backend::vulkan::APIVersion::Vulkan_1_1:
            preferredAPIVersion = VK_API_VERSION_1_1;
            break;
        case snap::rhi::backend::vulkan::APIVersion::Vulkan_1_2:
            preferredAPIVersion = VK_API_VERSION_1_2;
            break;
        case snap::rhi::backend::vulkan::APIVersion::Vulkan_1_3:
            preferredAPIVersion = VK_API_VERSION_1_3;
            break;
        case snap::rhi::backend::vulkan::APIVersion::Vulkan_1_4:
            preferredAPIVersion = VK_API_VERSION_1_4;
            break;
        default:
            preferredAPIVersion = VK_API_VERSION_1_0;
    }

    if (appInfo.apiVersion >= preferredAPIVersion) {
        appInfo.apiVersion = preferredAPIVersion;
    } else {
        snap::rhi::common::throwException("[Vulkan::Device] Unsupported Vulkan API version");
    }

    SNAP_RHI_LOGI("[Vulkan][Device][VkInstance] Target API version: %u.%u.%u",
                  VK_API_VERSION_MAJOR(appInfo.apiVersion),
                  VK_API_VERSION_MINOR(appInfo.apiVersion),
                  VK_API_VERSION_PATCH(appInfo.apiVersion));

    // ==========================================================================
    // MARK: - Instance Create Info Setup
    // ==========================================================================

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    // ==========================================================================
    // MARK: - Layer Configuration
    // ==========================================================================

    std::vector<VkLayerProperties> layerProperties = getInstanceLayerProperties(validationLayer);

    SNAP_RHI_LOGI("[Vulkan][Device] Available instance layers (%zu):", layerProperties.size());
    for (const auto& layer : layerProperties) {
        SNAP_RHI_LOGI("[Vulkan][Device]   %s (v%u.%u.%u, impl %u)",
                      layer.layerName,
                      VK_API_VERSION_MAJOR(layer.specVersion),
                      VK_API_VERSION_MINOR(layer.specVersion),
                      VK_API_VERSION_PATCH(layer.specVersion),
                      layer.implementationVersion);
    }

    std::vector<const char*> enabledLayers = prepareLayerPropertiesList(layerProperties, InstanceLayerNames);

    if (enabledLayers.empty()) {
        SNAP_RHI_LOGW("[Vulkan][Device] No matching validation layers found — "
                      "VVL may not be installed or discoverable on this platform");
    } else {
        for (const auto* layerName : enabledLayers) {
            SNAP_RHI_LOGI("[Vulkan][Device] Enabling layer: %s", layerName);
        }
    }

    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();

    // ==========================================================================
    // MARK: - Extension Configuration (Using ExtensionManager)
    // ==========================================================================

    std::vector<VkExtensionProperties> extensionProperties = getSupportedInstanceExtensions(layerProperties);

    // Build extensions using the new manager (handles dependencies, promotions, platform requirements)
    this->enabledInstanceExtensions =
        ExtensionManager::buildInstanceExtensions(extensionProperties, appInfo.apiVersion, validationLayer);

    // Add any additional extensions passed by the caller
    for (const auto* ext : additionalInstanceExtensions) {
        if (ExtensionManager::isExtensionSupported(ext, extensionProperties)) {
            this->enabledInstanceExtensions.push_back(ext);
            SNAP_RHI_LOGI("[Vulkan][Extensions] Enabling additional instance extension %s", ext);
        } else {
            SNAP_RHI_LOGW("[Vulkan][Extensions] Additional instance extension %s is not supported", ext);
        }
    }

#if SNAP_RHI_OS_APPLE() && SNAP_RHI_VULKAN_DYNAMIC_LOADING()
    // When using the Vulkan Loader on Apple, portability enumeration is required
    // so the loader includes MoltenVK (a portability driver) in device enumeration.
    // Not needed when MoltenVK is linked statically (no loader in the path).
    createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif

    createInfo.enabledExtensionCount = static_cast<uint32_t>(this->enabledInstanceExtensions.size());
    createInfo.ppEnabledExtensionNames = this->enabledInstanceExtensions.data();

    // ==========================================================================
    // MARK: - Instance Creation
    // ==========================================================================

    result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        snap::rhi::common::throwException(
            "[Vulkan::Device] vkCreateInstance failed (VkResult=" + std::to_string(static_cast<int>(result)) +
            "). On Apple platforms, ensure the MoltenVK ICD is discoverable "
            "by the Vulkan Loader (set VK_ICD_FILENAMES or VK_DRIVER_FILES).");
    }

    SNAP_RHI_LOGI("[Vulkan][Device] Instance created with %zu extensions enabled",
                  this->enabledInstanceExtensions.size());
}

void Device::initValidationLayerMessages() {
    VkResult result = VK_ERROR_UNKNOWN;
    SNAP_RHI_ON_SCOPE_EXIT {
        SNAP_RHI_LOGI("[Vulkan][Device] Vulkan validation layers: %s", result == VK_SUCCESS ? "ENABLED" : "DISABLED");
    };

    bool debugUtilsEnabled = false;
#if SNAP_RHI_VULKAN_NATIVE_VALIDATION_LAYERS
    for (const auto extName : enabledInstanceExtensions) {
        if (std::string_view(VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == std::string_view(extName)) {
            debugUtilsEnabled = true;
            break;
        }
    }
#endif

    if (!debugUtilsEnabled) {
        return;
    }

    // Guard against null function pointers — the Vulkan Loader on some
    // platforms (e.g. Android) may not resolve the debug-utils entry points
    // even when the extension name is reported as available.
    if (!vkCreateDebugUtilsMessengerEXT) {
        SNAP_RHI_LOGI("[Vulkan][Device] vkCreateDebugUtilsMessengerEXT not loaded — skipping");
        return;
    }

    mainMessenger = std::shared_ptr<VkDebugUtilsMessengerEXT>(
        new VkDebugUtilsMessengerEXT(), [instance = instance](VkDebugUtilsMessengerEXT* ptr) {
            if (!ptr) {
                return;
            }

            if (vkDestroyDebugUtilsMessengerEXT) {
                vkDestroyDebugUtilsMessengerEXT(instance, *ptr, nullptr);
            }

            delete ptr;
        });

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = nullptr,
        .flags = 0,
        .messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                       VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = mainDebugMessageCallback,
        .pUserData = &validationLayer};

    result = vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, nullptr, mainMessenger.get());
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] cannot create debug utils messenger");
}

void Device::choosePhysicalDevice() {
    std::vector<VkPhysicalDevice> devices = getPhysicalDevices(instance, validationLayer);
    SNAP_RHI_VALIDATE(validationLayer,
                      !devices.empty(),
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] no physical devices available");

    std::array<VkPhysicalDeviceType, 4> deviceTypeCreateOrder{VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
                                                              VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
                                                              VK_PHYSICAL_DEVICE_TYPE_CPU,
                                                              VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU};

    for (VkPhysicalDeviceType deviceType : deviceTypeCreateOrder) {
        physicalDevice = choosePhysicalDeviceByType(
            devices, deviceType, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);
        if (physicalDevice) {
            SNAP_RHI_LOGI("[Vulkan][Device][choosePhysicalDevice] deviceType: %d", static_cast<uint32_t>(deviceType));
            break;
        }
    }

    SNAP_RHI_VALIDATE(validationLayer,
                      physicalDevice != nullptr,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] cannot find physical device");

    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

#if SNAP_RHI_OS_ANDROID()
    if (preferredAPIVersion >= VK_API_VERSION_1_1) { // Check sync fd import/export support
        VkPhysicalDeviceExternalSemaphoreInfo externalSemaphoreInfo{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTERNAL_SEMAPHORE_INFO,
            .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT};

        vkGetPhysicalDeviceExternalSemaphoreProperties(
            physicalDevice, &externalSemaphoreInfo, &externalSemaphoreProperties);
    }
#endif

    physicalDeviceProperties.apiVersion = std::min(preferredAPIVersion, physicalDeviceProperties.apiVersion);

    SNAP_RHI_LOGI("[Vulkan][Device] Physical device API version: %u.%u.%u",
                  VK_VERSION_MAJOR(physicalDeviceProperties.apiVersion),
                  VK_VERSION_MINOR(physicalDeviceProperties.apiVersion),
                  VK_VERSION_PATCH(physicalDeviceProperties.apiVersion));
    if (preferredAPIVersion > physicalDeviceProperties.apiVersion) {
        snap::rhi::common::throwException(
            "[Vulkan::Device] Physical device does not support the preferred API version");
    }

    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_2) {
        physicalDevice_1_3_Features = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
                                       .pNext = nullptr};

        physicalDevice_1_2_Features = {
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
            .pNext = physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3 ? &physicalDevice_1_3_Features : nullptr,
        };

#if SNAP_RHI_OS_APPLE()
        {
            /**
             * For VK_KHR_portability_subset should be used VkPhysicalDevicePortabilitySubsetFeaturesKHR
             * in order to configureate features propery.
             */
            auto* features_1_2_pNext = physicalDevice_1_2_Features.pNext;
            portabilitySubsetFeatures = {
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR,
                .pNext = nullptr,
            };
            physicalDevice_1_2_Features.pNext = &portabilitySubsetFeatures;
            portabilitySubsetFeatures.pNext = features_1_2_pNext;
        }
#endif

        vkGetPhysicalDeviceFeatures2(physicalDevice, &physicalDevice_1_2_Features);
    }

    logPhysicalDeviceFeatures();

    queueFamilyProperties = getQueueFamilyProperties(physicalDevice);
    platformDeviceString = physicalDeviceProperties.deviceName;
}

void Device::initLogicalDevice(std::span<const char* const> additionalDeviceExtensions) {
    // ==========================================================================
    // MARK: - Physical Device Features Setup
    // ==========================================================================

    auto features = preparePhysicalDeviceFeatures(physicalDeviceFeatures);
    auto features_1_3 = preparePhysicalDeviceFeatures_1_3(physicalDevice_1_3_Features);

    // ==========================================================================
    // MARK: - Queue Configuration
    // ==========================================================================

    queueCreateInfos = prepareDeviceQueueCreateInfo(
        queueFamilyProperties, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

    // ==========================================================================
    // MARK: - Layer Configuration
    // ==========================================================================

    std::vector<VkLayerProperties> layerProperties = getDeviceLayerProperties(physicalDevice, validationLayer);
    std::vector<const char*> enabledLayers = prepareLayerPropertiesList(layerProperties, deviceLayersName);

    // ==========================================================================
    // MARK: - Extension Configuration (Using ExtensionManager)
    // ==========================================================================

    std::vector<VkExtensionProperties> extensionProperties =
        getSupportedDeviceExtensions(physicalDevice, layerProperties);

    // Build extensions using the new manager (handles dependencies, promotions, platform requirements)
    std::vector<const char*> enabledExtensions = ExtensionManager::buildDeviceExtensions(
        extensionProperties, physicalDeviceProperties.apiVersion, validationLayer);

    // Store enabled extensions for later queries
    this->enabledDeviceExtensions = enabledExtensions;

    // Add any additional extensions passed by the caller
    for (const auto* ext : additionalDeviceExtensions) {
        if (ExtensionManager::isExtensionSupported(ext, extensionProperties)) {
            enabledExtensions.push_back(ext);
            this->enabledDeviceExtensions.push_back(ext);
            SNAP_RHI_LOGI("[Vulkan][Extensions] Enabling additional device extension %s", ext);
        } else {
            SNAP_RHI_LOGW("[Vulkan][Extensions] Additional device extension %s is not supported", ext);
        }
    }

    // ==========================================================================
    // MARK: - Feature Chain Setup (pNext chain)
    // ==========================================================================

    void* pNext = nullptr;

    // Dynamic rendering support
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES,
        .pNext = nullptr,
        .dynamicRendering = VK_TRUE,
    };

    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3) {
        // Dynamic rendering is core in Vulkan 1.3
        pNext = &features_1_3;
        caps.isDynamicRenderingSupported = true;
        SNAP_RHI_LOGI("[Vulkan][Device] Dynamic rendering available (core in Vulkan 1.3)");
    } else if (ExtensionManager::isExtensionEnabled(
                   VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME, enabledExtensions, physicalDeviceProperties.apiVersion)) {
        // Enable via extension for Vulkan 1.2
        pNext = &dynamicRenderingFeatures;
        caps.isDynamicRenderingSupported = true;
        SNAP_RHI_LOGI("[Vulkan][Device] Dynamic rendering available (via extension)");
    } else {
        caps.isDynamicRenderingSupported = false;
        SNAP_RHI_LOGW("[Vulkan][Device] Dynamic rendering not available");
    }

#if SNAP_RHI_OS_APPLE()
    // ==========================================================================
    // MARK: - Portability Subset Features (Apple/MoltenVK)
    // ==========================================================================

    VkPhysicalDevicePortabilitySubsetFeaturesKHR enabledPortabilitySubsetFeatures{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PORTABILITY_SUBSET_FEATURES_KHR,
        .pNext = nullptr,
    };

    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_2) {
        enabledPortabilitySubsetFeatures.constantAlphaColorBlendFactors =
            portabilitySubsetFeatures.constantAlphaColorBlendFactors;
        enabledPortabilitySubsetFeatures.imageViewFormatSwizzle = portabilitySubsetFeatures.imageViewFormatSwizzle;
        enabledPortabilitySubsetFeatures.imageView2DOn3DImage = portabilitySubsetFeatures.imageView2DOn3DImage;

        // Insert into pNext chain
        enabledPortabilitySubsetFeatures.pNext = pNext;
        pNext = &enabledPortabilitySubsetFeatures;

        SNAP_RHI_LOGI("[Vulkan][Device] Portability subset features enabled for MoltenVK");
    }
#endif

    // ==========================================================================
    // MARK: - Device Creation
    // ==========================================================================

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pNext = pNext;
    createInfo.flags = 0;
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(enabledLayers.size());
    createInfo.ppEnabledLayerNames = enabledLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensions.size());
    createInfo.ppEnabledExtensionNames = enabledExtensions.data();
    createInfo.pEnabledFeatures = &features;

    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::CriticalError,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] cannot create logical device");
}

void Device::initCapabilities() {
    // ==========================================================================
    // MARK: - Core Feature Flags
    // ==========================================================================
    // These features are universally supported by Vulkan implementations

    caps.isNPOTGenerateMipmapCmdSupported = true;
    caps.isAlphaToCoverageSupported = true;
    caps.isClampToZeroSupported = false; // Vulkan uses VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER with zero border
    caps.isClampToBorderSupported = true;
    caps.isUInt32IndexSupported = true;
    caps.isMinMaxBlendModeSupported = true;
    caps.isNPOTWrapModeSupported = true;
    caps.isRasterizerDisableSupported = true;
    caps.isRenderToMipmapSupported = true;
    caps.isTexture3DSupported = true;
    caps.isTextureArraySupported = true;
    caps.isPolygonOffsetClampSupported = true;
    caps.isSamplerMinMaxLodSupported = true;
    caps.isSamplerCompareFuncSupported = true;
    caps.isDepthCubeMapSupported = true;
    caps.isShaderStorageBufferSupported = true;
    caps.isNonFillPolygonModeSupported = true;
    caps.isTextureFormatSwizzleSupported = true;
    caps.isSamplerUnnormalizedCoordsSupported = true;
    caps.isVertexInputRatePerInstanceSupported = true;
    caps.supportsPersistentMapping = true; // Vulkan mapping pointers remain valid while memory stays mapped

    /**
     * Primitive restart functionality is enabled with the largest unsigned integer index value:
     * - VK_INDEX_TYPE_UINT32: 0xFFFFFFFF
     * - VK_INDEX_TYPE_UINT16: 0xFFFF
     * - VK_INDEX_TYPE_UINT8_KHR: 0xFF
     * Reference:
     * https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPipelineInputAssemblyStateCreateInfo.html
     */
    caps.isPrimitiveRestartIndexEnabled = true;

    // Framebuffer fetch is not natively supported in Vulkan
    // Reference:
    // https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/framebuffer-fetch-in-vulkan
    caps.isFramebufferFetchSupported = false;

    // All data types supported for constant input rate
    caps.constantInputRateSupportingBits = FormatDataTypeBits::All;

    // ==========================================================================
    // MARK: - Texture Dimension Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================
    // Reference: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceLimits.html

    caps.maxTextureArrayLayers = physicalDeviceProperties.limits.maxImageArrayLayers;
    caps.maxTextureDimension2D = physicalDeviceProperties.limits.maxImageDimension2D;
    caps.maxTextureDimension3D = physicalDeviceProperties.limits.maxImageDimension3D;
    caps.maxTextureDimensionCube = physicalDeviceProperties.limits.maxImageDimensionCube;

    // ==========================================================================
    // MARK: - Framebuffer Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================

    caps.maxFramebufferLayers = physicalDeviceProperties.limits.maxFramebufferLayers;
    caps.maxFramebufferColorAttachmentCount = physicalDeviceProperties.limits.maxColorAttachments;

    // Multiview not currently implemented
    caps.maxMultiviewViewCount = 0;
    caps.isMultiviewMSAAImplicitResolveEnabled = false;

    // ==========================================================================
    // MARK: - Clip/Cull Distance Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================

    caps.maxClipDistances = physicalDeviceProperties.limits.maxClipDistances;
    caps.maxCullDistances = physicalDeviceProperties.limits.maxCullDistances;
    caps.maxCombinedClipAndCullDistances = physicalDeviceProperties.limits.maxCombinedClipAndCullDistances;

    // ==========================================================================
    // MARK: - Uniform Buffer Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================
    // Reference: https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkPhysicalDeviceLimits.html

    caps.maxPerStageUniformBuffers = physicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers;
    caps.maxUniformBuffers = physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers;

    // Maximum uniform buffer range - use actual device limit
    const uint64_t maxUniformBufferRange = static_cast<uint64_t>(physicalDeviceProperties.limits.maxUniformBufferRange);
    caps.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] = maxUniformBufferRange;
    caps.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] = maxUniformBufferRange;
    caps.maxPerStageUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] = maxUniformBufferRange;

    // Total uniform buffer size per stage (maxUniformBufferRange * maxPerStageDescriptorUniformBuffers)
    const uint64_t totalUniformSize = maxUniformBufferRange * static_cast<uint64_t>(caps.maxPerStageUniformBuffers);

    caps.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] = totalUniformSize;
    caps.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] = totalUniformSize;
    caps.maxPerStageTotalUniformBufferSize[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] = totalUniformSize;

    // Minimum uniform buffer offset alignment
    caps.minUniformBufferOffsetAlignment =
        static_cast<uint64_t>(physicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    // ==========================================================================
    // MARK: - Texture/Sampler Binding Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================

    caps.maxTextures = physicalDeviceProperties.limits.maxDescriptorSetSampledImages;
    caps.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages;
    caps.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages;
    caps.maxPerStageTextures[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSampledImages;

    caps.maxSamplers = physicalDeviceProperties.limits.maxDescriptorSetSamplers;
    caps.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Vertex)] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSamplers;
    caps.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Fragment)] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSamplers;
    caps.maxPerStageSamplers[static_cast<uint32_t>(snap::rhi::ShaderStage::Compute)] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSamplers;

    // ==========================================================================
    // MARK: - Vertex Input Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================

    caps.maxVertexInputAttributes = physicalDeviceProperties.limits.maxVertexInputAttributes;
    caps.maxVertexInputBindings = physicalDeviceProperties.limits.maxVertexInputBindings;
    caps.maxVertexInputAttributeOffset = physicalDeviceProperties.limits.maxVertexInputAttributeOffset;
    caps.maxVertexInputBindingStride = physicalDeviceProperties.limits.maxVertexInputBindingStride;
    caps.maxVertexAttribDivisor = Unlimited; // VK_EXT_vertex_attribute_divisor provides unlimited divisor

    // ==========================================================================
    // MARK: - Compute Shader Limits (from VkPhysicalDeviceLimits)
    // ==========================================================================

    caps.maxComputeWorkGroupInvocations = physicalDeviceProperties.limits.maxComputeWorkGroupInvocations;

    // Query subgroup size for thread execution width
    // This represents the SIMD width of the GPU
    // Note: Vulkan 1.1+ provides subgroupSize, earlier versions need VK_EXT_subgroup_size_control
    caps.threadExecutionWidth = 32u;
    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_1) {
        VkPhysicalDeviceSubgroupProperties subgroupProperties{};
        subgroupProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_PROPERTIES;

        VkPhysicalDeviceProperties2 deviceProperties2{};
        deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
        deviceProperties2.pNext = &subgroupProperties;

        vkGetPhysicalDeviceProperties2(physicalDevice, &deviceProperties2);
        caps.threadExecutionWidth = subgroupProperties.subgroupSize;
    }

    caps.maxComputeWorkGroupCount[0] = physicalDeviceProperties.limits.maxComputeWorkGroupCount[0];
    caps.maxComputeWorkGroupCount[1] = physicalDeviceProperties.limits.maxComputeWorkGroupCount[1];
    caps.maxComputeWorkGroupCount[2] = physicalDeviceProperties.limits.maxComputeWorkGroupCount[2];

    caps.maxComputeWorkGroupSize[0] = physicalDeviceProperties.limits.maxComputeWorkGroupSize[0];
    caps.maxComputeWorkGroupSize[1] = physicalDeviceProperties.limits.maxComputeWorkGroupSize[1];
    caps.maxComputeWorkGroupSize[2] = physicalDeviceProperties.limits.maxComputeWorkGroupSize[2];

    // ==========================================================================
    // MARK: - Other Limits
    // ==========================================================================

    caps.maxSpecializationConstants = snap::rhi::SupportedLimit::MaxSpecializationConstants;

    // Anisotropic filtering - query actual device limit
    const float maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;
    if (maxAnisotropy >= 16.0f) {
        caps.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count16;
    } else if (maxAnisotropy >= 8.0f) {
        caps.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count8;
    } else if (maxAnisotropy >= 4.0f) {
        caps.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count4;
    } else if (maxAnisotropy >= 2.0f) {
        caps.maxAnisotropic = snap::rhi::AnisotropicFiltering::Count2;
    } else {
        caps.maxAnisotropic = snap::rhi::AnisotropicFiltering::None;
    }

    // Non-coherent atom size for memory flush/invalidate operations
    caps.nonCoherentAtomSize = static_cast<uint64_t>(physicalDeviceProperties.limits.nonCoherentAtomSize);

    // ==========================================================================
    // MARK: - NDC Layout and API Description
    // ==========================================================================

    caps.ndcLayout = {snap::rhi::NDCLayout::YAxis::Down, snap::rhi::NDCLayout::DepthRange::ZeroToOne};
    caps.textureConvention = snap::rhi::TextureOriginConvention::TopLeft;
    caps.apiDescription = {snap::rhi::API::Vulkan, snap::rhi::APIVersionNone};

    // ==========================================================================
    // MARK: - Queue Family Properties
    // ==========================================================================

    caps.queueFamiliesCount =
        std::min(snap::rhi::SupportedLimit::MaxQueueFamilies, static_cast<uint32_t>(queueCreateInfos.size()));
    caps.queueFamilyProperties.fill({});

    for (size_t i = 0; i < queueCreateInfos.size(); ++i) {
        const uint32_t queueFamilyIndex = queueCreateInfos[i].queueFamilyIndex;
        assert(queueFamilyProperties.size() > queueFamilyIndex);

        caps.queueFamilyProperties[i].queueCount = queueCreateInfos[i].queueCount;
        caps.queueFamilyProperties[i].queueFlags = convertTo(queueFamilyProperties[queueFamilyIndex].queueFlags);
        caps.queueFamilyProperties[i].isTimestampQuerySupported =
            getTimestampQuerySupported(getPhysicalDeviceProperties(), queueFamilyProperties[queueFamilyIndex]);
    }

    // ==========================================================================
    // MARK: - Vertex Attribute Format Support
    // ==========================================================================

    // Vulkan supports all standard vertex attribute formats
    std::ranges::fill(caps.vertexAttributeFormatProperties, true);

    // ==========================================================================
    // MARK: - Pixel Format Properties (queried per-format from vkGetPhysicalDeviceFormatProperties)
    // ==========================================================================

    caps.formatProperties.fill({});
    buildPixelFormatProperties(physicalDevice, physicalDeviceProperties, caps);

    // ==========================================================================
    // MARK: - Memory Properties (from vkGetPhysicalDeviceMemoryProperties)
    // ==========================================================================

    caps.physicalDeviceMemoryProperties = {};
    for (uint32_t i = 0; i < std::min<uint32_t>(snap::rhi::MaxMemoryTypes, memoryProperties.memoryTypeCount); ++i) {
        const VkMemoryPropertyFlags flags = memoryProperties.memoryTypes[i].propertyFlags;
        snap::rhi::MemoryProperties bits = snap::rhi::MemoryProperties::None;

        if ((flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) != 0) {
            bits |= snap::rhi::MemoryProperties::DeviceLocal;
        }
        if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0) {
            bits |= snap::rhi::MemoryProperties::HostVisible;
        }
        if ((flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0) {
            bits |= snap::rhi::MemoryProperties::HostCoherent;
        }
        if ((flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) != 0) {
            bits |= snap::rhi::MemoryProperties::HostCached;
        }
        caps.physicalDeviceMemoryProperties.memoryTypes[caps.physicalDeviceMemoryProperties.memoryTypeCount++]
            .memoryProperties = bits;
    }
}

void Device::initCommandQueues() {
    SNAP_RHI_LOGI("[Vulkan][Device] initCommandQueues: queueCreateInfos.size() = %zu", queueCreateInfos.size());
    commandQueues.resize(queueCreateInfos.size());
    for (size_t i = 0; i < queueCreateInfos.size(); ++i) {
        const auto& info = queueCreateInfos[i];
        SNAP_RHI_LOGI("[Vulkan][Device] Queue family %u has %u queues", info.queueFamilyIndex, info.queueCount);

        for (uint32_t j = 0; j < info.queueCount; ++j) {
            SNAP_RHI_LOGI("[Vulkan][Device] Creating CommandQueue for family %u, queue %u", info.queueFamilyIndex, j);
            commandQueues[i].emplace_back(
                std::make_unique<snap::rhi::backend::vulkan::CommandQueue>(this, info.queueFamilyIndex, j));
        }
    }
    SNAP_RHI_LOGI("[Vulkan][Device] initCommandQueues completed");
}

void Device::initMemoryProperties() {
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
}

VkInstance Device::getVkInstance() const {
    return instance;
}

VkPhysicalDevice Device::getVkPhysicalDevice() const {
    return physicalDevice;
}

VkDevice Device::getVkLogicalDevice() const {
    return device;
}

uint32_t Device::chooseMemoryTypeIndex(const uint32_t memoryTypeBits,
                                       const VkMemoryPropertyFlags memoryProperty) const {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        const auto& memoryTypeProperties = memoryProperties.memoryTypes[i];
        if ((memoryTypeBits & (1 << i)) && (memoryTypeProperties.propertyFlags & memoryProperty) == memoryProperty) {
            return i;
        }
    }

    SNAP_RHI_REPORT(validationLayer,
                    snap::rhi::ReportLevel::CriticalError,
                    snap::rhi::ValidationTag::CreateOp,
                    "[Vulkan::Device] cannot choose memory type");
    return std::numeric_limits<uint32_t>::max();
}

std::shared_ptr<snap::rhi::QueryPool> Device::createQueryPool(const snap::rhi::QueryPoolCreateInfo& info) {
    // TODO: implement Vulkan QueryPool
    // return createNonRetainable<snap::rhi::QueryPool, snap::rhi::backend::vulkan::QueryPool>(this, info);
    return nullptr;
}

std::shared_ptr<snap::rhi::CommandBuffer> Device::createCommandBuffer(const snap::rhi::CommandBufferCreateInfo& info) {
    return createResource<snap::rhi::CommandBuffer, snap::rhi::backend::vulkan::CommandBuffer>(this, info);
}

std::shared_ptr<snap::rhi::Sampler> Device::createSampler(const snap::rhi::SamplerCreateInfo& info) {
    return createResource<snap::rhi::Sampler, snap::rhi::backend::vulkan::Sampler>(this, info);
}

std::shared_ptr<snap::rhi::Semaphore> Device::createSemaphore(
    const snap::rhi::SemaphoreCreateInfo& info,
    const std::shared_ptr<snap::rhi::PlatformSyncHandle>& platformSyncHandle) {
    if (platformSyncHandle) {
#if SNAP_RHI_OS_ANDROID()
        auto* androidHandle =
            dynamic_cast<snap::rhi::backend::common::platform::android::SyncHandle*>(platformSyncHandle.get());
        if (androidHandle && isExternalSemaphoreSupported()) {
            int32_t fenceFd = androidHandle->releaseFenceFd();
            if (fenceFd >= 0) {
                return createResource<snap::rhi::Semaphore, snap::rhi::backend::vulkan::platform::android::Semaphore>(
                    this, info, fenceFd);
            }
        }
#endif
        auto* commonHandle = snap::rhi::backend::common::smart_cast<snap::rhi::backend::common::PlatformSyncHandle>(
            platformSyncHandle.get());
        if (const auto& fence = commonHandle->getFence(); fence) {
            fence->waitForComplete();
        }
    }
    return createResource<snap::rhi::Semaphore, snap::rhi::backend::vulkan::Semaphore>(this, info);
}

std::shared_ptr<snap::rhi::Fence> Device::createFence(const snap::rhi::FenceCreateInfo& info) {
    return createResource<snap::rhi::Fence, snap::rhi::backend::vulkan::Fence>(this, info);
}

std::shared_ptr<snap::rhi::Buffer> Device::createBuffer(const snap::rhi::BufferCreateInfo& info) {
    return createResource<snap::rhi::Buffer, snap::rhi::backend::vulkan::Buffer>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::vulkan::Texture>(this, info);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info,
                                                          void* imageCreateInfoNext) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::vulkan::Texture>(this, info, imageCreateInfoNext);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const std::shared_ptr<TextureInterop>& textureInterop) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::vulkan::Texture>(this, textureInterop);
}

std::shared_ptr<snap::rhi::Texture> Device::createTexture(const snap::rhi::TextureCreateInfo& info,
                                                          VkImage image,
                                                          bool isOwner,
                                                          VkImageLayout defaultLayout) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::vulkan::Texture>(
        this, info, image, isOwner, defaultLayout);
}

std::shared_ptr<snap::rhi::Texture> Device::createTextureView(const snap::rhi::TextureViewCreateInfo& info) {
    return createResource<snap::rhi::Texture, snap::rhi::backend::vulkan::Texture>(this, info);
}

std::shared_ptr<snap::rhi::RenderPass> Device::createRenderPass(const RenderPassCreateInfo& info) {
    return createResource<snap::rhi::RenderPass, snap::rhi::backend::vulkan::RenderPass>(this, info);
}

std::shared_ptr<snap::rhi::Framebuffer> Device::createFramebuffer(const snap::rhi::FramebufferCreateInfo& info) {
    return createResource<snap::rhi::Framebuffer, snap::rhi::backend::vulkan::Framebuffer>(this, info);
}

std::shared_ptr<snap::rhi::ShaderLibrary> Device::createShaderLibrary(const snap::rhi::ShaderLibraryCreateInfo& info) {
    return createResource<snap::rhi::ShaderLibrary, snap::rhi::backend::vulkan::ShaderLibrary>(this, info);
}

std::shared_ptr<snap::rhi::ShaderModule> Device::createShaderModule(const snap::rhi::ShaderModuleCreateInfo& info) {
    return createResource<snap::rhi::ShaderModule, snap::rhi::backend::vulkan::ShaderModule>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSetLayout> Device::createDescriptorSetLayout(
    const snap::rhi::DescriptorSetLayoutCreateInfo& info) {
    return createResource<snap::rhi::DescriptorSetLayout, snap::rhi::backend::vulkan::DescriptorSetLayout>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorPool> Device::createDescriptorPool(
    const snap::rhi::DescriptorPoolCreateInfo& info) {
    return createResource<snap::rhi::DescriptorPool, snap::rhi::backend::vulkan::DescriptorPool>(this, info);
}

std::shared_ptr<snap::rhi::DescriptorSet> Device::createDescriptorSet(const snap::rhi::DescriptorSetCreateInfo& info) {
    auto descriptorSet =
        createResource<snap::rhi::DescriptorSet, snap::rhi::backend::vulkan::DescriptorSet>(this, info);
    auto* descriptorSetImpl =
        snap::rhi::backend::common::smart_cast<snap::rhi::backend::vulkan::DescriptorSet>(descriptorSet.get());
    if (descriptorSetImpl->getVkDescriptorSet() == VK_NULL_HANDLE) {
        return nullptr;
    }

    return descriptorSet;
}

std::shared_ptr<snap::rhi::PipelineLayout> Device::createPipelineLayout(
    const snap::rhi::PipelineLayoutCreateInfo& info) {
    return createResource<snap::rhi::PipelineLayout, snap::rhi::backend::vulkan::PipelineLayout>(this, info);
}

std::shared_ptr<snap::rhi::PipelineCache> Device::createPipelineCache(const snap::rhi::PipelineCacheCreateInfo& info) {
    return createResource<snap::rhi::PipelineCache, snap::rhi::backend::vulkan::PipelineCache>(this, info);
}

std::shared_ptr<snap::rhi::RenderPipeline> Device::createRenderPipeline(
    const snap::rhi::RenderPipelineCreateInfo& info) {
    return createResource<snap::rhi::RenderPipeline, snap::rhi::backend::vulkan::RenderPipeline>(this, info);
}

std::shared_ptr<snap::rhi::ComputePipeline> Device::createComputePipeline(
    const snap::rhi::ComputePipelineCreateInfo& info) {
    return createResource<snap::rhi::ComputePipeline, snap::rhi::backend::vulkan::ComputePipeline>(this, info);
}

std::shared_ptr<snap::rhi::DebugMessenger> Device::createDebugMessenger(snap::rhi::DebugMessengerCreateInfo&& info) {
    return nullptr;
}

TextureFormatProperties Device::getTextureFormatProperties(const TextureFormatInfo& info) {
    const auto format = toVkFormat(info.format);
    const auto type = toVkImageType(info.type);
    constexpr VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
    const auto usage = toVkImageUsageFlags(info.usage);
    const auto flag = toVkImageCreateFlags(info.type);

    VkImageFormatProperties imageFormatProperties{};

    auto result = vkGetPhysicalDeviceImageFormatProperties(
        physicalDevice, format, type, tiling, usage, flag, &imageFormatProperties);
    SNAP_RHI_VALIDATE(validationLayer,
                      result == VK_SUCCESS,
                      snap::rhi::ReportLevel::Error,
                      snap::rhi::ValidationTag::CreateOp,
                      "[Vulkan::Device] cannot get image format properties");
    return {
        .maxExtent =
            {
                imageFormatProperties.maxExtent.width,
                imageFormatProperties.maxExtent.height,
                imageFormatProperties.maxExtent.depth,
            },
        .maxMipLevels = imageFormatProperties.maxMipLevels,
        .maxArrayLayers = imageFormatProperties.maxArrayLayers,
        .sampleCounts = static_cast<snap::rhi::SampleCount>(imageFormatProperties.sampleCounts),
        /*
         * maxResourceSize must be at least 2^31
         *
         * https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageFormatProperties.html
         **/
        .maxResourceSize = std::max(static_cast<uint64_t>(1ull << 31), imageFormatProperties.maxResourceSize),
    };
}

uint64_t Device::getCPUMemoryUsage() const {
    return 0;
}

uint64_t Device::getGPUMemoryUsage() const {
    return 0;
}

bool Device::isExternalSemaphoreSupported() const {
    return (externalSemaphoreProperties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_EXPORTABLE_BIT) &&
           (externalSemaphoreProperties.externalSemaphoreFeatures & VK_EXTERNAL_SEMAPHORE_FEATURE_IMPORTABLE_BIT);
}

void Device::logPhysicalDeviceFeatures() const {
    SNAP_RHI_LOGI("[Vulkan][Device][Features] robustBufferAccess=%u", physicalDeviceFeatures.robustBufferAccess);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] fullDrawIndexUint32=%u", physicalDeviceFeatures.fullDrawIndexUint32);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] imageCubeArray=%u", physicalDeviceFeatures.imageCubeArray);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] independentBlend=%u", physicalDeviceFeatures.independentBlend);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] geometryShader=%u", physicalDeviceFeatures.geometryShader);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] tessellationShader=%u", physicalDeviceFeatures.tessellationShader);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sampleRateShading=%u", physicalDeviceFeatures.sampleRateShading);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] dualSrcBlend=%u", physicalDeviceFeatures.dualSrcBlend);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] logicOp=%u", physicalDeviceFeatures.logicOp);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] multiDrawIndirect=%u", physicalDeviceFeatures.multiDrawIndirect);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] drawIndirectFirstInstance=%u",
                  physicalDeviceFeatures.drawIndirectFirstInstance);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] depthClamp=%u", physicalDeviceFeatures.depthClamp);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] depthBiasClamp=%u", physicalDeviceFeatures.depthBiasClamp);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] fillModeNonSolid=%u", physicalDeviceFeatures.fillModeNonSolid);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] depthBounds=%u", physicalDeviceFeatures.depthBounds);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] wideLines=%u", physicalDeviceFeatures.wideLines);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] largePoints=%u", physicalDeviceFeatures.largePoints);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] alphaToOne=%u", physicalDeviceFeatures.alphaToOne);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] multiViewport=%u", physicalDeviceFeatures.multiViewport);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] samplerAnisotropy=%u", physicalDeviceFeatures.samplerAnisotropy);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] textureCompressionETC2=%u",
                  physicalDeviceFeatures.textureCompressionETC2);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] textureCompressionASTC_LDR=%u",
                  physicalDeviceFeatures.textureCompressionASTC_LDR);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] textureCompressionBC=%u", physicalDeviceFeatures.textureCompressionBC);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] occlusionQueryPrecise=%u", physicalDeviceFeatures.occlusionQueryPrecise);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] pipelineStatisticsQuery=%u",
                  physicalDeviceFeatures.pipelineStatisticsQuery);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] vertexPipelineStoresAndAtomics=%u",
                  physicalDeviceFeatures.vertexPipelineStoresAndAtomics);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] fragmentStoresAndAtomics=%u",
                  physicalDeviceFeatures.fragmentStoresAndAtomics);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderTessellationAndGeometryPointSize=%u",
                  physicalDeviceFeatures.shaderTessellationAndGeometryPointSize);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderImageGatherExtended=%u",
                  physicalDeviceFeatures.shaderImageGatherExtended);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderStorageImageExtendedFormats=%u",
                  physicalDeviceFeatures.shaderStorageImageExtendedFormats);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderStorageImageMultisample=%u",
                  physicalDeviceFeatures.shaderStorageImageMultisample);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderStorageImageReadWithoutFormat=%u",
                  physicalDeviceFeatures.shaderStorageImageReadWithoutFormat);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderStorageImageWriteWithoutFormat=%u",
                  physicalDeviceFeatures.shaderStorageImageWriteWithoutFormat);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderUniformBufferArrayDynamicIndexing=%u",
                  physicalDeviceFeatures.shaderUniformBufferArrayDynamicIndexing);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderSampledImageArrayDynamicIndexing=%u",
                  physicalDeviceFeatures.shaderSampledImageArrayDynamicIndexing);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderStorageBufferArrayDynamicIndexing=%u",
                  physicalDeviceFeatures.shaderStorageBufferArrayDynamicIndexing);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderStorageImageArrayDynamicIndexing=%u",
                  physicalDeviceFeatures.shaderStorageImageArrayDynamicIndexing);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderClipDistance=%u", physicalDeviceFeatures.shaderClipDistance);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderCullDistance=%u", physicalDeviceFeatures.shaderCullDistance);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderFloat64=%u", physicalDeviceFeatures.shaderFloat64);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderInt64=%u", physicalDeviceFeatures.shaderInt64);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderInt16=%u", physicalDeviceFeatures.shaderInt16);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderResourceResidency=%u",
                  physicalDeviceFeatures.shaderResourceResidency);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] shaderResourceMinLod=%u", physicalDeviceFeatures.shaderResourceMinLod);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseBinding=%u", physicalDeviceFeatures.sparseBinding);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidencyBuffer=%u", physicalDeviceFeatures.sparseResidencyBuffer);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidencyImage2D=%u",
                  physicalDeviceFeatures.sparseResidencyImage2D);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidencyImage3D=%u",
                  physicalDeviceFeatures.sparseResidencyImage3D);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidency2Samples=%u",
                  physicalDeviceFeatures.sparseResidency2Samples);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidency4Samples=%u",
                  physicalDeviceFeatures.sparseResidency4Samples);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidency8Samples=%u",
                  physicalDeviceFeatures.sparseResidency8Samples);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidency16Samples=%u",
                  physicalDeviceFeatures.sparseResidency16Samples);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] sparseResidencyAliased=%u",
                  physicalDeviceFeatures.sparseResidencyAliased);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] variableMultisampleRate=%u",
                  physicalDeviceFeatures.variableMultisampleRate);
    SNAP_RHI_LOGI("[Vulkan][Device][Features] inheritedQueries=%u", physicalDeviceFeatures.inheritedQueries);

    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_3) {
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] dynamicRendering=%u",
                      physicalDevice_1_3_Features.dynamicRendering);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] shaderIntegerDotProduct=%u",
                      physicalDevice_1_3_Features.shaderIntegerDotProduct);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] maintenance4=%u", physicalDevice_1_3_Features.maintenance4);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] pipelineCreationCacheControl=%u",
                      physicalDevice_1_3_Features.pipelineCreationCacheControl);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] privateData=%u", physicalDevice_1_3_Features.privateData);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] shaderDemoteToHelperInvocation=%u",
                      physicalDevice_1_3_Features.shaderDemoteToHelperInvocation);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] shaderTerminateInvocation=%u",
                      physicalDevice_1_3_Features.shaderTerminateInvocation);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] inlineUniformBlock=%u",
                      physicalDevice_1_3_Features.inlineUniformBlock);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] descriptorBindingInlineUniformBlockUpdateAfterBind=%u",
                      physicalDevice_1_3_Features.descriptorBindingInlineUniformBlockUpdateAfterBind);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] synchronization2=%u",
                      physicalDevice_1_3_Features.synchronization2);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] textureCompressionASTC_HDR=%u",
                      physicalDevice_1_3_Features.textureCompressionASTC_HDR);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] shaderZeroInitializeWorkgroupMemory=%u",
                      physicalDevice_1_3_Features.shaderZeroInitializeWorkgroupMemory);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] robustImageAccess=%u",
                      physicalDevice_1_3_Features.robustImageAccess);
        SNAP_RHI_LOGI("[Vulkan][Device][Features1_3] inlineUniformBlock=%u",
                      physicalDevice_1_3_Features.inlineUniformBlock);
    }

#if SNAP_RHI_OS_APPLE()
    if (physicalDeviceProperties.apiVersion >= VK_API_VERSION_1_2) {
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] constantAlphaColorBlendFactors=%u",
                      portabilitySubsetFeatures.constantAlphaColorBlendFactors);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] events=%u", portabilitySubsetFeatures.events);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] imageViewFormatReinterpretation=%u",
                      portabilitySubsetFeatures.imageViewFormatReinterpretation);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] imageViewFormatSwizzle=%u",
                      portabilitySubsetFeatures.imageViewFormatSwizzle);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] imageView2DOn3DImage=%u",
                      portabilitySubsetFeatures.imageView2DOn3DImage);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] multisampleArrayImage=%u",
                      portabilitySubsetFeatures.multisampleArrayImage);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] mutableComparisonSamplers=%u",
                      portabilitySubsetFeatures.mutableComparisonSamplers);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] pointPolygons=%u", portabilitySubsetFeatures.pointPolygons);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] samplerMipLodBias=%u",
                      portabilitySubsetFeatures.samplerMipLodBias);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] separateStencilMaskRef=%u",
                      portabilitySubsetFeatures.separateStencilMaskRef);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] shaderSampleRateInterpolationFunctions=%u",
                      portabilitySubsetFeatures.shaderSampleRateInterpolationFunctions);
        SNAP_RHI_LOGI("[Vulkan][Device][PortabilitySubset] vertexAttributeAccessBeyondStride=%u",
                      portabilitySubsetFeatures.vertexAttributeAccessBeyondStride);
    }
#endif
}
} // namespace snap::rhi::backend::vulkan
