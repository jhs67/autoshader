
#version 450

layout(push_constant) uniform Push {
	layout(offset=0) mat4 test0;
	layout(offset=80) mat4 test2;
} push;

void main() {
}
