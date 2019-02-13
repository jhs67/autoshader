
#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec4 worldPos;

layout(set = 0, binding = 0) uniform TheScene {
	mat4 projection;
	mat4 view;
	vec4 camerapos;
	vec4 lightdir;
} scene;

layout(set = 1, binding = 0) uniform sampler2D colorTexture;

layout(location = 0) out vec4 outColor;

vec3 applyNormalMap() {
	vec3 N = normalize(normal);
	return N;
}

void main() {
	vec3 mapnormal = applyNormalMap();
	float d = max(0.0, -dot(mapnormal, scene.lightdir.xyz));
	vec3 v = normalize(scene.camerapos.xyz - worldPos.xyz);
	vec3 r = -normalize(reflect(v, mapnormal));
	float s = pow(max(0.0, dot(v, r)), 5.0) * 0.5f;
	outColor = texture(colorTexture, texCoord) * vec4(d, d, d, 1) + vec4(s, s, s, 0);
}
