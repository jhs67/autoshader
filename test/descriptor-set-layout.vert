
#version 450

layout(std140, set = 0, binding = 0) uniform Uniform00 {
	mat4 view;
	mat4 proj;
};

layout(set = 1, binding = 0, r16) uniform readonly image2D heights;

layout(set = 1, binding = 1, std430) buffer OutputBuffer {
	vec4 normal[];
};

void main() {
}
