#include "tr_local.h"

/*
glConfig.driverType == VULKAN && r_vertexLight->value == 2
*/

#define RTX_IS_DYNAMIC tess.shader->stages[0]->bundle[0].numImageAnimations > 0 || \
						tess.shader->stages[0] != NULL ? ((tess.shader->stages[0]->bundle[0].tcGen != TCGEN_BAD) && tess.shader->stages[0]->bundle[0].numTexMods > 0) : qfalse || \
						tess.shader->numDeforms > 0 || \
						backEnd.currentEntity->e.frame != backEnd.currentEntity->e.oldframe;

void RB_CreateBottomAS(vkbottomAS_t** bAS, qboolean dynamic);
void RB_CreateNewBottomAS(surfaceType_t* surface, shader_t* shader, vkbottomAS_t** bAS) {
	tess.numVertexes = 0;
	tess.numIndexes = 0;
	tess.shader = shader;
	rb_surfaceTable[*surface](surface);
	RB_CreateBottomAS(bAS, qfalse);
	tess.numVertexes = 0;
	tess.numIndexes = 0;
}

void RB_CreateBottomAS(vkbottomAS_t** bAS, qboolean dynamic) {
	int j;
	//if (*surface == SF_SKIP) return;
	//if (*surface == SF_BAD) return;
	//if (*surface == SF_FACE) RB_SurfaceFace((srfSurfaceFace_t*)surface);
	//else if (*surface == SF_GRID) RB_SurfaceGrid((srfGridMesh_t*)surface);
	//else if (*surface == SF_TRIANGLES) RB_SurfaceTriangles((srfTriangles_t*)surface);
	//else if (*surface == SF_POLY) RB_SurfacePolychain((srfPoly_t*)surface);
	//else if (*surface == SF_MD3) RB_SurfaceMesh((md3Surface_t*)surface);
	///*else if (*surface == SF_MD4) {
	//	RB_SurfaceAnim((md4Surface_t*)surface);
	//}*/
	////else if (type == SF_FLARE) RB_SurfaceFlare((srfFlare_t*)s_worldData.surfaces[i].data);
	//else return;
	
	for (int stage = 0; stage < 1/*MAX_SHADER_STAGES*/; stage++)
	{
		shaderStage_t* pStage = tess.shader->stages[stage];
		if (!pStage || pStage->bundle[0].isLightmap || !pStage->active) continue;
		ComputeColors(pStage);
		ComputeTexCoords(pStage);

		// set offsets and 
		uint32_t* idxOffset;
		uint32_t* xyzOffset;
		// save bas in static or dynamic list
		vkbottomAS_t* bASList;
		if (dynamic) {
			idxOffset = &vk_d.geometry.idxDynamicOffset;
			xyzOffset = &vk_d.geometry.xyzDynamicOffset;
			bASList = &vk_d.bottomASDynamicList[vk.swapchain.currentImage][vk_d.bottomASDynamicCount[vk.swapchain.currentImage]];
		}
		else {
			idxOffset = &vk_d.geometry.idxStaticOffset;
			xyzOffset = &vk_d.geometry.xyzStaticOffset;
			bASList = &vk_d.bottomASList[vk_d.bottomASCount];
		}

		// define as geometry
		bASList->geometries.sType = VK_STRUCTURE_TYPE_GEOMETRY_NV;
		bASList->geometries.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		bASList->geometries.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		bASList->geometries.geometry.triangles.vertexData = vk_d.geometry.xyz.buffer;
		bASList->geometries.geometry.triangles.vertexOffset = (*xyzOffset) * sizeof(float[12]);
		bASList->geometries.geometry.triangles.vertexCount = tess.numVertexes;
		bASList->geometries.geometry.triangles.vertexStride = 12 * sizeof(float);
		bASList->geometries.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		bASList->geometries.geometry.triangles.indexData = vk_d.geometry.idx.buffer;
		bASList->geometries.geometry.triangles.indexOffset = (*idxOffset) * sizeof(uint32_t);
		bASList->geometries.geometry.triangles.indexCount = tess.numIndexes;
		bASList->geometries.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		bASList->geometries.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
		bASList->geometries.flags = VK_GEOMETRY_OPAQUE_BIT_NV;

		// write idx
		for (j = 0; j < tess.numIndexes; j++) {
			uint32_t idx = (uint32_t)tess.indexes[j];
			VK_UploadBufferDataOffset(&vk_d.geometry.idx, (*idxOffset) * sizeof(uint32_t) + (j * sizeof(uint32_t)), sizeof(uint32_t), (void*)&idx);
		}

		// write xyz and other vertex attribs
		qboolean tcGen = (tess.shader->stages[0]->bundle[0].tcGen != TCGEN_BAD || tess.shader->stages[0]->bundle[0].numTexMods > 0);
		for (j = 0; j < tess.numVertexes; j++) {
			float p[12] = {
				p[0] = tess.xyz[j][0],
				p[1] = tess.xyz[j][1],
				p[2] = tess.xyz[j][2],
				p[3] = 0,
				p[4] = tcGen ? tess.svars.texcoords[0][j][0] : tess.texCoords[j][0][0],
				p[5] = tcGen ? tess.svars.texcoords[0][j][1] : tess.texCoords[j][0][1],
				p[6] = tess.texCoords[j][1][0],
				p[7] = tess.texCoords[j][1][1],
				p[8] = (float)tess.svars.colors[j][0],
				p[9] = (float)tess.svars.colors[j][1],
				p[10] = (float)tess.svars.colors[j][2],
				p[11] = 0
			};
			VK_UploadBufferDataOffset(&vk_d.geometry.xyz, (*xyzOffset) * 12 * sizeof(float) + (j * 12 * sizeof(float)), 12 * sizeof(float), (void*)&p);
		}

		// set offsets for in-shader lookup
		bASList->data.offsetIdx = (*idxOffset);
		bASList->data.offsetXYZ = (*xyzOffset);

		qboolean deform = /*surface == SF_MD3 ||*/ tess.shader->numDeforms > 0;
		if (!dynamic) {
			VkCommandBuffer commandBuffer = {0};
			VK_BeginSingleTimeCommands(&commandBuffer);
			if (deform) VK_CreateBottomAS(commandBuffer, bASList, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV, &vk_d.basBufferStaticOffset);
			else VK_CreateBottomAS(commandBuffer, bASList, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV, &vk_d.basBufferStaticOffset);
			vk_d.bottomASCount++;
			VK_EndSingleTimeCommands(&commandBuffer);
		}
		else {
			VkCommandBuffer commandBuffer = vk.swapchain.CurrentCommandBuffer();
			VK_CreateBottomAS(commandBuffer, bASList, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV, &vk_d.basBufferDynamicOffset);
			vk_d.bottomASDynamicCount[vk.swapchain.currentImage]++;
		}

		if (bAS != NULL)  (*bAS) = bASList;

		(*idxOffset) += tess.shader->numDeforms > 0 ? 3 * tess.numIndexes : 1 * tess.numIndexes;
		(*xyzOffset) += tess.shader->numDeforms > 0 ? 3 * tess.numVertexes : 1 * tess.numVertexes;
	}
}

