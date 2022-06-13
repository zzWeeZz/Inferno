
#include "vk_engine.h"

#include <fstream>
#include <iostream>
#include <SDL.h>
#include <SDL_vulkan.h>

#include <vk_types.h>
#include <vk_initializers.h>
#include <glm/gtx/transform.hpp>
#include "VkBootstrap.h"
#include "tiny_obj_loader.h"
#include <iostream>
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"



#define VKCHECK(X) do { VkResult error = X; if(error) { std::cout <<"Detected Vulkan error: " << error << std::endl; exit(1); } } while(0)

void VulkanEngine::Init()
{
	// We initialize SDL and create a window with it. 
	SDL_Init(SDL_INIT_VIDEO);

	constexpr SDL_WindowFlags window_flags = SDL_WINDOW_VULKAN;

	_window = SDL_CreateWindow(
		"Vulkan Engine",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		m_WindowExtent.width,
		m_WindowExtent.height,
		window_flags
	);

	//everything went fine
	InitVulkan();
	InitSwapchain();
	InitCommands();
	InitDefaultRenderpass();
	InitFramebuffer();
	InitSyncStructures();
	InitPipelines();
	LoadMeshes();
	_isInitialized = true;
}
void VulkanEngine::Cleanup()
{
	if (_isInitialized)
	{
		vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
		vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);

		for (size_t i = 0; i < m_SwapchainImageViews.size(); ++i)
		{
			vkDestroyFramebuffer(m_Device, m_Framebuffers[i], nullptr);
			vkDestroyImageView(m_Device, m_SwapchainImageViews[i], nullptr);
		}

		vkDestroyDevice(m_Device, nullptr);
		vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
		vkb::destroy_debug_utils_messenger(m_Instance, m_DebugMessenger);
		vkDestroyInstance(m_Instance, nullptr);
		SDL_DestroyWindow(_window);
	}
}

void VulkanEngine::draw()
{
	VKCHECK(vkWaitForFences(m_Device, 1, &GetCurrentFrame().renderFence, true, 1000000000));
	VKCHECK(vkResetFences(m_Device, 1, &GetCurrentFrame().renderFence));

	uint32_t swapchainImageIndex;
	VKCHECK(vkAcquireNextImageKHR(m_Device, m_Swapchain, 1000000000, GetCurrentFrame().presentSmeraphore, nullptr, &swapchainImageIndex));
	VKCHECK(vkResetCommandBuffer(GetCurrentFrame().commandBuffer, 0));

	VkCommandBuffer cmd = GetCurrentFrame().commandBuffer;
	VkCommandBufferBeginInfo cmdBeginInfo{};
	cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBeginInfo.pNext = nullptr;

	cmdBeginInfo.pInheritanceInfo = nullptr;
	cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VKCHECK(vkBeginCommandBuffer(cmd, &cmdBeginInfo));

	VkClearValue clearValue;
	clearValue.color = { {0.0f, 1, 1, 1.0f} };
	VkClearValue depthClear;
	depthClear.depthStencil.depth = 1.0;


	VkRenderPassBeginInfo rpInfo{};
	rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rpInfo.pNext = nullptr;

	rpInfo.renderPass = m_RenderPass;
	rpInfo.renderArea.offset.x = 0;
	rpInfo.renderArea.offset.y = 0;
	rpInfo.renderArea.extent = m_WindowExtent;
	rpInfo.framebuffer = m_Framebuffers[swapchainImageIndex];

	rpInfo.clearValueCount = 2;
	VkClearValue clears[2] = { clearValue, depthClear };
	rpInfo.pClearValues = &clears[0];


	vkCmdBeginRenderPass(cmd, &rpInfo, VK_SUBPASS_CONTENTS_INLINE);


	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_MeshPipeline);
	glm::vec3 camPos = { 0.f, 0.f, -2.f };
	glm::mat4 view = glm::translate(glm::mat4(1.0f), camPos);
	glm::mat4 projection = glm::perspective(glm::radians(70.f), 1700.f / 900.f, 0.1f, 200.f);
	projection[1][1] *= -1;

	glm::mat4 model = glm::rotate(glm::mat4(1.f), glm::radians(m_FrameNumber * 0.4f), glm::vec3(0, 1, 0));

	glm::mat4 meshMatrix = projection * view * model;

	MeshPushConstants constants;
	constants.renderMatrix = meshMatrix;

	vkCmdPushConstants(cmd, m_MeshPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);
	VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(cmd, 0, 1, &m_Monke.vertexBuffer.buffer, &offset);
	vkCmdDraw(cmd, m_Monke.vertices.size(), 1, 0, 0);

	vkCmdEndRenderPass(cmd);
	VKCHECK(vkEndCommandBuffer(cmd));

	VkSubmitInfo submit{};
	submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit.pNext = nullptr;

	VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	submit.pWaitDstStageMask = &waitStage;

	submit.waitSemaphoreCount = 1;
	submit.pWaitSemaphores = &GetCurrentFrame().presentSmeraphore;

	submit.signalSemaphoreCount = 1;
	submit.pSignalSemaphores = &GetCurrentFrame().renderSemaphore;

	submit.commandBufferCount = 1;
	submit.pCommandBuffers = &cmd;

	VKCHECK(vkQueueSubmit(m_GraphicsQueue, 1, &submit, GetCurrentFrame().renderFence));


	VkPresentInfoKHR presentInfo{};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.pNext = nullptr;

	presentInfo.pSwapchains = &m_Swapchain;
	presentInfo.swapchainCount = 1;

	presentInfo.pWaitSemaphores = &GetCurrentFrame().renderSemaphore;
	presentInfo.waitSemaphoreCount = 1;

	presentInfo.pImageIndices = &swapchainImageIndex;

	VKCHECK(vkQueuePresentKHR(m_GraphicsQueue, &presentInfo));

	_frameNumber++;
}

