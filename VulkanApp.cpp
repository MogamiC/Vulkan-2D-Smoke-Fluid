#include "VulkanApp.h"

void VulkanApp::InitlializeApp() {
	CreateGlfwWindow(800, 600, "vulkan");
	CreateInstance({ "VK_LAYER_KHRONOS_validation" });
	CreateSurface();
	PickPhysicalDevice({  });
	CreateLogicalDevice({ "VK_LAYER_KHRONOS_validation" }, { VK_KHR_SWAPCHAIN_EXTENSION_NAME });
	CreateCommandPool();

	CreateSwapChain();
	CreateSwapChainImageViews();
	CreateDepthResources();
	CreateRenderPass();
	CreateFramebuffers();
}
void VulkanApp::DestroyApp() {
	vkDestroyImageView(mDevice, mDepthImageView, nullptr);
	vkDestroyImage(mDevice, mDepthImage, nullptr);
	vkFreeMemory(mDevice, mDepthImageMemory, nullptr);
	for (auto framebuffer : mSwapChainFramebuffers) {
		vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
	}
	for (auto imageView : mSwapChainImageViews) {
		vkDestroyImageView(mDevice, imageView, nullptr);
	}
	vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
	vkDestroyRenderPass(mDevice, mRenderPass, nullptr);


	vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
	vkDestroyDevice(mDevice, nullptr);
	vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
	vkDestroyInstance(mInstance, nullptr);

	glfwDestroyWindow(mWindowPtr);
	glfwTerminate();
}


//Device
void VulkanApp::CreateGlfwWindow(uint32_t fWidth, uint32_t fHeight, std::string fName) {
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	mWindowPtr = glfwCreateWindow(fWidth, fHeight, fName.c_str(), nullptr, nullptr);

	// mLastTime = glfwGetTime();
}
void VulkanApp::CreateInstance(std::vector<const char*> fValidationLayers) {
	/* pApplicationInfo start */
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = nullptr;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;
	/* end */

	/* layer start */
	if (enableValidationLayers && !CheckValidationLayerSupport(fValidationLayers)) {
		throw std::runtime_error("validation layers requested, but not available!");
	}
	/* end */

	/* Extension start */
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	/* end */


	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledLayerCount = fValidationLayers.size();
	createInfo.ppEnabledLayerNames = fValidationLayers.data();
	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;

	if (vkCreateInstance(&createInfo, nullptr, &mInstance) != VK_SUCCESS) {
		throw std::runtime_error("failed to create instance!");
	}


}
void VulkanApp::CreateSurface() {
	if (glfwCreateWindowSurface(mInstance, mWindowPtr, nullptr, &mSurface) != VK_SUCCESS) {
		throw std::runtime_error("failed to create window surface!");
	}
}
void VulkanApp::PickPhysicalDevice(std::vector<const char*> fDeviceExtensions) {
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
	if (deviceCount == 0) {
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(mInstance, &deviceCount, devices.data());

	for (const auto& device : devices) {
		if (IsDeviceSuitable(device, fDeviceExtensions)) {
			mPhysicalDevice = device;
			break;
		}
	}

	if (mPhysicalDevice == VK_NULL_HANDLE) {
		throw std::runtime_error("failed to find a suitable GPU!");
	}

	vkGetPhysicalDeviceProperties(mPhysicalDevice, &mProperties);
	vkGetPhysicalDeviceFeatures(mPhysicalDevice, &mDeviceFeatures);

	std::cout << mProperties.deviceName << std::endl;
}
void VulkanApp::CreateLogicalDevice(std::vector<const char*> fValidationLayers, std::vector<const char*> fDeviceExtensions) {
	QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsAndComputeFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.pNext = nullptr;
		queueCreateInfo.flags = 0;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(fValidationLayers.size());
		createInfo.ppEnabledLayerNames = fValidationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}
	createInfo.enabledExtensionCount = static_cast<uint32_t>(fDeviceExtensions.size());
	createInfo.ppEnabledExtensionNames = fDeviceExtensions.data();
	createInfo.pEnabledFeatures = &deviceFeatures;

	if (vkCreateDevice(mPhysicalDevice, &createInfo, nullptr, &mDevice) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(mDevice, indices.graphicsAndComputeFamily.value(), 0, &mGraphicsQueue);
	vkGetDeviceQueue(mDevice, indices.graphicsAndComputeFamily.value(), 0, &mComputeQueue);
	vkGetDeviceQueue(mDevice, indices.presentFamily.value(), 0, &mPresentQueue);
}
void VulkanApp::CreateCommandPool() {
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(mPhysicalDevice);

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

	if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}
//Device Helped Fiction
bool VulkanApp::CheckValidationLayerSupport(std::vector<const char*> fValidationLayers) {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : fValidationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}

		if (!layerFound) {
			return false;
		}
	}

	return true;
}
bool VulkanApp::IsDeviceSuitable(VkPhysicalDevice fPhysicalDevice, std::vector<const char*> fDeviceExtensions) {
	QueueFamilyIndices indices = FindQueueFamilies(fPhysicalDevice);

	bool extensionsSupported = CheckDeviceExtensionSupport(fPhysicalDevice, fDeviceExtensions);

	bool swapChainAdequate = false;
	if (extensionsSupported) {
		SwapChainSupportDetailsS swapChainSupport = QuerySwapChainSupport(fPhysicalDevice);
		swapChainAdequate = !swapChainSupport.mFormats.empty() && !swapChainSupport.mPresentModes.empty();
	}

	return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}