void RB_UpdateInstanceBuffer(vkbottomAS_t* bAS) {
	bAS->geometryInstance.instanceCustomIndex = 0;
	// set visibility for first and third person (eye and mirror)
	if ((backEnd.currentEntity->e.renderfx & RF_THIRD_PERSON)) bAS->geometryInstance.mask = RTX_MIRROR_VISIBLE;
	else if ((backEnd.currentEntity->e.renderfx & RF_FIRST_PERSON)) bAS->geometryInstance.mask = RTX_FIRST_PERSON_VISIBLE;
	else bAS->geometryInstance.mask = RTX_FIRST_PERSON_MIRROR_VISIBLE;

	bAS->geometryInstance.instanceOffset = 0;

	switch (tess.shader->cullType) {
	case CT_FRONT_SIDED:
		bAS->geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_FRONT_COUNTERCLOCKWISE_BIT_NV; break;
	case CT_BACK_SIDED:
		bAS->geometryInstance.flags = 0; break;
	case CT_TWO_SIDED:
		bAS->geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV; break;
	}
	bAS->geometryInstance.accelerationStructureHandle = bAS->handle;

	VK_UploadBufferDataOffset(&vk_d.instanceBuffer, vk_d.instanceBufferOffset + vk_d.bottomASTraceListCount * sizeof(VkGeometryInstanceNV), sizeof(VkGeometryInstanceNV), (void*)&bAS->geometryInstance);
}