void VulkanEngine::run()
{
	SDL_Event e;
	bool bQuit = false;

	//main loop
	while (!bQuit)
	{
		//Handle events on queue
		while (SDL_PollEvent(&e) != 0)
		{
			//close the window when user alt-f4s or clicks the X button			
			if (e.type == SDL_QUIT) bQuit = true;
		}

		draw();
		m_FrameNumber++;
	}
}

bool VulkanEngine::LoadShaderModule(const std::string& filename, VkShaderModule* shaderModule)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
	{
		std::cout << "Failed to open file: " << filename << std::endl;
		return false;
	}

	size_t fileSize = (size_t)file.tellg();

	std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

	file.seekg(0);
	file.read(reinterpret_cast<char*>(buffer.data()), fileSize);

	file.close();

	VkShaderModuleCreateInfo shaderModuleCreateInfo{};
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = nullptr;

	shaderModuleCreateInfo.codeSize = buffer.size() * sizeof(uint32_t);
	shaderModuleCreateInfo.pCode = buffer.data();

	VkShaderModule shaderHandle;
	if (vkCreateShaderModule(m_Device, &shaderModuleCreateInfo, nullptr, &shaderHandle) != VK_SUCCESS)
	{
		std::cout << "Failed to create shader module" << std::endl;
		return false;
	}
	*shaderModule = shaderHandle;
	return true;
}

void VulkanEngine::InitVulkan()
{
	vkb::InstanceBuilder builder;

	auto instanceRect = builder.set_app_name("Göteborg")
		.request_validation_layers(true)
		.require_api_version(1, 1)
		.use_default_debug_messenger()
		.build();

	vkb::Instance vkbInstance = instanceRect.value();
	m_FrameNumber = 0;
	m_Instance = vkbInstance.instance;

	m_DebugMessenger = vkbInstance.debug_messenger;

	SDL_Vulkan_CreateSurface(_window, m_Instance, &m_Surface);

	vkb::PhysicalDeviceSelector selector{ vkbInstance };
	vkb::PhysicalDevice physicalDevice = selector.set_minimum_version(1, 1).set_surface(m_Surface).select().value();

	vkb::DeviceBuilder deviceBuilder{ physicalDevice };

	vkb::Device vkbDevice = deviceBuilder.build().value();

	m_Device = vkbDevice.device;
	m_PhysicalDevice = physicalDevice.physical_device;

	m_GraphicsQueue = vkbDevice.get_queue(vkb::QueueType::graphics).value();
	m_GraphicsQueueFamily = vkbDevice.get_queue_index(vkb::QueueType::graphics).value();

	VmaAllocatorCreateInfo allocatorInfo{};
	allocatorInfo.physicalDevice = m_PhysicalDevice;
	allocatorInfo.device = m_Device;
	allocatorInfo.instance = m_Instance;
	vmaCreateAllocator(&allocatorInfo, &m_Allocator);
}

