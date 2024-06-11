#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <vector>
#include <array>
#include <stdexcept>
#include <optional>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <set>
#include <algorithm>
#include <limits>
#include <memory>
#include <random>
#include <chrono>

// Velocity UV deltax/ms
// Pressure P
// Density ¦Ñ
// Concentration s
// Temperature k

// First Step Adevction():
// Change Velocity, Concentration and Temperature
// 
// Second Step Buoyancy() 
// 1. Change Concentration and Temperature based on the results of Adevction()
// 2. Calculate Buoyancy based on the Concentration and Temperature from 1
// 3. Change Velocity based on Buoyancy
//
// Third Step Projection
// 1. Calculate Density based on the Concentration and Temperature from Buoyancy() 
// 2. Calculate Pressure based on the Density from 1 and Velocity from Adevction()
// 3. Calculate New Velocity based on the Density from 1 and the Pressure from 2

const int MAX_FRAMES_IN_FLIGHT = 2;
const uint32_t Count = 1024;

const uint32_t cMaxX = 200;
const uint32_t cMaxY = 200;

const float cDensityAir = 1.28;
const float cDensitySoot = 1.5;
const float cAlpha = (cDensitySoot - cDensityAir) / cDensityAir;

const float cTemperatureAmbient = 273.0;
const float cBelta = 1 / cTemperatureAmbient;

const float cDeltaX = 1.0;


const uint32_t cIterationTimes = 20;

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsAndComputeFamily;
	std::optional<uint32_t> presentFamily;

	bool IsComplete() {
		return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
	}
};
struct SwapChainSupportDetailsS {
	VkSurfaceCapabilitiesKHR mCapabilities;
	std::vector<VkSurfaceFormatKHR> mFormats;
	std::vector<VkPresentModeKHR> mPresentModes;
};

struct SmokeGridCell2D {
	// This is 2D MAC grid
	// The horizontal u-component of velocity is sampled at the centers of the vertical cell faces,
	// The vertical v-component of velocity is sampled at the centers of the horizontal cell faces,
	float mVelocityU = 0;
	float mVelocityV = 0;

	// Following data is sampled at the center of the cell
	float mPressure = 0;
	float mDensity = 0;
	float mConcentration = 0;
	float mTemperature = cTemperatureAmbient;

	// Type of the Cell
	// 0 means Fluid, 1 means Solid, 2 means Const Cell whose data will never change 
	// If cell is solid, mIndexCloest point to the cloest cell which is not fluid
	uint32_t mCellType = 0;
	uint32_t mIndexCloest = 0;
};
struct FluidUniform {
	uint32_t mMaxX = cMaxX;
	uint32_t mMaxY = cMaxY;

	float mDeltaTime = 33;
	float mDeltaX = cDeltaX;

	float mDensityAir = cDensityAir;
	float mDensitySoot = cDensitySoot;
	float mAlpha = cAlpha;

	float mTemperatureAmbient = cTemperatureAmbient;
	float mBelta = cBelta;
};

struct ProjectionConst {
	// Martix A
	uint32_t mCenterIndex = 0;
	uint32_t mRightIndex = 0;
	uint32_t mLeftIndex = 0;
	uint32_t mUpIndex = 0;
	uint32_t mDownIndex = 0;

	float mCenterCoefficient = 0;
	float mRightCoefficient = 0;
	float mLeftCoefficient = 0;
	float mUpCoefficient = 0;
	float mDownCoefficient = 0;


	// Source term B
	float mSourceB = 0;
};
struct ProjectionVector {
	float mPositionX = 0;
	float mDirectionP = 0;
	float mResidualR = 0;
};
struct ProjectionUniform {
	float mRkRk = 1;
	float mPkAPk = 1;
	// mDistanceAlpha = RkRk/PkAPk
	float mDistanceAlpha = 1;
	// mDirectionBelta = R(k+1)R(k+1)/RkRk
	float mDirectionBelta = 1;

	// 0. Init() change mRkRk
	// 1. PkaPk() change mPkAPk & mDistanceAlpha
	// 4. Rk+1Rk+1() change mDirectionBelta then RkRk = R(k+1)R(k+1) 

	float mRkBiggest = 0;
};

struct SumData {
	float sum = 0;
	float biggest = 0;
};

class VulkanTest {
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif
	FluidUniform mUFO;
public:
	VulkanTest() {
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

		// Layout&Pipeline
		CreateDrawInitLayoutNPipeline();

		CreateAdvectionLayoutNPipeline();
		CreateBuoyancyLayoutNPipeline();
		CreateProjectionInitLayoutNPipeline();

		CreateRkRkLayoutNPipeline();
		CreatePkAPkLayoutNPipeline();
		CreateRkRkSumLayoutNPipeline();
		CreatePkAPkSumLayoutNPipeline();
		CreateXkLayoutNPipeline();
		CreateRkLayoutNPipeline();
		CreatePkLayoutNPipeline();
		CreateFinalLayoutNPipeline();

		// Buffer
		CreateDescriptorPool();
		CreateDrawBuffers();
		CreateShaderStorageBuffers();
		CreateUniformBuffer();

		CreateProjectionConstBuffer();
		CreateProjectionVectorBuffer();
		CreateProjectionUniformBuffer();
		CreateSumBuffer();



		//// Descriptor Set
		CreateDrawInitComputeDescriptorSets();

		CreateAdvectionComputeDescriptorSets();
		CreateBuoyancyComputeDescriptorSets();
		CreateProjectionInitDescriptorSet();
		CreateRkRkDescriptorSet();
		CreateRkRkSumDescriptorSet();
		CreatePkAPkDescriptorSet();
		CreatePkAPkSumDescriptorSet();
		CreateXkDescriptorSet();
		CreateRkDescriptorSet();
		CreatePkDescriptorSet();
		CreateFinalDescriptorSets();

		CreateDrawInitCommandBuffers();

		CreateAdvectionCommandBuffers();
		CreateBuoyancyCommandBuffers();
		CreateProjectionInitCommandBuffers();
		CreateRkRkCommandBuffers();
		CreateRkRkSumCommandBuffers();
		CreatePkAPkCommandBuffers();
		CreatePkAPkSumCommandBuffers();
		CreateXkCommandBuffers();
		CreateRkCommandBuffers();
		CreatePkCommandBuffers();
		CreateFinalCommandBuffers();

		CreateSyncObjects();


		CreateDrawSyncObjects();
		CreateDrawCommandBuffer();
		CreateDrawPipeline();
	}
	~VulkanTest() {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(mDevice, mRkRkInSumFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mRkRkInFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mRkFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mXkFinishedSemaphores[i], nullptr);

			vkDestroySemaphore(mDevice, mPkAPkSumFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mPkAPkFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mRkRkFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mProjectionInitFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mBuoyancyFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mAdvectionFinishedSemaphores[i], nullptr);


			vkDestroyFence(mDevice, mPerpareFinishedFences[i], nullptr);
			vkDestroyFence(mDevice, mComputeInFlightFences[i], nullptr);
			vkDestroyFence(mDevice, mDrawInitFences[i], nullptr);

			vkDestroySemaphore(mDevice, mImageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(mDevice, mRenderFinishedSemaphores[i], nullptr);
			vkDestroyFence(mDevice, mDrawFences[i], nullptr);


			vkDestroyBuffer(mDevice, mDrawBuffers[i], nullptr);
			vkFreeMemory(mDevice, mDrawBuffersMemory[i], nullptr);


			vkDestroyBuffer(mDevice, mStorageBuffers[i], nullptr);
			vkFreeMemory(mDevice, mStorageBuffersMemory[i], nullptr);
		}


		// Buffer
		vkDestroyBuffer(mDevice, mSumBuffer, nullptr);
		vkFreeMemory(mDevice, mSumBufferMemory, nullptr);
		vkDestroyBuffer(mDevice, mProjectionConstBuffer, nullptr);
		vkFreeMemory(mDevice, mProjectionConstBufferMemory, nullptr);
		vkDestroyBuffer(mDevice, mProjectionVectorBuffer, nullptr);
		vkFreeMemory(mDevice, mProjectionVectorBufferMemory, nullptr);
		vkDestroyBuffer(mDevice, mProjectionUniformBuffer, nullptr);
		vkFreeMemory(mDevice, mProjectionUniformBufferMemory, nullptr);

		
		vkDestroyBuffer(mDevice, mBuoyancyStorageBuffer, nullptr);
		vkFreeMemory(mDevice, mBuoyancyStorageBufferMemory, nullptr); 
		vkDestroyBuffer(mDevice, mAdvectionStorageBuffer, nullptr);
		vkFreeMemory(mDevice, mAdvectionStorageBufferMemory, nullptr);
		vkDestroyBuffer(mDevice, mUniformBuffer, nullptr);
		vkFreeMemory(mDevice, mUniformBufferMemory, nullptr);

		vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);