void RB_UpdateInstanceDataBuffer(vkbottomAS_t* bAS) {
	// set texture id and calc texture animation
	int indexAnim = 0;
	if (tess.shader->stages[0]->bundle[0].numImageAnimations > 1) {
		indexAnim = (int)(tess.shaderTime * tess.shader->stages[0]->bundle[0].imageAnimationSpeed * FUNCTABLE_SIZE);
		indexAnim >>= FUNCTABLE_SIZE2;
		if (indexAnim < 0) {
			indexAnim = 0;	// may happen with shader time offsets
		}
		indexAnim %= tess.shader->stages[0]->bundle[0].numImageAnimations;	
	}
	if (bAS->data.texIdx != (float)tess.shader->stages[0]->bundle[0].image[indexAnim]->index) {
		bAS->data.texIdx = (float)tess.shader->stages[0]->bundle[0].image[indexAnim]->index;
		tess.shader->stages[0]->bundle[0].image[indexAnim]->frameUsed = tr.frameCount;
	}
	bAS->data.blendfunc = (uint32_t)(tess.shader->stages[0]->stateBits);
	// set if surface is a mirror
	bAS->data.isMirror = tess.shader->sort == SS_PORTAL && strstr(tess.shader->name, "mirror") != NULL;

	VK_UploadBufferDataOffset(&vk_d.instanceDataBuffer[vk.swapchain.currentImage], vk_d.bottomASTraceListCount * sizeof(ASInstanceData), sizeof(ASInstanceData), (void*)&bAS->data);
}

