/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2016-2017 Dustin Land

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#pragma hdrstop
#include "../GLState.h"
#include "../GLMatrix.h"
#include "../RenderCommon.h"
#include "../RenderLog.h"
#include "Allocator_VK.h"
#include "Staging_VK.h"

#include <SDL.h>
#include <SDL_vulkan.h>
#include <vector>

vulkanContext_t vkcontext;

idCVar r_vkEnableValidationLayers( "r_vkEnableValidationLayers", "1", CVAR_BOOL | CVAR_INIT, "" );

extern idCVar r_multiSamples;
extern idCVar r_skipRender;
extern idCVar r_skipShadows;
extern idCVar r_showShadows;
extern idCVar r_shadowPolygonFactor;
extern idCVar r_shadowPolygonOffset;
extern idCVar r_useScissor;
extern idCVar r_useShadowDepthBounds;
extern idCVar r_forceZPassStencilShadows;
extern idCVar r_useStencilShadowPreload;
extern idCVar r_useLightDepthBounds;
extern idCVar r_swapInterval;

void PrintState( uint64 stateBits );

static const int g_numDebugInstanceExtensions = 1;
static const char * g_debugInstanceExtensions[ g_numDebugInstanceExtensions ] = {
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME
};

static const int g_numDeviceExtensions = 1;
static const char * g_deviceExtensions[ g_numDeviceExtensions ] = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

static const int g_numValidationLayers = 1;
static const char * g_validationLayers[ g_numValidationLayers ] = {
	"VK_LAYER_LUNARG_standard_validation"
};

#define ID_VK_ERROR_STRING( x ) case static_cast< int >( x ): return #x

/*
=============
VK_ErrorToString
=============
*/
const char * VK_ErrorToString( VkResult result ) {
	switch ( result ) {
		ID_VK_ERROR_STRING( VK_SUCCESS );
		ID_VK_ERROR_STRING( VK_NOT_READY );
		ID_VK_ERROR_STRING( VK_TIMEOUT );
		ID_VK_ERROR_STRING( VK_EVENT_SET );
		ID_VK_ERROR_STRING( VK_EVENT_RESET );
		ID_VK_ERROR_STRING( VK_INCOMPLETE );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_HOST_MEMORY );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_DEVICE_MEMORY );
		ID_VK_ERROR_STRING( VK_ERROR_INITIALIZATION_FAILED );
		ID_VK_ERROR_STRING( VK_ERROR_DEVICE_LOST );
		ID_VK_ERROR_STRING( VK_ERROR_MEMORY_MAP_FAILED );
		ID_VK_ERROR_STRING( VK_ERROR_LAYER_NOT_PRESENT );
		ID_VK_ERROR_STRING( VK_ERROR_EXTENSION_NOT_PRESENT );
		ID_VK_ERROR_STRING( VK_ERROR_FEATURE_NOT_PRESENT );
		ID_VK_ERROR_STRING( VK_ERROR_INCOMPATIBLE_DRIVER );
		ID_VK_ERROR_STRING( VK_ERROR_TOO_MANY_OBJECTS );
		ID_VK_ERROR_STRING( VK_ERROR_FORMAT_NOT_SUPPORTED );
		ID_VK_ERROR_STRING( VK_ERROR_SURFACE_LOST_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR );
		ID_VK_ERROR_STRING( VK_SUBOPTIMAL_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_OUT_OF_DATE_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_INCOMPATIBLE_DISPLAY_KHR );
		ID_VK_ERROR_STRING( VK_ERROR_VALIDATION_FAILED_EXT );
		ID_VK_ERROR_STRING( VK_ERROR_INVALID_SHADER_NV );
		ID_VK_ERROR_STRING( VK_RESULT_BEGIN_RANGE );
		ID_VK_ERROR_STRING( VK_RESULT_RANGE_SIZE );
		default: return "UNKNOWN";
	};
}

/*
=============================
R_GetModeParms

r_fullScreen -1		borderless window at exact desktop coordinates
r_fullScreen 0		bordered window at exact desktop coordinates
r_fullScreen 1		fullscreen on monitor 1 at r_vidMode
r_fullScreen 2		fullscreen on monitor 2 at r_vidMode
...

r_vidMode -1		use r_customWidth / r_customHeight, even if they don't appear on the mode list
r_vidMode 0			use first mode returned by EnumDisplaySettings()
r_vidMode 1			use second mode returned by EnumDisplaySettings()
...

r_displayRefresh 0	don't specify refresh
r_displayRefresh 70	specify 70 hz, etc
=============================
*/
gfxImpParms_t R_GetModeParms() {
	// try up to three different configurations

	gfxImpParms_t parms;

	if ( r_fullscreen.GetInteger() <= 0 ) {
		// use explicit position / size for window
		parms.x = r_windowX.GetInteger();
		parms.y = r_windowY.GetInteger();
		parms.width = r_windowWidth.GetInteger();
		parms.height = r_windowHeight.GetInteger();
		// may still be -1 to force a borderless window
		parms.fullScreen = r_fullscreen.GetInteger();
		parms.displayHz = 0;		// ignored
	} else {
		// get the mode list for this monitor
		idList<vidMode_t> modeList;
		if ( !R_GetModeListForDisplay( r_fullscreen.GetInteger()-1, modeList ) ) {
			idLib::Printf( "r_fullscreen reset from %i to 1 because mode list failed.", r_fullscreen.GetInteger() );
			r_fullscreen.SetInteger( 1 );
			R_GetModeListForDisplay( r_fullscreen.GetInteger()-1, modeList );
		}
		if ( modeList.Num() < 1 ) {
			idLib::FatalError( "No modes available." );
		}

		parms.x = 0;		// ignored
		parms.y = 0;		// ignored
		parms.fullScreen = r_fullscreen.GetInteger();

		// set the parameters we are trying
		if ( r_vidMode.GetInteger() < 0 ) {
			// try forcing a specific mode, even if it isn't on the list
			parms.width = r_customWidth.GetInteger();
			parms.height = r_customHeight.GetInteger();
			parms.displayHz = r_displayRefresh.GetInteger();
		} else {
			if ( r_vidMode.GetInteger() > modeList.Num() ) {
				idLib::Printf( "r_vidMode reset from %i to 0.\n", r_vidMode.GetInteger() );
				r_vidMode.SetInteger( 0 );
			}

			parms.width = modeList[ r_vidMode.GetInteger() ].width;
			parms.height = modeList[ r_vidMode.GetInteger() ].height;
			parms.displayHz = modeList[ r_vidMode.GetInteger() ].displayHz;
		}
	}

	parms.multiSamples = r_multiSamples.GetInteger();

	return parms;
}

/*
===================
SetScreenParms

Sets up the screen based on passed parms.. 
===================
*/
bool SetScreenParms( gfxImpParms_t parms ) {
	idLib::Printf("SetScreenParms NI");
	return true;
}

/*
===========================================================================

idRenderBackend

===========================================================================
*/

static VkDebugReportCallbackEXT debugReportCallback = VK_NULL_HANDLE;

/*
=============
DebugCallback
=============
*/
static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback( 
	VkDebugReportFlagsEXT flags, 
	VkDebugReportObjectTypeEXT objType, 
	uint64 obj, size_t location, int32 code,
	const char * layerPrefix, const char * msg, void * userData ) {

	idLib::Printf( "VK_DEBUG::%s: %s flags=%d, objType=%d, obj=%llu, location=%zd, code=%d\n", 
		layerPrefix, msg, flags, objType, obj, location, code );

	return VK_FALSE;
}

/*
=============
CreateDebugReportCallback
=============
*/
static void CreateDebugReportCallback( VkInstance instance ) {
	VkDebugReportCallbackCreateInfoEXT callbackInfo = {};
	callbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
	callbackInfo.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT;
	callbackInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT) DebugCallback;

	PFN_vkCreateDebugReportCallbackEXT func = (PFN_vkCreateDebugReportCallbackEXT) vkGetInstanceProcAddr( instance, "vkCreateDebugReportCallbackEXT" );
	ID_VK_VALIDATE( func != NULL, "Could not find vkCreateDebugReportCallbackEXT" );
	ID_VK_CHECK( func( instance, &callbackInfo, NULL, &debugReportCallback ) );
}

/*
=============
DestroyDebugReportCallback
=============
*/
static void DestroyDebugReportCallback( VkInstance instance ) {
	PFN_vkDestroyDebugReportCallbackEXT func = (PFN_vkDestroyDebugReportCallbackEXT) vkGetInstanceProcAddr( instance, "vkDestroyDebugReportCallbackEXT" );
	ID_VK_VALIDATE( func != NULL, "Could not find vkDestroyDebugReportCallbackEXT" );
	func( instance, debugReportCallback, NULL );
}

