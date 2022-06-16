// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <vk_types.h>

namespace vkinit {

	VkSubmitInfo SubmitInfo(VkCommandBuffer* commandBuffer);
	VkImageCreateInfo ImageCreateInfo(VkFormat format, VkImageUsageFlags usageFlags, VkExtent3D extent);
	VkImageViewCreateInfo ImageViewCreateInfo(VkFormat format, VkImage image, VkImageAspectFlags aspectFlags);

	VkSamplerCreateInfo SamplerCreateInfo(VkFilter magFilter, VkSamplerAddressMode addressMode);
	VkWriteDescriptorSet WriteDescriptorSet(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* bufferInfo);

	VkCommandBufferBeginInfo CommandBufferBeginInfo(VkCommandBufferUsageFlags usageFlags = 0);
	VkCommandPoolCreateInfo CommandPoolCreateInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlags flags = 0);
	VkCommandBufferAllocateInfo CommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	VkPipelineShaderStageCreateInfo PipelineShaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule shaderModule);
	VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfo();
	VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateCreateInfo(VkPrimitiveTopology topology);
	VkPipelineRasterizationStateCreateInfo PipelineRasterizationStateCreateInfo(VkPolygonMode polygonMode);
	VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateCreateInfo();
	VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentCreateInfo();
	VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilCreateInfo(bool depthTest, bool depthWrite, VkCompareOp compareOp);
	VkPipelineLayoutCreateInfo PipelineLayoutCreateInfo();
}