void VulkanEngine::InitSwapchain()
{
	vkb::SwapchainBuilder swapchainBuilder{ m_PhysicalDevice, m_Device, m_Surface };

	vkb::Swapchain vkbSwapchain = swapchainBuilder
		.use_default_format_selection()
		.set_desired_present_mode(VK_PRESENT_MODE_MAILBOX_KHR)
		.set_desired_extent(m_WindowExtent.width, m_WindowExtent.height)
		.build()
		.value();

	m_Swapchain = vkbSwapchain.swapchain;
	m_SwapchainImages = vkbSwapchain.get_images().value();
	m_SwapchainImageViews = vkbSwapchain.get_image_views().value();
	m_SwapchainImageFormat = vkbSwapchain.image_format;

	VkExtent3D depthImageExtent = { m_WindowExtent.width, m_WindowExtent.height, 1 };

	m_DepthFormat = VK_FORMAT_D32_SFLOAT;

	VkImageCreateInfo depthImInfo = vkinit::ImageCreateInfo(m_DepthFormat, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, depthImageExtent);
	VmaAllocationCreateInfo depthAllocInfo{};
	depthAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	depthAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	vmaCreateImage(m_Allocator, &depthImInfo, &depthAllocInfo, &m_DepthImage.image, &m_DepthImage.allocation, nullptr);

	VkImageViewCreateInfo depthImViewInfo = vkinit::ImageViewCreateInfo(m_DepthFormat, m_DepthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);
	VKCHECK(vkCreateImageView(m_Device, &depthImViewInfo, nullptr, &m_DepthImageView));
}

void VulkanEngine::InitCommands()
{
	VkCommandPoolCreateInfo poolInfo = vkinit::CommandPoolCreateInfo(m_GraphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
	for (size_t i = 0; i < 2; i++)
	{
		VKCHECK(vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_Frames[i].commandPool));
		VkCommandBufferAllocateInfo cmdAllocInfo = vkinit::CommandBufferAllocateInfo(m_Frames[i].commandPool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		VKCHECK(vkAllocateCommandBuffers(m_Device, &cmdAllocInfo, &m_Frames[i].commandBuffer));
	}

}

void VulkanEngine::InitPipelines()
{
	VkShaderModule triangleFragShader;
	if (!LoadShaderModule("../../shaders/triangle.frag.spv", &triangleFragShader))
	{
		std::cout << "Failed to load Fragment shader\n";
	}
	VkShaderModule triangleVertShader;
	if (!LoadShaderModule("../../shaders/triangle.vert.spv", &triangleVertShader))
	{
		std::cout << "Failed to load Vertex shader\n";
	}

	VkPipelineLayoutCreateInfo pipelineLayout = vkinit::PipelineLayoutCreateInfo();
	VKCHECK(vkCreatePipelineLayout(m_Device, &pipelineLayout, nullptr, &m_TrianglePipelineLayout));

	PipelineBuilder pipelineBuilder;

	pipelineBuilder.m_ShaderStages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, triangleVertShader));
	pipelineBuilder.m_ShaderStages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

	pipelineBuilder.m_VertexInputState = vkinit::PipelineVertexInputStateCreateInfo();

	pipelineBuilder.m_InputAssemblyState = vkinit::PipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

	pipelineBuilder.m_Viewport.x = 0.0f;
	pipelineBuilder.m_Viewport.y = 0.0f;
	pipelineBuilder.m_Viewport.width = static_cast<float>(m_WindowExtent.width);
	pipelineBuilder.m_Viewport.height = static_cast<float>(m_WindowExtent.height);
	pipelineBuilder.m_Viewport.minDepth = 0.0f;
	pipelineBuilder.m_Viewport.maxDepth = 1.0f;

	pipelineBuilder.m_Scissor.offset = { 0,0 };
	pipelineBuilder.m_Scissor.extent = m_WindowExtent;
	pipelineBuilder.m_DepthStencilState = vkinit::PipelineDepthStencilCreateInfo(true, true, VK_COMPARE_OP_LESS_OR_EQUAL);

	pipelineBuilder.m_Rasterizer = vkinit::PipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL);
	pipelineBuilder.m_Multisampling = vkinit::PipelineMultisampleStateCreateInfo();
	pipelineBuilder.m_ColorBlendAttachmentState = vkinit::PipelineColorBlendAttachmentCreateInfo();
	pipelineBuilder.m_PipelineLayout = m_TrianglePipelineLayout;
	m_TrianglePipeline = pipelineBuilder.BuildPipeline(m_Device, m_RenderPass);

	VertexInputDescription vertexDescription = Vertex::GetVertexDescription();
	pipelineBuilder.m_VertexInputState.pVertexAttributeDescriptions = vertexDescription.attributes.data();
	pipelineBuilder.m_VertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexDescription.attributes.size());

	pipelineBuilder.m_VertexInputState.pVertexBindingDescriptions = vertexDescription.bindings.data();
	pipelineBuilder.m_VertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexDescription.bindings.size());

	pipelineBuilder.m_ShaderStages.clear();

	VkShaderModule meshVertShader;
	if (!LoadShaderModule("../../shaders/tri_mesh.vert.spv", &meshVertShader))
	{
		std::cout << "Failed to load Vert shader\n";
	}

	pipelineBuilder.m_ShaderStages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, meshVertShader));
	pipelineBuilder.m_ShaderStages.push_back(vkinit::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, triangleFragShader));

	VkPipelineLayoutCreateInfo meshPipelineLayout = vkinit::PipelineLayoutCreateInfo();

	VkPushConstantRange pushConstant;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(MeshPushConstants);
	pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	meshPipelineLayout.pPushConstantRanges = &pushConstant;
	meshPipelineLayout.pushConstantRangeCount = 1;

	VKCHECK(vkCreatePipelineLayout(m_Device, &meshPipelineLayout, nullptr, &m_MeshPipelineLayout));

	pipelineBuilder.m_PipelineLayout = m_MeshPipelineLayout;


	m_MeshPipeline = pipelineBuilder.BuildPipeline(m_Device, m_RenderPass);


	vkDestroyShaderModule(m_Device, triangleFragShader, nullptr);
	vkDestroyShaderModule(m_Device, triangleVertShader, nullptr);
	vkDestroyShaderModule(m_Device, meshVertShader, nullptr);
}