/*
=============
ValidateValidationLayers
=============
*/
static void ValidateValidationLayers() {
	uint32 instanceLayerCount = 0;
	vkEnumerateInstanceLayerProperties( &instanceLayerCount, NULL );

	idList< VkLayerProperties > instanceLayers;
	instanceLayers.SetNum( instanceLayerCount );
	vkEnumerateInstanceLayerProperties( &instanceLayerCount, instanceLayers.Ptr() );

	bool found = false;
	for ( uint32 i = 0; i < g_numValidationLayers; ++i ) {
		for ( uint32 j = 0; j < instanceLayerCount; ++j ) {
			if ( idStr::Icmp( g_validationLayers[i], instanceLayers[j].layerName ) == 0 ) {
				found = true;
				break;
			}
		}
		if ( !found ) {
			idLib::FatalError( "Cannot find validation layer: %s.\n", g_validationLayers[ i ] );
		}
	}
}

/*
=============
ChooseSupportedFormat
=============
*/
static VkFormat ChooseSupportedFormat( VkPhysicalDevice physicalDevice, VkFormat * formats, int numFormats, VkImageTiling tiling, VkFormatFeatureFlags features ) {
	for ( int i = 0; i < numFormats; ++i ) {
		VkFormat format = formats[ i ];

		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties( physicalDevice, format, &props );

		if ( tiling == VK_IMAGE_TILING_LINEAR && ( props.linearTilingFeatures & features ) == features ) {
			return format;
		} else if ( tiling == VK_IMAGE_TILING_OPTIMAL && ( props.optimalTilingFeatures & features ) == features ) {
			return format;
		}
	}

	idLib::FatalError( "Failed to find a supported format." );

	return VK_FORMAT_UNDEFINED;
}

