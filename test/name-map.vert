
#version 450


struct Foo {
	float a, b;
};

struct Faa {
	vec3 a;
	vec3 b;
	mat2 c;
	float d;
};

layout(std140) uniform Uniform1 {
	Foo foo;
	Faa faa;
};

layout(std430) buffer Buffer1 {
	Faa blah;
};

void main() {
}
