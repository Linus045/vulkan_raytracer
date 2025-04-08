#include <stdexcept>
#include <vulkan/vulkan_core.h>
#include "device_procedures.hpp"

namespace tracer
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
	    logicalDevice, "vkGetBufferDeviceAddress");
	if (pvkGetBufferDeviceAddressKHR == nullptr)
	{
		throw std::runtime_error("Error: grabDeviceProcAddr - vkGetBufferDeviceAddressKHR is NULL. "
		                         "Is this feature enabled?");
	}

	pvkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkCreateRayTracingPipelinesKHR");
	if (pvkCreateRayTracingPipelinesKHR == nullptr)
	{
		throw std::runtime_error(
		    "Error: grabDeviceProcAddr - vkCreateRayTracingPipelinesKHR is NULL. Is "
		    "this feature enabled?");
	}

	pvkGetAccelerationStructureBuildSizesKHR
	    = (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkGetAccelerationStructureBuildSizesKHR");
	if (pvkGetAccelerationStructureBuildSizesKHR == nullptr)
	{
		throw std::runtime_error("Error: grabDeviceProcAddr - "
		                         "vkGetAccelerationStructureBuildSizesKHR is NULL. Is this feature "
		                         "enabled?");
	}

	pvkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkCreateAccelerationStructureKHR");
	if (pvkCreateAccelerationStructureKHR == nullptr)
	{
		throw std::runtime_error(
		    "Error: grabDeviceProcAddr - vkCreateAccelerationStructureKHR is NULL. Is this feature "
		    "enabled?");
	}

	pvkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(
	    logicalDevice, "vkDestroyAccelerationStructureKHR");
	if (pvkDestroyAccelerationStructureKHR == nullptr)
	{
		throw std::runtime_error("Error: grabDeviceProcAddr - vkDestroyAccelerationStructureKHR is "
		                         "NULL. Is this feature "
		                         "enabled?");
	}

	pvkGetAccelerationStructureDeviceAddressKHR
	    = (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkGetAccelerationStructureDeviceAddressKHR");
	if (pvkGetAccelerationStructureDeviceAddressKHR == nullptr)
	{
		throw std::runtime_error("Error: grabDeviceProcAddr - "
		                         "vkGetAccelerationStructureDeviceAddressKHR is NULL. Is this "
		                         "feature enabled?");
	}

	pvkCmdBuildAccelerationStructuresKHR
	    = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkCmdBuildAccelerationStructuresKHR");
	if (pvkCmdBuildAccelerationStructuresKHR == nullptr)
	{
		throw std::runtime_error("Error: grabDeviceProcAddr - vkCmdBuildAccelerationStructuresKHR "
		                         "is NULL. Is this feature "
		                         "enabled?");
	}

	pvkGetRayTracingShaderGroupHandlesKHR
	    = (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(
	        logicalDevice, "vkGetRayTracingShaderGroupHandlesKHR");
	if (pvkGetRayTracingShaderGroupHandlesKHR == nullptr)
	{
		throw std::runtime_error("Error: grabDeviceProcAddr - vkGetRayTracingShaderGroupHandlesKHR "
		                         "is NULL. Is this feature "
		                         "enabled?");
	}

	pvkCmdTraceRaysKHR
	    = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(logicalDevice, "vkCmdTraceRaysKHR");
	if (pvkCmdTraceRaysKHR == nullptr)
	{
		throw std::runtime_error(
		    "Error: grabDeviceProcAddr - vkCmdTraceRaysKHR is NULL. Is this feature enabled?");
	}
}

} // namespace procedures
} // namespace tracer