/*
=============
idRenderBackend::CreateInstance
=============
*/
void idRenderBackend::CreateInstance() {
	SDL_Window* window = GLimp_GetWindow();
	if (!window)
		idLib::FatalError("SDL Window not available");

	unsigned int extension_count;
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, nullptr))
		Sys_Error("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());

	std::vector<const char*> instance_extensions(extension_count);
	if (!SDL_Vulkan_GetInstanceExtensions(window, &extension_count, instance_extensions.data()))
		Sys_Error("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "DOOM";
	appInfo.applicationVersion = 1;
	appInfo.pEngineName = "idTech 4.5";
	appInfo.engineVersion = 1;
	appInfo.apiVersion = VK_MAKE_VERSION( 1, 0, VK_HEADER_VERSION );

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;

	const bool enableLayers = r_vkEnableValidationLayers.GetBool();

	vkcontext.instanceExtensions.Clear();
	vkcontext.deviceExtensions.Clear();
	vkcontext.validationLayers.Clear();

	for ( int i = 0; i < instance_extensions.size(); ++i ) {
		vkcontext.instanceExtensions.Append( instance_extensions[ i ] );
	}

	for ( int i = 0; i < g_numDeviceExtensions; ++i ) {
		vkcontext.deviceExtensions.Append( g_deviceExtensions[ i ] );
	}

	if ( enableLayers ) {
		for ( int i = 0; i < g_numDebugInstanceExtensions; ++i ) {
			vkcontext.instanceExtensions.Append( g_debugInstanceExtensions[ i ] );
		}

		for ( int i = 0; i < g_numValidationLayers; ++i ) {
			vkcontext.validationLayers.Append( g_validationLayers[ i ] );
		}

		ValidateValidationLayers();
	}

	createInfo.enabledExtensionCount = vkcontext.instanceExtensions.Num();
	createInfo.ppEnabledExtensionNames = vkcontext.instanceExtensions.Ptr();
	createInfo.enabledLayerCount = vkcontext.validationLayers.Num();
	createInfo.ppEnabledLayerNames = vkcontext.validationLayers.Ptr();

	ID_VK_CHECK( vkCreateInstance( &createInfo, NULL, &vkcontext.instance ) );

	if ( enableLayers ) {
		CreateDebugReportCallback( vkcontext.instance );
	}
}

/*
=============
CheckPhysicalDeviceExtensionSupport
=============
*/
static bool CheckPhysicalDeviceExtensionSupport( gpuInfo_t & gpu, idList< const char * > & requiredExt ) {
	int required = requiredExt.Num();
	int available = 0;

	for ( int i = 0; i < requiredExt.Num(); ++i ) {
		for ( int j = 0; j < gpu.extensionProps.Num(); ++j ) {
			if ( idStr::Icmp( requiredExt[ i ], gpu.extensionProps[ j ].extensionName ) == 0 ) {
				available++;
				break;
			}
		}
	}

	return available == required;
}

/*
=============
idRenderBackend::SelectSuitablePhysicalDevice
=============
*/
void idRenderBackend::SelectSuitablePhysicalDevice() {
	uint32 numDevices = 0;
	ID_VK_CHECK( vkEnumeratePhysicalDevices( vkcontext.instance, &numDevices, NULL ) );
	ID_VK_VALIDATE( numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices." );

	idList< VkPhysicalDevice > devices;
	devices.SetNum( numDevices );
	
	ID_VK_CHECK( vkEnumeratePhysicalDevices( vkcontext.instance, &numDevices, devices.Ptr() ) );
	ID_VK_VALIDATE( numDevices > 0, "vkEnumeratePhysicalDevices returned zero devices." );

	idList< gpuInfo_t > gpus;
	gpus.SetNum( numDevices );

	for ( uint32 i = 0; i < numDevices; ++i ) {
		gpuInfo_t & gpu = gpus[ i ];
		gpu.device = devices[ i ];

		{
			uint32 numQueues = 0;
			vkGetPhysicalDeviceQueueFamilyProperties( gpu.device, &numQueues, NULL );
			ID_VK_VALIDATE( numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues." );

			gpu.queueFamilyProps.SetNum( numQueues );
			vkGetPhysicalDeviceQueueFamilyProperties( gpu.device, &numQueues, gpu.queueFamilyProps.Ptr() );
			ID_VK_VALIDATE( numQueues > 0, "vkGetPhysicalDeviceQueueFamilyProperties returned zero queues." );
		}

		{
			uint32 numExtension;
			ID_VK_CHECK( vkEnumerateDeviceExtensionProperties( gpu.device, NULL, &numExtension, NULL ) );
			ID_VK_VALIDATE( numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions." );

			gpu.extensionProps.SetNum( numExtension );
			ID_VK_CHECK( vkEnumerateDeviceExtensionProperties( gpu.device, NULL, &numExtension, gpu.extensionProps.Ptr() ) );
			ID_VK_VALIDATE( numExtension > 0, "vkEnumerateDeviceExtensionProperties returned zero extensions." );
		}

		ID_VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( gpu.device, vkcontext.surface, &gpu.surfaceCaps ) );

		{
			uint32 numFormats;
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu.device, vkcontext.surface, &numFormats, NULL ) );
			ID_VK_VALIDATE( numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats." );

			gpu.surfaceFormats.SetNum( numFormats );
			ID_VK_CHECK( vkGetPhysicalDeviceSurfaceFormatsKHR( gpu.device, vkcontext.surface, &numFormats, gpu.surfaceFormats.Ptr() ) );
			ID_VK_VALIDATE( numFormats > 0, "vkGetPhysicalDeviceSurfaceFormatsKHR returned zero surface formats." );
		}

		{
			uint32 numPresentModes;
			ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu.device, vkcontext.surface, &numPresentModes, NULL ) );
			ID_VK_VALIDATE( numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes." );

			gpu.presentModes.SetNum( numPresentModes );
			ID_VK_CHECK( vkGetPhysicalDeviceSurfacePresentModesKHR( gpu.device, vkcontext.surface, &numPresentModes, gpu.presentModes.Ptr() ) );
			ID_VK_VALIDATE( numPresentModes > 0, "vkGetPhysicalDeviceSurfacePresentModesKHR returned zero present modes." );
		}

		vkGetPhysicalDeviceMemoryProperties( gpu.device, &gpu.memProps );
		vkGetPhysicalDeviceProperties( gpu.device, &gpu.props );
		vkGetPhysicalDeviceFeatures( gpu.device, &gpu.features );
	}

	// Now try to select one
	for ( int i = 0; i < gpus.Num(); ++i ) {
		gpuInfo_t & gpu = gpus[ i ];

		int graphicsIdx = -1;
		int presentIdx = -1;

		if ( !CheckPhysicalDeviceExtensionSupport( gpu, vkcontext.deviceExtensions ) ) {
			continue;
		}

		if ( gpu.surfaceFormats.Num() == 0 ) {
			continue;
		}

		if ( gpu.presentModes.Num() == 0 ) {
			continue;
		}

		// Find graphics queue family
		for ( int j = 0; j < gpu.queueFamilyProps.Num(); ++j ) {
			VkQueueFamilyProperties & props = gpu.queueFamilyProps[ j ];

			if ( props.queueCount == 0 ) {
				continue;
			}

			if ( props.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
				graphicsIdx = j;
				break;
			}
		}

		// Find present queue family
		for ( int j = 0; j < gpu.queueFamilyProps.Num(); ++j ) {
			VkQueueFamilyProperties & props = gpu.queueFamilyProps[ j ];

			if ( props.queueCount == 0 ) {
				continue;
			}

			VkBool32 supportsPresent = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( gpu.device, j, vkcontext.surface, &supportsPresent );
			if ( supportsPresent ) {
				presentIdx = j;
				break;
			}
		}

		// Did we find a device supporting both graphics and present.
		if ( graphicsIdx >= 0 && presentIdx >= 0 ) {
			vkcontext.graphicsFamilyIdx = graphicsIdx;
			vkcontext.presentFamilyIdx = presentIdx;
			vkcontext.physicalDevice = gpu.device;
			vkcontext.gpu = gpu;

			return;
		}
	}

	// If we can't render or present, just bail.
	idLib::FatalError( "Could not find a physical device which fits our desired profile" );
}

/*
=============
idRenderBackend::CreateSurface
=============
*/
void idRenderBackend::CreateSurface() 
{
	if (!SDL_Vulkan_CreateSurface(GLimp_GetWindow(), vkcontext.instance, &vkcontext.surface))
		Sys_Error("SDL_Vulkan_CreateSurface failed: %s", SDL_GetError());	
}

/*
=============
idRenderBackend::CreateLogicalDeviceAndQueues
=============
*/
void idRenderBackend::CreateLogicalDeviceAndQueues() {
	idList< int > uniqueIdx;
	uniqueIdx.AddUnique( vkcontext.graphicsFamilyIdx );
	uniqueIdx.AddUnique( vkcontext.presentFamilyIdx );
	
	idList< VkDeviceQueueCreateInfo > devqInfo;

	const float priority = 1.0f;
	for ( int i = 0; i < uniqueIdx.Num(); ++i ) {
		VkDeviceQueueCreateInfo qinfo = {};
		qinfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		qinfo.queueFamilyIndex = uniqueIdx[ i ];
		qinfo.queueCount = 1;
		qinfo.pQueuePriorities = &priority;

		devqInfo.Append( qinfo );
	}

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.textureCompressionBC = VK_TRUE;
	deviceFeatures.imageCubeArray = VK_TRUE;
	deviceFeatures.depthClamp = VK_TRUE;
	deviceFeatures.depthBiasClamp = VK_TRUE;
	deviceFeatures.depthBounds = vkcontext.gpu.features.depthBounds;
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	VkDeviceCreateInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	info.queueCreateInfoCount = devqInfo.Num();
	info.pQueueCreateInfos = devqInfo.Ptr();
	info.pEnabledFeatures = &deviceFeatures;
	info.enabledExtensionCount = vkcontext.deviceExtensions.Num();
	info.ppEnabledExtensionNames = vkcontext.deviceExtensions.Ptr();

	if ( r_vkEnableValidationLayers.GetBool() ) {
		info.enabledLayerCount = vkcontext.validationLayers.Num();
		info.ppEnabledLayerNames = vkcontext.validationLayers.Ptr();
	} else {
		info.enabledLayerCount = 0;
	}

	ID_VK_CHECK( vkCreateDevice( vkcontext.physicalDevice, &info, NULL, &vkcontext.device ) );

	vkGetDeviceQueue( vkcontext.device, vkcontext.graphicsFamilyIdx, 0, &vkcontext.graphicsQueue );
	vkGetDeviceQueue( vkcontext.device, vkcontext.presentFamilyIdx, 0, &vkcontext.presentQueue );
}

/*
=============
ChooseSurfaceFormat
=============
*/
VkSurfaceFormatKHR ChooseSurfaceFormat( idList< VkSurfaceFormatKHR > & formats ) {
	VkSurfaceFormatKHR result;
	
	if ( formats.Num() == 1 && formats[ 0 ].format == VK_FORMAT_UNDEFINED ) {
		result.format = VK_FORMAT_B8G8R8A8_UNORM;
		result.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		return result;
	}

	for ( int i = 0; i < formats.Num(); ++i ) {
		VkSurfaceFormatKHR & fmt = formats[ i ];
		if ( fmt.format == VK_FORMAT_B8G8R8A8_UNORM && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ) {
			return fmt;
		}
	}

	return formats[ 0 ];
}

/*
=============
ChoosePresentMode
=============
*/
VkPresentModeKHR ChoosePresentMode( idList< VkPresentModeKHR > & modes ) {
	VkPresentModeKHR desiredMode = VK_PRESENT_MODE_FIFO_KHR;

	if (r_swapInterval.GetInteger() < 1) {
		for (int i = 0; i < modes.Num(); i++) {
			if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
				return VK_PRESENT_MODE_MAILBOX_KHR;
			}
			if ((modes[i] != VK_PRESENT_MODE_MAILBOX_KHR) && (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
				return VK_PRESENT_MODE_IMMEDIATE_KHR;
			}
		}
	}

	for (int i = 0; i < modes.Num(); ++i) {
		if (modes[i] == desiredMode) {
			return desiredMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}

/*
=============
ChooseSurfaceExtent
=============
*/
VkExtent2D ChooseSurfaceExtent( VkSurfaceCapabilitiesKHR & caps ) {
	VkExtent2D extent;

	if ( caps.currentExtent.width == -1 ) {
		int width, height;
		SDL_GetWindowSize(GLimp_GetWindow(), &width, &height);
		extent.width = width;
		extent.height = height;
	} else {
		extent = caps.currentExtent;
	}

	return extent;
}

/*
=============
idRenderBackend::CreateSwapChain
=============
*/
void idRenderBackend::CreateSwapChain() {
	gpuInfo_t & gpu = vkcontext.gpu;

	VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat( gpu.surfaceFormats );
	VkPresentModeKHR presentMode = ChoosePresentMode( gpu.presentModes );
	VkExtent2D extent = ChooseSurfaceExtent( gpu.surfaceCaps );

	VkSwapchainCreateInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	info.surface = vkcontext.surface;
	info.minImageCount = NUM_FRAME_DATA;
	info.imageFormat = surfaceFormat.format;
	info.imageColorSpace = surfaceFormat.colorSpace;
	info.imageExtent = extent;
	info.imageArrayLayers = 1;
	info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

	if ( vkcontext.graphicsFamilyIdx != vkcontext.presentFamilyIdx ) {
		uint32 indices[] = { (uint32)vkcontext.graphicsFamilyIdx, (uint32)vkcontext.presentFamilyIdx };

		info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		info.queueFamilyIndexCount = 2;
		info.pQueueFamilyIndices = indices;
	} else {
		info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	info.presentMode = presentMode;
	info.clipped = VK_TRUE;

	ID_VK_CHECK( vkCreateSwapchainKHR( vkcontext.device, &info, NULL, &vkcontext.swapchain ) );

	vkcontext.swapchainFormat = surfaceFormat.format;
	vkcontext.presentMode = presentMode;
	vkcontext.swapchainExtent = extent;
	vkcontext.fullscreen = true;//win32.isFullscreen;

	uint32 numImages = 0;
	ID_VK_CHECK( vkGetSwapchainImagesKHR( vkcontext.device, vkcontext.swapchain, &numImages, NULL ) );
	ID_VK_VALIDATE( numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count." );

	ID_VK_CHECK( vkGetSwapchainImagesKHR( vkcontext.device, vkcontext.swapchain, &numImages, vkcontext.swapchainImages.Ptr() ) );
	ID_VK_VALIDATE( numImages > 0, "vkGetSwapchainImagesKHR returned a zero image count." );

	for ( uint32 i = 0; i < NUM_FRAME_DATA; ++i ) {
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = vkcontext.swapchainImages[ i ];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = vkcontext.swapchainFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_A;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;
		imageViewCreateInfo.flags = 0;

		ID_VK_CHECK( vkCreateImageView( vkcontext.device, &imageViewCreateInfo, NULL, &vkcontext.swapchainViews[ i ] ) );
	}
}

/*
=============
idRenderBackend::DestroySwapChain
=============
*/
void idRenderBackend::DestroySwapChain() {
	for ( uint32 i = 0; i < NUM_FRAME_DATA; ++i ) {
		vkDestroyImageView( vkcontext.device, vkcontext.swapchainViews[ i ], NULL );
	}
	vkcontext.swapchainViews.Zero();

	vkDestroySwapchainKHR( vkcontext.device, vkcontext.swapchain, NULL );
}

/*
=============
idRenderBackend::CreateCommandPool
=============
*/
void idRenderBackend::CreateCommandPool() {
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = vkcontext.graphicsFamilyIdx;

	ID_VK_CHECK( vkCreateCommandPool( vkcontext.device, &commandPoolCreateInfo, NULL, &vkcontext.commandPool ) );
}

/*
=============
idRenderBackend::CreateCommandBuffer
=============
*/
void idRenderBackend::CreateCommandBuffer() {
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocateInfo.commandPool = vkcontext.commandPool;
	commandBufferAllocateInfo.commandBufferCount = NUM_FRAME_DATA;

	ID_VK_CHECK( vkAllocateCommandBuffers( vkcontext.device, &commandBufferAllocateInfo, vkcontext.commandBuffers.Ptr() ) );

	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		ID_VK_CHECK( vkCreateFence( vkcontext.device, &fenceCreateInfo, NULL, &vkcontext.commandBufferFences[ i ] ) );
	}
}

/*
=============
idRenderBackend::CreateSemaphores
=============
*/
void idRenderBackend::CreateSemaphores() {
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		ID_VK_CHECK( vkCreateSemaphore( vkcontext.device, &semaphoreCreateInfo, NULL, &vkcontext.acquireSemaphores[ i ] ) );
		ID_VK_CHECK( vkCreateSemaphore( vkcontext.device, &semaphoreCreateInfo, NULL, &vkcontext.renderCompleteSemaphores[ i ] ) );
	}
}

/*
===============
idRenderBackend::CreateQueryPool
===============
*/
void idRenderBackend::CreateQueryPool() {
	VkQueryPoolCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	createInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	createInfo.queryCount = NUM_TIMESTAMP_QUERIES;

	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		ID_VK_CHECK( vkCreateQueryPool( vkcontext.device, &createInfo, NULL, &vkcontext.queryPools[ i ] ) );
	}
}

/*
=============
idRenderBackend::CreateRenderTargets
=============
*/
void idRenderBackend::CreateRenderTargets() {
	// Determine samples before creating depth
	VkImageFormatProperties fmtProps = {};
	vkGetPhysicalDeviceImageFormatProperties( vkcontext.physicalDevice, vkcontext.swapchainFormat, 
		VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, 0, &fmtProps );

	const int samples = r_multiSamples.GetInteger();

	if ( samples >= 16 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_16_BIT ) ) {
		vkcontext.sampleCount = VK_SAMPLE_COUNT_16_BIT;
	} else if ( samples >= 8 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_8_BIT ) ) {
		vkcontext.sampleCount = VK_SAMPLE_COUNT_8_BIT;
	} else if ( samples >= 4 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_4_BIT ) ) {
		vkcontext.sampleCount = VK_SAMPLE_COUNT_4_BIT;
	} else if ( samples >= 2 && ( fmtProps.sampleCounts & VK_SAMPLE_COUNT_2_BIT ) ) {
		vkcontext.sampleCount = VK_SAMPLE_COUNT_2_BIT;
	}

	// Select Depth Format
	{
		VkFormat formats[] = {  
			VK_FORMAT_D32_SFLOAT_S8_UINT, 
			VK_FORMAT_D24_UNORM_S8_UINT 
		};
		vkcontext.depthFormat = ChooseSupportedFormat( 
			vkcontext.physicalDevice,
			formats, 3, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT );
	}

	idImageOpts depthOptions;
	depthOptions.format = FMT_DEPTH;
	depthOptions.width = renderSystem->GetWidth();
	depthOptions.height = renderSystem->GetHeight();
	depthOptions.numLevels = 1;
	depthOptions.samples = static_cast< textureSamples_t >( vkcontext.sampleCount );

	globalImages->ScratchImage( "_viewDepth", &depthOptions, TF_DEFAULT, TR_CLAMP, TD_DEFAULT );

	if ( vkcontext.sampleCount > VK_SAMPLE_COUNT_1_BIT ) {
		vkcontext.supersampling = vkcontext.gpu.features.sampleRateShading == VK_TRUE;

		VkImageCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		createInfo.imageType = VK_IMAGE_TYPE_2D;
		createInfo.format = vkcontext.swapchainFormat;
		createInfo.extent.width = vkcontext.swapchainExtent.width;
		createInfo.extent.height = vkcontext.swapchainExtent.height;
		createInfo.extent.depth = 1;
		createInfo.mipLevels = 1;
		createInfo.arrayLayers = 1;
		createInfo.samples = vkcontext.sampleCount;
		createInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		createInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
		createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		ID_VK_CHECK( vkCreateImage( vkcontext.device, &createInfo, NULL, &vkcontext.msaaImage ) );

#if defined( ID_USE_AMD_ALLOCATOR )
	VmaMemoryRequirements vmaReq = {};
	vmaReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;

	ID_VK_CHECK( vmaCreateImage( vmaAllocator, &createInfo, &vmaReq, &vkcontext.msaaImage, &m_msaaVmaAllocation, &vkcontext.msaaAllocation ) );
#else
		VkMemoryRequirements memoryRequirements = {};
		vkGetImageMemoryRequirements( vkcontext.device, vkcontext.msaaImage, &memoryRequirements );

		vkcontext.msaaAllocation = vulkanAllocator.Allocate( 
			memoryRequirements.size,
			memoryRequirements.alignment,
			memoryRequirements.memoryTypeBits, 
			VULKAN_MEMORY_USAGE_GPU_ONLY,
			VULKAN_ALLOCATION_TYPE_IMAGE_OPTIMAL );

		ID_VK_CHECK( vkBindImageMemory( vkcontext.device, vkcontext.msaaImage, vkcontext.msaaAllocation.deviceMemory, vkcontext.msaaAllocation.offset ) );
#endif

		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.format = vkcontext.swapchainFormat;
		viewInfo.image = vkcontext.msaaImage;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		ID_VK_CHECK( vkCreateImageView( vkcontext.device, &viewInfo, NULL, &vkcontext.msaaImageView ) );
	}
}

