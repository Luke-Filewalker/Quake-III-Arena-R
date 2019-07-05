#include "../tr_local.h"
#include <stdlib.h>

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR* availableFormats, uint32_t availableFormatsCount);
static VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR* availablePresentModes, uint32_t availablePresentModesCount);

void VK_CreateSwapChain() {
	vk.swapchain.depthStencilFormat = VK_FORMAT_D24_UNORM_S8_UINT;

	swapChainSupportDetails_t swapChainSupport = querySwapChainSupport(vk.physical_device, vk.surface);

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(&swapChainSupport.formats[0], swapChainSupport.formatCount);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(&swapChainSupport.presentModes[0], swapChainSupport.presentModeCount);
	
	// set extent
	vk.swapchain.extent = swapChainSupport.capabilities.currentExtent;
	vk.swapchain.extent.width = max(swapChainSupport.capabilities.minImageExtent.width, min(swapChainSupport.capabilities.maxImageExtent.width, vk.swapchain.extent.width));
	vk.swapchain.extent.height = max(swapChainSupport.capabilities.minImageExtent.height, min(swapChainSupport.capabilities.maxImageExtent.height, vk.swapchain.extent.height));

	vk.swapchain.imageFormat = surfaceFormat.format;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = vk.surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = vk.swapchain.extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	queueFamilyIndices_t indices = findQueueFamilies(vk.physical_device, vk.surface);
	uint32_t queueFamilyIndices[] = { indices.graphicsFamily, indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK(vkCreateSwapchainKHR(vk.device, &createInfo, NULL, &vk.swapchain.handle), "failed to create Swapchain!");

	vkGetSwapchainImagesKHR(vk.device, vk.swapchain.handle, &vk.swapchain.imageCount, NULL);
	vk.swapchain.images = malloc(vk.swapchain.imageCount * sizeof(VkImage));
	vkGetSwapchainImagesKHR(vk.device, vk.swapchain.handle, &imageCount, &vk.swapchain.images[0]);

}

void VK_CreateImageViews() {
	vk.swapchain.imageViews = malloc(vk.swapchain.imageCount * sizeof(VkImageView));

	for (size_t i = 0; i < vk.swapchain.imageCount; i++) {
		VkImageViewCreateInfo createInfo = { 0 };
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = vk.swapchain.images[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = vk.swapchain.imageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		VK_CHECK(vkCreateImageView(vk.device, &createInfo, NULL, &vk.swapchain.imageViews[i]), "failed to create ImageView for Swapchain!");
	}
}

void VK_CreateDepthStencil() {

	// Buffer
	VkImageCreateInfo imageInfo = {0};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = vk.swapchain.extent.width;
	imageInfo.extent.height = vk.swapchain.extent.height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = vk.swapchain.depthStencilFormat;
	imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;
	imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VK_CHECK(vkCreateImage(vk.device, &imageInfo, NULL, &vk.swapchain.depthImage), "failed to create DepthStencil Image for Swapchain!");
	
	VkMemoryRequirements memRequirements = {0};
	vkGetImageMemoryRequirements(vk.device, vk.swapchain.depthImage, &memRequirements);

	int32_t memoryTypeIndex = VK_FindMemoryTypeIndex(vk.physical_device, memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	VkMemoryAllocateInfo allocInfo = {0};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = memoryTypeIndex != -1 ? memoryTypeIndex : VK_DeviceLocalMemoryIndex(vk.physical_device);

	VK_CHECK(vkAllocateMemory(vk.device, &allocInfo, NULL, &vk.swapchain.depthImageMemory), "failed to allocate DepthStencil image memory for Swapchain!");
	

	VK_CHECK(vkBindImageMemory(vk.device, vk.swapchain.depthImage, vk.swapchain.depthImageMemory, 0), "failed to bind DepthStencil image memory for Swapchain!");

	// Image View
	VkImageViewCreateInfo viewInfo = {0};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = vk.swapchain.depthImage;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = vk.swapchain.depthStencilFormat;
	viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VK_CHECK(vkCreateImageView(vk.device, &viewInfo, NULL, &vk.swapchain.depthImageView), "failed to create DepthStencil image view for Swapchain!");
	
}


void VK_CreateRenderPass() {
	VkAttachmentDescription colorAttachment = {0};
	colorAttachment.format = vk.swapchain.imageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {0};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {0};
	depthAttachment.format = vk.swapchain.depthStencilFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {0};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {0};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	if (vk.swapchain.depthImageView) subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {0};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkAttachmentDescription attachments[2];
	attachments[0] = colorAttachment;
	attachments[1] = depthAttachment;

	VkRenderPassCreateInfo renderPassInfo = {0};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = (uint32_t) (vk.swapchain.depthImageView ? 2 : 1);
	renderPassInfo.pAttachments = &attachments[0];
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	VK_CHECK(vkCreateRenderPass(vk.device, &renderPassInfo, NULL, &vk.swapchain.renderpass), "failed to create RenderPass for Swapchain!");
}

void VK_CreateFramebuffers() {
	vk.swapchain.framebuffers = malloc(vk.swapchain.imageCount * sizeof(VkFramebuffer));

	for (size_t i = 0; i < vk.swapchain.imageCount; i++) {
		VkImageView attachments[2];
		attachments[0] = vk.swapchain.imageViews[i];
		attachments[1] = vk.swapchain.depthImageView;

		VkFramebufferCreateInfo framebufferInfo = {0};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = vk.swapchain.renderpass;
		framebufferInfo.attachmentCount = (uint32_t) (vk.swapchain.depthImageView ? 2 : 1);
		framebufferInfo.pAttachments = &attachments[0];
		framebufferInfo.width = vk.swapchain.extent.width;
		framebufferInfo.height = vk.swapchain.extent.height;
		framebufferInfo.layers = 1;

		VK_CHECK(vkCreateFramebuffer(vk.device, &framebufferInfo, NULL, &vk.swapchain.framebuffers[i]), "failed to create Framebuffer for Swapchain!");
	}
}

/*
==============================================================================

SwapChain Helper Function

==============================================================================
*/

static VkSurfaceFormatKHR chooseSwapSurfaceFormat(VkSurfaceFormatKHR *availableFormats, uint32_t availableFormatsCount) {

	if (availableFormatsCount == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
		return (VkSurfaceFormatKHR) {	.format = VK_FORMAT_B8G8R8A8_UNORM,
										.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
									};
	}

	for (int i = 0; i < availableFormatsCount; i++) {
		if (availableFormats[i].format == VK_FORMAT_B8G8R8A8_UNORM && availableFormats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormats[i];
		}
	}

	return availableFormats[0];
}

static VkPresentModeKHR chooseSwapPresentMode(VkPresentModeKHR *availablePresentModes, uint32_t availablePresentModesCount) {
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (int i = 0; i < availablePresentModesCount; i++) {
		if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentModes[i];
		}
		else if (availablePresentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) {
			bestMode = availablePresentModes[i];
		}
	}

	/*
	std::cout << "Present Mode..." << std::endl;
	switch (bestMode) {
	case VK_PRESENT_MODE_IMMEDIATE_KHR:
		std::cout << "\t" << "VK_PRESENT_MODE_IMMEDIATE_KHR" << std::endl;
		break;
	case VK_PRESENT_MODE_MAILBOX_KHR:
		std::cout << "\t" << "VK_PRESENT_MODE_MAILBOX_KHR" << std::endl;
		break;
	case VK_PRESENT_MODE_FIFO_KHR:
		std::cout << "\t" << "VK_PRESENT_MODE_FIFO_KHR" << std::endl;
		break;
	default:
		std::cout << "\t" << "Other" << std::endl;
		break;
	}
	*/
	return bestMode;
}