void RB_AddBottomAS(vkbottomAS_t* bAS, qboolean dynamic, qboolean forceUpdate) {
	shaderCommands_t* input  = &tess;
	vkbottomAS_t* currentAS;

	if (input->numIndexes == 0) {
		return;
	}
	if (input->indexes[SHADER_MAX_INDEXES - 1] != 0) {
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_INDEXES hit");
	}
	if (input->xyz[SHADER_MAX_VERTEXES - 1][0] != 0) {
		ri.Error(ERR_DROP, "RB_EndSurface() - SHADER_MAX_VERTEXES hit");
	}
	if (tess.shader == tr.shadowShader) return;
	
	// for debugging of sort order issues, stop rendering after a given sort value
	if (r_debugSort->integer && r_debugSort->integer < tess.shader->sort) return;
	if (tess.shader->stages[0] == NULL) return;

	// just stuff
	if (!strcmp(tess.shader->name, "textures/common/mirror2")) {
		int aaaaa = 2;
	}
	if (input->shader->sort == SS_PORTAL) {
		int x = 2l;
	}
	if (input->shader->sort == SS_STENCIL_SHADOW) {
		int x = 2l;
	}
	
	qboolean anim = tess.shader->stages[0]->bundle[0].numImageAnimations > 0;
	qboolean cTex = tess.shader->stages[0] != NULL ? ((tess.shader->stages[0]->bundle[0].tcGen != TCGEN_BAD) && tess.shader->stages[0]->bundle[0].numTexMods > 0) : qfalse;
	qboolean deform = tess.shader->numDeforms > 0 /*|| (*drawSurf->surface == SF_MD3 && ((md3Surface_t*)drawSurf->surface)->numFrames > 1)*/;
	qboolean frames = backEnd.currentEntity->e.frame > 0 || backEnd.currentEntity->e.oldframe > 0;//backEnd.currentEntity->e.frame != backEnd.currentEntity->e.oldframe;//(*drawSurf->surface == SF_MD3 && ((md3Surface_t*)drawSurf->surface)->numFrames > 1);
	qboolean sky = qfalse;//tess.shader->isSky;
	if (!dynamic) {
		currentAS = bAS;
	}
	else {
		if (deform || frames || forceUpdate) {
			currentAS = &vk_d.bottomASDynamicList[vk.swapchain.currentImage][vk_d.bottomASDynamicCount[vk.swapchain.currentImage]];
			currentAS->geometries = bAS->geometries;
			currentAS->data = bAS->data;
			currentAS->geometryInstance = bAS->geometryInstance;

			currentAS->geometries.geometry.triangles.indexOffset = vk_d.geometry.idxDynamicOffset * sizeof(uint32_t);
			currentAS->geometries.geometry.triangles.vertexOffset = vk_d.geometry.xyzDynamicOffset * sizeof(float[12]);
			currentAS->data.offsetIdx = vk_d.geometry.idxDynamicOffset;
			currentAS->data.offsetXYZ = vk_d.geometry.xyzDynamicOffset;
		}
		else currentAS = bAS;
	}

	if (deform) RB_DeformTessGeometry();
	if (cTex) ComputeTexCoords(tess.shader->stages[0]);
	ComputeColors(tess.shader->stages[0]);

	
	if (dynamic || frames || deform || cTex) {
		for (int j = 0; j < tess.numIndexes; j++) {
			uint32_t idx = (uint32_t)tess.indexes[j];
			VK_UploadBufferDataOffset(&vk_d.geometry.idx, currentAS->geometries.geometry.triangles.indexOffset + (j * sizeof(uint32_t)), sizeof(uint32_t), (void*)&idx);
		}
		//}
		for (int j = 0; j < tess.numVertexes; j++) {
			float p[12] = {
				p[0] = tess.xyz[j][0],
				p[1] = tess.xyz[j][1],
				p[2] = tess.xyz[j][2],
				p[3] = 0,
				p[4] = cTex ? tess.svars.texcoords[0][j][0] : tess.texCoords[j][0][0],
				p[5] = cTex ? tess.svars.texcoords[0][j][1] : tess.texCoords[j][0][1],
				p[6] = tess.texCoords[j][1][0],
				p[7] = tess.texCoords[j][1][1],
				p[8] = (float)tess.svars.colors[j][0],
				p[9] = (float)tess.svars.colors[j][1],
				p[10] = (float)tess.svars.colors[j][2],
				p[11] = 0
			};
			VK_UploadBufferDataOffset(&vk_d.geometry.xyz, currentAS->geometries.geometry.triangles.vertexOffset + (j * sizeof(float[12])), 12 * sizeof(float), (void*)&p);
		}
	}
	if (!dynamic && (deform || frames)) {

		qboolean updateBottom = (currentAS->geometries.geometry.triangles.vertexCount >= tess.numVertexes &&
			currentAS->geometries.geometry.triangles.indexCount == tess.numIndexes);

		currentAS->geometries.geometry.triangles.vertexCount = tess.numVertexes;
		currentAS->geometries.geometry.triangles.indexCount = tess.numIndexes;

		if (updateBottom) VK_UpdateBottomAS(vk.swapchain.CurrentCommandBuffer(), currentAS, currentAS, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV, NULL);
		else  VK_RecreateBottomAS(vk.swapchain.CurrentCommandBuffer(), currentAS, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV);
	}
	if (dynamic) {
		if (currentAS != bAS) { // (deform || frames)
			qboolean updateBottom = (currentAS->geometries.geometry.triangles.vertexCount >= tess.numVertexes &&
				currentAS->geometries.geometry.triangles.indexCount == tess.numIndexes);

			currentAS->geometries.geometry.triangles.vertexCount = tess.numVertexes;
			currentAS->geometries.geometry.triangles.indexCount = tess.numIndexes;

			VK_UpdateBottomAS(vk.swapchain.CurrentCommandBuffer(), bAS, currentAS, VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_NV, &vk_d.basBufferDynamicOffset);
			vk_d.bottomASDynamicCount[vk.swapchain.currentImage]++;
			vk_d.geometry.idxDynamicOffset += tess.numIndexes;
			vk_d.geometry.xyzDynamicOffset += tess.numVertexes;
		}
	}
		
	RB_UpdateInstanceBuffer(currentAS);
	RB_UpdateInstanceDataBuffer(currentAS);
	// add bottom to trace as list
	memcpy(&vk_d.bottomASTraceList[vk_d.bottomASTraceListCount], currentAS, sizeof(vkbottomAS_t));
	vk_d.bottomASTraceListCount++;
	//if (!dynamic || (dynamic && !deform && !frames)) {
	//}
	//else {
	//	/*vk_d.bottomASDynamicList[vk.swapchain.currentImage][vk_d.bottomASDynamicCount[vk.swapchain.currentImage]].geometries = bAS->geometries;
	//	vk_d.bottomASDynamicList[vk.swapchain.currentImage][vk_d.bottomASDynamicCount[vk.swapchain.currentImage]].data = bAS->data;
	//	vk_d.bottomASDynamicList[vk.swapchain.currentImage][vk_d.bottomASDynamicCount[vk.swapchain.currentImage]].geometryInstance = bAS->geometryInstance;*/
	//	
	//	memcpy(&vk_d.bottomASTraceList[vk_d.bottomASTraceListCount], &vk_d.bottomASDynamicList[vk.swapchain.currentImage][vk_d.bottomASDynamicCount[vk.swapchain.currentImage] -1], sizeof(vkbottomAS_t));
	//}
}