/*
=============
idRenderBackend::DestroyRenderTargets
=============
*/
void idRenderBackend::DestroyRenderTargets() {
	vkDestroyImageView( vkcontext.device, vkcontext.msaaImageView, NULL );
#if defined( ID_USE_AMD_ALLOCATOR )
	vmaDestroyImage( vmaAllocator, vkcontext.msaaImage, m_msaaVmaAllocation );
	vkcontext.msaaAllocation = VmaAllocationInfo();
	m_msaaVmaAllocation = NULL;

#else
	vkDestroyImage( vkcontext.device, vkcontext.msaaImage, NULL );
	vulkanAllocator.Free( vkcontext.msaaAllocation );
	vkcontext.msaaAllocation = vulkanAllocation_t();
#endif

	vkcontext.msaaImage = VK_NULL_HANDLE;
	vkcontext.msaaImageView = VK_NULL_HANDLE;
}

/*
=============
idRenderBackend::CreateRenderPass
=============
*/
void idRenderBackend::CreateRenderPass() {
	VkAttachmentDescription attachments[ 3 ];
	memset( attachments, 0, sizeof( attachments ) );

	const bool resolve = vkcontext.sampleCount > VK_SAMPLE_COUNT_1_BIT;

	VkAttachmentDescription & colorAttachment = attachments[ 0 ];
	colorAttachment.format = vkcontext.swapchainFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkAttachmentDescription & depthAttachment = attachments[ 1 ];
	depthAttachment.format = vkcontext.depthFormat;
	depthAttachment.samples = vkcontext.sampleCount;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription & resolveAttachment = attachments[ 2 ];
	resolveAttachment.format = vkcontext.swapchainFormat;
	resolveAttachment.samples = vkcontext.sampleCount;
	resolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	resolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	resolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	resolveAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

	VkAttachmentReference colorRef = {};
	colorRef.attachment = resolve ? 2 : 0;
	colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthRef = {};
	depthRef.attachment = 1;
	depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveRef = {};
	resolveRef.attachment = 0;
	resolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorRef;
	subpass.pDepthStencilAttachment = &depthRef;
	if ( resolve ) {
		subpass.pResolveAttachments = &resolveRef;
	}

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = resolve ? 3 : 2;
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = 0;

	ID_VK_CHECK( vkCreateRenderPass( vkcontext.device, &renderPassCreateInfo, NULL, &vkcontext.renderPass ) );
}