bool VulkanApp::CheckDeviceExtensionSupport(VkPhysicalDevice fPhysicalDevice, std::vector<const char*> fDeviceExtensions) {
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(fPhysicalDevice, nullptr, &extensionCount, nullptr);
	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(fPhysicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(fDeviceExtensions.begin(), fDeviceExtensions.end());

	for (const auto& extension : availableExtensions) {
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}
QueueFamilyIndices VulkanApp::FindQueueFamilies(VkPhysicalDevice fPhysicalDevice) {
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(fPhysicalDevice, &queueFamilyCount, nullptr);
	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(fPhysicalDevice, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies) {
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
			indices.graphicsAndComputeFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(fPhysicalDevice, i, mSurface, &presentSupport);

		if (presentSupport) {
			indices.presentFamily = i;
		}

		if (indices.IsComplete()) {
			break;
		}

		i++;
	}
	return indices;
}
SwapChainSupportDetailsS VulkanApp::QuerySwapChainSupport(VkPhysicalDevice fPhysicalDevice) {
	SwapChainSupportDetailsS details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(fPhysicalDevice, mSurface, &details.mCapabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(fPhysicalDevice, mSurface, &formatCount, nullptr);
	if (formatCount != 0) {
		details.mFormats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(fPhysicalDevice, mSurface, &formatCount, details.mFormats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(fPhysicalDevice, mSurface, &presentModeCount, nullptr);
	if (presentModeCount != 0) {
		details.mPresentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(fPhysicalDevice, mSurface, &presentModeCount, details.mPresentModes.data());
	}

	return details;
}



// SwapChain 
void VulkanApp::CreateSwapChain() {
	SwapChainSupportDetailsS swapChainSupport = QuerySwapChainSupport(mPhysicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.mFormats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.mPresentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.mCapabilities);

	uint32_t imageCount = swapChainSupport.mCapabilities.minImageCount + 1;
	if (swapChainSupport.mCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.mCapabilities.maxImageCount) {
		imageCount = swapChainSupport.mCapabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = mSurface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	QueueFamilyIndices indices = FindQueueFamilies(mPhysicalDevice);
	uint32_t queueFamilyIndices[] = { indices.graphicsAndComputeFamily.value(), indices.presentFamily.value() };

	if (indices.graphicsAndComputeFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.mCapabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(mDevice, &createInfo, nullptr, &mSwapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, nullptr);
	mSwapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(mDevice, mSwapChain, &imageCount, mSwapChainImages.data());

	mSwapChainImageFormat = surfaceFormat.format;
	mSwapChainExtent = extent;
}
void VulkanApp::CreateSwapChainImageViews() {
	size_t size = mSwapChainImages.size();
	mSwapChainImageViews.resize(size);
	for (size_t i = 0; i < size; i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = mSwapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = mSwapChainImageFormat;

		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(mDevice, &createInfo, nullptr, &mSwapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}
void VulkanApp::CreateFramebuffers() {
	mSwapChainFramebuffers.resize(mSwapChainImageViews.size());

	for (size_t i = 0; i < mSwapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			mSwapChainImageViews[i],
			mDepthImageView
		};

		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = mRenderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());;
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = mSwapChainExtent.width;
		framebufferInfo.height = mSwapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(mDevice, &framebufferInfo, nullptr, &mSwapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}
void VulkanApp::CreateDepthResources() {
	VkFormat depthFormat = FindDepthFormat();

	CreateImage(mSwapChainExtent.width, mSwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);
	mDepthImageView = CreateImageView(mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}
void VulkanApp::CreateRenderPass() {
	VkAttachmentDescription colorAttachment{};
	colorAttachment.format = mSwapChainImageFormat;
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


	VkAttachmentDescription depthAttachment{};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef{};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass{};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;


	VkSubpassDependency dependency{};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcAccessMask = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(mDevice, &renderPassInfo, nullptr, &mRenderPass) != VK_SUCCESS) {
		throw std::runtime_error("failed to create render pass!");
	}
}
// SwapChain Helped Fiction
VkFormat VulkanApp::FindDepthFormat() {
	return FindSupportedFormat(
		{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}
bool VulkanApp::HasStencilComponent(VkFormat format) {
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}
VkFormat VulkanApp::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}
uint32_t VulkanApp::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}
VkImageView VulkanApp::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
	VkImageViewCreateInfo viewInfo{};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(mDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}
void VulkanApp::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(mDevice, &imageInfo, nullptr, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(mDevice, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(mDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(mDevice, image, imageMemory, 0);
}
VkSurfaceFormatKHR VulkanApp::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& fAvailableFormats) {
	for (const auto& availableFormat : fAvailableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			return availableFormat;
		}
	}

	return fAvailableFormats[0];
}
VkPresentModeKHR VulkanApp::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& fAvailablePresentModes) {
	for (const auto& availablePresentMode : fAvailablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			return availablePresentMode;
		}
	}

	return VK_PRESENT_MODE_FIFO_KHR;
}
VkExtent2D VulkanApp::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& fCapabilities) {
	if (fCapabilities.currentExtent.width != (std::numeric_limits<uint32_t>::max)()) {
		return fCapabilities.currentExtent;
	}
	else {
		int width, height;
		glfwGetFramebufferSize(mWindowPtr, &width, &height);

		VkExtent2D actualExtent = {
		static_cast<uint32_t>(width),
		static_cast<uint32_t>(height)
		};

		actualExtent.width = std::clamp(actualExtent.width, fCapabilities.minImageExtent.width, fCapabilities.maxImageExtent.width);
		actualExtent.height = std::clamp(actualExtent.height, fCapabilities.minImageExtent.height, fCapabilities.maxImageExtent.height);

		return actualExtent;
	}
}


// Pipeline & DescriptorSet
void VulkanApp::CreateDescriptorSetLayout(VkDescriptorSetLayout& fDescriptorSetLayout,
	std::vector<VkDescriptorType> fDescriptorType, VkShaderStageFlagBits fShaderStage) {
	size_t size = fDescriptorType.size();
	std::vector<VkDescriptorSetLayoutBinding> SetLayoutBindings;
	SetLayoutBindings.resize(size);
	for (size_t i = 0; i < size; ++i) {
		VkDescriptorSetLayoutBinding SetLayoutBinding{};
		SetLayoutBinding.binding = i;
		SetLayoutBinding.descriptorType = fDescriptorType[i];
		SetLayoutBinding.descriptorCount = 1;
		SetLayoutBinding.stageFlags = fShaderStage;

		SetLayoutBindings[i] = SetLayoutBinding;
	}

	VkDescriptorSetLayoutCreateInfo DescriptorSetlayoutInfo{};
	DescriptorSetlayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	DescriptorSetlayoutInfo.bindingCount = SetLayoutBindings.size();
	DescriptorSetlayoutInfo.pBindings = SetLayoutBindings.data();

	if (vkCreateDescriptorSetLayout(mDevice, &DescriptorSetlayoutInfo, nullptr, &fDescriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}
void VulkanApp::CreatePipelineLayout(VkPipelineLayout& fPipelineLayout, VkDescriptorSetLayout fDescriptorSetLayout) {
	std::vector<VkDescriptorSetLayout> DescriptorSetLayouts{ fDescriptorSetLayout };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = DescriptorSetLayouts.size();
	pipelineLayoutInfo.pSetLayouts = DescriptorSetLayouts.data();

	if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &fPipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}
void VulkanApp::CreateComputePipeline(VkPipeline& fPipeline, VkPipelineLayout fPipelineLayout, const std::string& fFilename) {
	auto ShaderCode = ReadFile(fFilename);
	VkShaderModule ShaderModule = CreateShaderModule(ShaderCode);

	VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
	computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	computeShaderStageInfo.module = ShaderModule;
	computeShaderStageInfo.pName = "main";

	VkComputePipelineCreateInfo pipelineInfo{};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfo.layout = fPipelineLayout;
	pipelineInfo.stage = computeShaderStageInfo;

	if (vkCreateComputePipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &fPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create compute pipeline!");
	}

	vkDestroyShaderModule(mDevice, ShaderModule, nullptr);
}
// Pipeline & DescriptorSet Helped Fiction
std::vector<char> VulkanApp::ReadFile(const std::string& fFilename) {
	std::ifstream file(fFilename, std::ios::ate | std::ios::binary);

	if (!file.is_open()) {
		throw std::runtime_error("failed to open file!");
	}

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);

	file.close();

	return buffer;
}
VkShaderModule VulkanApp::CreateShaderModule(const std::vector<char>& fCode) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.pNext = nullptr;
	createInfo.flags = 0;
	createInfo.codeSize = fCode.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(fCode.data());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(mDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}	//Step3 Helped Fiction
void VulkanApp::CreateBuffer(
	VkBuffer& fBuffer, VkDeviceMemory& fMemory, VkDeviceSize fSize,
	VkBufferUsageFlags fUsage, VkMemoryPropertyFlags fProperty)
{
	VkBufferCreateInfo StagingBufferInfo{};
	StagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	StagingBufferInfo.size = fSize;
	StagingBufferInfo.usage = fUsage;
	StagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateBuffer(mDevice, &StagingBufferInfo, nullptr, &fBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements StagingMemRequirements;
	vkGetBufferMemoryRequirements(mDevice, fBuffer, &StagingMemRequirements);

	VkMemoryAllocateInfo StagingAllocInfo{};
	StagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	StagingAllocInfo.allocationSize = StagingMemRequirements.size;
	StagingAllocInfo.memoryTypeIndex = FindMemoryType(StagingMemRequirements.memoryTypeBits, fProperty);

	if (vkAllocateMemory(mDevice, &StagingAllocInfo, nullptr, &fMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(mDevice, fBuffer, fMemory, 0);
}
void VulkanApp::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = mCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(mDevice, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion{};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(mComputeQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(mComputeQueue);

	vkFreeCommandBuffers(mDevice, mCommandPool, 1, &commandBuffer);
}