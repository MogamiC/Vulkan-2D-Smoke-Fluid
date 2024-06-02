#pragma once


#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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

const uint32_t cMaxX = 800;
const uint32_t cMaxY = 600;

const float cDensityAir = 1.28;
const float cDensitySoot = 2;
const float cAlpha = (cDensitySoot - cDensityAir) / cDensityAir;

const float cTemperatureAmbient = 273.0;
const float cBelta = 1 / cTemperatureAmbient;

const float cDeltaX = 1.0;


struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsAndComputeFamily;

	bool IsComplete() {
		return graphicsAndComputeFamily.has_value();
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
};


class VulkanTest {
#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

public:
	VulkanTest() {
		CreateInstance({ "VK_LAYER_KHRONOS_validation" });
		PickPhysicalDevice({  });
		CreateLogicalDevice({ "VK_LAYER_KHRONOS_validation" }, {  });
		CreateCommandPool();


		// Layout&Pipeline
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
		CreateShaderStorageBuffers();
		CreateUniformBuffer();


		CreateProjectionConstBuffer();
		CreateProjectionVectorBuffer();
		CreateProjectionUniformBuffer();
		CreateSumBuffer();



		//// Descriptor Set
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


			vkDestroyFence(mDevice, mRkRk[i], nullptr);
			vkDestroyFence(mDevice, mPkAPk[i], nullptr);
			vkDestroyFence(mDevice, mXk[i], nullptr);
			vkDestroyFence(mDevice, mRk[i], nullptr);
			vkDestroyFence(mDevice, mRkRkIn[i], nullptr);
			vkDestroyFence(mDevice, mPkAPk1[i], nullptr);
			vkDestroyFence(mDevice, mRkRkIn1[i], nullptr);




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

		vkDestroyCommandPool(mDevice, mCommandPool, nullptr);
		vkDestroyDevice(mDevice, nullptr);
		vkDestroyInstance(mInstance, nullptr);
	}

	void Run() {
		
		uint32_t i = 0;
		std::vector<long long> v1;
		v1.reserve(10000);

		auto LastTime = std::chrono::system_clock::now();
		auto LastTime_ms = std::chrono::time_point_cast<std::chrono::microseconds>(LastTime);
		auto LastTime_value = LastTime_ms.time_since_epoch().count();

		auto time = std::chrono::system_clock::now();;
		auto time_ms = std::chrono::time_point_cast<std::chrono::microseconds>(time);
		auto time_value = time_ms.time_since_epoch().count(); 

		for(i = 0; i < 40; ++i){
			LastTime = std::chrono::system_clock::now();
			LastTime_ms = std::chrono::time_point_cast<std::chrono::microseconds>(LastTime);
			LastTime_value = LastTime_ms.time_since_epoch().count(); 
		
			Advection();
			Buoyancy();
			ProjectionInit();
			RkRk();
			for (int i = 0; i < 20; ++i) {
				PkAPk();
				Xk();
				Rk();
				RkRkIn();
				Pk();
			}
			Final();

			
			time = std::chrono::system_clock::now();;
			time_ms = std::chrono::time_point_cast<std::chrono::microseconds>(time);
			time_value = time_ms.time_since_epoch().count();
			time_value = time_value - LastTime_value;
			v1.push_back(time_value); 
		}
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
		long long avg = sum / 20;
		std::cout << sum << "   " << largest << "   " << smallest << "   " << avg << std::endl;
	}


private:
	// Descriptor Set
	void CreateDescriptorPool() {
		std::array<VkDescriptorPoolSize, 2> poolSizes{};

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[0].descriptorCount = 20 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);


		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = 10 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = 7 * static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

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
		sSum1024Buffer.range = 1024 * sizeof(float);

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
		sSum1024Buffer.range = 1024 * sizeof(float);

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
		sSum1024Buffer.range = 1024 * sizeof(float);

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
		sSum1024Buffer.range = 1024 * sizeof(float);

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
			std::array<VkWriteDescriptorSet, 6> descriptorWrites{};

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

			vkUpdateDescriptorSets(mDevice, 6, descriptorWrites.data(), 0, nullptr);
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

		mComputeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		mPerpareFinishedFences.resize(MAX_FRAMES_IN_FLIGHT);