/*
=============
CreatePipelineCache
=============
*/
static void CreatePipelineCache() {
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	ID_VK_CHECK( vkCreatePipelineCache( vkcontext.device, &pipelineCacheCreateInfo, NULL, &vkcontext.pipelineCache ) );
}

/*
=============
idRenderBackend::CreateFrameBuffers
=============
*/
void idRenderBackend::CreateFrameBuffers() {
	VkImageView attachments[ 3 ] = {};

	// depth attachment is the same
	idImage * depthImg = globalImages->GetImage( "_viewDepth" );
	if ( depthImg == NULL ) {
		idLib::FatalError( "CreateFrameBuffers: No _viewDepth image." );
	} else {
		attachments[ 1 ] = depthImg->GetView();
	}

	const bool resolve = vkcontext.sampleCount > VK_SAMPLE_COUNT_1_BIT;
	if ( resolve ) {
		attachments[ 2 ] = vkcontext.msaaImageView;
	}

	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.renderPass = vkcontext.renderPass;
	frameBufferCreateInfo.attachmentCount = resolve ? 3 : 2;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = renderSystem->GetWidth();
	frameBufferCreateInfo.height = renderSystem->GetHeight();
	frameBufferCreateInfo.layers = 1;

	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		attachments[ 0 ] = vkcontext.swapchainViews[ i ];
		ID_VK_CHECK( vkCreateFramebuffer( vkcontext.device, &frameBufferCreateInfo, NULL, &vkcontext.frameBuffers[ i ] ) );
	}
}

/*
=============
idRenderBackend::DestroyFrameBuffers
=============
*/
void idRenderBackend::DestroyFrameBuffers() {
	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		vkDestroyFramebuffer( vkcontext.device, vkcontext.frameBuffers[ i ], NULL );
	}
	vkcontext.frameBuffers.Zero();
}

/*
=============
ClearContext
=============
*/
static void ClearContext() {
	vkcontext.jointCacheHandle = 0;
	vkcontext.gpu = gpuInfo_t();
	vkcontext.device = VK_NULL_HANDLE;
	vkcontext.graphicsFamilyIdx = -1;
	vkcontext.presentFamilyIdx = -1;
	vkcontext.graphicsQueue = VK_NULL_HANDLE;
	vkcontext.presentQueue = VK_NULL_HANDLE;
	vkcontext.depthFormat = VK_FORMAT_UNDEFINED;
	vkcontext.renderPass = VK_NULL_HANDLE;
	vkcontext.pipelineCache = VK_NULL_HANDLE;
	vkcontext.sampleCount = VK_SAMPLE_COUNT_1_BIT;
	vkcontext.supersampling = false;
	vkcontext.imageParms.Zero();
}

/*
=============
idRenderBackend::idRenderBackend
=============
*/
idRenderBackend::idRenderBackend() {
	ClearContext();
	Clear();

	memset( gammaTable, 0, sizeof( gammaTable ) );
}

/*
=============
idRenderBackend::~idRenderBackend
=============
*/
idRenderBackend::~idRenderBackend() {

}

/*
=============
idRenderBackend::Clear
=============
*/
void idRenderBackend::Clear() {
	vkcontext.counter = 0;
	vkcontext.currentFrameData = 0;

	vkcontext.instance = VK_NULL_HANDLE;
	vkcontext.physicalDevice = VK_NULL_HANDLE;

	debugReportCallback = VK_NULL_HANDLE;
	vkcontext.instanceExtensions.Clear();
	vkcontext.deviceExtensions.Clear();
	vkcontext.validationLayers.Clear();

	vkcontext.surface = VK_NULL_HANDLE;
	vkcontext.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
	
	vkcontext.fullscreen = 0;
	vkcontext.swapchain = VK_NULL_HANDLE;
	vkcontext.swapchainFormat = VK_FORMAT_UNDEFINED;
	vkcontext.currentSwapIndex = 0;
	vkcontext.msaaImage = VK_NULL_HANDLE;
	vkcontext.msaaImageView = VK_NULL_HANDLE;
	vkcontext.commandPool = VK_NULL_HANDLE;

	vkcontext.swapchainImages.Zero();
	vkcontext.swapchainViews.Zero();
	vkcontext.frameBuffers.Zero();

	vkcontext.commandBuffers.Zero();
	vkcontext.commandBufferFences.Zero();
	vkcontext.commandBufferRecorded.Zero();
	vkcontext.acquireSemaphores.Zero();
	vkcontext.renderCompleteSemaphores.Zero();

	vkcontext.queryIndex.Zero();
	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		vkcontext.queryResults[ i ].Zero();
	}
	vkcontext.queryPools.Zero();
}

/*
=============
idRenderBackend::Print
=============
*/
void idRenderBackend::Print() {

}

/*
=============
idRenderBackend::Init
=============
*/
void idRenderBackend::Init() {
	idLib::Printf( "----- idRenderBackend::Init -----\n" );

	// input and sound systems need to be tied to the new window
	Sys_InitInput();

	// Create the instance
	CreateInstance();

	// Create presentation surface
	CreateSurface();

	// Enumerate physical devices and get their properties
	SelectSuitablePhysicalDevice();

	// Create logical device and queues
	CreateLogicalDeviceAndQueues();

	// Create semaphores for image acquisition and rendering completion
	CreateSemaphores();

	// Create Query Pool
	CreateQueryPool();

	// Create Command Pool
	CreateCommandPool();

	// Create Command Buffer
	CreateCommandBuffer();

	// Setup the allocator
#if defined( ID_USE_AMD_ALLOCATOR )
	extern idCVar r_vkHostVisibleMemoryMB;
	extern idCVar r_vkDeviceLocalMemoryMB;

	VmaAllocatorCreateInfo createInfo = {};
	createInfo.physicalDevice = vkcontext.physicalDevice;
	createInfo.device = vkcontext.device;
	createInfo.preferredSmallHeapBlockSize = r_vkHostVisibleMemoryMB.GetInteger() * 1024 * 1024;
	createInfo.preferredLargeHeapBlockSize = r_vkDeviceLocalMemoryMB.GetInteger() * 1024 * 1024;

	vmaCreateAllocator( &createInfo, &vmaAllocator );
#else
	vulkanAllocator.Init();
#endif

	// Start the Staging Manager
	stagingManager.Init();

	// Create Swap Chain
	CreateSwapChain();

	// Create Render Targets
	CreateRenderTargets();

	// Create Render Pass
	CreateRenderPass();

	// Create Pipeline Cache
	CreatePipelineCache();

	// Create Frame Buffers
	CreateFrameBuffers();

	// Init RenderProg Manager
	renderProgManager.Init();

	// Init Vertex Cache
	vertexCache.Init( vkcontext.gpu.props.limits.minUniformBufferOffsetAlignment );
}

/*
=============
idRenderBackend::Shutdown
=============
*/
void idRenderBackend::Shutdown() {
	// Shutdown input
	Sys_ShutdownInput();

	vkDeviceWaitIdle( vkcontext.device );

	renderProgManager.Shutdown();

	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		idImage::EmptyGarbage();
	}

	// Detroy Frame Buffers
	DestroyFrameBuffers();

	// Destroy Pipeline Cache
	vkDestroyPipelineCache( vkcontext.device, vkcontext.pipelineCache, NULL );

	// Destroy Render Pass
	vkDestroyRenderPass( vkcontext.device, vkcontext.renderPass, NULL );

	// Destroy Render Targets
	DestroyRenderTargets();

	// Destroy Swap Chain
	DestroySwapChain();

	// Stop the Staging Manager
	stagingManager.Shutdown();

	// Destroy Command Buffer
	vkFreeCommandBuffers( vkcontext.device, vkcontext.commandPool, NUM_FRAME_DATA, vkcontext.commandBuffers.Ptr() );
	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		vkDestroyFence( vkcontext.device, vkcontext.commandBufferFences[ i ], NULL );
	}

	// Destroy Command Pool
	vkDestroyCommandPool( vkcontext.device, vkcontext.commandPool, NULL );

	// Destroy Query Pools
	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		vkDestroyQueryPool( vkcontext.device, vkcontext.queryPools[ i ], NULL );
	}

	// Destroy Semaphores
	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
		vkDestroySemaphore( vkcontext.device, vkcontext.acquireSemaphores[ i ], NULL );
		vkDestroySemaphore( vkcontext.device, vkcontext.renderCompleteSemaphores[ i ], NULL );
	}

	// Destroy Debug Callback
	if ( r_vkEnableValidationLayers.GetBool() ) {
		DestroyDebugReportCallback( vkcontext.instance );
	}

	// Dump all our memory