static void RB_UpdateRayTraceAS(drawSurf_t* drawSurfs, int numDrawSurfs) {
	shader_t*		shader;
	int				fogNum;
	int				entityNum;
	int				dlighted;
	int				i;
	drawSurf_t*		drawSurf;
	float			originalTime;
	qboolean		dynamic, forceUpdate;

	// save original time for entity shader offsets
	originalTime = backEnd.refdef.floatTime;
	backEnd.currentEntity = &tr.worldEntity;
	
	for (i = 0, drawSurf = drawSurfs; i < numDrawSurfs; i++, drawSurf++) {
		R_DecomposeSort(drawSurf->sort, &entityNum, &shader, &fogNum, &dlighted);
		if (shader->isSky /*|| shader->polygonOffset == qtrue*/) {
			//continue;
		}
		forceUpdate = qfalse;
		// just to clean backend state
		RB_BeginSurface(shader, fogNum);

		/*if (entityNum != ENTITYNUM_WORLD) {
			if (drawSurf->bAS == NULL) {
				int x = 2;
			}
		}*/
		//if (tess.numIndexes == 0) continue;
		float tM[12];
		if (entityNum != ENTITYNUM_WORLD) {
			backEnd.currentEntity = &backEnd.refdef.entities[entityNum];
			backEnd.refdef.floatTime = originalTime - backEnd.currentEntity->e.shaderTime;
			tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

			R_RotateForEntity(backEnd.currentEntity, &backEnd.viewParms, &backEnd. or );
			
			tM[0] = backEnd.currentEntity->e.axis[0][0]; tM[1] = backEnd.currentEntity->e.axis[1][0]; tM[2] = backEnd.currentEntity->e.axis[2][0]; tM[3] = backEnd.currentEntity->e.origin[0];
			tM[4] = backEnd.currentEntity->e.axis[0][1]; tM[5] = backEnd.currentEntity->e.axis[1][1]; tM[6] = backEnd.currentEntity->e.axis[2][1]; tM[7] = backEnd.currentEntity->e.origin[1];
			tM[8] = backEnd.currentEntity->e.axis[0][2]; tM[9] = backEnd.currentEntity->e.axis[1][2]; tM[10] = backEnd.currentEntity->e.axis[2][2]; tM[11] = backEnd.currentEntity->e.origin[2];
			dynamic = qtrue;

			if (backEnd.currentEntity->e.reType & (RT_SPRITE | RT_BEAM | RT_LIGHTNING | RT_RAIL_CORE | RT_RAIL_RINGS)) {
				tM[0] = 1; tM[1] = 0; tM[2] = 0; tM[3] = 0;
				tM[4] = 0; tM[5] = 1; tM[6] = 0; tM[7] = 0;
				tM[8] = 0; tM[9] = 0; tM[10] = 1; tM[11] = 0;
			}
		}
		else {
			backEnd.currentEntity = &tr.worldEntity;
			backEnd.refdef.floatTime = originalTime;
			backEnd. or = backEnd.viewParms.world;
			tess.shaderTime = backEnd.refdef.floatTime - tess.shader->timeOffset;

			tM[0] = 1; tM[1] = 0; tM[2] = 0; tM[3] = 0;
			tM[4] = 0; tM[5] = 1; tM[6] = 0; tM[7] = 0;
			tM[8] = 0; tM[9] = 0; tM[10] = 1; tM[11] = 0;

			dynamic = qfalse;
		}
		// add the triangles for this surface
		rb_surfaceTable[*drawSurf->surface](drawSurf->surface);
		
		if (drawSurf->bAS == NULL && drawSurf->surface == SF_ENTITY) {
			int x = 2;
		}

		// handle dynamic objects
		if (backEnd.currentEntity->e.reType == RT_BEAM || backEnd.currentEntity->e.reType == RT_RAIL_CORE || backEnd.currentEntity->e.reType == RT_RAIL_RINGS) {
			int x = 2;
		}
		if (drawSurf->bAS == NULL && tess.numIndexes == 6 && tess.numVertexes == 4){//backEnd.currentEntity->e.reType == RT_SPRITE) {RT_BEAM
			drawSurf->bAS = &vk_d.bottomASList[0];
			dynamic = qtrue;
			forceUpdate = qtrue;
		}
		if (drawSurf->bAS == NULL && tess.shader->stages[0] != NULL) {
			dynamic = qtrue;
			forceUpdate = qtrue;
			RB_CreateBottomAS(drawSurf->bAS, dynamic);
		}
		if (drawSurf->bAS == NULL) continue;
		
		Com_Memcpy(&drawSurf->bAS->geometryInstance.transform, &tM, sizeof(float[12]));
		RB_AddBottomAS(drawSurf->bAS, dynamic, forceUpdate);
	}
	backEnd.refdef.floatTime = originalTime;

	VK_DestroyTopAccelerationStructure(&vk_d.topAS[vk.swapchain.currentImage]);
	VK_MakeTop(vk.swapchain.CurrentCommandBuffer(), &vk_d.topAS[vk.swapchain.currentImage], vk_d.bottomASTraceList, vk_d.bottomASTraceListCount, VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_BUILD_BIT_NV);
	
	tess.numIndexes = 0;
	tess.numVertexes = 0;
}

