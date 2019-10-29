#include "../tr_local.h"

#define max(a,b) (((a)>(b))?(a):(b))

void VK_CreateBottomAS(VkCommandBuffer commandBuffer, vkbottomAS_t* bas, VkBuildAccelerationStructureFlagsNV flag, VkDeviceSize* offset) {
	// create
	VkAccelerationStructureInfoNV accelerationStructureInfo = { 0 };
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	accelerationStructureInfo.flags = flag;
	accelerationStructureInfo.instanceCount = 0;
	accelerationStructureInfo.geometryCount = 1;
	accelerationStructureInfo.pGeometries = &bas->geometries;

	VkAccelerationStructureCreateInfoNV accelerationStructureCI = { 0 };
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	VK_CHECK(vkCreateAccelerationStructureNV(vk.device, &accelerationStructureCI, NULL, &bas->accelerationStructure), "failed to create Bottom Level Acceleration Structure NV");

	// Get Memory Info
	VkMemoryRequirements2 memoryRequirements2Scratch = { 0 };
	VK_GetAccelerationStructureMemoryRequirements(bas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV, &memoryRequirements2Scratch);

	VkMemoryRequirements2 memoryRequirements2 = { 0 };
	VK_GetAccelerationStructureMemoryRequirements(bas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV, &memoryRequirements2);

	VkBindAccelerationStructureMemoryInfoNV memoryInfo = { 0 };
	memoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	memoryInfo.accelerationStructure = bas->accelerationStructure;
	memoryInfo.memory = vk_d.basBuffer.memory;
	if (offset != NULL) {
		memoryInfo.memoryOffset = bas->offset = *offset;
		*offset += (memoryRequirements2.memoryRequirements.size);
	}
	else memoryInfo.memoryOffset = bas->offset;
	
	VK_CHECK(vkBindAccelerationStructureMemoryNV(vk.device, 1, &memoryInfo), "failed to bind Acceleration Structure Memory NV");
	VK_CHECK(vkGetAccelerationStructureHandleNV(vk.device, bas->accelerationStructure, sizeof(uint64_t), &bas->handle), "failed to get Acceleration Structure Handle NV");
	
	// build
	VkAccelerationStructureInfoNV buildInfo = { 0 };
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.flags = flag;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &bas->geometries;

	vkCmdBuildAccelerationStructureNV(
		commandBuffer,
		&buildInfo,
		VK_NULL_HANDLE,
		0,
		VK_FALSE,
		bas->accelerationStructure,
		VK_NULL_HANDLE,
		vk_d.scratchBuffer.buffer,
		vk_d.scratchBufferOffset);
	vk_d.scratchBufferOffset += memoryRequirements2Scratch.memoryRequirements.size;
}
void VK_UpdateBottomAS(VkCommandBuffer commandBuffer, vkbottomAS_t* oldBas, vkbottomAS_t* newBas, VkBuildAccelerationStructureFlagsNV flag, VkDeviceSize* offset) {
	
	VkMemoryRequirements2 memoryRequirements2Scratch = { 0 };
	VkMemoryRequirements2 memoryRequirements2 = { 0 };
	// if old and new are the same do not create new as
	if (oldBas == newBas) {
		// Get Memory Info
		VK_GetAccelerationStructureMemoryRequirements(oldBas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV, &memoryRequirements2Scratch);
		VK_GetAccelerationStructureMemoryRequirements(oldBas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV, &memoryRequirements2);
	}
	else {
		// create as
		VkAccelerationStructureInfoNV accelerationStructureInfo = { 0 };
		accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		accelerationStructureInfo.flags = flag;
		accelerationStructureInfo.instanceCount = 0;
		accelerationStructureInfo.geometryCount = 1;
		accelerationStructureInfo.pGeometries = &newBas->geometries;

		VkAccelerationStructureCreateInfoNV accelerationStructureCI = { 0 };
		accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
		accelerationStructureCI.info = accelerationStructureInfo;
		VK_CHECK(vkCreateAccelerationStructureNV(vk.device, &accelerationStructureCI, NULL, &newBas->accelerationStructure), "failed to create Bottom Level Acceleration Structure NV");

		// Get Memory Info
		VK_GetAccelerationStructureMemoryRequirements(newBas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV, &memoryRequirements2Scratch);
		VK_GetAccelerationStructureMemoryRequirements(newBas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV, &memoryRequirements2);

		VkBindAccelerationStructureMemoryInfoNV memoryInfo = { 0 };
		memoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
		memoryInfo.accelerationStructure = newBas->accelerationStructure;
		memoryInfo.memory = vk_d.basBuffer.memory;
		memoryInfo.memoryOffset = newBas->offset = *offset;
		*offset += (memoryRequirements2.memoryRequirements.size);

		VK_CHECK(vkBindAccelerationStructureMemoryNV(vk.device, 1, &memoryInfo), "failed to bind Acceleration Structure Memory NV");
		VK_CHECK(vkGetAccelerationStructureHandleNV(vk.device, newBas->accelerationStructure, sizeof(uint64_t), &newBas->handle), "failed to get Acceleration Structure Handle NV");
	}

	// build
	VkAccelerationStructureInfoNV buildInfo = { 0 };
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
	buildInfo.flags = flag;
	buildInfo.geometryCount = 1;
	buildInfo.pGeometries = &newBas->geometries;

	VkMemoryBarrier memoryBarrier = { 0 };
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	vkCmdBuildAccelerationStructureNV(
		commandBuffer,
		&buildInfo,
		VK_NULL_HANDLE,
		0,
		VK_TRUE,
		newBas->accelerationStructure,
		oldBas->accelerationStructure,
		vk_d.scratchBuffer.buffer,
		vk_d.scratchBufferOffset);
	vk_d.scratchBufferOffset += memoryRequirements2Scratch.memoryRequirements.size;

	//vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);
}