#if defined( ID_USE_AMD_ALLOCATOR )
	vmaDestroyAllocator( vmaAllocator );
#else
	vulkanAllocator.Shutdown();
#endif

	// Destroy Logical Device
	vkDestroyDevice( vkcontext.device, NULL );

	// Destroy Surface
	vkDestroySurfaceKHR( vkcontext.instance, vkcontext.surface, NULL );

	// Destroy the Instance
	vkDestroyInstance( vkcontext.instance, NULL );

	ClearContext();
	Clear();
}

/*
====================
idRenderBackend::ResizeImages
====================
*/
void idRenderBackend::ResizeImages() {
	VkExtent2D extent = ChooseSurfaceExtent( vkcontext.gpu.surfaceCaps );
	if ( vkcontext.swapchainExtent.width == extent.width && vkcontext.swapchainExtent.height == extent.height)
		return;

	stagingManager.Flush();
	
	vkDeviceWaitIdle( vkcontext.device );
	
	idImage::EmptyGarbage();

	// Destroy Frame Buffers
	DestroyFrameBuffers();

	// Destroy Render Targets
	DestroyRenderTargets();

	// Destroy Current Swap Chain
	DestroySwapChain();

	// Destroy Current Surface
	vkDestroySurfaceKHR( vkcontext.instance, vkcontext.surface, NULL );

#if !defined( ID_USE_AMD_ALLOCATOR )
	vulkanAllocator.EmptyGarbage();
#endif

	// Create New Surface
	CreateSurface();

	// Refresh Surface Capabilities
	ID_VK_CHECK( vkGetPhysicalDeviceSurfaceCapabilitiesKHR( vkcontext.physicalDevice, vkcontext.surface, &vkcontext.gpu.surfaceCaps ) );

	// Recheck presentation support
	VkBool32 supportsPresent = VK_FALSE;
	ID_VK_CHECK( vkGetPhysicalDeviceSurfaceSupportKHR( vkcontext.physicalDevice, vkcontext.presentFamilyIdx, vkcontext.surface, &supportsPresent ) );
	if ( supportsPresent == VK_FALSE ) {
		idLib::FatalError( "idRenderBackend::ResizeImages: New surface does not support present?" );
	}

	// Create New Swap Chain
	CreateSwapChain();

	// Create New Render Targets
	CreateRenderTargets();

	// Create New Frame Buffers
	CreateFrameBuffers();
}

/*
====================
idRenderBackend::BlockingSwapBuffers
====================
*/
void idRenderBackend::BlockingSwapBuffers() {
	RENDERLOG_PRINTF( "***************** BlockingSwapBuffers *****************\n\n\n" );

	if ( vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] == false ) {
		return;
	}	

	ID_VK_CHECK( vkWaitForFences( vkcontext.device, 1, &vkcontext.commandBufferFences[ vkcontext.currentFrameData ], VK_TRUE, UINT64_MAX ) );

	ID_VK_CHECK( vkResetFences( vkcontext.device, 1, &vkcontext.commandBufferFences[ vkcontext.currentFrameData ] ) );
	vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] = false;
		
	VkSemaphore * finished = &vkcontext.renderCompleteSemaphores[ vkcontext.currentFrameData ];

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = finished;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &vkcontext.swapchain;
	presentInfo.pImageIndices = &vkcontext.currentSwapIndex;

	ID_VK_CHECK( vkQueuePresentKHR( vkcontext.presentQueue, &presentInfo ) );

	vkcontext.counter++;
	vkcontext.currentFrameData = vkcontext.counter % NUM_FRAME_DATA;

	//vkDeviceWaitIdle( vkcontext.device );
}

/*
==================
idRenderBackend::CheckCVars
==================
*/
void idRenderBackend::CheckCVars() {

}

/*
=========================================================================================================

BACKEND COMMANDS

=========================================================================================================
*/

/*
=============
idRenderBackend::ExecuteBackEndCommands

This function will be called syncronously if running without
smp extensions, or asyncronously by another thread.
=============
*/
void idRenderBackend::ExecuteBackEndCommands( const emptyCommand_t* cmds )
{
	CheckCVars();

	//resolutionScale.SetCurrentGPUFrameTime( commonLocal.m_mainFrameTiming.gpuTime );

	ResizeImages();

	// renderLog.StartFrame();
	GL_StartFrame();

	uint64 backEndStartTime = Sys_Microseconds();

	// needed for editor rendering
	GL_SetDefaultState();

	for( ; cmds != NULL; cmds = ( const emptyCommand_t* )cmds->next )
	{
		switch( cmds->commandId ) {
			case RC_NOP:
				break;
			case RC_DRAW_VIEW_GUI:
			case RC_DRAW_VIEW_3D:
				DrawView( cmds, 0 );
				break;
			case RC_COPY_RENDER:
				CopyRender( cmds );
				break;
			case RC_SET_BUFFER:
				//RB_SetBuffer( cmds );
				//c_setBuffers++;
				break;
			case RC_POST_PROCESS:
				PostProcess( cmds );
				break;
			default:
				idLib::Error( "ExecuteBackEndCommands: bad commandId" );
				break;
		}
	}

	GL_EndFrame();

	// // stop rendering on this thread
	// //m_pc.totalMicroSec = Sys_Microseconds() - backEndStartTime;

	// renderLog.EndFrame();
}

