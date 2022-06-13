// vulkan_guide.h : Include file for standard system include files,
// or project specific include files.

#pragma once

#include <string>
#include <vk_types.h>
#include <vector>

#include "vk_Mesh.h"
#include "glm/glm.hpp"

struct MeshPushConstants
{
	glm::vec4 data;
	glm::mat4 renderMatrix;
};
class VulkanEngine
{
public:

	bool _isInitialized{ false };
	int _frameNumber {0};

	VkExtent2D m_WindowExtent{ 1700 , 900 };

	struct SDL_Window* _window{ nullptr };

	//initializes everything in the engine
	void Init();

	//shuts down the engine
	void Cleanup();

	//draw loop
	void draw();

	//run main loop
	void run();
	bool LoadShaderModule(const std::string& filename, VkShaderModule* shaderModule);

private:
	void InitVulkan();
	void InitSwapchain();
	void InitCommands();
	void InitPipelines();
	void InitDefaultRenderpass();
	void InitFramebuffer();
	void InitSyncStructures();
	void LoadMeshes();
	void UploadMesh(Mesh& mesh);
	bool LoadFromObj(const char* filename);
	float m_FrameNumber;

	VkSemaphore m_PresentSemaphore;
	VkSemaphore m_RenderSemaphore;
	VkFence m_RenderFence;

	VkRenderPass m_RenderPass;
	std::vector<VkFramebuffer> m_Framebuffers;

	VkQueue m_GraphicsQueue;
	uint32_t m_GraphicsQueueFamily;

	VkCommandPool m_CommandPool;
	VkCommandBuffer m_CommandBuffer;

	VkSwapchainKHR m_Swapchain;
	VkFormat m_SwapchainImageFormat;
	std::vector<VkImage> m_SwapchainImages;
	std::vector<VkImageView> m_SwapchainImageViews;

	VkInstance m_Instance;
	VkDebugUtilsMessengerEXT m_DebugMessenger;

	VkDevice m_Device;
	VkPhysicalDevice m_PhysicalDevice;

	VkSurfaceKHR m_Surface;

	VmaAllocator m_Allocator;

	VkPipelineLayout m_TrianglePipelineLayout;
	VkPipeline m_TrianglePipeline;

	VkPipelineLayout m_MeshPipelineLayout;
	VkPipeline m_MeshPipeline;
	Mesh m_TriMesh;
	Mesh m_Monke;
};

class PipelineBuilder
{
public:
	std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages;
	VkPipelineVertexInputStateCreateInfo m_VertexInputState;
	VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState;
	VkViewport m_Viewport;
	VkRect2D m_Scissor;
	VkPipelineRasterizationStateCreateInfo m_Rasterizer;
	VkPipelineColorBlendAttachmentState m_ColorBlendAttachmentState;
	VkPipelineMultisampleStateCreateInfo m_Multisampling;
	VkPipelineLayout m_PipelineLayout;

	VkPipeline BuildPipeline(VkDevice device, VkRenderPass renderPass);
};
