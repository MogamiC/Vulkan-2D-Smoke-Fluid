#pragma once

#include "DefNConst.h"


class VulkanApp {
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
public:
	VulkanApp() {
		InitlializeApp();
		mApp = this;
	}
	~VulkanApp() {
		DestroyApp();
	}


	virtual void Update() = 0;
	virtual void Run() = 0;


// protected:

	VulkanApp* mApp = nullptr;

	VulkanApp* GetApp() {
		return mApp;
	}

	void InitlializeApp();
	void DestroyApp();


	//Device
	void CreateGlfwWindow(uint32_t fWidth, uint32_t fHeight, std::string fName);
	void CreateInstance(std::vector<const char*> fValidationLayers);
	void CreateSurface();
	void PickPhysicalDevice(std::vector<const char*> fDeviceExtensions);
	void CreateLogicalDevice(std::vector<const char*> fValidationLayers, std::vector<const char*> fDeviceExtensions);
	void CreateCommandPool();
	//Device Helped Fiction
	bool CheckValidationLayerSupport(std::vector<const char*> fValidationLayers);
	bool IsDeviceSuitable(VkPhysicalDevice fPhysicalDevice, std::vector<const char*> fDeviceExtensions);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice fPhysicalDevice, std::vector<const char*> fDeviceExtensions);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice fPhysicalDevice);
	SwapChainSupportDetailsS QuerySwapChainSupport(VkPhysicalDevice fPhysicalDevice);


	// SwapChain 
	void CreateSwapChain();
	void CreateSwapChainImageViews();
	void CreateFramebuffers();
	void CreateDepthResources();
	void CreateRenderPass();
	// SwapChain Helped Fiction
	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& fAvailableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& fAvailablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& fCapabilities);


	// Pipeline & DescriptorSet
	void CreateDescriptorSetLayout(VkDescriptorSetLayout& fDescriptorSetLayout,
		std::vector<VkDescriptorType> fDescriptorType, VkShaderStageFlagBits fShaderStage);
	void CreatePipelineLayout(VkPipelineLayout& fPipelineLayout, VkDescriptorSetLayout fDescriptorSetLayout);
	void CreateComputePipeline(VkPipeline& fPipeline, VkPipelineLayout fPipelineLayout, const std::string& fFilename);
	// Pipeline & DescriptorSet Helped Fiction
	std::vector<char> ReadFile(const std::string& fFilename);
	VkShaderModule CreateShaderModule(const std::vector<char>& fCode);
	void CreateBuffer(
		VkBuffer& fBuffer, VkDeviceMemory& fMemory, VkDeviceSize fSize,
		VkBufferUsageFlags fUsage, VkMemoryPropertyFlags fProperty);
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);


	GLFWwindow* mWindowPtr;
	bool mFramebufferResized = false;
	VkInstance mInstance;
	VkSurfaceKHR mSurface;
	VkPhysicalDevice mPhysicalDevice;
	VkPhysicalDeviceProperties mProperties;
	VkPhysicalDeviceFeatures mDeviceFeatures;
	VkDevice mDevice;
	VkQueue mComputeQueue;
	VkQueue mGraphicsQueue;
	VkQueue mPresentQueue;
	VkCommandPool mCommandPool;

	VkSwapchainKHR mSwapChain;
	VkFormat mSwapChainImageFormat;
	VkExtent2D mSwapChainExtent;
	std::vector<VkImage> mSwapChainImages;
	std::vector<VkImageView> mSwapChainImageViews;
	std::vector<VkFramebuffer> mSwapChainFramebuffers;
	VkImage mDepthImage;
	VkDeviceMemory mDepthImageMemory;
	VkImageView mDepthImageView;
	VkRenderPass mRenderPass;
	bool IsRenderPassBegin = false;
};