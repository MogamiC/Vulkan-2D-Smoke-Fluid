#pragma once

#include "VulkanApp.h"
class VulkanComputePipelineNLayout {
public:
	VulkanComputePipelineNLayout(VulkanApp* fApp, std::vector<VkDescriptorType> fDescriptorType, VkShaderStageFlagBits fShaderStage, const std::string& fFilename) :mApp(fApp) {
		CreateProjectionInitLayoutNPipeline(fDescriptorType, fShaderStage, fFilename);
	}
	~VulkanComputePipelineNLayout() {
		vkDestroyPipeline(mApp->mDevice, mComputePipeline, nullptr);
		vkDestroyPipelineLayout(mApp->mDevice, mPipelineLayout, nullptr);
		vkDestroyDescriptorSetLayout(mApp->mDevice, mDescriptorSetLayout, nullptr);
	}

	VkDescriptorSetLayout& GetDescriptorSetLayout(){ return mDescriptorSetLayout; }
	VkPipelineLayout& GetPipelineLayout() { return mPipelineLayout; };
	VkPipeline& GetComputePipeline() { return mComputePipeline; };
	VulkanApp* GetApp() { return mApp; }

 private:
	void CreateProjectionInitLayoutNPipeline(
		std::vector<VkDescriptorType> fDescriptorType, 
		VkShaderStageFlagBits fShaderStage, 
		const std::string& fFilename) {
		mApp->CreateDescriptorSetLayout(
			mDescriptorSetLayout,
			fDescriptorType,
			fShaderStage);
		mApp->CreatePipelineLayout(mPipelineLayout, mDescriptorSetLayout);
		mApp->CreateComputePipeline(mComputePipeline, mPipelineLayout, fFilename);
	}


	VulkanApp* mApp;

	VkDescriptorSetLayout mDescriptorSetLayout;
	VkPipelineLayout mPipelineLayout;
	VkPipeline mComputePipeline;
};

class VulkanBuffer {
public:
	VulkanBuffer(VulkanApp* fApp, VkDeviceSize fBufferSize, VkBufferUsageFlagBits fUsageFlag, VkMemoryPropertyFlagBits fPropertyFlag) :
		mApp(fApp), mBufferSize(fBufferSize),mUsageFlag(fUsageFlag),mPropertyFlag(fPropertyFlag){
		CreatBuffer(fUsageFlag, fPropertyFlag);
	}
	~VulkanBuffer() {
		vkDestroyBuffer(mApp->mDevice, mBuffer, nullptr);
		vkFreeMemory(mApp->mDevice, mBufferMemory, nullptr);
	}

	void Map() {
		vkMapMemory(mApp->mDevice, mBufferMemory, 0, mBufferSize, 0, &mDataMapped);
	}
	void Unmap() {
		vkUnmapMemory(mApp->mDevice, mBufferMemory);
	}
	void Copy(void* fCopyData) {
		memcpy(mDataMapped, &fCopyData, (size_t)mBufferSize);
	}

	VulkanApp* GetApp() { return mApp; }
	VkBuffer* GetBuffer() { return &mBuffer; }

private:
	void CreatBuffer(VkBufferUsageFlagBits fUsageFlag, VkMemoryPropertyFlagBits fPropertyFlag) {
		mApp->CreateBuffer(
			mBuffer, mBufferMemory, mBufferSize,
			fUsageFlag,
			fPropertyFlag);
		vkBindBufferMemory(mApp->mDevice, mBuffer, mBufferMemory, 0);
	}

	VulkanApp* mApp;
	VkDeviceSize mBufferSize;

	VkBuffer mBuffer;
	VkDeviceMemory mBufferMemory;
	VkBufferUsageFlagBits mUsageFlag;
	VkMemoryPropertyFlagBits mPropertyFlag;
	void* mDataMapped;
};