これは二次元の煙シミュレーションのプログラム。ヤコビ法前処理共役勾配法(PCG)を使用しています。このプログラムには、初期化と煙の生成はCPUで実行する、計算とレンダリングはGPUで実行する。

このプログラムを順調に実行するのために、以下の三つのことは必要です。
Vulkan SDK  https://vulkan.lunarg.com/
GLFW https://www.glfw.org/
GLM https://github.com/g-truc/glm/releases

ソースコードをみたいならソースコード、“SomkeSimulationVulkan.vcxproj”をチェックしてください。

IDEはVisual Studio 2022
以上の三つのことのインストールとVisual Studioのセットアップは以下のウェブサイトの内容を参考してください
https://vulkan-tutorial.com/Development_environment

GPUはNVIDIA GeForce GTX 1060 6GB、Cellの数は512*400、PCGのIteration Timesは1フレームで10回
こんな状況で1フレームは6ミリセカンドから8ミリセカンドまでの時間をかかる、最大のフレームは12ミリセカンドです。

煙の生成のコントロールについては、“main1.cpp”の中の二つの関数で行いています。“VulkanFluid::InputInit()”と“VulkanFluid::InputUpdate()”です。最大四つの長方形を生成し出来ます。
物理相関の数値は“DefNConst.h”にあります。

参考したものは：
Vulkan Tuorial https://vulkan-tutorial.com/
《Fluid Simulation for Computer Graphics Second_Edition》 Robert Bridson