		// Layout&Pipeline

		vkDestroyPipeline(mDevice, mDrawPipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mDrawPipelineLayout, nullptr);

		vkDestroyPipeline(mDevice, mFinalComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mFinalPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mFinalDescriptorSetLayout, nullptr);

		vkDestroyPipeline(mDevice, mPkComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mPkPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mPkDescriptorSetLayout, nullptr);
		vkDestroyPipeline(mDevice, mRkComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mRkPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mRkDescriptorSetLayout, nullptr);
		vkDestroyPipeline(mDevice, mXkComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mXkPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mXkDescriptorSetLayout, nullptr);


		vkDestroyPipeline(mDevice, mPkAPkSumComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mPkAPkSumPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mPkAPkSumDescriptorSetLayout, nullptr);
		vkDestroyPipeline(mDevice, mRkRkSumComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mRkRkSumPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mRkRkSumDescriptorSetLayout, nullptr);

		vkDestroyPipeline(mDevice, mPkAPkComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mPkAPkPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mPkAPkDescriptorSetLayout, nullptr);
		vkDestroyPipeline(mDevice, mRkRkComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mRkRkPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mRkRkDescriptorSetLayout, nullptr);

		vkDestroyPipeline(mDevice, mProjectionInitComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mProjectionInitPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mProjectionInitDescriptorSetLayout, nullptr);
		vkDestroyPipeline(mDevice, mBuoyancyComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mBuoyancyPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mBuoyancyDescriptorSetLayout, nullptr);
		vkDestroyPipeline(mDevice, mAdvectionComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mAdvectionPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mAdvectionDescriptorSetLayout, nullptr);


		vkDestroyPipeline(mDevice, mDrawInitComputePipeline, nullptr);
		vkDestroyPipelineLayout(mDevice, mDrawInitPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mDevice, mDrawInitDescriptorSetLayout, nullptr);




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