static void RB_TraceRays() {
	static float	s_flipMatrix[16] = {
		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		0, 0, -1, 0,
		-1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	};

	float	invViewMatrix[16];
	float	invProjMatrix[16];

	float	viewMatrix[16];
	float	viewMatrixFlipped[16];
	vec3_t	origin;	// player position
	VectorCopy(backEnd.viewParms. or .origin, origin);

	// projection matrix calculation
	const float* p = backEnd.viewParms.projectionMatrix;
	// update q3's proj matrix (opengl) to vulkan conventions: z - [0, 1] instead of [-1, 1] and invert y direction
	float zNear = r_znear->value;
	float zFar = backEnd.viewParms.zFar;
	float P10 = -zFar / (zFar - zNear);
	float P14 = -zFar * zNear / (zFar - zNear);
	float P5 = -p[5];

	float projMatrix[16] = {
		p[0],  p[1],  p[2], p[3],
		p[4],  P5,    p[6], p[7],
		p[8],  p[9],  P10,  p[11],
		p[12], p[13], P14,  p[15]
	};

	// view
	viewMatrix[0] = backEnd.viewParms. or .axis[0][0];
	viewMatrix[4] = backEnd.viewParms. or .axis[0][1];
	viewMatrix[8] = backEnd.viewParms. or .axis[0][2];
	viewMatrix[12] = -origin[0] * viewMatrix[0] + -origin[1] * viewMatrix[4] + -origin[2] * viewMatrix[8];

	viewMatrix[1] = backEnd.viewParms. or .axis[1][0];
	viewMatrix[5] = backEnd.viewParms. or .axis[1][1];
	viewMatrix[9] = backEnd.viewParms. or .axis[1][2];
	viewMatrix[13] = -origin[0] * viewMatrix[1] + -origin[1] * viewMatrix[5] + -origin[2] * viewMatrix[9];

	viewMatrix[2] = backEnd.viewParms. or .axis[2][0];
	viewMatrix[6] = backEnd.viewParms. or .axis[2][1];
	viewMatrix[10] = backEnd.viewParms. or .axis[2][2];
	viewMatrix[14] = -origin[0] * viewMatrix[2] + -origin[1] * viewMatrix[6] + -origin[2] * viewMatrix[10];

	viewMatrix[3] = 0;
	viewMatrix[7] = 0;
	viewMatrix[11] = 0;
	viewMatrix[15] = 1;

	// flip matrix for vulkan
	myGlMultMatrix(viewMatrix, s_flipMatrix, viewMatrixFlipped);

	// inverse view matrix
	myGLInvertMatrix(&viewMatrixFlipped, &invViewMatrix);
	// inverse proj matrix
	myGLInvertMatrix(&projMatrix, &invProjMatrix);
	// mvp
	myGlMultMatrix(&viewMatrixFlipped[0], projMatrix, vk_d.mvp);

	// bind rt pipeline
	VK_BindRayTracingPipeline(&vk_d.accelerationStructures.pipeline);
	// bind descriptor (rt data and texture array)
	VK_Bind2RayTracingDescriptorSets(&vk_d.accelerationStructures.pipeline, &vk_d.accelerationStructures.descriptor[vk.swapchain.currentImage], &vk_d.imageDescriptor);

	// push constants
	VK_SetRayTracingPushConstant(&vk_d.accelerationStructures.pipeline, VK_SHADER_STAGE_RAYGEN_BIT_NV, 0 * sizeof(float), 16 * sizeof(float), &invViewMatrix[0]);
	VK_SetRayTracingPushConstant(&vk_d.accelerationStructures.pipeline, VK_SHADER_STAGE_RAYGEN_BIT_NV, 16 * sizeof(float), 16 * sizeof(float), &invProjMatrix[0]);
	VK_SetRayTracingPushConstant(&vk_d.accelerationStructures.pipeline, VK_SHADER_STAGE_RAYGEN_BIT_NV, 32 * sizeof(float), sizeof(vec3_t), &origin);
	VK_SetRayTracingPushConstant(&vk_d.accelerationStructures.pipeline, VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV, 40 * sizeof(float), 16 * sizeof(float), &vk_d.mvp[0]);

	VK_TraceRays(&vk_d.accelerationStructures.pipeline.shaderBindingTableBuffer);
}

