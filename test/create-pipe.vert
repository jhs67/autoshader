
#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 bones;
layout(location = 4) in vec3 weights;

layout(set = 0, binding = 0) uniform TheScene {
	mat4 projection;
	mat4 view;
	vec4 camerapos;
	vec4 lightdir;
} scene;

layout(set = 1, binding = 1) uniform Skeleton {
	mat4 binding[64];
} skeleton;

layout(set = 2, binding = 0) uniform Pose {
	mat4 transform[64];
} pose;

layout(location = 0) out vec2 outTexCoord;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec4 outWorldPos;

out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
	mat4 bx = pose.transform[int(bones.x * 255)] * skeleton.binding[int(bones.x * 255)] * weights.x;
	mat4 by = pose.transform[int(bones.y * 255)] * skeleton.binding[int(bones.y * 255)] * weights.y;
	mat4 bz = pose.transform[int(bones.z * 255)] * skeleton.binding[int(bones.z * 255)] * weights.z;
	mat4 b = bx + by + bz;
	outTexCoord = texCoord;
	outNormal = normalize(mat3(b) * normal);
	outWorldPos = b * position;
	gl_Position = scene.projection * scene.view * outWorldPos;
}
