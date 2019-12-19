#include "constants.h"

// out data
struct Triangle {
	mat3 pos;
	mat3x2 uv0;
	mat3x2 uv1;
	mat3x2 uv2;
	mat3x2 uv3;
	vec3 normal;
	mat3x4 color0;
	mat3x4 color1;
	mat3x4 color2;
	mat3x4 color3;
	uint tex0;
	uint tex1;
};
struct HitPoint {
	vec3 pos;
	vec3 normal;
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;
	vec4 color0;
	vec4 color1;
	vec4 color2;
	vec4 color3;
	uint tex0;
	uint tex1;
};

struct TextureData {
	int tex0;
	int tex1;
	bool tex0Blend;
	bool tex1Blend;
	bool tex0Color;
	bool tex1Color;
};

// buffer with instance data
layout(binding = BINDING_OFFSET_INSTANCE_DATA, set = 0) buffer Instance { ASInstanceData data[]; } iData;
// Buffer with indices and vertices
layout(binding = BINDING_OFFSET_IDX_STATIC, set = 0) buffer Indices_static { uint i[]; } indices_static;
layout(binding = BINDING_OFFSET_XYZ_STATIC, set = 0) buffer Vertices_static { VertexBuffer v[]; } vertices_static;
layout(binding = BINDING_OFFSET_IDX_DYNAMIC, set = 0) buffer Indices_dynamic { uint i[]; } indices_dynamic;
layout(binding = BINDING_OFFSET_XYZ_DYNAMIC, set = 0) buffer Vertices_dynamic { VertexBuffer v[]; } vertices_dynamic;

layout(binding = BINDING_OFFSET_IDX_WORLD_STATIC, set = 0) buffer Indices_World_static { uint i[]; } indices_world_static;
layout(binding = BINDING_OFFSET_XYZ_WORLD_STATIC, set = 0) buffer Vertices_World_static { VertexBuffer v[]; } vertices_world_static;
layout(binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_DATA, set = 0) buffer Indices_dynamic_data { uint i[]; } indices_dynamic_data;
layout(binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_DATA, set = 0) buffer Vertices_dynamic_data { VertexBuffer v[]; } vertices_dynamic_data;
layout(binding = BINDING_OFFSET_IDX_WORLD_DYNAMIC_AS, set = 0) buffer Indices_dynamic_as { uint i[]; } indices_dynamic_as;
layout(binding = BINDING_OFFSET_XYZ_WORLD_DYNAMIC_AS, set = 0) buffer Vertices_dynamic_as { VertexBuffer v[]; } vertices_dynamic_as;

vec3 getBarycentricCoordinates(vec2 hitAttribute) { return vec3(1.0f - hitAttribute.x - hitAttribute.y, hitAttribute.x, hitAttribute.y); }

vec4 unpackColor(uint color) {
	return vec4(
		color & 0xff,
		(color & (0xff << 8)) >> 8,
		(color & (0xff << 16)) >> 16,
		(color & (0xff << 24)) >> 24
	);
}