void VulkanEngine::InitDefaultRenderpass()
{
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = m_SwapchainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef{};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;

	VkAttachmentDescription depthAttachment{};
	depthAttachment.flags = 0;
	depthAttachment.format = m_DepthFormat;
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription depthSubpass{};
	depthSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	depthSubpass.colorAttachmentCount = 1;
	depthSubpass.pColorAttachments = &colorAttachmentRef;
	depthSubpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkAttachmentDescription attachments[2] = { colorAttachment, depthAttachment };

	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency depthDependency{};
	depthDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	depthDependency.dstSubpass = 0;
	depthDependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.srcAccessMask = 0;
	depthDependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	depthDependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkSubpassDependency dependencies[2] = { dependency, depthDependency };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = 2;
	renderPassInfo.pAttachments = &attachments[0];
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &depthSubpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = &dependencies[0];

	VKCHECK(vkCreateRenderPass(m_Device, &renderPassInfo, nullptr, &m_RenderPass));
}

void VulkanEngine::InitFramebuffer()
{
	VkFramebufferCreateInfo framebufferInfo{};
	framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferInfo.pNext = nullptr;

	framebufferInfo.renderPass = m_RenderPass;
	framebufferInfo.attachmentCount = 1;
	framebufferInfo.width = m_WindowExtent.width;
	framebufferInfo.height = m_WindowExtent.height;
	framebufferInfo.layers = 1;

	const uint32_t swapchainImageCount = m_SwapchainImages.size();
	m_Framebuffers = std::vector<VkFramebuffer>(swapchainImageCount);

	for (size_t i = 0; i < swapchainImageCount; ++i)
	{
		VkImageView attachments[2];
		attachments[0] = m_SwapchainImageViews[i];
		attachments[1] = m_DepthImageView;

		framebufferInfo.attachmentCount = 2;
		framebufferInfo.pAttachments = &attachments[0];


		VKCHECK(vkCreateFramebuffer(m_Device, &framebufferInfo, nullptr, &m_Framebuffers[i]));
	}
}

void VulkanEngine::InitSyncStructures()
{
	VkFenceCreateInfo fenceCreateInfo{};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.pNext = nullptr;

	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	VkSemaphoreCreateInfo semaphoreCreateInfo{};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = nullptr;

	semaphoreCreateInfo.flags = 0;
	for (size_t i = 0; i < 2; ++i)
	{
		VKCHECK(vkCreateFence(m_Device, &fenceCreateInfo, nullptr, &m_Frames[i].renderFence));

		VKCHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].presentSmeraphore));
		VKCHECK(vkCreateSemaphore(m_Device, &semaphoreCreateInfo, nullptr, &m_Frames[i].renderSemaphore));

	}
}