	void Run() {
		DrawInit();

		uint32_t i = 0;
		std::vector<long long> v1;
		v1.reserve(10000);

		auto LastTime = std::chrono::system_clock::now();
		auto LastTime_ms = std::chrono::time_point_cast<std::chrono::microseconds>(LastTime);
		auto LastTime_value = LastTime_ms.time_since_epoch().count();

		auto time = std::chrono::system_clock::now();;
		auto time_ms = std::chrono::time_point_cast<std::chrono::microseconds>(time);
		auto time_value = time_ms.time_since_epoch().count();

		auto RunTimes = 0;


		while (!glfwWindowShouldClose(mWindowPtr)) {
			glfwPollEvents();
			Draw();

			LastTime = std::chrono::system_clock::now();
			LastTime_ms = std::chrono::time_point_cast<std::chrono::microseconds>(LastTime);
			LastTime_value = LastTime_ms.time_since_epoch().count();

			Advection();
			Buoyancy();
			ProjectionInit();
			RkRk();
			auto RunTimes = 0;
			for (uint32_t i = 0; ; ++i) {

				PkAPk();
				Xk();
				Rk();
				RkRkIn();
				Pk();
				++RunTimes;
				float a = ((ProjectionUniform*)mProjectionUniformBufferMapped)->mRkBiggest;
//				std::cout << a << "    ";
				if (a < 0.0001) {
					std::cout << std::endl;
					std::cout << RunTimes << std::endl;
					break;
				}
				if (a < 0.0001) {
					std::cout << std::endl;
					std::cout << RunTimes << std::endl;
					break;
				}

			}
			Final();


			time = std::chrono::system_clock::now();;
			time_ms = std::chrono::time_point_cast<std::chrono::microseconds>(time);
			time_value = time_ms.time_since_epoch().count();
			time_value = time_value - LastTime_value;


			mUFO.mDeltaTime = time_value / 1000;
			std::cout <<"    "<< mUFO.mDeltaTime;
			std::cout << std::endl;
			if (mUFO.mDeltaTime > 33)
				mUFO.mDeltaTime = 33;

			std::cout << "    " << mUFO.mDeltaTime;
			std::cout << std::endl;
		}
		/*
		long long sum = 0;
		long long largest = 0;
		long long smallest = 100000000;
		for (int32_t i = 0; i < v1.size(); ++i) {
			sum = sum + v1[i];
			if (v1[i] > largest) {
				largest = v1[i];
			}
			if (v1[i] < smallest) {
				smallest = v1[i];
			}
		}
		long long avg = sum / RunTimes;
		std::cout << sum << "   " << largest << "   " << smallest << "   " << avg << std::endl;
		for (int i = v1.size(); i > v1.size() - 10; --i){
			std::cout << v1[i - 1] << "    ";
		}
		*/
	}


private:
	//Step1 Device
	void CreateGlfwWindow(uint32_t fWidth, uint32_t fHeight, std::string fName) {
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

		mWindowPtr = glfwCreateWindow(fWidth, fHeight, fName.c_str(), nullptr, nullptr);

		// mLastTime = glfwGetTime();
	}
	void CreateInstance(std::vector<const char*> fValidationLayers) {
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
	void CreateSurface() {
		if (glfwCreateWindowSurface(mInstance, mWindowPtr, nullptr, &mSurface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}
	void PickPhysicalDevice(std::vector<const char*> fDeviceExtensions) {
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
	}
	void CreateLogicalDevice(std::vector<const char*> fValidationLayers, std::vector<const char*> fDeviceExtensions) {
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
	void CreateCommandPool() {
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(mPhysicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

		if (vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mCommandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}
	//Step1 Device Helped Fiction
	bool CheckValidationLayerSupport(std::vector<const char*> fValidationLayers) {
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
	bool IsDeviceSuitable(VkPhysicalDevice fPhysicalDevice, std::vector<const char*> fDeviceExtensions) {
		QueueFamilyIndices indices = FindQueueFamilies(fPhysicalDevice);

		bool extensionsSupported = CheckDeviceExtensionSupport(fPhysicalDevice, fDeviceExtensions);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetailsS swapChainSupport = QuerySwapChainSupport(fPhysicalDevice);
			swapChainAdequate = !swapChainSupport.mFormats.empty() && !swapChainSupport.mPresentModes.empty();
		}

		return indices.IsComplete() && extensionsSupported && swapChainAdequate;
	}
	bool CheckDeviceExtensionSupport(VkPhysicalDevice fPhysicalDevice, std::vector<const char*> fDeviceExtensions) {
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
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice fPhysicalDevice) {
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
	SwapChainSupportDetailsS QuerySwapChainSupport(VkPhysicalDevice fPhysicalDevice) {
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

	// SwapChain 
	void CreateSwapChain() {
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
	void CreateSwapChainImageViews() {
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
	void CreateFramebuffers() {
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
	void CreateDepthResources() {
		VkFormat depthFormat = FindDepthFormat();

		CreateImage(mSwapChainExtent.width, mSwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, mDepthImage, mDepthImageMemory);
		mDepthImageView = CreateImageView(mDepthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	}
	void CreateRenderPass() {
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
	VkFormat FindDepthFormat() {
		return FindSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}
	bool HasStencilComponent(VkFormat format) {
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
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
	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		throw std::runtime_error("failed to find suitable memory type!");
	}
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
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
	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
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
	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& fAvailableFormats) {
		for (const auto& availableFormat : fAvailableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		return fAvailableFormats[0];
	}
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& fAvailablePresentModes) {
		for (const auto& availablePresentMode : fAvailablePresentModes) {
			if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
				return availablePresentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& fCapabilities) {
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



	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(glm::vec4);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}
	static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = 0;

		return attributeDescriptions;
	}
	void CreateDrawPipeline() {
		auto VertShaderCode = ReadFile("shaders/fluid/DrawVert.spv");
		VkShaderModule VertShaderModule = CreateShaderModule(VertShaderCode);

		auto FragShaderCode = ReadFile("shaders/fluid/DrawFrag.spv");
		VkShaderModule FragShaderModule = CreateShaderModule(FragShaderCode);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = VertShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = FragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = getBindingDescription();
		auto attributeDescriptions = getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0;
		pipelineLayoutInfo.pushConstantRangeCount = 0;

		if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &mDrawPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = mDrawPipelineLayout;
		pipelineInfo.renderPass = mRenderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		if (vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mDrawPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!");
		}


		vkDestroyShaderModule(mDevice, VertShaderModule, nullptr);
		vkDestroyShaderModule(mDevice, FragShaderModule, nullptr);
	}
	VkDescriptorSetLayout mDrawDescriptorSetLayout;
	VkPipelineLayout mDrawPipelineLayout;
	VkPipeline mDrawPipeline;
	void BeginCommandBuffer(VkCommandBuffer fCommandBuffer) {
		if (IsBufferBeigin != false) {
			throw std::runtime_error("CommandBuffer has begun");
		}

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		if (vkBeginCommandBuffer(fCommandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording command buffer!");
		}

		IsBufferBeigin = true;
	}
	void EndCommandBuffer(VkCommandBuffer fCommandBuffer) {
		if (IsBufferBeigin != true) {
			throw std::runtime_error("CommandBuffer has not begun");
		}

		if (vkEndCommandBuffer(fCommandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}

		IsBufferBeigin = false;
	}
	void BeginRenderPass(VkCommandBuffer fCommandBuffer, uint32_t fImageIndex) {
		if (IsRenderPassBegin != false) {
			throw std::runtime_error("RenderPass has begun");
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = mRenderPass;
		renderPassInfo.framebuffer = mSwapChainFramebuffers[fImageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = mSwapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {1.0f, 1.0f, 1.0f, 1.0f} };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(fCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		IsRenderPassBegin = true;
	}
	void EndRenderPass(VkCommandBuffer fCommandBuffer) {
		if (IsRenderPassBegin != true) {
			throw std::runtime_error("RenderPass has not begun");
		}

		vkCmdEndRenderPass(fCommandBuffer);

		IsRenderPassBegin = false;
	}
	void SetViewport(VkCommandBuffer fCommandBuffer) {
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(mSwapChainExtent.width);
		viewport.height = static_cast<float>(mSwapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(fCommandBuffer, 0, 1, &viewport);
	}
	void SetScissor(VkCommandBuffer fCommandBuffer) {
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = mSwapChainExtent;
		vkCmdSetScissor(fCommandBuffer, 0, 1, &scissor);
	}

	void RecordCommandBuffer(VkCommandBuffer fCommandBuffer, uint32_t mCurrentFrame, uint32_t fImageIndex) {
		BeginCommandBuffer(fCommandBuffer);
		BeginRenderPass(fCommandBuffer, fImageIndex);


		vkCmdBindPipeline(fCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDrawPipeline);
		SetViewport(fCommandBuffer);
		SetScissor(fCommandBuffer);
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(fCommandBuffer, 0, 1, &mDrawBuffers[mCurrentFrame], offsets);

		vkCmdDraw(fCommandBuffer, 6 * cMaxX * cMaxY, 1, 0, 0);

		EndRenderPass(fCommandBuffer);
		EndCommandBuffer(fCommandBuffer);
	}		
	void Draw() {
		vkWaitForFences(mDevice, 1, &mDrawFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(mDevice, mSwapChain, UINT64_MAX, mImageAvailableSemaphores[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);


		vkResetFences(mDevice, 1, &mDrawFences[mCurrentFrame]);

		vkResetCommandBuffer(mDrawCommandBuffers[mCurrentFrame], 0);
		RecordCommandBuffer(mDrawCommandBuffers[mCurrentFrame], mCurrentFrame, imageIndex);


		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = { mImageAvailableSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mDrawCommandBuffers[mCurrentFrame];
		VkSemaphore signalSemaphores[] = { mRenderFinishedSemaphores[mCurrentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;
		if (vkQueueSubmit(mGraphicsQueue, 1, &submitInfo, mDrawFences[mCurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		VkSwapchainKHR swapChains[] = { mSwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;

		vkQueuePresentKHR(mPresentQueue, &presentInfo);

		vkWaitForFences(mDevice, 1, &mDrawFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
	}
	std::vector<VkCommandBuffer> mDrawCommandBuffers;
	bool IsBufferBeigin = false;

	void CreateDrawCommandBuffer() {
		mDrawCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mDrawCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mDrawCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}
	}
	void CreateDrawSyncObjects() {
		mImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mDrawFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mImageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRenderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(mDevice, &fenceInfo, nullptr, &mDrawFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create semaphores!");
			}
		}
	}
	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mDrawFences;


	// Layout & Pipeline
	void CreateAdvectionLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mAdvectionDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mAdvectionPipelineLayout, mAdvectionDescriptorSetLayout);
		CreateComputePipeline(mAdvectionComputePipeline, mAdvectionPipelineLayout, "shaders/fluid/Advection.spv");
	}
	void CreateBuoyancyLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mBuoyancyDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mBuoyancyPipelineLayout, mBuoyancyDescriptorSetLayout);
		CreateComputePipeline(mBuoyancyComputePipeline, mBuoyancyPipelineLayout, "shaders/fluid/Buoyancy.spv");
	}
	VkDescriptorSetLayout mAdvectionDescriptorSetLayout;
	VkPipelineLayout mAdvectionPipelineLayout;
	VkPipeline mAdvectionComputePipeline;
	VkDescriptorSetLayout mBuoyancyDescriptorSetLayout;
	VkPipelineLayout mBuoyancyPipelineLayout;
	VkPipeline mBuoyancyComputePipeline;
	// Conjugate Gradient Layout & Pipeline
	void CreateProjectionInitLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mProjectionInitDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mProjectionInitPipelineLayout, mProjectionInitDescriptorSetLayout);
		CreateComputePipeline(mProjectionInitComputePipeline, mProjectionInitPipelineLayout, "shaders/fluid/ProjectionInit.spv");
	}
	void CreateRkRkLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mRkRkDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mRkRkPipelineLayout, mRkRkDescriptorSetLayout);
		CreateComputePipeline(mRkRkComputePipeline, mRkRkPipelineLayout, "shaders/fluid/RkRk.spv");
	}
	void CreatePkAPkLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mPkAPkDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mPkAPkPipelineLayout, mPkAPkDescriptorSetLayout);
		CreateComputePipeline(mPkAPkComputePipeline, mPkAPkPipelineLayout, "shaders/fluid/PkAPk.spv");
	}
	void CreateRkRkSumLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mRkRkSumDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mRkRkSumPipelineLayout, mRkRkSumDescriptorSetLayout);
		CreateComputePipeline(mRkRkSumComputePipeline, mRkRkSumPipelineLayout, "shaders/fluid/RkRkSum.spv");
	}
	void CreatePkAPkSumLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mPkAPkSumDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mPkAPkSumPipelineLayout, mPkAPkSumDescriptorSetLayout);
		CreateComputePipeline(mPkAPkSumComputePipeline, mPkAPkSumPipelineLayout, "shaders/fluid/PkAPkSum.spv");
	}
	void CreateXkLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mXkDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mXkPipelineLayout, mXkDescriptorSetLayout);
		CreateComputePipeline(mXkComputePipeline, mXkPipelineLayout, "shaders/fluid/Xk.spv");
	}
	void CreateRkLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mRkDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mRkPipelineLayout, mRkDescriptorSetLayout);
		CreateComputePipeline(mRkComputePipeline, mRkPipelineLayout, "shaders/fluid/Rk.spv");
	}
	void CreatePkLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mPkDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mPkPipelineLayout, mPkDescriptorSetLayout);
		CreateComputePipeline(mPkComputePipeline, mPkPipelineLayout, "shaders/fluid/Pk.spv");
	}
	void CreateFinalLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mFinalDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mFinalPipelineLayout, mFinalDescriptorSetLayout);
		CreateComputePipeline(mFinalComputePipeline, mFinalPipelineLayout, "shaders/fluid/Final.spv");
	}
	VkDescriptorSetLayout mProjectionInitDescriptorSetLayout;
	VkPipelineLayout mProjectionInitPipelineLayout;
	VkPipeline mProjectionInitComputePipeline;
	VkDescriptorSetLayout mRkRkDescriptorSetLayout;
	VkPipelineLayout mRkRkPipelineLayout;
	VkPipeline mRkRkComputePipeline;
	VkDescriptorSetLayout mPkAPkDescriptorSetLayout;
	VkPipelineLayout mPkAPkPipelineLayout;
	VkPipeline mPkAPkComputePipeline;
	VkDescriptorSetLayout mRkRkSumDescriptorSetLayout;
	VkPipelineLayout mRkRkSumPipelineLayout;
	VkPipeline mRkRkSumComputePipeline;
	VkDescriptorSetLayout mPkAPkSumDescriptorSetLayout;
	VkPipelineLayout mPkAPkSumPipelineLayout;
	VkPipeline mPkAPkSumComputePipeline;
	VkDescriptorSetLayout mXkDescriptorSetLayout;
	VkPipelineLayout mXkPipelineLayout;
	VkPipeline mXkComputePipeline;
	VkDescriptorSetLayout mRkDescriptorSetLayout;
	VkPipelineLayout mRkPipelineLayout;
	VkPipeline mRkComputePipeline;
	VkDescriptorSetLayout mPkDescriptorSetLayout;
	VkPipelineLayout mPkPipelineLayout;
	VkPipeline mPkComputePipeline;


	VkDescriptorSetLayout mFinalDescriptorSetLayout;
	VkPipelineLayout mFinalPipelineLayout;
	VkPipeline mFinalComputePipeline;


	//Buffer
	void CreateShaderStorageBuffers() {

		SmokeGridCell2D sDefaultGrid;
		std::vector<SmokeGridCell2D> sArrayGridCell2D(cMaxX * cMaxY, sDefaultGrid);

		SmokeGridCell2D sConstGrid;
		sConstGrid.mCellType = 2;
		sConstGrid.mVelocityU = 0.1;
		sConstGrid.mVelocityV = 0.1;
		sConstGrid.mConcentration = 1;
		sConstGrid.mTemperature = 300;


		SmokeGridCell2D sSolidGrid;
		sSolidGrid.mCellType = 1;
		sSolidGrid.mVelocityU = 0;
		sSolidGrid.mVelocityV = 0;


		int a = 0;
		for (uint32_t i = a; i <= a + cMaxX * 6; ++i) {
			if(i % cMaxX > (cMaxX / 4) && i % cMaxX < (3 * cMaxX / 4))
			sArrayGridCell2D[i] = sConstGrid;
		}

		for (uint32_t i = 0; i < cMaxX * cMaxY; ++i) {
			if ((i % cMaxX) == 0) {
				sArrayGridCell2D[i] = sSolidGrid;
			}
			if ((i % cMaxX) == cMaxX - 1) {
				sArrayGridCell2D[i] = sSolidGrid;
			}
			if (i < cMaxX) {
				sArrayGridCell2D[i] = sSolidGrid;
			}
		}
		/*
		int a = 800 * 5;		
		for (uint32_t i = a; i <= a + 800 * 5; ++i) {
			if (i % 800 < 55 && i % 800 > 50) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 105 && i % 800 > 100) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 155 && i % 800 > 150) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 205 && i % 800 > 200) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 255 && i % 800 > 250) {
				sArrayGridCell2D[i] = sConstGrid;
			}

			if (i % 800 < 405 && i % 800 > 400) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 455 && i % 800 > 450) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 505 && i % 800 > 500) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 555 && i % 800 > 550) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 605 && i % 800 > 600) {
				sArrayGridCell2D[i] = sConstGrid;
			}


			if (i % 800 < 800 && i % 800 > 795) {
				sArrayGridCell2D[i] = sConstGrid;
			}
		}
		int b = 800 * 300;
		for (uint32_t i = b; i <= b + 800 * 5; ++i) {
			if (i % 800 < 55 && i % 800 > 50) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 105 && i % 800 > 100) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 155 && i % 800 > 150) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 205 && i % 800 > 200) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 255 && i % 800 > 250) {
				sArrayGridCell2D[i] = sConstGrid;
			}

			if (i % 800 < 405 && i % 800 > 400) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 455 && i % 800 > 450) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 505 && i % 800 > 500) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 555 && i % 800 > 550) {
				sArrayGridCell2D[i] = sConstGrid;
			}
			if (i % 800 < 605 && i % 800 > 600) {
				sArrayGridCell2D[i] = sConstGrid;
			}


			if (i % 800 < 800 && i % 800 > 795) {
				sArrayGridCell2D[i] = sConstGrid;
			}
		}
		*/

		size_t bufferSize = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

		// Create a staging buffer used to upload data to the gpu
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, sArrayGridCell2D.data(), bufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		mStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		mStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		mStorageBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		// Copy initial particle data to all storage buffers
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(mStorageBuffers[i], mStorageBuffersMemory[i], bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			vkBindBufferMemory(mDevice, mStorageBuffers[i], mStorageBuffersMemory[i], 0);
			CopyBuffer(stagingBuffer, mStorageBuffers[i], bufferSize);

			vkMapMemory(mDevice, mStorageBuffersMemory[i], 0, bufferSize, 0, &mStorageBuffersMapped[i]);
		}

		CreateBuffer(mAdvectionStorageBuffer, mAdvectionStorageBufferMemory, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vkBindBufferMemory(mDevice, mAdvectionStorageBuffer, mAdvectionStorageBufferMemory, 0);
		CopyBuffer(stagingBuffer, mAdvectionStorageBuffer, bufferSize);
		vkMapMemory(mDevice, mAdvectionStorageBufferMemory, 0, bufferSize, 0, &mAdvectionStorageBufferMapped);

		CreateBuffer(mBuoyancyStorageBuffer, mBuoyancyStorageBufferMemory, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vkBindBufferMemory(mDevice, mBuoyancyStorageBuffer, mBuoyancyStorageBufferMemory, 0);
		CopyBuffer(stagingBuffer, mBuoyancyStorageBuffer, bufferSize);
		vkMapMemory(mDevice, mBuoyancyStorageBufferMemory, 0, bufferSize, 0, &mBuoyancyStorageBufferMapped);

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

	}
	void CreateUniformBuffer() {
		FluidUniform sFluidUfo;

		size_t bufferSize = sizeof(FluidUniform);

		CreateBuffer(
			mUniformBuffer, mUniformBufferMemory, bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, mUniformBuffer, mUniformBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, mUniformBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &sFluidUfo, (size_t)bufferSize);
		vkUnmapMemory(mDevice, mUniformBufferMemory);
	}
	void CreateProjectionConstBuffer() {
		uint32_t count = cMaxX * cMaxY;

		ProjectionConst sDefaultConst;
		std::vector<ProjectionConst> sProjectionConstArray(count, sDefaultConst);
		size_t sConstBufferSize = count * sizeof(ProjectionConst);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, sConstBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, sConstBufferSize, 0, &data);
		memcpy(data, sProjectionConstArray.data(), sConstBufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		CreateBuffer(mProjectionConstBuffer, mProjectionConstBufferMemory, sConstBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vkBindBufferMemory(mDevice, mProjectionConstBuffer, mProjectionConstBufferMemory, 0);
		CopyBuffer(stagingBuffer, mProjectionConstBuffer, sConstBufferSize);
		vkMapMemory(mDevice, mProjectionConstBufferMemory, 0, sConstBufferSize, 0, &mProjectionConstStorageBufferMapped);

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
	}
	void CreateProjectionVectorBuffer() {
		uint32_t count = cMaxX * cMaxY;

		ProjectionVector sDefaultConst;
		std::vector<ProjectionVector> sProjectionVectorArray(count, sDefaultConst);
		size_t sVectorBufferSize = count * sizeof(ProjectionVector);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, sVectorBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, sVectorBufferSize, 0, &data);
		memcpy(data, sProjectionVectorArray.data(), sVectorBufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		CreateBuffer(mProjectionVectorBuffer, mProjectionVectorBufferMemory, sVectorBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vkBindBufferMemory(mDevice, mProjectionVectorBuffer, mProjectionVectorBufferMemory, 0);
		CopyBuffer(stagingBuffer, mProjectionVectorBuffer, sVectorBufferSize);
		vkMapMemory(mDevice, mProjectionVectorBufferMemory, 0, sVectorBufferSize, 0, &mProjectionVectorStorageBufferMapped);

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
	}
	void CreateProjectionUniformBuffer() {
		ProjectionUniform sProjectionUniform;

		size_t bufferSize = sizeof(ProjectionUniform);

		CreateBuffer(
			mProjectionUniformBuffer, mProjectionUniformBufferMemory, bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, mProjectionUniformBuffer, mProjectionUniformBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, mProjectionUniformBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &sProjectionUniform, (size_t)bufferSize);
		vkUnmapMemory(mDevice, mProjectionUniformBufferMemory);


		vkMapMemory(mDevice, mProjectionUniformBufferMemory, 0, bufferSize, 0, &mProjectionUniformBufferMapped);
	}
	void CreateSumBuffer() {
		uint32_t count = 1024;

		SumData sData;

		std::vector<SumData> sSumArray(count, sData);
		size_t sSumBufferSize = count * sizeof(SumData);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, sSumBufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, sSumBufferSize, 0, &data);
		memcpy(data, sSumArray.data(), sSumBufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		CreateBuffer(mSumBuffer, mSumBufferMemory, sSumBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		vkBindBufferMemory(mDevice, mSumBuffer, mSumBufferMemory, 0);
		CopyBuffer(stagingBuffer, mSumBuffer, sSumBufferSize);
		vkMapMemory(mDevice, mSumBufferMemory, 0, sSumBufferSize, 0, &mSumStorageBufferMapped);

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
	}
	VkBuffer mUniformBuffer;
	VkDeviceMemory mUniformBufferMemory;
	std::vector<VkBuffer> mStorageBuffers;
	std::vector<VkDeviceMemory> mStorageBuffersMemory;
	std::vector<void*> mStorageBuffersMapped;
	VkBuffer mAdvectionStorageBuffer;
	VkDeviceMemory mAdvectionStorageBufferMemory;
	void* mAdvectionStorageBufferMapped;
	VkBuffer mBuoyancyStorageBuffer;
	VkDeviceMemory mBuoyancyStorageBufferMemory;
	void* mBuoyancyStorageBufferMapped; 
	// Conjugate Gradient Buffer
	VkBuffer mProjectionConstBuffer;
	VkDeviceMemory mProjectionConstBufferMemory;
	void* mProjectionConstStorageBufferMapped;
	VkBuffer mProjectionVectorBuffer;
	VkDeviceMemory mProjectionVectorBufferMemory;
	void* mProjectionVectorStorageBufferMapped;
	VkBuffer mProjectionUniformBuffer;
	VkDeviceMemory mProjectionUniformBufferMemory;
	void* mProjectionUniformBufferMapped;
	VkBuffer mSumBuffer;
	VkDeviceMemory mSumBufferMemory;
	void* mSumStorageBufferMapped;



	void CreateDrawBuffers() {
		std::vector<glm::vec4> sArray(6 * cMaxX * cMaxY, { 0,0,0,0 });

		size_t bufferSize = 6 * cMaxX * cMaxY * sizeof(glm::vec4);

		// Create a staging buffer used to upload data to the gpu
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		vkBindBufferMemory(mDevice, stagingBuffer, stagingBufferMemory, 0);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, sArray.data(), bufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		mDrawBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		mDrawBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		mDrawBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		// Copy initial particle data to all storage buffers
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(mDrawBuffers[i], mDrawBuffersMemory[i], bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			vkBindBufferMemory(mDevice, mDrawBuffers[i], mDrawBuffersMemory[i], 0);
			CopyBuffer(stagingBuffer, mDrawBuffers[i], bufferSize);
			vkMapMemory(mDevice, mDrawBuffersMemory[i], 0, bufferSize, 0, &mDrawBuffersMapped[i]);
		}

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

	}
	std::vector<VkBuffer> mDrawBuffers;
	std::vector<VkDeviceMemory> mDrawBuffersMemory;
	std::vector<void*> mDrawBuffersMapped;

	void CreateDrawInitLayoutNPipeline() {
		CreateDescriptorSetLayout(
			mDrawInitDescriptorSetLayout,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT);
		CreatePipelineLayout(mDrawInitPipelineLayout, mDrawInitDescriptorSetLayout);
		CreateComputePipeline(mDrawInitComputePipeline, mDrawInitPipelineLayout, "shaders/fluid/DrawInit.spv");
	}
	VkDescriptorSetLayout mDrawInitDescriptorSetLayout;
	VkPipelineLayout mDrawInitPipelineLayout;
	VkPipeline mDrawInitComputePipeline;

	void CreateDrawInitComputeDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mDrawInitDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		mDrawInitDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(mDevice, &allocInfo, mDrawInitDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
			VkDescriptorBufferInfo storageBufferInfoLastFrame{};
			storageBufferInfoLastFrame.buffer = mDrawBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
			storageBufferInfoLastFrame.offset = 0;
			storageBufferInfoLastFrame.range = 6 * cMaxX * cMaxY * sizeof(glm::vec4);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = mDrawInitDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &storageBufferInfoLastFrame;

			VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
			storageBufferInfoCurrentFrame.buffer = mDrawBuffers[i];
			storageBufferInfoCurrentFrame.offset = 0;
			storageBufferInfoCurrentFrame.range = 6 * cMaxX * cMaxY * sizeof(glm::vec4);

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = mDrawInitDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &storageBufferInfoCurrentFrame;

			VkDescriptorBufferInfo uniformBufferInfo{};
			uniformBufferInfo.buffer = mUniformBuffer;
			uniformBufferInfo.offset = 0;
			uniformBufferInfo.range = sizeof(FluidUniform);

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = mDrawInitDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &uniformBufferInfo;

			vkUpdateDescriptorSets(mDevice, 3, descriptorWrites.data(), 0, nullptr);
		}
	}
	std::vector<VkDescriptorSet> mDrawInitDescriptorSets;

	void CreateDrawInitCommandBuffers() {
		mDrawInitCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mDrawInitCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mDrawInitCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordDrawInitBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mDrawInitComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mDrawInitPipelineLayout, 0, 1, &mDrawInitDescriptorSets[mCurrentFrame], 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void DrawInit() {
		vkResetFences(mDevice, 1, &mDrawInitFences[mCurrentFrame]);

		vkResetCommandBuffer(mDrawInitCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordDrawInitBuffer(mDrawInitCommandBuffers[mCurrentFrame]);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mDrawInitCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, mDrawInitFences[mCurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
		vkWaitForFences(mDevice, 1, &mDrawInitFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

		/*
		for (uint32_t i = 0; i < 36; ++i) {
			if (i % 6 == 0) {
				std::cout << std::endl;
			}
			std::cout << ((glm::vec4*)mDrawBuffersMapped[1])[i].x << "   ";
			std::cout << ((glm::vec4*)mDrawBuffersMapped[1])[i].y << "   ";
			std::cout << ((glm::vec4*)mDrawBuffersMapped[1])[i].z << "   ";
			std::cout << std::endl;
		}
		std::cout << std::endl;
		std::cout << std::endl; 
		*/
	}
	std::vector<VkCommandBuffer> mDrawInitCommandBuffers;

	// Descriptor Set
	void CreateDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[0].descriptorCount = 30 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = 10 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 10 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		if (vkCreateDescriptorPool(mDevice, &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}
	VkDescriptorPool mDescriptorPool;
	void CreateAdvectionComputeDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mAdvectionDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		mAdvectionDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(mDevice, &allocInfo, mAdvectionDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
			VkDescriptorBufferInfo storageBufferInfoLastFrame{};
			storageBufferInfoLastFrame.buffer = mStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
			storageBufferInfoLastFrame.offset = 0;
			storageBufferInfoLastFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = mAdvectionDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &storageBufferInfoLastFrame;

			VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
			// storageBufferInfoCurrentFrame.buffer = mStorageBuffers[i];
			storageBufferInfoCurrentFrame.buffer = mAdvectionStorageBuffer;
			storageBufferInfoCurrentFrame.offset = 0;
			storageBufferInfoCurrentFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = mAdvectionDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &storageBufferInfoCurrentFrame;

			VkDescriptorBufferInfo uniformBufferInfo{};
			uniformBufferInfo.buffer = mUniformBuffer;
			uniformBufferInfo.offset = 0;
			uniformBufferInfo.range = sizeof(FluidUniform);

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = mAdvectionDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &uniformBufferInfo;

			vkUpdateDescriptorSets(mDevice, 3, descriptorWrites.data(), 0, nullptr);
		}
	}
	void CreateBuoyancyComputeDescriptorSets() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mBuoyancyDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mBuoyancyDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		VkDescriptorBufferInfo storageBufferInfoLastFrame{};
		storageBufferInfoLastFrame.buffer = mAdvectionStorageBuffer;
		storageBufferInfoLastFrame.offset = 0;
		storageBufferInfoLastFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mBuoyancyDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &storageBufferInfoLastFrame;

		VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
		storageBufferInfoCurrentFrame.buffer = mBuoyancyStorageBuffer;
		//storageBufferInfoCurrentFrame.buffer = mStorageBuffers[i];
		storageBufferInfoCurrentFrame.offset = 0;
		storageBufferInfoCurrentFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mBuoyancyDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &storageBufferInfoCurrentFrame;

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = mUniformBuffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(FluidUniform);

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = mBuoyancyDescriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(mDevice, 3, descriptorWrites.data(), 0, nullptr);
	}
	std::vector<VkDescriptorSet> mAdvectionDescriptorSets;
	VkDescriptorSet mBuoyancyDescriptorSet;
	// Conjugate Gradient Descriptor Set
	void CreateProjectionInitDescriptorSet() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mProjectionInitDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mProjectionInitDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mProjectionInitDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
		VkDescriptorBufferInfo sBuoyancyStorageBuffer{};
		sBuoyancyStorageBuffer.buffer = mBuoyancyStorageBuffer;
		sBuoyancyStorageBuffer.offset = 0;
		sBuoyancyStorageBuffer.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mProjectionInitDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sBuoyancyStorageBuffer;

		VkDescriptorBufferInfo sProjectionConstBuffer{};
		sProjectionConstBuffer.buffer = mProjectionConstBuffer;
		sProjectionConstBuffer.offset = 0;
		sProjectionConstBuffer.range = cMaxX * cMaxY * sizeof(ProjectionConst);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mProjectionInitDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sProjectionConstBuffer;

		VkDescriptorBufferInfo sProjectionVectorBuffer{};
		sProjectionVectorBuffer.buffer = mProjectionVectorBuffer;
		sProjectionVectorBuffer.offset = 0;
		sProjectionVectorBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = mProjectionInitDescriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &sProjectionVectorBuffer;

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = mUniformBuffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(FluidUniform);

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = mProjectionInitDescriptorSet;
		descriptorWrites[3].dstBinding = 3;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pBufferInfo = &uniformBufferInfo;

		vkUpdateDescriptorSets(mDevice, 4, descriptorWrites.data(), 0, nullptr);
	}
	void CreateRkRkDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mRkRkDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mRkRkDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		VkDescriptorBufferInfo sProjectionVectorStorageBuffer{};
		sProjectionVectorStorageBuffer.buffer = mProjectionVectorBuffer;
		sProjectionVectorStorageBuffer.offset = 0;
		sProjectionVectorStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mRkRkDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sProjectionVectorStorageBuffer;

		VkDescriptorBufferInfo sSum1024Buffer{};
		sSum1024Buffer.buffer = mSumBuffer;
		sSum1024Buffer.offset = 0;
		sSum1024Buffer.range = 1024 * sizeof(SumData);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mRkRkDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sSum1024Buffer;


		vkUpdateDescriptorSets(mDevice, 2, descriptorWrites.data(), 0, nullptr);
	}
	void CreateRkRkSumDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mRkRkSumDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mRkRkSumDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		VkDescriptorBufferInfo sSum1024Buffer{};
		sSum1024Buffer.buffer = mSumBuffer;
		sSum1024Buffer.offset = 0;
		sSum1024Buffer.range = 1024 * sizeof(SumData);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mRkRkSumDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sSum1024Buffer;

		VkDescriptorBufferInfo sUniformBuffer{};
		sUniformBuffer.buffer = mProjectionUniformBuffer;
		sUniformBuffer.offset = 0;
		sUniformBuffer.range = sizeof(ProjectionUniform);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mRkRkSumDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sUniformBuffer;


		vkUpdateDescriptorSets(mDevice, 2, descriptorWrites.data(), 0, nullptr);
	}
	void CreatePkAPkDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mPkAPkDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mPkAPkDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		VkDescriptorBufferInfo sProjectionConstStorageBuffer{};
		sProjectionConstStorageBuffer.buffer = mProjectionConstBuffer;
		sProjectionConstStorageBuffer.offset = 0;
		sProjectionConstStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionConst);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mPkAPkDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sProjectionConstStorageBuffer;

		VkDescriptorBufferInfo sProjectionVectorStorageBuffer{};
		sProjectionVectorStorageBuffer.buffer = mProjectionVectorBuffer;
		sProjectionVectorStorageBuffer.offset = 0;
		sProjectionVectorStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mPkAPkDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sProjectionVectorStorageBuffer;

		VkDescriptorBufferInfo sSum1024Buffer{};
		sSum1024Buffer.buffer = mSumBuffer;
		sSum1024Buffer.offset = 0;
		sSum1024Buffer.range = 1024 * sizeof(SumData);

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = mPkAPkDescriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &sSum1024Buffer;

		vkUpdateDescriptorSets(mDevice, 3, descriptorWrites.data(), 0, nullptr);
	}
	void CreatePkAPkSumDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mPkAPkSumDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mPkAPkSumDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		VkDescriptorBufferInfo sSum1024Buffer{};
		sSum1024Buffer.buffer = mSumBuffer;
		sSum1024Buffer.offset = 0;
		sSum1024Buffer.range = 1024 * sizeof(SumData);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mPkAPkSumDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sSum1024Buffer;

		VkDescriptorBufferInfo sUniformBuffer{};
		sUniformBuffer.buffer = mProjectionUniformBuffer;
		sUniformBuffer.offset = 0;
		sUniformBuffer.range = sizeof(ProjectionUniform);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mPkAPkSumDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sUniformBuffer;


		vkUpdateDescriptorSets(mDevice, 2, descriptorWrites.data(), 0, nullptr);
	}
	void CreateXkDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mXkDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mXkDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		VkDescriptorBufferInfo sProjectionVectorStorageBuffer{};
		sProjectionVectorStorageBuffer.buffer = mProjectionVectorBuffer;
		sProjectionVectorStorageBuffer.offset = 0;
		sProjectionVectorStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mXkDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sProjectionVectorStorageBuffer;

		VkDescriptorBufferInfo sUniformBuffer{};
		sUniformBuffer.buffer = mProjectionUniformBuffer;
		sUniformBuffer.offset = 0;
		sUniformBuffer.range = sizeof(ProjectionUniform);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mXkDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sUniformBuffer;


		vkUpdateDescriptorSets(mDevice, 2, descriptorWrites.data(), 0, nullptr);
	}
	void CreateRkDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mRkDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mRkDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

		VkDescriptorBufferInfo sProjectionConstStorageBuffer{};
		sProjectionConstStorageBuffer.buffer = mProjectionConstBuffer;
		sProjectionConstStorageBuffer.offset = 0;
		sProjectionConstStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionConst);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mRkDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sProjectionConstStorageBuffer;

		VkDescriptorBufferInfo sProjectionVectorStorageBuffer{};
		sProjectionVectorStorageBuffer.buffer = mProjectionVectorBuffer;
		sProjectionVectorStorageBuffer.offset = 0;
		sProjectionVectorStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mRkDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sProjectionVectorStorageBuffer;

		VkDescriptorBufferInfo sUniformBuffer{};
		sUniformBuffer.buffer = mProjectionUniformBuffer;
		sUniformBuffer.offset = 0;
		sUniformBuffer.range = sizeof(ProjectionUniform);

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = mRkDescriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &sUniformBuffer;

		vkUpdateDescriptorSets(mDevice, 3, descriptorWrites.data(), 0, nullptr);
	}
	void CreatePkDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mPkDescriptorSetLayout;

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mPkDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
		VkDescriptorBufferInfo sProjectionVectorStorageBuffer{};
		sProjectionVectorStorageBuffer.buffer = mProjectionVectorBuffer;
		sProjectionVectorStorageBuffer.offset = 0;
		sProjectionVectorStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mPkDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sProjectionVectorStorageBuffer;

		VkDescriptorBufferInfo sUniformBuffer{};
		sUniformBuffer.buffer = mProjectionUniformBuffer;
		sUniformBuffer.offset = 0;
		sUniformBuffer.range = sizeof(ProjectionUniform);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mPkDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &sUniformBuffer;


		vkUpdateDescriptorSets(mDevice, 2, descriptorWrites.data(), 0, nullptr);
	}
	VkDescriptorSet mProjectionInitDescriptorSet;
	VkDescriptorSet mRkRkDescriptorSet;
	VkDescriptorSet mRkRkSumDescriptorSet;
	VkDescriptorSet mPkAPkDescriptorSet;
	VkDescriptorSet mPkAPkSumDescriptorSet;
	VkDescriptorSet mXkDescriptorSet;
	VkDescriptorSet mRkDescriptorSet;
	VkDescriptorSet mPkDescriptorSet;
	//Final
	void CreateFinalDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mFinalDescriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		mFinalDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		if (vkAllocateDescriptorSets(mDevice, &allocInfo, mFinalDescriptorSets.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			std::array<VkWriteDescriptorSet, 7> descriptorWrites{};

			VkDescriptorBufferInfo storageBufferInfoLastFrame{};
			storageBufferInfoLastFrame.buffer = mStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
			storageBufferInfoLastFrame.offset = 0;
			storageBufferInfoLastFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &storageBufferInfoLastFrame;

			VkDescriptorBufferInfo storageBufferBuoyancy{};
			storageBufferBuoyancy.buffer = mBuoyancyStorageBuffer;
			storageBufferBuoyancy.offset = 0;
			storageBufferBuoyancy.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = &storageBufferBuoyancy;





			VkDescriptorBufferInfo sProjectionConstStorageBuffer{};
			sProjectionConstStorageBuffer.buffer = mProjectionConstBuffer;
			sProjectionConstStorageBuffer.offset = 0;
			sProjectionConstStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionConst);

			descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[2].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[2].dstBinding = 2;
			descriptorWrites[2].dstArrayElement = 0;
			descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[2].descriptorCount = 1;
			descriptorWrites[2].pBufferInfo = &sProjectionConstStorageBuffer;

			VkDescriptorBufferInfo sProjectionVectorStorageBuffer{};
			sProjectionVectorStorageBuffer.buffer = mProjectionVectorBuffer;
			sProjectionVectorStorageBuffer.offset = 0;
			sProjectionVectorStorageBuffer.range = cMaxX * cMaxY * sizeof(ProjectionVector);

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[3].dstBinding = 3;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pBufferInfo = &sProjectionVectorStorageBuffer;





			VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
			storageBufferInfoCurrentFrame.buffer = mStorageBuffers[i];
			storageBufferInfoCurrentFrame.offset = 0;
			storageBufferInfoCurrentFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

			descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[4].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[4].dstBinding = 4;
			descriptorWrites[4].dstArrayElement = 0;
			descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[4].descriptorCount = 1;
			descriptorWrites[4].pBufferInfo = &storageBufferInfoCurrentFrame;

			VkDescriptorBufferInfo uniformBufferInfo{};
			uniformBufferInfo.buffer = mUniformBuffer;
			uniformBufferInfo.offset = 0;
			uniformBufferInfo.range = sizeof(FluidUniform);

			descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[5].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[5].dstBinding = 5;
			descriptorWrites[5].dstArrayElement = 0;
			descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[5].descriptorCount = 1;
			descriptorWrites[5].pBufferInfo = &uniformBufferInfo;

			VkDescriptorBufferInfo DrawBufferInfo{};
			DrawBufferInfo.buffer = mDrawBuffers[i];
			DrawBufferInfo.offset = 0;
			DrawBufferInfo.range = 6 * cMaxX * cMaxY * sizeof(glm::vec4);

			descriptorWrites[6].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[6].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[6].dstBinding = 6;
			descriptorWrites[6].dstArrayElement = 0;
			descriptorWrites[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[6].descriptorCount = 1;
			descriptorWrites[6].pBufferInfo = &DrawBufferInfo;

			vkUpdateDescriptorSets(mDevice, 7, descriptorWrites.data(), 0, nullptr);
		}
	}
	std::vector<VkDescriptorSet>  mFinalDescriptorSets;