// destroy and create same AS
void VK_RecreateBottomAS(VkCommandBuffer commandBuffer, vkbottomAS_t* bas, VkBuildAccelerationStructureFlagsNV flag) {
	vkDestroyAccelerationStructureNV(vk.device, bas->accelerationStructure, NULL);
	bas->handle = 0;
	VK_CreateBottomAS(commandBuffer, bas, flag, NULL);
}

void VK_MakeTop(VkCommandBuffer commandBuffer, vktopAS_t* tas, vkbottomAS_t* basList, uint32_t basCount, VkBuildAccelerationStructureFlagsNV flag) {
	VkAccelerationStructureInfoNV accelerationStructureInfo = { 0 };
	accelerationStructureInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.flags = flag;
	accelerationStructureInfo.instanceCount = basCount;
	accelerationStructureInfo.geometryCount = 0;

	VkAccelerationStructureCreateInfoNV accelerationStructureCI = { 0 };
	accelerationStructureCI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV;
	accelerationStructureCI.info = accelerationStructureInfo;
	VK_CHECK(vkCreateAccelerationStructureNV(vk.device, &accelerationStructureCI, NULL, &tas->accelerationStructure), "failed to create Bottom Level Acceleration Structure NV");

	// Get Memory Info
	VkMemoryRequirements2 memoryRequirements2Scratch = { 0 };
	VK_GetAccelerationStructureMemoryRequirements(tas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV, &memoryRequirements2Scratch);

	VkMemoryRequirements2 memoryRequirements2 = { 0 };
	VK_GetAccelerationStructureMemoryRequirements(tas->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV, &memoryRequirements2);


	/*
	VkMemoryAllocateInfo memoryAllocateInfo = { 0 };
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = VK_FindMemoryTypeIndex(memoryRequirements2.memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VK_CHECK(vkAllocateMemory(vk.device, &memoryAllocateInfo, NULL, &bList->topMemory), "failed to Allocate Memory NV");
	*/

	VkBindAccelerationStructureMemoryInfoNV memoryInfo = { 0 };
	memoryInfo.sType = VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV;
	memoryInfo.accelerationStructure = tas->accelerationStructure;
	memoryInfo.memoryOffset = vk_d.tasBufferOffset;
	memoryInfo.memory = vk_d.tasBuffer.memory;
	//tas->offset = vk_d.tasBufferOffset;
	//vk_d.tasBufferOffset += (memoryRequirements2.memoryRequirements.size* 2);

	VK_CHECK(vkBindAccelerationStructureMemoryNV(vk.device, 1, &memoryInfo), "failed to bind Acceleration Structure Memory NV");
	VK_CHECK(vkGetAccelerationStructureHandleNV(vk.device, tas->accelerationStructure, sizeof(uint64_t), &tas->handle), "failed to get Acceleration Structure Handle NV");

	// build


	VkAccelerationStructureInfoNV buildInfo = { 0 };
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.flags = flag;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = basCount;

	uint32_t numIndex = 0;

	/*VkCommandBuffer commandBuffer = { 0 };
	VK_BeginSingleTimeCommands(&commandBuffer);*/

	VkMemoryBarrier memoryBarrier = { 0 };
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	memoryBarrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_NV | VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	vkCmdBuildAccelerationStructureNV(
		commandBuffer,
		&buildInfo,
		vk_d.instanceBuffer.buffer,
		vk_d.instanceBufferOffset,//vk_d.instanceBufferOffset,
		VK_FALSE,
		tas->accelerationStructure,
		VK_NULL_HANDLE,
		vk_d.scratchBuffer.buffer,
		vk_d.scratchBufferOffset);
	vk_d.scratchBufferOffset += memoryRequirements2Scratch.memoryRequirements.size;
	vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_NV, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	//VK_EndSingleTimeCommands(&commandBuffer);

}


