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

const int MAX_FRAMES_IN_FLIGHT = 2;
const uint32_t Count = 1024;

const uint32_t cMaxX = 512;
const uint32_t cMaxY = 400;

const float cDensityAir = 1.28;
const float cDensitySoot = 1.5;
const float cAlpha = (cDensitySoot - cDensityAir) / cDensityAir;

const float cTemperatureAmbient = 272.0;
const float cBelta = 1 / cTemperatureAmbient;

const float cDeltaX = 1.0;

const float cDefaultVelocityU = 0;
const float cDefaultVelocityV = 0;
const float cDefaultConcentration = 0;

const float cConstVelocityU = 0.1;
const float cConstVelocityV = 0.1;
const float cConstConcentration = 1;
const float cConstTemperature = 300;

const float cConatRectangleMinX = cMaxX / 2 - 5;
const float cConatRectangleMaxX = cMaxX / 2 + 5;
const float cConatRectangleMinY = 0;
const float cConatRectangleMaxY = 20;


const float cResidual = 0.00001;
const uint32_t cIterationTimes = 10;


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
	float mVelocityU = cDefaultVelocityU;
	float mVelocityV = cDefaultVelocityV;

	// Following data is sampled at the center of the cell
	float mPressure = 0;
	float mDensity = 0;
	float mConcentration = cDefaultConcentration;
	float mTemperature = cTemperatureAmbient + 1;

	// Type of the Cell
	// 0 means Fluid, 1 means Solid, 2 means Const Cell whose data will not change in the frame 
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

struct InputData {
	float mCenterX = 0;
	float mCenterY = 0;

	uint32_t mLengthHalfX = 0;
	uint32_t mLengthHalfY = 0;

	float mVelocityX = 0;
	float mVelocityY = 0;
	float mConcentration = cDefaultConcentration;
	float mTemperature = cConstTemperature;
};