	//Step4
	void CreateSyncObjects() {
		mAdvectionFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mBuoyancyFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mProjectionInitFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRkRkFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mPkAPkFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

		mPkAPkSumFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mXkFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRkFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRkRkInFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		mRkRkInSumFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

		mDrawInitFences.resize(MAX_FRAMES_IN_FLIGHT);
		mComputeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		mPerpareFinishedFences.resize(MAX_FRAMES_IN_FLIGHT);


		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mAdvectionFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mBuoyancyFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mProjectionInitFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRkRkFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mPkAPkFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mPkAPkSumFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mXkFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRkFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRkRkInFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mRkRkInSumFinishedSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}

			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mDrawInitFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mComputeInFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mPerpareFinishedFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
		}
	}
	// Semaphore
	std::vector<VkSemaphore> mAdvectionFinishedSemaphores;
	std::vector<VkSemaphore> mBuoyancyFinishedSemaphores;
	std::vector<VkSemaphore> mProjectionInitFinishedSemaphores;
	std::vector<VkSemaphore> mRkRkFinishedSemaphores;
	std::vector<VkSemaphore> mPkAPkFinishedSemaphores;
	std::vector<VkSemaphore> mPkAPkSumFinishedSemaphores;
	std::vector<VkSemaphore> mXkFinishedSemaphores;
	std::vector<VkSemaphore> mRkFinishedSemaphores;
	std::vector<VkSemaphore> mRkRkInFinishedSemaphores;
	std::vector<VkSemaphore> mRkRkInSumFinishedSemaphores;
	std::vector<VkFence> mDrawInitFences;
	std::vector<VkFence> mComputeInFlightFences;
	std::vector<VkFence> mPerpareFinishedFences;

	void Update() {
		size_t bufferSize = sizeof(FluidUniform);
		void* data;
		vkMapMemory(mDevice, mUniformBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &mUFO, (size_t)bufferSize);
		vkUnmapMemory(mDevice, mUniformBufferMemory);
	}
	void CreateAdvectionCommandBuffers() {
		mAdvectionCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mAdvectionCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mAdvectionCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordAdvectionBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAdvectionComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAdvectionPipelineLayout, 0, 1, &mAdvectionDescriptorSets[mCurrentFrame], 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Advection() {
		Update();
		vkResetFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame]);


		vkWaitForFences(mDevice, 1, &mComputeInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(mDevice, 1, &mComputeInFlightFences[mCurrentFrame]);

		vkResetCommandBuffer(mAdvectionCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordAdvectionBuffer(mAdvectionCommandBuffers[mCurrentFrame]);

		VkSemaphore signalSemaphores[] = { mAdvectionFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mAdvectionCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
	}
	void CreateBuoyancyCommandBuffers() {
		mBuoyancyCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mBuoyancyCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mBuoyancyCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordBuoyancyBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mBuoyancyComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mBuoyancyPipelineLayout, 0, 1, &mBuoyancyDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Buoyancy() {
		vkResetCommandBuffer(mBuoyancyCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordBuoyancyBuffer(mBuoyancyCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mAdvectionFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSemaphores[] = { mBuoyancyFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mBuoyancyCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


		//mStorageBuffersMapped[0]
		//mBuoyancyStorageBufferMapped
		/*
		std::cout << std::endl;
		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((SmokeGridCell2D*)mBuoyancyStorageBufferMapped)[i].mConcentration << "   ";
		}
		std::cout << std::endl;

		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT; */
	}
	void CreateProjectionInitCommandBuffers() {
		mProjectionInitCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mProjectionInitCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mProjectionInitCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordProjectionInitBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mProjectionInitComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mProjectionInitPipelineLayout, 0, 1, &mProjectionInitDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void ProjectionInit() {

		vkResetCommandBuffer(mProjectionInitCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordProjectionInitBuffer(mProjectionInitCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mBuoyancyFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };


		VkSemaphore signalSemaphores[] = { mProjectionInitFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mProjectionInitCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


	}

	void CreateRkRkCommandBuffers() {
		mRkRkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mRkRkCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mRkRkCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordRkRkBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkPipelineLayout, 0, 1, &mRkRkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 1024 + 1, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void CreateRkRkSumCommandBuffers() {
		mRkRkSumCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mRkRkSumCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mRkRkSumCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordRkRkSumBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkSumComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkSumPipelineLayout, 0, 1, &mRkRkSumDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, 1, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void RkRk() {
		vkResetCommandBuffer(mRkRkCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordRkRkBuffer(mRkRkCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mProjectionInitFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSemaphores[] = { mRkRkFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mRkRkCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


		vkResetCommandBuffer(mRkRkSumCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordRkRkSumBuffer(mRkRkSumCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSumSemaphores[] = { mRkRkFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitSumStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSumSemaphores[] = { mRkRkFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitSumInfo{};
		submitSumInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitSumInfo.commandBufferCount = 1;
		submitSumInfo.pCommandBuffers = &mRkRkSumCommandBuffers[mCurrentFrame];
		submitSumInfo.waitSemaphoreCount = 1;
		submitSumInfo.pWaitSemaphores = waitSumSemaphores;
		submitSumInfo.pWaitDstStageMask = waitSumStages;
		submitSumInfo.signalSemaphoreCount = 0;
		submitSumInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(mComputeQueue, 1, &submitSumInfo, mPerpareFinishedFences[mCurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
	}

	void CreatePkAPkCommandBuffers() {
		mPkAPkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mPkAPkCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mPkAPkCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordPkAPkBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkPipelineLayout, 0, 1, &mPkAPkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 1024 + 1, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void CreatePkAPkSumCommandBuffers() {
		mPkAPkSumCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mPkAPkSumCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mPkAPkSumCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordPkAPkSumBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkSumComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkSumPipelineLayout, 0, 1, &mPkAPkSumDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, 1, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void PkAPk() {
		vkWaitForFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame]);


		vkResetCommandBuffer(mPkAPkCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordPkAPkBuffer(mPkAPkCommandBuffers[mCurrentFrame]);


		VkSemaphore signalSemaphores[] = { mPkAPkFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mPkAPkCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};



		vkResetCommandBuffer(mPkAPkSumCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordPkAPkSumBuffer(mPkAPkSumCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSumSemaphores[] = { mPkAPkFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitSumStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSumSemaphores[] = { mPkAPkSumFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitSumInfo{};
		submitSumInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitSumInfo.commandBufferCount = 1;
		submitSumInfo.pCommandBuffers = &mPkAPkSumCommandBuffers[mCurrentFrame];
		submitSumInfo.waitSemaphoreCount = 1;
		submitSumInfo.pWaitSemaphores = waitSumSemaphores;
		submitSumInfo.pWaitDstStageMask = waitSumStages;
		submitSumInfo.signalSemaphoreCount = 1;
		submitSumInfo.pSignalSemaphores = signalSumSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitSumInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
	}

	void CreateXkCommandBuffers() {
		mXkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mXkCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mXkCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordXkBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mXkComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mXkPipelineLayout, 0, 1, &mXkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Xk() {
		vkResetCommandBuffer(mXkCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordXkBuffer(mXkCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mPkAPkSumFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSemaphores[] = { mXkFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mXkCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};

	}
	void CreateRkCommandBuffers() {
		mRkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mRkCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mRkCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordRkBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkPipelineLayout, 0, 1, &mRkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Rk() {
		vkResetCommandBuffer(mRkCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordRkBuffer(mRkCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mXkFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSemaphores[] = { mRkFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mRkCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
	}
	void RkRkIn() {
		vkResetCommandBuffer(mRkRkCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordRkRkBuffer(mRkRkCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mRkFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSemaphores[] = { mRkRkInFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mRkRkCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


		vkResetCommandBuffer(mRkRkSumCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordRkRkSumBuffer(mRkRkSumCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSumSemaphores[] = { mRkRkInFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitSumStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSumSemaphores[] = { mRkRkInSumFinishedSemaphores[mCurrentFrame] };

		VkSubmitInfo submitSumInfo{};
		submitSumInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitSumInfo.commandBufferCount = 1;
		submitSumInfo.pCommandBuffers = &mRkRkSumCommandBuffers[mCurrentFrame];
		submitSumInfo.waitSemaphoreCount = 1;
		submitSumInfo.pWaitSemaphores = waitSumSemaphores;
		submitSumInfo.pWaitDstStageMask = waitSumStages;
		submitSumInfo.signalSemaphoreCount = 1;
		submitSumInfo.pSignalSemaphores = signalSumSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitSumInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
	}

	void CreatePkCommandBuffers() {
		mPkCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mPkCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mPkCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordPkBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkPipelineLayout, 0, 1, &mPkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Pk() {
		vkResetCommandBuffer(mPkCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordPkBuffer(mPkCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mRkRkInSumFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };



		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mPkCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, mPerpareFinishedFences[mCurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


		vkWaitForFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame], VK_TRUE, UINT64_MAX);
	}

	void CreateFinalCommandBuffers() {
		mFinalCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mFinalCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mFinalCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordFinalBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mFinalComputePipeline);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mFinalPipelineLayout, 0, 1, &mFinalDescriptorSets[mCurrentFrame], 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Final() {

		vkWaitForFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

		vkResetCommandBuffer(mFinalCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordFinalBuffer(mFinalCommandBuffers[mCurrentFrame]);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mFinalCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, mComputeInFlightFences[mCurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


		vkWaitForFences(mDevice, 1, &mComputeInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);



		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}


	// Command Buffer
	uint32_t mCurrentFrame = 0;
	std::vector<VkCommandBuffer> mAdvectionCommandBuffers;
	std::vector<VkCommandBuffer> mBuoyancyCommandBuffers;
	std::vector<VkCommandBuffer> mProjectionInitCommandBuffers;
	std::vector<VkCommandBuffer> mRkRkCommandBuffers;
	std::vector<VkCommandBuffer> mRkRkSumCommandBuffers;
	std::vector<VkCommandBuffer> mPkAPkCommandBuffers;
	std::vector<VkCommandBuffer> mPkAPkSumCommandBuffers;


	std::vector<VkCommandBuffer> mXkCommandBuffers;
	std::vector<VkCommandBuffer> mRkCommandBuffers;
	std::vector<VkCommandBuffer> mRkRk2CommandBuffers;
	std::vector<VkCommandBuffer> mPkCommandBuffers;


	std::vector<VkCommandBuffer> mFinalCommandBuffers;



	double mLastTime = 0.0f;


	//Step2
	void CreateDescriptorSetLayout(VkDescriptorSetLayout& fDescriptorSetLayout,
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
	void CreatePipelineLayout(VkPipelineLayout& fPipelineLayout, VkDescriptorSetLayout fDescriptorSetLayout) {
		std::vector<VkDescriptorSetLayout> DescriptorSetLayouts{ fDescriptorSetLayout };

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = DescriptorSetLayouts.size();
		pipelineLayoutInfo.pSetLayouts = DescriptorSetLayouts.data();

		if (vkCreatePipelineLayout(mDevice, &pipelineLayoutInfo, nullptr, &fPipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}
	}
	void CreateComputePipeline(VkPipeline& fPipeline, VkPipelineLayout fPipelineLayout, const std::string& fFilename) {
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
	//Step2 Helped Fiction
	std::vector<char> ReadFile(const std::string& fFilename) {
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
	VkShaderModule CreateShaderModule(const std::vector<char>& fCode) {
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
	void CreateBuffer(
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
	}
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
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
};

