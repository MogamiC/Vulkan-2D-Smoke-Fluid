#pragma once

#include "VulkanMember.h"

class VulkanFluid : VulkanApp {
public:
	VulkanFluid() {
		// Buffer
		CreateDescriptorPool();
		CreateDrawBuffers();
		CreateShaderStorageBuffers();
		CreateUniformBuffer(); 
		CreateInputDataBuffer();
		CreateInputDataStorageBuffer();

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

		CreateInputDataDescriptorSet();

		// CommandBuffer
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

		CreateInputDataCommandBuffers();

		CreateSyncObjects();

		CreateDrawSyncObjects();
		CreateDrawCommandBuffer();
		CreateDrawPipeline();

		InputInit();
	}
	~VulkanFluid() {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroySemaphore(mDevice, mInputDataSemaphores[i], nullptr);
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
		vkDestroyBuffer(mDevice, mInputStorageBuffer, nullptr);
		vkFreeMemory(mDevice, mInputStorageBufferMemory, nullptr);
		vkDestroyBuffer(mDevice, mInputDataBuffer, nullptr);
		vkFreeMemory(mDevice, mInputDataBufferMemory, nullptr);
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
	}

	void Run() override {
		DrawInit();

		auto LastTime = std::chrono::system_clock::now();
		auto LastTime_ms = std::chrono::time_point_cast<std::chrono::microseconds>(LastTime);
		auto LastTime_value = LastTime_ms.time_since_epoch().count();

		auto time = std::chrono::system_clock::now();;
		auto time_ms = std::chrono::time_point_cast<std::chrono::microseconds>(time);
		auto time_value = time_ms.time_since_epoch().count();

		auto RunTimes = 0;


		while (!glfwWindowShouldClose(mWindowPtr)) {
			glfwPollEvents();


			Update();
			Draw();

			LastTime = std::chrono::system_clock::now();
			LastTime_ms = std::chrono::time_point_cast<std::chrono::microseconds>(LastTime);
			LastTime_value = LastTime_ms.time_since_epoch().count();

			Advection();
			Buoyancy();
			InputDataF();
			ProjectionInit();
			RkRk();
			for (uint32_t i = 0; i<= cIterationTimes; ++i) {

				PkAPk();
				Xk();
				Rk();
				RkRkIn();
				Pk();
				float a = ((ProjectionUniform*)mProjectionUniformBufferMapped)->mRkBiggest;
				if (a < cResidual) {
					break;
				}
				if (a < cResidual) {
					break;
				}

			}
			Final();

			time = std::chrono::system_clock::now();;
			time_ms = std::chrono::time_point_cast<std::chrono::microseconds>(time);
			time_value = time_ms.time_since_epoch().count();
			time_value = time_value - LastTime_value;


			mUFO.mDeltaTime = time_value / 1000;
			std::cout << mUFO.mDeltaTime << std::endl;
			if (mUFO.mDeltaTime > 33)
				mUFO.mDeltaTime = 33;
		}
	}


protected:
	void InputInit();
	void InputUpdate();

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
	void Update() override {
		InputUpdate();

		size_t bufferSize = sizeof(FluidUniform);
		void* data;
		vkMapMemory(mDevice, mUniformBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, &mUFO, (size_t)bufferSize);
		vkUnmapMemory(mDevice, mUniformBufferMemory);


		size_t bufferSize2 = 4 * sizeof(InputData);
		void* data2;
		vkMapMemory(mDevice, mInputDataBufferMemory, 0, bufferSize2, 0, &data2);
		memcpy(data2, &mInputData, (size_t)bufferSize2);
		vkUnmapMemory(mDevice, mInputDataBufferMemory);
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

	//Buffer
	void CreateShaderStorageBuffers() {
		SmokeGridCell2D sDefaultGrid;
		std::vector<SmokeGridCell2D> sArrayGridCell2D(cMaxX * cMaxY, sDefaultGrid);
		size_t bufferSize = cMaxX * cMaxY * sizeof(SmokeGridCell2D);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, sArrayGridCell2D.data(), bufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		mStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		mStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(mStorageBuffers[i], mStorageBuffersMemory[i], bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			CopyBuffer(stagingBuffer, mStorageBuffers[i], bufferSize);
		}

		CreateBuffer(mAdvectionStorageBuffer, mAdvectionStorageBufferMemory, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		CopyBuffer(stagingBuffer, mAdvectionStorageBuffer, bufferSize);

		CreateBuffer(mBuoyancyStorageBuffer, mBuoyancyStorageBufferMemory, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		CopyBuffer(stagingBuffer, mBuoyancyStorageBuffer, bufferSize);

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

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, sConstBufferSize, 0, &data);
		memcpy(data, sProjectionConstArray.data(), sConstBufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		CreateBuffer(mProjectionConstBuffer, mProjectionConstBufferMemory, sConstBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		CopyBuffer(stagingBuffer, mProjectionConstBuffer, sConstBufferSize);

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

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, sVectorBufferSize, 0, &data);
		memcpy(data, sProjectionVectorArray.data(), sVectorBufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		CreateBuffer(mProjectionVectorBuffer, mProjectionVectorBufferMemory, sVectorBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		CopyBuffer(stagingBuffer, mProjectionVectorBuffer, sVectorBufferSize);

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

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, sSumBufferSize, 0, &data);
		memcpy(data, sSumArray.data(), sSumBufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		CreateBuffer(mSumBuffer, mSumBufferMemory, sSumBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		CopyBuffer(stagingBuffer, mSumBuffer, sSumBufferSize);

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);
	}
	void CreateDrawBuffers() {
		std::vector<glm::vec4> sArray(6 * cMaxX * cMaxY, { 0,0,0,0 });

		size_t bufferSize = 6 * cMaxX * cMaxY * sizeof(glm::vec4);

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, sArray.data(), bufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);

		mDrawBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		mDrawBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			CreateBuffer(mDrawBuffers[i], mDrawBuffersMemory[i], bufferSize,
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
			CopyBuffer(stagingBuffer, mDrawBuffers[i], bufferSize);
		}

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

	}




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
	void CreateDrawInitComputeDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mDrawInitPipelineNLayout.GetDescriptorSetLayout());
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

	void CreateAdvectionComputeDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mAdvectionPipelineNLayout.GetDescriptorSetLayout());
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
		allocInfo.pSetLayouts = &mBuoyancyPipelineNLayout.GetDescriptorSetLayout();

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
	// Conjugate Gradient Descriptor Set
	void CreateProjectionInitDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mProjectionInitPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mRkRkPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mRkRkSumPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mPkAPkPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mPkAPkSumPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mXkPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mRkPipelineNLayout.GetDescriptorSetLayout();

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
		allocInfo.pSetLayouts = &mPkPipelineNLayout.GetDescriptorSetLayout();

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
	void CreateFinalDescriptorSets() {
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mFinalPipelineNLayout.GetDescriptorSetLayout());
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
			std::array<VkWriteDescriptorSet, 8> descriptorWrites{};

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


			VkDescriptorBufferInfo InputBufferBuoyancy{};
			InputBufferBuoyancy.buffer = mInputStorageBuffer;
			InputBufferBuoyancy.offset = 0;
			InputBufferBuoyancy.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

			descriptorWrites[7].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[7].dstSet = mFinalDescriptorSets[i];
			descriptorWrites[7].dstBinding = 7;
			descriptorWrites[7].dstArrayElement = 0;
			descriptorWrites[7].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			descriptorWrites[7].descriptorCount = 1;
			descriptorWrites[7].pBufferInfo = &InputBufferBuoyancy;

			vkUpdateDescriptorSets(mDevice, 8, descriptorWrites.data(), 0, nullptr);
		}
	}


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

