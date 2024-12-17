#version 430 core
#extension GL_ARB_bindless_texture: require

layout (location = 0) out vec4 color;
layout (location = 1) out vec4 out_screenSpacePositions;
layout (location = 2) out ivec3 out_normals;
layout (location = 3) out vec3 out_bloom;
layout (location = 4) out float out_materials; //just roughness for now

in vec2 v_uv;
in vec3 v_normals;
in vec3 v_vertexPosition;
in vec3 v_color;

 
uniform mat4 u_view;

uniform float u_exposure;

ivec3 fromFloatTouShort(vec3 a)
{
	//[-1 1] -> [0 2]
	a += 1.f;

	//[0 2] -> [0 1]
	a /= 2.f;

	//[0 1] -> [0 65536]
	a *= 65536;

	return ivec3(a);
}

flat in int test;

readonly restrict layout(std430) buffer u_entityTextureSamplerers
{
	uvec2 textureSamplerers[];
};

flat in uvec2 v_textureSampler;
//flat in float v_ambient;


void main()
{

	color.rgba = texture2D(sampler2D(v_textureSampler), v_uv).rgba;
	//color.rgba = texture2D(sampler2D(textureSamplerers[1]), v_uv).rgba;
	//color.rgba = vec4(v_uv,0,1);

	if(color.a < 0.5)discard;

	color.rgb = pow(color.rgb, vec3(2.2)) * v_color;
	//color.rgb *= v_ambient;
	

	out_screenSpacePositions.xyzw = vec4((u_view * vec4(v_vertexPosition,1)).xyz,1);
	out_normals = fromFloatTouShort(v_normals);

	out_bloom = vec3(0,0,0);
	out_materials.r = 0.9;

	//color.rgb = vec3(1.f/float(test));
}