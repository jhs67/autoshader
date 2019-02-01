
#version 450

struct Foo {
	vec4 c, d;
};

layout(std140) uniform Uniform2 {
	Foo foo;
};

void main() {
}
