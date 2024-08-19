#include "VulkanFluid.h"
void VulkanFluid::InputInit() {
	mInputData[0].mCenterX = cMaxX;
	mInputData[0].mCenterY = 40;
	mInputData[0].mConcentration = 1;
	mInputData[0].mLengthHalfX = 5;
	mInputData[0].mLengthHalfY = 5;
	mInputData[0].mTemperature = 372;
	mInputData[0].mVelocityX = 0.1;
	mInputData[0].mVelocityY = 0.5;

	mInputData[1].mCenterX = 0;
	mInputData[1].mCenterY = cMaxY - 40;
	mInputData[1].mConcentration = 1;
	mInputData[1].mLengthHalfX = 5;
	mInputData[1].mLengthHalfY = 5;
	mInputData[1].mTemperature = 372;
	mInputData[1].mVelocityX = -0.1;
	mInputData[1].mVelocityY = -0.5;

	mVelocityU[0] = 0.2;
	mVelocityU[1] = 0.2;
}
void VulkanFluid::InputUpdate() {
	if (mInputData[1].mCenterX > (cMaxX - 40)) {
		mVelocityU[0] = 0.2;
		mInputData[0].mVelocityX = -0.1;
		mVelocityU[1] = -1 * 0.2;
		mInputData[1].mVelocityX = 0.1;
	}
	if (mInputData[1].mCenterX < 040) {
		mVelocityU[0] = -0.2;
		mInputData[0].mVelocityX = 0.1;
		mVelocityU[1] = 0.2;
		mInputData[1].mVelocityX = -0.1;
	}


	float a = (mInputData[1].mCenterX + (mVelocityU[1] * mUFO.mDeltaTime));
	mInputData[1].mCenterX = a;


	float b = (mInputData[0].mCenterX + (mVelocityU[0] * mUFO.mDeltaTime));
	mInputData[0].mCenterX = b;
}


int main() {
	FreeConsole();

	VulkanFluid test;
	test.Run();
}