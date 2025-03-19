#include <vulkan/vulkan_core.h>
#include "device_procedures.hpp"

namespace ltracer
{
namespace procedures
{

// Device Pointer Functions
PFN_vkCmdTraceRaysKHR pvkCmdTraceRaysKHR = NULL;
PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = NULL;
PFN_vkCreateRayTracingPipelinesKHR pvkCreateRayTracingPipelinesKHR = NULL;
PFN_vkGetAccelerationStructureBuildSizesKHR pvkGetAccelerationStructureBuildSizesKHR = NULL;
PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR = NULL;
PFN_vkGetAccelerationStructureDeviceAddressKHR pvkGetAccelerationStructureDeviceAddressKHR = NULL;
PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR = NULL;
PFN_vkDestroyAccelerationStructureKHR pvkDestroyAccelerationStructureKHR = NULL;
PFN_vkGetRayTracingShaderGroupHandlesKHR pvkGetRayTracingShaderGroupHandlesKHR = NULL;

void grabDeviceProcAddr(VkDevice logicalDevice)
{
	pvkGetBufferDeviceAddressKHR = (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkGetBufferDeviceAddressKHR");

	pvkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkCreateRayTracingPipelinesKHR");

	pvkGetAccelerationStructureBuildSizesKHR
	    = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");

	pvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkCreateAccelerationStructureKHR");

	pvkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkDestroyAccelerationStructureKHR");

	pvkGetAccelerationStructureDeviceAddressKHR
	    = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");

	pvkCmdBuildAccelerationStructuresKHR
	    = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkCmdBuildAccelerationStructuresKHR");

	pvkGetRayTracingShaderGroupHandlesKHR
	    = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");

	pvkCmdTraceRaysKHR
	    = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(logicalDevice, "vkCmdTraceRaysKHR");
}

} // namespace procedures
} // namespace ltracer