Triangle getTriangle(RayPayload rp){
// if(iData.data[rp.instanceID].world) {
// 		ivec3 index = ivec3(indices_static.i[iData.data[rp.instanceID].offsetIDX + 3 * rp.primitiveID], indices_static.i[iData.data[rp.instanceID].offsetIDX + 3 * rp.primitiveID + 1], indices_static.i[iData.data[rp.instanceID].offsetIDX + 3 * rp.primitiveID + 2]);

// 		const vec3 barycentricCoords = vec3(1.0f - rp.barycentric.x - rp.barycentric.y, rp.barycentric.x, rp.barycentric.y);
// 		vec2 uv = 		vertices_static.v[index.x + iData.data[rp.instanceID].offsetXYZ].uv.xy * barycentricCoords.x +
//                   	vertices_static.v[index.y + iData.data[rp.instanceID].offsetXYZ].uv.xy * barycentricCoords.y +
//                   	vertices_static.v[index.z + iData.data[rp.instanceID].offsetXYZ].uv.xy * barycentricCoords.z;
// 		color = global_textureGrad(int(vertices_static.v[index.x + iData.data[rp.instanceID].offsetXYZ].pos.w), uv, tex_coord_x, tex_coord_y);
// 	}

  	uint customIndex = uint(iData.data[rp.instanceID].offsetIDX) + (rp.primitiveID * 3);
	ivec3 index;
	if(!iData.data[rp.instanceID].dynamic) index = (ivec3(indices_static.i[customIndex], indices_static.i[customIndex + 1], indices_static.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
	else index = (ivec3(indices_dynamic.i[customIndex], indices_dynamic.i[customIndex + 1], indices_dynamic.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
	//if(!iData.data[rp.instanceID].world) index + int(iData.data[rp.instanceID].offsetXYZ);

	if(iData.data[rp.instanceID].world == BAS_WORLD_STATIC) index = (ivec3(indices_world_static.i[customIndex], indices_world_static.i[customIndex + 1], indices_world_static.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
	else if(iData.data[rp.instanceID].world == BAS_WORLD_DYNAMIC_DATA)  index = (ivec3(indices_dynamic_data.i[customIndex], indices_dynamic_data.i[customIndex + 1], indices_dynamic_data.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
	else if(iData.data[rp.instanceID].world == BAS_WORLD_DYNAMIC_AS)  index = (ivec3(indices_dynamic_as.i[customIndex], indices_dynamic_as.i[customIndex + 1], indices_dynamic_as.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);

	VertexBuffer vData[3];
	if(!iData.data[rp.instanceID].dynamic) {
		vData[0] = vertices_static.v[index.x];
		vData[1] = vertices_static.v[index.y];
		vData[2] = vertices_static.v[index.z];
	}
	else {
		vData[0] = vertices_dynamic.v[index.x];
		vData[1] = vertices_dynamic.v[index.y];
		vData[2] = vertices_dynamic.v[index.z];
	}
	if(iData.data[rp.instanceID].world == BAS_WORLD_STATIC){
		vData[0] = vertices_world_static.v[index.x];
		vData[1] = vertices_world_static.v[index.y];
		vData[2] = vertices_world_static.v[index.z];
	}
	else if(iData.data[rp.instanceID].world == BAS_WORLD_DYNAMIC_DATA){
		vData[0] = vertices_dynamic_data.v[index.x];
		vData[1] = vertices_dynamic_data.v[index.y];
		vData[2] = vertices_dynamic_data.v[index.z];
	}else if(iData.data[rp.instanceID].world == BAS_WORLD_DYNAMIC_AS){
		vData[0] = vertices_dynamic_as.v[index.x];
		vData[1] = vertices_dynamic_as.v[index.y];
		vData[2] = vertices_dynamic_as.v[index.z];
	}

	Triangle hitTriangle;
	// POS
	// hitTriangle.pos[0] = vData[0].pos.xyz;
	// hitTriangle.pos[1] = vData[1].pos.xyz;
	// hitTriangle.pos[2] = vData[2].pos.xyz;
	// Some Entitiys are in Object Space
	hitTriangle.pos[0] = (rp.modelmat * vec4(vData[0].pos.xyz, 1)).xyz;
	hitTriangle.pos[1] = (rp.modelmat * vec4(vData[1].pos.xyz, 1)).xyz;
	hitTriangle.pos[2] = (rp.modelmat * vec4(vData[2].pos.xyz, 1)).xyz;
	// Color
	hitTriangle.color0[0] = unpackColor(vData[0].color0);
	hitTriangle.color0[1] = unpackColor(vData[1].color0);
	hitTriangle.color0[2] = unpackColor(vData[2].color0);

	hitTriangle.color1[0] = unpackColor(vData[0].color1);
	hitTriangle.color1[1] = unpackColor(vData[1].color1);
	hitTriangle.color1[2] = unpackColor(vData[2].color1);

	hitTriangle.color2[0] = unpackColor(vData[0].color2);
	hitTriangle.color2[1] = unpackColor(vData[1].color2);
	hitTriangle.color2[2] = unpackColor(vData[2].color2);
	
	hitTriangle.color3[0] = unpackColor(vData[0].color3);
	hitTriangle.color3[1] = unpackColor(vData[1].color3);
	hitTriangle.color3[2] = unpackColor(vData[2].color3);

	// UV
	hitTriangle.uv0[0] = vData[0].uv0.xy;
	hitTriangle.uv0[1] = vData[1].uv0.xy;
	hitTriangle.uv0[2] = vData[2].uv0.xy;

	hitTriangle.uv1[0] = vData[0].uv1.xy;
	hitTriangle.uv1[1] = vData[1].uv1.xy;
	hitTriangle.uv1[2] = vData[2].uv1.xy;

	hitTriangle.uv2[0] = vData[0].uv2.xy;
	hitTriangle.uv2[1] = vData[1].uv2.xy;
	hitTriangle.uv2[2] = vData[2].uv2.xy;

	hitTriangle.uv3[0] = vData[0].uv3.xy;
	hitTriangle.uv3[1] = vData[1].uv3.xy;
	hitTriangle.uv3[2] = vData[2].uv3.xy;
	// NORMAL
	//vec3 AB = vData[1].pos.xyz - vData[0].pos.xyz;
	//vec3 AC = vData[2].pos.xyz - vData[0].pos.xyz;
	vec3 AB = hitTriangle.pos[1] - hitTriangle.pos[0];
	vec3 AC = hitTriangle.pos[2] - hitTriangle.pos[0];
	hitTriangle.normal = normalize(cross(AB, AC));

	switch(iData.data[rp.instanceID].world){
		case BAS_WORLD_STATIC:
			hitTriangle.tex0 = (vertices_world_static.v[index.x + iData.data[rp.instanceID].offsetXYZ].texIdx0);
			hitTriangle.tex1 = (vertices_world_static.v[index.x + iData.data[rp.instanceID].offsetXYZ].texIdx1);
			break;
		case BAS_WORLD_DYNAMIC_DATA:
			hitTriangle.tex0 = (vertices_dynamic_data.v[index.x + iData.data[rp.instanceID].offsetXYZ].texIdx0);
			hitTriangle.tex1 = (vertices_dynamic_data.v[index.x + iData.data[rp.instanceID].offsetXYZ].texIdx1);
			break;
		case BAS_WORLD_DYNAMIC_AS:
			hitTriangle.tex0 = (vertices_dynamic_as.v[index.x + iData.data[rp.instanceID].offsetXYZ].texIdx0);
			hitTriangle.tex1 = (vertices_dynamic_as.v[index.x + iData.data[rp.instanceID].offsetXYZ].texIdx1);
			break;
		default:
			hitTriangle.tex0 = TEX1_IDX_MASK | (iData.data[rp.instanceID].texIdx & TEX0_IDX_MASK);
			hitTriangle.tex1 = UINT_MAX;
	}
	if(iData.data[rp.instanceID].dynamic) {
		hitTriangle.tex0 = vertices_dynamic.v[index.x].texIdx0;
		hitTriangle.tex1 = vertices_dynamic.v[index.x].texIdx1;
	}
	

	return hitTriangle;
}

HitPoint getHitPoint(RayPayload rp){
	const vec3 barycentricCoords = getBarycentricCoordinates(rp.barycentric);
	Triangle triangle = getTriangle(rp);

	HitPoint hitPoint;
	hitPoint.pos = triangle.pos[0] * barycentricCoords.x +
					triangle.pos[1] * barycentricCoords.y +
            		triangle.pos[2] * barycentricCoords.z;
	hitPoint.color0 = triangle.color0[0] * barycentricCoords.x +
  	          		triangle.color0[1] * barycentricCoords.y +
  	           		triangle.color0[2] * barycentricCoords.z;
	hitPoint.color1 = triangle.color1[0] * barycentricCoords.x +
  	          		triangle.color1[1] * barycentricCoords.y +
  	           		triangle.color1[2] * barycentricCoords.z;
	hitPoint.color2 = triangle.color2[0] * barycentricCoords.x +
  	          		triangle.color2[1] * barycentricCoords.y +
  	           		triangle.color2[2] * barycentricCoords.z;
	hitPoint.color3 = triangle.color3[0] * barycentricCoords.x +
  	          		triangle.color3[1] * barycentricCoords.y +
  	           		triangle.color3[2] * barycentricCoords.z;
	hitPoint.uv0 = triangle.uv0[0] * barycentricCoords.x +
            		triangle.uv0[1] * barycentricCoords.y +
            		triangle.uv0[2] * barycentricCoords.z;
	hitPoint.uv1 = triangle.uv1[0] * barycentricCoords.x +
            		triangle.uv1[1] * barycentricCoords.y +
            		triangle.uv1[2] * barycentricCoords.z;
	hitPoint.uv2 = triangle.uv2[0] * barycentricCoords.x +
            		triangle.uv2[1] * barycentricCoords.y +
            		triangle.uv2[2] * barycentricCoords.z;
	hitPoint.uv3 = triangle.uv3[0] * barycentricCoords.x +
            		triangle.uv3[1] * barycentricCoords.y +
            		triangle.uv3[2] * barycentricCoords.z;
	hitPoint.normal = triangle.normal;

	hitPoint.tex0 = triangle.tex0;
	hitPoint.tex1 = triangle.tex1;

	return hitPoint;
}

TextureData unpackTextureData(uint data){
	TextureData d;
	d.tex0 = int(data & TEX0_IDX_MASK);
	d.tex1 = int((data & TEX1_IDX_MASK) >> TEX_SHIFT_BITS);
	if(d.tex0 == TEX0_IDX_MASK) d.tex0 = -1;
	if(d.tex1 == TEX0_IDX_MASK) d.tex1 = -1;
	d.tex0Blend = (data & TEX0_BLEND_MASK) != 0;
	d.tex1Blend = (data & TEX1_BLEND_MASK) != 0;
	d.tex0Color = (data & TEX0_COLOR_MASK) != 0;
	d.tex1Color = (data & TEX1_COLOR_MASK) != 0;
	return d;
}

bool
found_intersection(RayPayload rp)
{
	return rp.instanceID != ~0u;
}
bool
is_light(RayPayload rp)
{
	if(iData.data[rp.instanceID].world == BAS_WORLD_STATIC) {
		uint customIndex = uint(iData.data[rp.instanceID].offsetIDX) + (rp.primitiveID * 3);
		ivec3 index = (ivec3(indices_world_static.i[customIndex], indices_world_static.i[customIndex + 1], indices_world_static.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
		return (vertices_world_static.v[index.x].material & MATERIAL_FLAG_LIGHT) == MATERIAL_FLAG_LIGHT;
	}
	return (iData.data[rp.instanceID].material & MATERIAL_FLAG_LIGHT) == MATERIAL_FLAG_LIGHT;
}
bool
is_mirror(RayPayload rp)
{
	if(iData.data[rp.instanceID].world == BAS_WORLD_STATIC) {
		uint customIndex = uint(iData.data[rp.instanceID].offsetIDX) + (rp.primitiveID * 3);
		ivec3 index = (ivec3(indices_world_static.i[customIndex], indices_world_static.i[customIndex + 1], indices_world_static.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
		return (vertices_world_static.v[index.x].material & MATERIAL_FLAG_MIRROR) == MATERIAL_FLAG_MIRROR;
	}
	else return (iData.data[rp.instanceID].material & MATERIAL_FLAG_MIRROR) == MATERIAL_FLAG_MIRROR;
}
bool
is_glass(RayPayload rp)
{
	return (iData.data[rp.instanceID].material & MATERIAL_KIND_MASK) == MATERIAL_KIND_GLASS;
}

bool
is_player(RayPayload rp) // for shadows etc, so the third person model does not cast shadows on the first person weapon
{
	if(iData.data[rp.instanceID].world == BAS_WORLD_STATIC) return false;
	return (iData.data[rp.instanceID].material & MATERIAL_FLAG_MASK) == MATERIAL_FLAG_PLAYER_OR_WEAPON;
}

bool
needs_color(RayPayload rp)
{
	uint customIndex = uint(iData.data[rp.instanceID].offsetIDX) + (rp.primitiveID * 3);
	ivec3 index;
	switch(iData.data[rp.instanceID].world){
		case BAS_WORLD_STATIC:
			index = (ivec3(indices_world_static.i[customIndex], indices_world_static.i[customIndex + 1], indices_world_static.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
			return (vertices_world_static.v[index.x].material & MATERIAL_FLAG_NEEDSCOLOR) == MATERIAL_FLAG_NEEDSCOLOR;
		case BAS_WORLD_DYNAMIC_DATA:
			index = (ivec3(indices_dynamic_data.i[customIndex], indices_dynamic_data.i[customIndex + 1], indices_dynamic_data.i[customIndex + 2])) + int(iData.data[rp.instanceID].offsetXYZ);
			return (vertices_dynamic_data.v[index.x].material & MATERIAL_FLAG_NEEDSCOLOR) == MATERIAL_FLAG_NEEDSCOLOR;
		default:
			return (iData.data[rp.instanceID].material & MATERIAL_FLAG_NEEDSCOLOR) == MATERIAL_FLAG_NEEDSCOLOR;
	}
}