void VulkanEngine::LoadMeshes()
{
	m_TriMesh.vertices.resize(3);

	m_TriMesh.vertices[0].position = { -0.5f, -0.5f, 0.0f };
	m_TriMesh.vertices[1].position = { 0.5f, -0.5f, 0.0f };
	m_TriMesh.vertices[2].position = { 0.0f, 0.5f, 0.0f };

	m_TriMesh.vertices[0].color = { 1.0f, 0.0f, 0.0f };
	m_TriMesh.vertices[1].color = { 0.0f, 1.0f, 0.0f };
	m_TriMesh.vertices[2].color = { 0.0f, 0.0f, 1.0f };

	m_Monke.LoadFromObj("../../assets/lost_empire.obj");

	UploadMesh(m_TriMesh);
	UploadMesh(m_Monke);

}

void VulkanEngine::UploadMesh(Mesh& mesh)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = sizeof(Vertex) * mesh.vertices.size();
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

	VmaAllocationCreateInfo allocInfo{};
	allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	VKCHECK(vmaCreateBuffer(m_Allocator, &bufferInfo, &allocInfo, &mesh.vertexBuffer.buffer, &mesh.vertexBuffer.allocation, nullptr));

	void* data;
	vmaMapMemory(m_Allocator, mesh.vertexBuffer.allocation, &data);
	memcpy(data, mesh.vertices.data(), sizeof(Vertex) * mesh.vertices.size());
	vmaUnmapMemory(m_Allocator, mesh.vertexBuffer.allocation);
}

bool VulkanEngine::LoadFromObj(const char* filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, nullptr);
	if (!warn.empty()) std::cout << "WARN: " << warn << std::endl;
	if (!err.empty())
	{
		std::cerr << err << std::endl;
		return false;
	}

	for (size_t s = 0; s < shapes.size(); s++)
	{
		size_t indexOffset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
		{
			int fv = 3;
			for (size_t v = 0; v < fv; v++)
			{
				tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];
				Vertex vertex;
				vertex.position = { attrib.vertices[3 * idx.vertex_index + 0], attrib.vertices[3 * idx.vertex_index + 1], attrib.vertices[3 * idx.vertex_index + 2] };
				vertex.normal = { attrib.normals[3 * idx.normal_index + 0], attrib.normals[3 * idx.normal_index + 1], attrib.normals[3 * idx.normal_index + 2] };
				vertex.color = { attrib.normals[3 * idx.normal_index + 0], attrib.normals[3 * idx.normal_index + 1], attrib.normals[3 * idx.normal_index + 2] };
				m_TriMesh.vertices.push_back(vertex);
			}
			indexOffset += fv;
		}
	}

	return true;
}

FrameData& VulkanEngine::GetCurrentFrame()
{
	return m_Frames[m_FrameNumber % 2];
}

VkPipeline PipelineBuilder::BuildPipeline(VkDevice device, VkRenderPass renderPass)
{
	VkPipelineViewportStateCreateInfo viewportState{};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.pNext = nullptr;

	viewportState.viewportCount = 1;
	viewportState.pViewports = &m_Viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &m_Scissor;

	VkPipelineColorBlendStateCreateInfo colorBlendState{};
	colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendState.pNext = nullptr;
	colorBlendState.logicOpEnable = VK_FALSE;
	colorBlendState.logicOp = VK_LOGIC_OP_COPY;
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = &m_ColorBlendAttachmentState;

	VkGraphicsPipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = nullptr;

	pipelineInfo.stageCount = m_ShaderStages.size();
	pipelineInfo.pStages = m_ShaderStages.data();
	pipelineInfo.pVertexInputState = &m_VertexInputState;
	pipelineInfo.pInputAssemblyState = &m_InputAssemblyState;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &m_Rasterizer;
	pipelineInfo.pMultisampleState = &m_Multisampling;
	pipelineInfo.pColorBlendState = &colorBlendState;
	pipelineInfo.layout = m_PipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

	pipelineInfo.pDepthStencilState = &m_DepthStencilState;

	VkPipeline newPipeline;
	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
	{
		std::cout << "failed to create pipeline\n";
		return VK_NULL_HANDLE;
	}
	else
	{
		return newPipeline;
	}
}