		mRkRk.resize(MAX_FRAMES_IN_FLIGHT);
		mPkAPk.resize(MAX_FRAMES_IN_FLIGHT);
		mXk.resize(MAX_FRAMES_IN_FLIGHT);
		mRk.resize(MAX_FRAMES_IN_FLIGHT);
		mRkRkIn.resize(MAX_FRAMES_IN_FLIGHT);
		mPkAPk1.resize(MAX_FRAMES_IN_FLIGHT);
		mRkRkIn1.resize(MAX_FRAMES_IN_FLIGHT);


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
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mComputeInFlightFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mPerpareFinishedFences[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}


			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mPkAPk[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mRkRk[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mXk[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mRk[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mRkRkIn[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mPkAPk1[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}
			if (vkCreateFence(mDevice, &fenceInfo, nullptr, &mRkRkIn1[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}


		}
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

		vkResetFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame]);


		vkResetFences(mDevice, 1, &mXk[mCurrentFrame]);
		vkResetFences(mDevice, 1, &mRk[mCurrentFrame]);



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


	void cout() {
		/*
		std::cout << std::endl;
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDirectionBelta << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mRkRk << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDistanceAlpha << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mPkAPk << "   ";

		std::cout << std::endl;
		std::cout << "    X ";
		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mPositionX << "   ";
		}*/

		/*
		std::cout << std::endl;
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDirectionBelta << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mRkRk << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDistanceAlpha << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mPkAPk << "   ";

		std::cout << std::endl;
		std::cout << "    X ";
		for (uint32_t i = 600 * 800 - 1; i > 600 * 800 - 11; --i) {
			std::cout << ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mPositionX << "   ";
		}
		std::cout << std::endl;
		std::cout << "    P ";
		for (uint32_t i = 600 * 800 - 1; i > 600 * 800 - 11; --i) {
			std::cout << ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mDirectionP << "   ";
		}
		std::cout << std::endl;
		std::cout << "    R ";
		for (uint32_t i = 600 * 800 - 1; i > 600 * 800 - 11; --i) {
			std::cout << ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mResidualR << "   ";
		}


		uint32_t i = 600 * 800 - 1;

		float CenterA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mCenterCoefficient;
		float LeftA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mLeftCoefficient;
		float RightA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mRightCoefficient;
		float DownA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mDownCoefficient;
		float UpA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mUpCoefficient;

		std::cout << std::endl;
		std::cout << "    A ";
		std::cout << CenterA << "   ";
		std::cout << LeftA << "   ";
		std::cout << RightA << "   ";
		std::cout << DownA << "   ";
		std::cout << UpA << "   ";*/




		/*
		uint32_t i = 300;

		float CenterA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mCenterCoefficient;
		float RightA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mRightCoefficient;
		float LeftA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mLeftCoefficient;
		float UpA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mUpCoefficient;
		float DownA = ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mDownCoefficient;

		std::cout << std::endl;
		std::cout << CenterA << "   ";
		std::cout << RightA << "   ";
		std::cout << LeftA << "   ";
		std::cout << UpA << "   ";
		std::cout << DownA << "   ";
		std::cout << std::endl;

		float CenterR = ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mDirectionP;
		float RightR = ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i-1].mDirectionP;
		float LeftR = ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i+1].mDirectionP;
		float UpR = ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mDirectionP;
		float DownR = ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i+800].mDirectionP;

		std::cout << CenterR << "   ";
		std::cout << RightR << "   ";
		std::cout << LeftR << "   ";
		std::cout << UpR << "   ";
		std::cout << DownR << "   ";
		std::cout << std::endl;

		std::cout << (CenterA * CenterR + RightA * RightR + LeftA * LeftR + UpA * UpR + DownA * DownR) * CenterR << "   "; */
		/*

		std::cout << std::endl;
		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mResidualR << "   ";
		}
		std::cout << std::endl;
		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((ProjectionVector*)mProjectionConstStorageBufferMapped)[i].mDirectionP << "   ";
		}
		std::cout << std::endl;
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDistanceAlpha << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mPkAPk << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDirectionBelta << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mRkRk << "   ";
		std::cout << std::endl;

		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mSourceB << "   ";
		}
		std::cout << std::endl;
		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((ProjectionVector*)mProjectionVectorStorageBufferMapped)[i].mResidualR << "   ";
		}
		std::cout << std::endl;
		std::cout << std::endl;


		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mRkRk << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDistanceAlpha << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mPkAPk << "   ";
		std::cout << std::endl;

		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mDirectionBelta << "   ";
		std::cout << ((ProjectionUniform*)mProjectionUniformBufferMapped)[0].mRkRk << "   ";
		std::cout << std::endl;

		std::cout << ((SmokeGridCell2D*)mBuoyancyStorageBufferMapped)[600*800-1].mDensity << "   ";
		std::cout << std::endl;
		for (uint32_t i = 295; i < 305; ++i) {
			std::cout << ((ProjectionConst*)mProjectionConstStorageBufferMapped)[i].mCenterCoefficient << "   ";
		}
		std::cout << std::endl;
		for (uint32_t i = 466; i < 476; ++i) {
			std::cout << ((float*)mSumStorageBufferMapped)[i]<< "   ";
		}
		std::cout << std::endl;*/
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
		
		/*
		int a = 6700;
		for (uint32_t i = a; i < a + 10; ++i) {
			std::cout << ((SmokeGridCell2D*)mStorageBuffersMapped[0])[i].mConcentration << "   ";
		}
		std::cout << std::endl;
		for (uint32_t i = a; i < a + 10; ++i) {
			std::cout << ((SmokeGridCell2D*)mStorageBuffersMapped[1])[i].mConcentration << "   ";
		}
		std::cout << std::endl;
		std::cout << std::endl;*/
		

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

	std::vector<VkFence> mComputeInFlightFences; 
	std::vector<VkFence> mPerpareFinishedFences;

	std::vector<VkFence> mRkRk;
	std::vector<VkFence> mPkAPk;
	std::vector<VkFence> mXk;
	std::vector<VkFence> mRk;
	std::vector<VkFence> mRkRkIn;
	std::vector<VkFence> mPkAPk1;
	std::vector<VkFence> mRkRkIn1;

	double mLastTime = 0.0f;

	VkInstance mInstance;
	VkPhysicalDevice mPhysicalDevice;
	VkPhysicalDeviceProperties mProperties;
	VkPhysicalDeviceFeatures mDeviceFeatures;
	VkDevice mDevice;
	VkQueue mComputeQueue;
	VkCommandPool mCommandPool;


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
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
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
		sConstGrid.mVelocityV = 0.01;
		sConstGrid.mConcentration = 2;
		sConstGrid.mTemperature = 300;

		for (uint32_t i = 0; i <= 200; ++i) {
			sArrayGridCell2D[i + 1900] = sConstGrid;
		}


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

		std::vector<float> sSumArray(count, 0);
		size_t sSumBufferSize = count * sizeof(float);

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










	//Step1 Device
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

		vkGetDeviceQueue(mDevice, indices.graphicsAndComputeFamily.value(), 0, &mComputeQueue);
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


		return indices.IsComplete() && extensionsSupported;
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

			if (indices.IsComplete()) {
				break;
			}

			i++;
		}

		return indices;
	}

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