		mInputDataSemaphores.resize(MAX_FRAMES_IN_FLIGHT);


		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {

			if (vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mInputDataSemaphores[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create compute synchronization objects for a frame!");
			}

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mDrawInitPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mDrawInitPipelineNLayout.GetPipelineLayout(), 0, 1, &mDrawInitDescriptorSets[mCurrentFrame], 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAdvectionPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAdvectionPipelineNLayout.GetPipelineLayout(), 0, 1, &mAdvectionDescriptorSets[mCurrentFrame], 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Advection() {
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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mBuoyancyPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mBuoyancyPipelineNLayout.GetPipelineLayout(), 0, 1, &mBuoyancyDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mProjectionInitPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mProjectionInitPipelineNLayout.GetPipelineLayout(), 0, 1, &mProjectionInitDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkPipelineNLayout.GetPipelineLayout(), 0, 1, &mRkRkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 1024, 1, 1);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkSumPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkRkSumPipelineNLayout.GetPipelineLayout(), 0, 1, &mRkRkSumDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkPipelineNLayout.GetPipelineLayout(), 0, 1, &mPkAPkDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 1024, 1, 1);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkSumPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkAPkSumPipelineNLayout.GetPipelineLayout(), 0, 1, &mPkAPkSumDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mXkPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mXkPipelineNLayout.GetPipelineLayout(), 0, 1, &mXkDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mRkPipelineNLayout.GetPipelineLayout(), 0, 1, &mRkDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mPkPipelineNLayout.GetPipelineLayout(), 0, 1, &mPkDescriptorSet, 0, nullptr);

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

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mFinalPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mFinalPipelineNLayout.GetPipelineLayout(), 0, 1, &mFinalDescriptorSets[mCurrentFrame], 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void Final() {

		vkWaitForFences(mDevice, 1, &mPerpareFinishedFences[mCurrentFrame], VK_TRUE, UINT64_MAX);

		vkResetCommandBuffer(mFinalCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordFinalBuffer(mFinalCommandBuffers[mCurrentFrame]);


		VkSemaphore waitSemaphores[] = { mInputDataSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mFinalCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.signalSemaphoreCount = 0;
		submitInfo.pSignalSemaphores = nullptr;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, mComputeInFlightFences[mCurrentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};


		vkWaitForFences(mDevice, 1, &mComputeInFlightFences[mCurrentFrame], VK_TRUE, UINT64_MAX);



		mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
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

	VkDescriptorSetLayout mDrawDescriptorSetLayout;
	VkPipelineLayout mDrawPipelineLayout;
	VkPipeline mDrawPipeline;


	// Layout & Pipeline
	VulkanComputePipelineNLayout mDrawInitPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/DrawInit.spv"
	};
	VulkanComputePipelineNLayout mAdvectionPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/Advection.spv"
	};
	VulkanComputePipelineNLayout mBuoyancyPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/Buoyancy.spv"
	};
	// Conjugate Gradient Layout & Pipeline
	VulkanComputePipelineNLayout mProjectionInitPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,  VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/ProjectionInit.spv"
	};
	VulkanComputePipelineNLayout mRkRkPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/RkRk.spv"
	};
	VulkanComputePipelineNLayout mPkAPkPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		 "shaders/fluid/PkAPk.spv"
	};
	VulkanComputePipelineNLayout mRkRkSumPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/RkRkSum.spv"
	};
	VulkanComputePipelineNLayout mPkAPkSumPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/PkAPkSum.spv"
	};
	VulkanComputePipelineNLayout mXkPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/Xk.spv"
	};
	VulkanComputePipelineNLayout mRkPipelineNLayout{
		this,
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT,
			"shaders/fluid/Rk.spv"
	};
	VulkanComputePipelineNLayout mPkPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER },
			VK_SHADER_STAGE_COMPUTE_BIT,
			"shaders/fluid/Pk.spv"
	};
	VulkanComputePipelineNLayout mFinalPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER} ,
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/Final.spv"
	};

	std::vector<VkBuffer> mDrawBuffers;
	std::vector<VkDeviceMemory> mDrawBuffersMemory;

	VkBuffer mUniformBuffer;
	VkDeviceMemory mUniformBufferMemory;
	std::vector<VkBuffer> mStorageBuffers;
	std::vector<VkDeviceMemory> mStorageBuffersMemory;
	VkBuffer mAdvectionStorageBuffer;
	VkDeviceMemory mAdvectionStorageBufferMemory;
	VkBuffer mBuoyancyStorageBuffer;
	VkDeviceMemory mBuoyancyStorageBufferMemory;
	// Conjugate Gradient Buffer
	VkBuffer mProjectionConstBuffer;
	VkDeviceMemory mProjectionConstBufferMemory;
	VkBuffer mProjectionVectorBuffer;
	VkDeviceMemory mProjectionVectorBufferMemory;
	VkBuffer mProjectionUniformBuffer;
	VkDeviceMemory mProjectionUniformBufferMemory;
	void* mProjectionUniformBufferMapped;
	VkBuffer mSumBuffer;
	VkDeviceMemory mSumBufferMemory;

	InputData mInputData[4];
	void CreateInputDataBuffer() {
		uint32_t count = 4;

		size_t sInputDataBufferSize = count * sizeof(InputData);

		CreateBuffer(mInputDataBuffer, mInputDataBufferMemory, sInputDataBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	}
	VkBuffer mInputDataBuffer;
	VkDeviceMemory mInputDataBufferMemory;
	void* mInputDataBufferMapped;
	void CreateInputDataStorageBuffer() {
		SmokeGridCell2D sDefaultGrid;
		std::vector<SmokeGridCell2D> sArrayGridCell2D(cMaxX * cMaxY, sDefaultGrid);

		size_t bufferSize = cMaxX * cMaxY * sizeof(SmokeGridCell2D);
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		CreateBuffer(
			stagingBuffer, stagingBufferMemory, bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		void* data;
		vkMapMemory(mDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, sArrayGridCell2D.data(), bufferSize);
		vkUnmapMemory(mDevice, stagingBufferMemory);


		CreateBuffer(mInputStorageBuffer, mInputStorageBufferMemory, bufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
		CopyBuffer(stagingBuffer, mInputStorageBuffer, bufferSize);

		vkDestroyBuffer(mDevice, stagingBuffer, nullptr);
		vkFreeMemory(mDevice, stagingBufferMemory, nullptr);

	}
	VkBuffer mInputStorageBuffer;
	VkDeviceMemory mInputStorageBufferMemory;
	VulkanComputePipelineNLayout mInputDataPipelineNLayout{
		this,
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER} ,
		VK_SHADER_STAGE_COMPUTE_BIT,
		"shaders/fluid/InputData.spv"
	};
	void CreateInputDataDescriptorSet() {
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = mDescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &mInputDataPipelineNLayout.GetDescriptorSetLayout();

		if (vkAllocateDescriptorSets(mDevice, &allocInfo, &mInputDataDescriptorSet) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate descriptor sets!");
		}


		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		VkDescriptorBufferInfo sInputDataBuffer{};
		sInputDataBuffer.buffer = mInputDataBuffer;
		sInputDataBuffer.offset = 0;
		sInputDataBuffer.range = 4 * sizeof(InputData);

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = mInputDataDescriptorSet;
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &sInputDataBuffer;

		VkDescriptorBufferInfo uniformBufferInfo{};
		uniformBufferInfo.buffer = mUniformBuffer;
		uniformBufferInfo.offset = 0;
		uniformBufferInfo.range = sizeof(FluidUniform);

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = mInputDataDescriptorSet;
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = &uniformBufferInfo;


		VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
		storageBufferInfoCurrentFrame.buffer = mInputStorageBuffer;
		storageBufferInfoCurrentFrame.offset = 0;
		storageBufferInfoCurrentFrame.range = cMaxX * cMaxY * sizeof(SmokeGridCell2D);

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = mInputDataDescriptorSet;
		descriptorWrites[2].dstBinding = 2;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;


		vkUpdateDescriptorSets(mDevice, 3, descriptorWrites.data(), 0, nullptr);
	}
	VkDescriptorSet mInputDataDescriptorSet;
	void CreateInputDataCommandBuffers() {
		mInputDataCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = mCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)mInputDataCommandBuffers.size();

		if (vkAllocateCommandBuffers(mDevice, &allocInfo, mInputDataCommandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate compute command buffers!");
		}
	}
	void RecordInputDataBuffer(VkCommandBuffer commandBuffer) {
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

		if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
			throw std::runtime_error("failed to begin recording compute command buffer!");
		}

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mInputDataPipelineNLayout.GetComputePipeline());

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mInputDataPipelineNLayout.GetPipelineLayout(), 0, 1, &mInputDataDescriptorSet, 0, nullptr);

		vkCmdDispatch(commandBuffer, cMaxX * cMaxY / 64, 1, 1);

		if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to record compute command buffer!");
		}
	}
	void InputDataF() {
		vkResetCommandBuffer(mInputDataCommandBuffers[mCurrentFrame], /*VkCommandBufferResetFlagBits*/ 0);
		RecordInputDataBuffer(mInputDataCommandBuffers[mCurrentFrame]);

		VkSemaphore waitSemaphores[] = { mAdvectionFinishedSemaphores[mCurrentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		VkSemaphore signalSemaphores[] = { mInputDataSemaphores[mCurrentFrame] };

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &mInputDataCommandBuffers[mCurrentFrame];
		submitInfo.waitSemaphoreCount = 0;
		submitInfo.pWaitSemaphores = nullptr;
		submitInfo.pWaitDstStageMask = nullptr;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(mComputeQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit compute command buffer!");
		};
	}
	std::vector<VkSemaphore> mInputDataSemaphores;
	std::vector<VkCommandBuffer> mInputDataCommandBuffers;



	VkDescriptorPool mDescriptorPool;
	std::vector<VkDescriptorSet> mDrawInitDescriptorSets;
	std::vector<VkCommandBuffer> mDrawInitCommandBuffers;


	std::vector<VkDescriptorSet> mAdvectionDescriptorSets;
	VkDescriptorSet mBuoyancyDescriptorSet;
	VkDescriptorSet mProjectionInitDescriptorSet;
	VkDescriptorSet mRkRkDescriptorSet;
	VkDescriptorSet mRkRkSumDescriptorSet;
	VkDescriptorSet mPkAPkDescriptorSet;
	VkDescriptorSet mPkAPkSumDescriptorSet;
	VkDescriptorSet mXkDescriptorSet;
	VkDescriptorSet mRkDescriptorSet;
	VkDescriptorSet mPkDescriptorSet;
	std::vector<VkDescriptorSet>  mFinalDescriptorSets;

	std::vector<VkCommandBuffer> mDrawCommandBuffers;
	bool IsBufferBeigin = false;

	std::vector<VkSemaphore> mImageAvailableSemaphores;
	std::vector<VkSemaphore> mRenderFinishedSemaphores;
	std::vector<VkFence> mDrawFences;

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


	FluidUniform mUFO;

	double mLastTime = 0.0f;

	float mVelocityU[4];
	float mVelocityV[4];
};

