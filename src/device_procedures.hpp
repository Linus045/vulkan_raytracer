#pragma once

#include <vulkan/vulkan_core.h>
namespace ltracer
{
namespace procedures
{

// Device Pointer Functions
static PFN_vkCmdTraceRaysKHR pvkCmdTraceRaysKHR = NULL;
static PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR = NULL;
static PFN_vkCreateRayTracingPipelinesKHR pvkCreateRayTracingPipelinesKHR = NULL;
static PFN_vkGetAccelerationStructureBuildSizesKHR pvkGetAccelerationStructureBuildSizesKHR = NULL;
static PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR = NULL;
static PFN_vkGetAccelerationStructureDeviceAddressKHR pvkGetAccelerationStructureDeviceAddressKHR
    = NULL;
static PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR = NULL;
static PFN_vkDestroyAccelerationStructureKHR pvkDestroyAccelerationStructureKHR = NULL;
static PFN_vkGetRayTracingShaderGroupHandlesKHR pvkGetRayTracingShaderGroupHandlesKHR = NULL;

inline void grabDeviceProcAddr(VkDevice logicalDevice)
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