/*
=============
idRenderBackend::DrawElementsWithCounters
=============
*/
void idRenderBackend::DrawElementsWithCounters( const drawSurf_t * surf ) {
	// get vertex buffer
	const vertCacheHandle_t vbHandle = surf->ambientCache;
	idVertexBuffer * vertexBuffer;
	if ( vertexCache.CacheIsStatic( vbHandle ) ) {
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	} else {
		const uint64 frameNum = (int)( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
			idLib::Warning( "idRenderBackend::DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[ vertexCache.drawListNum ].vertexBuffer;
	}
	int vertOffset = (int)( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	// get index buffer
	const vertCacheHandle_t ibHandle = surf->indexCache;
	idIndexBuffer * indexBuffer;
	if ( vertexCache.CacheIsStatic( ibHandle ) ) {
		indexBuffer = &vertexCache.staticData.indexBuffer;
	} else {
		const uint64 frameNum = (int)( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
			idLib::Warning( "idRenderBackend::DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[ vertexCache.drawListNum ].indexBuffer;
	}
	int indexOffset = (int)( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	//idLib::Printf( "Binding Buffers(%d): %p:%i %p:%i\n", surf->numIndexes, vertexBuffer, vertOffset, indexBuffer, indexOffset );

	const renderProg_t & prog = renderProgManager.GetCurrentRenderProg();

	if ( surf->jointCache ) {
		assert( prog.usesJoints );
		if ( !prog.usesJoints ) {
			return;
		}
	} else {
		assert( !prog.usesJoints || prog.optionalSkinning );
		if ( prog.usesJoints && !prog.optionalSkinning ) {
			return;
		}
	}

	vkcontext.jointCacheHandle = surf->jointCache;

	VkCommandBuffer commandBuffer = vkcontext.commandBuffers[ vkcontext.currentFrameData ];

	CommitCurrent(commandBuffer);

	{
		const VkBuffer buffer = indexBuffer->GetAPIObject();
		const VkDeviceSize offset = indexBuffer->GetOffset();
		vkCmdBindIndexBuffer( commandBuffer, buffer, offset, VK_INDEX_TYPE_UINT16 );
	}
	{
		const VkBuffer buffer = vertexBuffer->GetAPIObject();
		const VkDeviceSize offset = vertexBuffer->GetOffset();
		vkCmdBindVertexBuffers( commandBuffer, 0, 1, &buffer, &offset );
	}

	vkCmdDrawIndexed( commandBuffer, surf->numIndexes, 1, ( indexOffset >> 1 ), vertOffset / sizeof( idDrawVert ), 0 );
}

void idRenderBackend::CommitCurrent(VkCommandBuffer commandBuffer)
{
	PrintState( glStateBits );
	renderProgManager.CommitCurrent( glStateBits, faceCulling, commandBuffer );
}

/*
=========================================================================================================

GL COMMANDS

=========================================================================================================
*/

/*
==================
idRenderBackend::GL_StartFrame
==================
*/
void idRenderBackend::GL_StartFrame() {
	ID_VK_CHECK( vkAcquireNextImageKHR( vkcontext.device, vkcontext.swapchain, UINT64_MAX, vkcontext.acquireSemaphores[ vkcontext.currentFrameData ], VK_NULL_HANDLE, &vkcontext.currentSwapIndex ) );

	idImage::EmptyGarbage();
#if !defined( ID_USE_AMD_ALLOCATOR )
	vulkanAllocator.EmptyGarbage();
#endif
	stagingManager.Flush();
	renderProgManager.StartFrame();

	VkQueryPool queryPool = vkcontext.queryPools[ vkcontext.currentFrameData ];
	idArray< uint64, NUM_TIMESTAMP_QUERIES > & results = vkcontext.queryResults[ vkcontext.currentFrameData ];

	if ( vkcontext.queryIndex[ vkcontext.currentFrameData ] > 0 ) {
		vkGetQueryPoolResults( vkcontext.device, queryPool, 0, 2, 
			results.ByteSize(), results.Ptr(), sizeof( uint64 ), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT );

		const uint64 gpuStart = results[ 0 ];
		const uint64 gpuEnd = results[ 1 ];
		const uint64 tick = ( 1000 * 1000 * 1000 ) / vkcontext.gpu.props.limits.timestampPeriod;
		pc.gpuMicroSec = ( ( gpuEnd - gpuStart ) * 1000 * 1000 ) / tick;

		vkcontext.queryIndex[ vkcontext.currentFrameData ] = 0;
	}

	VkCommandBuffer commandBuffer = vkcontext.commandBuffers[ vkcontext.currentFrameData ];

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	ID_VK_CHECK( vkBeginCommandBuffer( commandBuffer, &commandBufferBeginInfo ) );

	vkCmdResetQueryPool( commandBuffer, queryPool, 0, NUM_TIMESTAMP_QUERIES );

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkcontext.renderPass;
	renderPassBeginInfo.framebuffer = vkcontext.frameBuffers[ vkcontext.currentSwapIndex ];
	renderPassBeginInfo.renderArea.extent = vkcontext.swapchainExtent;

	vkCmdBeginRenderPass( commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

	vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, queryPool, vkcontext.queryIndex[ vkcontext.currentFrameData ]++ );
}

/*
==================
idRenderBackend::GL_EndFrame
==================
*/
void idRenderBackend::GL_EndFrame() {
	VkCommandBuffer commandBuffer = vkcontext.commandBuffers[ vkcontext.currentFrameData ];

	vkCmdWriteTimestamp( commandBuffer, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, vkcontext.queryPools[ vkcontext.currentFrameData ], vkcontext.queryIndex[ vkcontext.currentFrameData ]++ );

	vkCmdEndRenderPass( commandBuffer );

	// Transition our swap image to present.
	// Do this instead of having the renderpass do the transition
	// so we can take advantage of the general layout to avoid 
	// additional image barriers.
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = vkcontext.swapchainImages[ vkcontext.currentSwapIndex ];
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	barrier.dstAccessMask = 0;

	vkCmdPipelineBarrier( 
		commandBuffer, 
		VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		0, 0, NULL, 0, NULL, 1, &barrier );

	ID_VK_CHECK( vkEndCommandBuffer( commandBuffer ) )
	vkcontext.commandBufferRecorded[ vkcontext.currentFrameData ] = true;

	VkSemaphore * acquire = &vkcontext.acquireSemaphores[ vkcontext.currentFrameData ];
	VkSemaphore * finished = &vkcontext.renderCompleteSemaphores[ vkcontext.currentFrameData ];

	VkPipelineStageFlags dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = acquire;
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = finished;
	submitInfo.pWaitDstStageMask = &dstStageMask;

	ID_VK_CHECK( vkQueueSubmit( vkcontext.graphicsQueue, 1, &submitInfo, vkcontext.commandBufferFences[ vkcontext.currentFrameData ] ) );
}

/*
========================
idRenderBackend::GL_SetDefaultState

This should initialize all GL state that any part of the entire program
may touch, including the editor.
========================
*/
void idRenderBackend::GL_SetDefaultState() {
	RENDERLOG_PRINTF( "--- GL_SetDefaultState ---\n" );
	glStateBits = 0;
	faceCulling = CT_INVALID;

	GL_State( 0, true );

	GL_Scissor( 0, 0, renderSystem->GetWidth(), renderSystem->GetHeight() );
}

/*
====================
idRenderBackend::GL_State

This routine is responsible for setting the most commonly changed state
====================
*/
void idRenderBackend::GL_State( uint64 stateBits, bool forceGlState ) {
	glStateBits = stateBits | ( glStateBits & GLS_KEEP );
	if ( viewDef != NULL && viewDef->isMirror ) {
		glStateBits |= GLS_MIRROR_VIEW;
	}
}

/*
====================
idRenderBackend::GL_BindTexture
====================
*/
void idRenderBackend::GL_BindTexture( int index, idImage * image ) {
	//idLib::Printf( "GL_BindTexture( %d, %s ) repeat %d\n", index, image->GetName(), image->repeat );

	vkcontext.imageParms[ index ] = image;
}

/*
====================
idRenderBackend::GL_CopyFrameBuffer
====================
*/
void idRenderBackend::GL_CopyFrameBuffer( idImage * image, int x, int y, int imageWidth, int imageHeight ) {
	VkCommandBuffer commandBuffer = vkcontext.commandBuffers[ vkcontext.currentFrameData ];
	vkCmdEndRenderPass( commandBuffer );

	VkImageMemoryBarrier dstBarrier = {};
	dstBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	dstBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	dstBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	dstBarrier.image = image->GetImage();
	dstBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	dstBarrier.subresourceRange.baseMipLevel = 0;
	dstBarrier.subresourceRange.levelCount = 1;
	dstBarrier.subresourceRange.baseArrayLayer = 0;
	dstBarrier.subresourceRange.layerCount = 1;

	// Pre copy transitions
	{
		// Transition the color dst image so we can transfer to it.
		dstBarrier.oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		dstBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		dstBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dstBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		vkCmdPipelineBarrier( 
			commandBuffer, 
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			0, 0, NULL, 0, NULL, 1, &dstBarrier );
	}

	// Perform the blit/copy
	{
		VkImageBlit region = {};
		region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.srcSubresource.baseArrayLayer = 0;
		region.srcSubresource.mipLevel = 0;
		region.srcSubresource.layerCount = 1;
		region.srcOffsets[ 1 ] = { imageWidth, imageHeight, 1 };

		region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.dstSubresource.baseArrayLayer = 0;
		region.dstSubresource.mipLevel = 0;
		region.dstSubresource.layerCount = 1;
		region.dstOffsets[ 1 ] = { imageWidth, imageHeight, 1 };

		vkCmdBlitImage( 
			commandBuffer, 
			vkcontext.swapchainImages[ vkcontext.currentSwapIndex ], VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image->GetImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, &region, VK_FILTER_NEAREST );
	}

	// Post copy transitions
	{
		// Transition the color dst image so we can transfer to it.
		dstBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		dstBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		dstBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		dstBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier( 
			commandBuffer, 
			VK_PIPELINE_STAGE_TRANSFER_BIT, 
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 
			0, 0, NULL, 0, NULL, 1, &dstBarrier );
	}

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = vkcontext.renderPass;
	renderPassBeginInfo.framebuffer = vkcontext.frameBuffers[ vkcontext.currentSwapIndex ];
	renderPassBeginInfo.renderArea.extent = vkcontext.swapchainExtent;

	vkCmdBeginRenderPass( commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void idRenderBackend::GL_CopyDepthBuffer( idImage * image, int x, int y, int imageWidth, int imageHeight ) 
{
	idLib::Printf("GL_CopyDepthBuffer NI");
	assert(false);
}

/*
====================
idRenderBackend::GL_Color
====================
*/
void idRenderBackend::GL_Color( float r, float g, float b, float a )
{
	float parm[4];
	parm[0] = idMath::ClampFloat( 0.0f, 1.0f, r );
	parm[1] = idMath::ClampFloat( 0.0f, 1.0f, g );
	parm[2] = idMath::ClampFloat( 0.0f, 1.0f, b );
	parm[3] = idMath::ClampFloat( 0.0f, 1.0f, a );
	renderProgManager.SetRenderParm( RENDERPARM_COLOR, parm );
}

void idRenderBackend::GL_Cull( cullType_t cullType )
{
	assert(cullType != CT_INVALID);
	faceCulling = cullType;
}

/*
========================
idRenderBackend::GL_Clear
========================
*/
void idRenderBackend::GL_Clear( bool color, bool depth, bool stencil, byte stencilValue, float r, float g, float b, float a, bool clearHdr ) {
	//idLib::Printf( "GL_Clear( color=%d, depth=%d, stencil=%d, stencil=%d, r=%f, g=%f, b=%f, a=%f )\n", 
	//	color, depth, stencil, stencilValue, r, g, b, a );
	assert(!clearHdr);

	uint32 numAttachments = 0;
	VkClearAttachment attachments[ 2 ];
	memset( attachments, 0, sizeof( attachments ) );

	if ( color ) {
		VkClearAttachment & attachment = attachments[ numAttachments++ ];
		attachment.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		attachment.colorAttachment = 0;
		VkClearColorValue & color = attachment.clearValue.color;
		color.float32[ 0 ] = 1;
		color.float32[ 1 ] = 1;
		color.float32[ 2 ] = 1;
		color.float32[ 3 ] = 1;
	}

	if ( depth || stencil ) {
		VkClearAttachment & attachment = attachments[ numAttachments++ ];
		if ( depth ) {
			attachment.aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
		}
		if ( stencil ) {
			attachment.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
		attachment.clearValue.depthStencil.depth = 1.0f;
		attachment.clearValue.depthStencil.stencil = stencilValue;
	}

	VkClearRect clearRect = {};
	clearRect.baseArrayLayer = 0;
	clearRect.layerCount = 1;
	clearRect.rect.extent = vkcontext.swapchainExtent;

	vkCmdClearAttachments( vkcontext.commandBuffers[ vkcontext.currentFrameData ], numAttachments, attachments, 1, &clearRect );
}

/*
========================
idRenderBackend::GL_DepthBoundsTest
========================
*/
void idRenderBackend::GL_DepthBoundsTest( const float zmin, const float zmax ) {
	if ( !vkcontext.gpu.features.depthBounds || zmin > zmax ) {
		return;
	}

	assert(false);
	if ( zmin == 0.0f && zmax == 0.0f ) {
		glStateBits = glStateBits & ~GLS_DEPTH_TEST_MASK;
	} else {
		glStateBits |= GLS_DEPTH_TEST_MASK;
		vkCmdSetDepthBounds( vkcontext.commandBuffers[ vkcontext.currentFrameData ], zmin, zmax );
	}

	idLib::Printf( "GL_DepthBoundsTest( zmin=%f, zmax=%f )\n", zmin, zmax );
}

/*
====================
idRenderBackend::GL_PolygonOffset
====================
*/
void idRenderBackend::GL_PolygonOffset( float scale, float bias ) {
	vkCmdSetDepthBias( vkcontext.commandBuffers[ vkcontext.currentFrameData ], bias, 0.0f, scale );
	
	RENDERLOG_PRINTF( "GL_PolygonOffset( scale=%f, bias=%f )\n", scale, bias );
}

/*
====================
idRenderBackend::GL_Scissor
====================
*/
void idRenderBackend::GL_Scissor( int x /* left*/, int y /* bottom */, int w, int h ) {
	VkRect2D scissor;
	scissor.offset.x = x;
	scissor.offset.y = y;
	scissor.extent.width = w;
	scissor.extent.height = h;
	vkCmdSetScissor( vkcontext.commandBuffers[ vkcontext.currentFrameData ], 0, 1, &scissor );
}

/*
====================
idRenderBackend::GL_Viewport
====================
*/
void idRenderBackend::GL_Viewport( int x /* left */, int y /* bottom */, int w, int h ) {
	VkViewport viewport;
	viewport.x = x;
	viewport.y = y;
	viewport.width = w;
	viewport.height = h;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport( vkcontext.commandBuffers[ vkcontext.currentFrameData ], 0, 1, &viewport );
}

/*
==============================================================================================

STENCIL SHADOW RENDERING

==============================================================================================
*/

/*
=====================
idRenderBackend::DrawStencilShadowPass
=====================
*/
void idRenderBackend::DrawStencilShadowPass( const drawSurf_t * drawSurf, const bool renderZPass ) {
	if ( renderZPass ) {
		// Z-pass
		uint64 stencil = GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR
						| GLS_BACK_STENCIL_OP_FAIL_KEEP | GLS_BACK_STENCIL_OP_ZFAIL_KEEP | GLS_BACK_STENCIL_OP_PASS_DECR;
		GL_State( glStateBits & ~GLS_STENCIL_OP_BITS | stencil );
	} else if ( r_useStencilShadowPreload.GetBool() ) {
		// preload + Z-pass
		uint64 stencil = GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_DECR | GLS_STENCIL_OP_PASS_DECR
						| GLS_BACK_STENCIL_OP_FAIL_KEEP | GLS_BACK_STENCIL_OP_ZFAIL_INCR | GLS_BACK_STENCIL_OP_PASS_INCR;
		GL_State( glStateBits & ~GLS_STENCIL_OP_BITS | stencil );
	} else {
		// Z-fail
	}

	// get vertex buffer
	const vertCacheHandle_t vbHandle = drawSurf->shadowCache;
	idVertexBuffer * vertexBuffer;
	if ( vertexCache.CacheIsStatic( vbHandle ) ) {
		vertexBuffer = &vertexCache.staticData.vertexBuffer;
	} else {
		const uint64 frameNum = (int)( vbHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
			idLib::Warning( "RB_DrawElementsWithCounters, vertexBuffer == NULL" );
			return;
		}
		vertexBuffer = &vertexCache.frameData[ vertexCache.drawListNum ].vertexBuffer;
	}
	const int vertOffset = (int)( vbHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	// get index buffer
	const vertCacheHandle_t ibHandle = drawSurf->indexCache;
	idIndexBuffer * indexBuffer;
	if ( vertexCache.CacheIsStatic( ibHandle ) ) {
		indexBuffer = &vertexCache.staticData.indexBuffer;
	} else {
		const uint64 frameNum = (int)( ibHandle >> VERTCACHE_FRAME_SHIFT ) & VERTCACHE_FRAME_MASK;
		if ( frameNum != ( ( vertexCache.currentFrame - 1 ) & VERTCACHE_FRAME_MASK ) ) {
			idLib::Warning( "RB_DrawElementsWithCounters, indexBuffer == NULL" );
			return;
		}
		indexBuffer = &vertexCache.frameData[ vertexCache.drawListNum ].indexBuffer;
	}
	int indexOffset = (int)( ibHandle >> VERTCACHE_OFFSET_SHIFT ) & VERTCACHE_OFFSET_MASK;

	RENDERLOG_PRINTF( "Binding Buffers(%d): %p:%i %p:%i\n", drawSurf->numIndexes, vertexBuffer, vertOffset, indexBuffer, indexOffset );
	
	vkcontext.jointCacheHandle = drawSurf->jointCache;
	
	VkCommandBuffer commandBuffer = vkcontext.commandBuffers[ vkcontext.currentFrameData ];
	
	CommitCurrent( commandBuffer );

	{
		const VkBuffer buffer = indexBuffer->GetAPIObject();
		const VkDeviceSize offset = indexBuffer->GetOffset();
		vkCmdBindIndexBuffer( commandBuffer, buffer, offset, VK_INDEX_TYPE_UINT16 );
	}
	{
		const VkBuffer buffer = vertexBuffer->GetAPIObject();
		const VkDeviceSize offset = vertexBuffer->GetOffset();
		vkCmdBindVertexBuffers( commandBuffer, 0, 1, &buffer, &offset );
	}

	const int baseVertex = vertOffset / ( drawSurf->jointCache ? sizeof( idShadowVertSkinned ) : sizeof( idShadowVert ) );

	vkCmdDrawIndexed( commandBuffer, drawSurf->numIndexes, 1, ( indexOffset >> 1 ), baseVertex, 0 );

	if ( !renderZPass && r_useStencilShadowPreload.GetBool() ) {
		// render again with Z-pass
		uint64 stencil = GLS_STENCIL_OP_FAIL_KEEP | GLS_STENCIL_OP_ZFAIL_KEEP | GLS_STENCIL_OP_PASS_INCR
						| GLS_BACK_STENCIL_OP_FAIL_KEEP | GLS_BACK_STENCIL_OP_ZFAIL_KEEP | GLS_BACK_STENCIL_OP_PASS_DECR;
		GL_State( glStateBits & ~GLS_STENCIL_OP_BITS | stencil );

		CommitCurrent(commandBuffer );

		vkCmdDrawIndexed( commandBuffer, drawSurf->numIndexes, 1, ( indexOffset >> 1 ), baseVertex, 0 );
	}
}
