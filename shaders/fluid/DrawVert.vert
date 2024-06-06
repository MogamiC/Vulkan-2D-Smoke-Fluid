#version 450
layout(location = 0) in vec4 inPosition;

layout(location = 0) out vec4 fragColor;

mat4 PosiM=mat4(1.0/400,0,0,0,
0,-1.0/300,0,0,
0,0,1,0,
-1,1,0,1);

void main() {
    gl_Position = PosiM * vec4(inPosition.xy, 0.0, 1.0);
    fragColor = vec4(1.0, 0.0, 0.0, inPosition.z * 0.75);
}