void VK_UpdateTop(VkCommandBuffer commandBuffer, vktopAS_t* top, vkbottomAS_t* bottomList, uint32_t bottomCount)
{
	// Get Memory Info
	VkMemoryRequirements2 memoryRequirements2Scratch = { 0 };
	VK_GetAccelerationStructureMemoryRequirements(top->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_UPDATE_SCRATCH_NV, &memoryRequirements2Scratch);

	VkMemoryRequirements2 memoryRequirements2 = { 0 };
	VK_GetAccelerationStructureMemoryRequirements(top->accelerationStructure, VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV, &memoryRequirements2);

	// build

	// create instance buffer
	VkGeometryInstanceNV geometryInstance = { 0 };
	float transform[12] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f
	};
	Com_Memcpy(&geometryInstance.transform, &transform, sizeof(float[12]));
	geometryInstance.instanceCustomIndex = 0;
	geometryInstance.mask = 0xff;
	geometryInstance.instanceOffset = 0;
	geometryInstance.flags = 0;//VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;


	VkAccelerationStructureInfoNV buildInfo = { 0 };
	buildInfo.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV;
	buildInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	buildInfo.flags = VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV;
	buildInfo.pGeometries = 0;
	buildInfo.geometryCount = 0;
	buildInfo.instanceCount = bottomCount;

	uint32_t numIndex = 0;
	
	vkCmdBuildAccelerationStructureNV(
		commandBuffer,
		&buildInfo,
		vk_d.instanceBuffer.buffer,
		0,
		VK_TRUE,
		top->accelerationStructure,
		top->accelerationStructure,
		vk_d.scratchBuffer.buffer,
		vk_d.scratchBufferOffset);
	vk_d.scratchBufferOffset += memoryRequirements2Scratch.memoryRequirements.size;

}
void VK_DestroyTopAccelerationStructure(vktopAS_t* as) {
	if (as->accelerationStructure != VK_NULL_HANDLE) {
		vkDestroyAccelerationStructureNV(vk.device, as->accelerationStructure, NULL);
		as->accelerationStructure = VK_NULL_HANDLE;
	}
	memset(as, 0, sizeof(vktopAS_t));
}

void VK_DestroyBottomAccelerationStructure(vkbottomAS_t* as) {
	if (as->accelerationStructure != VK_NULL_HANDLE) {
		vkDestroyAccelerationStructureNV(vk.device, as->accelerationStructure, NULL);
		as->accelerationStructure = VK_NULL_HANDLE;
	}
	memset(as, 0, sizeof(vkbottomAS_t));
}

void VK_DestroyAllAccelerationStructures() {
	
	VK_DestroyTopAccelerationStructure(&vk_d.topAS[0]);
	VK_DestroyTopAccelerationStructure(&vk_d.topAS[1]);
	VK_DestroyTopAccelerationStructure(&vk_d.topAS[2]);
	int i = 0;
	for (i = 0; i < vk_d.bottomASCount; ++i) {
		VK_DestroyBottomAccelerationStructure(&vk_d.bottomASList[i]);
	}

	vk_d.bottomASCount = 0;
}