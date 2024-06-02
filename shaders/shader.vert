#version 450

layout(set = 1, binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
} ubo;

layout(set = 0, binding = 0) uniform Position{
    vec3 Pos;
} posi;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;

mat4 PosiM=mat4(1,0,0,0,
0,1,0,0,
0,0,1,0,
posi.Pos.x,posi.Pos.y,posi.Pos.z,1);

void main() {
	gl_Position =  ubo.proj * ubo.view * PosiM * vec4(inPosition, 1.0);
	fragColor = inColor;
}