void RB_RayTraceScene(drawSurf_t* drawSurfs, int numDrawSurfs) {
	vkCmdEndRenderPass(vk.swapchain.CurrentCommandBuffer());

	for (int i = 0; i < vk_d.bottomASDynamicCount[vk.swapchain.currentImage]; i++) {
		VK_DestroyBottomAccelerationStructure(&vk_d.bottomASDynamicList[vk.swapchain.currentImage][i]);
	}
	vk_d.bottomASDynamicCount[vk.swapchain.currentImage] = 0;

	vk_d.bottomASTraceListCount = 0;
	vk_d.scratchBufferOffset = 0;
	vk_d.instanceBufferOffset = vk.swapchain.currentImage * (5000 * sizeof(VkGeometryInstanceNV));

	vk_d.tasBufferOffset = vk.swapchain.currentImage * (65536 * 20);
	vk_d.geometry.xyzDynamicOffset = vk.swapchain.currentImage * 500000 + 500000;
	vk_d.geometry.idxDynamicOffset = vk.swapchain.currentImage * 500000 + 500000;
	vk_d.basBufferDynamicOffset = vk_d.basBufferStaticOffset;

	RB_UpdateRayTraceAS(drawSurfs, numDrawSurfs);
	RB_TraceRays();

	VkMemoryBarrier memoryBarrier = { 0 };
	memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
	memoryBarrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_NV| VK_ACCESS_MEMORY_WRITE_BIT;
	memoryBarrier.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
	vkCmdPipelineBarrier(vk.swapchain.CurrentCommandBuffer(), VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_NV, VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, 0, 1, &memoryBarrier, 0, 0, 0, 0);

	// draw rt results to swap chain
	VK_BeginRenderClear();
	VK_DrawFullscreenRect(&vk_d.accelerationStructures.resultImage);
}

