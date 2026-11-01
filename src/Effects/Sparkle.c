/****************************/
/*   	SPARKLE.C  			*/
/* By Brian Greenstone      */
/* (c)2001 Pangea Software  */
/* (c)2023 Iliyas Jorio     */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static void DrawSparkles(ObjNode*);


/****************************/
/*    CONSTANTS             */
/****************************/



/*********************/
/*    VARIABLES      */
/*********************/

SparkleType	gSparkles[MAX_SPARKLES];
Pool*		gSparklePool = NULL;

static float	gPlayerSparkleColor = 0;

static OGLPoint3D gSparklePoints[MAX_SPARKLES * 4];
static OGLTextureCoord gSparkleUVs[MAX_SPARKLES * 4];
static OGLColorRGBA gSparkleColors[MAX_SPARKLES * 4];
static MOTriangleIndecies gSparkleTriangles[MAX_SPARKLES * 2];
static MOVertexArrayData gSparkleMesh =
{
	.numMaterials = 1,
	.points = gSparklePoints,
	.uvs = {gSparkleUVs},
	.colorsFloat = gSparkleColors,
	.triangles = gSparkleTriangles,
};
#ifdef __EMSCRIPTEN__
static MOVertexArrayBuffers gSparkleBuffers;
#endif

/*************************** INIT SPARKLES **********************************/

void InitSparkles(void)
{
	GAME_ASSERT(!gSparklePool);
	gSparklePool = Pool_New(MAX_SPARKLES);

	gPlayerSparkleColor = 0;

	for (int i = 0; i < MAX_SPARKLES; i++)
	{
		const GLuint v = i * 4;
		gSparkleUVs[v + 0] = (OGLTextureCoord) {0, 1};
		gSparkleUVs[v + 1] = (OGLTextureCoord) {1, 1};
		gSparkleUVs[v + 2] = (OGLTextureCoord) {1, 0};
		gSparkleUVs[v + 3] = (OGLTextureCoord) {0, 0};
		gSparkleTriangles[i * 2 + 0] = (MOTriangleIndecies) {{v, v + 1, v + 2}};
		gSparkleTriangles[i * 2 + 1] = (MOTriangleIndecies) {{v, v + 2, v + 3}};
	}

	NewObjectDefinitionType sparkleDrawerDef =
	{
		.genre = CUSTOM_GENRE,
		.slot = SPARKLE_SLOT,
		.scale = 1,
		.flags = STATUS_BIT_DONTCULL,
		.drawCall = DrawSparkles,
	};
	MakeNewObject(&sparkleDrawerDef);
}

/************************* DISPOSE SPARKLES ********************************/

void DisposeSparkles(void)
{
#ifdef __EMSCRIPTEN__
	MO_DisposeGeometryBuffers(&gSparkleBuffers);
#endif
	Pool_Free(gSparklePool);
	gSparklePool = NULL;
}


/*************************** GET FREE SPARKLE ******************************/
//
// OUTPUT:  -1 if none
//

int GetFreeSparkle(ObjNode *theNode)
{
			/* FIND A FREE SLOT */

	int i = Pool_AllocateIndex(gSparklePool);

	if (i >= 0)
	{
		gSparkles[i].owner = theNode;
	}

	return i;
}




/***************** DELETE SPARKLE *********************/

void DeleteSparkle(int i)
{
	if (i < 0)
		return;

	GAME_ASSERT(Pool_IsUsed(gSparklePool, i));

	gSparkles[i].owner = NULL;

	Pool_ReleaseIndex(gSparklePool, i);
}


static void DrawSparkleBatch(int texture, int count)
{
	gSparkleMesh.numPoints = count * 4;
	gSparkleMesh.numTriangles = count * 2;
	gSparkleMesh.materials[0] = gSpriteGroupList[SPRITE_GROUP_PARTICLES][texture].materialObject;
#ifdef __EMSCRIPTEN__
	gSparkleBuffers.dirty |= (1u << MO_BUFFER_POINTS) | (1u << MO_BUFFER_COLORS_FLOAT);
	MO_DrawGeometry_VertexArrayBuffered(&gSparkleMesh, &gSparkleBuffers);
#else
	MO_DrawGeometry_VertexArray(&gSparkleMesh);
#endif
}

/*************************** DRAW SPARKLES ******************************/

static void DrawSparkles(ObjNode* theNode)
{
float	dot,separation;
OGLMatrix4x4	m;
OGLVector3D	v;
OGLPoint3D	where;
static const OGLVector3D 	up = {0,1,0};
OGLPoint3D					*cameraLocation;
static OGLPoint3D		frame[4] =
{
	{-130,	130,	0},
	{130,	130,	0},
	{130,	-130,	0},
	{-130,	-130,	0},
};

	(void) theNode;

	if (!gSparklePool || Pool_Empty(gSparklePool))			// quick check if any sparkles at all
		return;


	OGL_PushState();

	OGL_DisableLighting();									// deactivate lighting
	glDisable(GL_NORMALIZE);								// disable vector normalizing since scale == 1
	glDisable(GL_CULL_FACE);								// deactivate culling
	glDisable(GL_FOG);										// deactivate fog
	glDepthMask(GL_FALSE);									// no z-writes
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);		// set blending mode


			/*********************/
			/* DRAW EACH SPARKLE */
			/*********************/

	cameraLocation = &gGameViewInfoPtr->cameraPlacement.cameraLocation;		// point to camera coord
	int batchTexture = -1;
	int batchCount = 0;

	for (int i = Pool_First(gSparklePool); i >= 0; i = Pool_Next(gSparklePool, i))
	{
		GAME_ASSERT(Pool_IsUsed(gSparklePool, i));			// must be active

		uint32_t flags = gSparkles[i].flags;				// get sparkle flags

		ObjNode* owner = gSparkles[i].owner;
		if (owner != nil)									// if owner is culled then dont draw
		{
			if (owner->StatusBits & (STATUS_BIT_ISCULLED|STATUS_BIT_HIDDEN))
				continue;

					/* SEE IF TRANSFORM WITH OWNER */

			if (flags & SPARKLE_FLAG_TRANSFORMWITHOWNER)
			{
				OGLPoint3D_Transform(&gSparkles[i].where, &owner->BaseTransformMatrix, &where);
			}
			else
				where = gSparkles[i].where;
		}
		else
			where = gSparkles[i].where;


			/* CALC ANGLE INFO */

		v.x = cameraLocation->x - where.x;			// calc vector from sparkle to camera
		v.y = cameraLocation->y - where.y;
		v.z = cameraLocation->z - where.z;
		FastNormalizeVector(v.x, v.y, v.z, &v);

		separation = gSparkles[i].separation;
		where.x += v.x * separation;									// offset the base point
		where.y += v.y * separation;
		where.z += v.z * separation;

		if (!(flags & SPARKLE_FLAG_OMNIDIRECTIONAL))		// if not omni then calc alpha based on angle
		{
			dot = OGLVector3D_Dot(&v, &gSparkles[i].aim);				// calc angle between
			if (dot <= 0.0f)
				continue;

			gSparkles[i].color.a = dot;									// make brighter as we look head-on
		}



			/* CALC TRANSFORM MATRIX */

		frame[0].x = -gSparkles[i].scale;								// set allocated of quad
		frame[0].y = gSparkles[i].scale;
		frame[1].x = gSparkles[i].scale;
		frame[1].y = gSparkles[i].scale;
		frame[2].x = gSparkles[i].scale;
		frame[2].y = -gSparkles[i].scale;
		frame[3].x = -gSparkles[i].scale;
		frame[3].y = -gSparkles[i].scale;




		// Batch consecutive equal textures without changing transparency order.
		OGLColorRGBA color = gSparkles[i].color;
		if (flags & SPARKLE_FLAG_FLICKER)
		{
			color.a += RandomFloat2() * .5f;
			if (color.a < 0.0f)
				continue;
			if (color.a > 1.0f)
				color.a = 1.0f;
		}

		const int texture = gSparkles[i].textureNum;
		if (batchCount && texture != batchTexture)
		{
			DrawSparkleBatch(batchTexture, batchCount);
			batchCount = 0;
		}
		batchTexture = texture;

		const int first = batchCount * 4;
		SetLookAtMatrixAndTranslate(&m, &up, &where, cameraLocation);
		OGLPoint3D_TransformArray(frame, &m, &gSparklePoints[first], 4);
		for (int vtx = 0; vtx < 4; vtx++)
			gSparkleColors[first + vtx] = color;
		batchCount++;
	}


	if (batchCount)
		DrawSparkleBatch(batchTexture, batchCount);

			/* RESTORE STATE */

	OGL_PopState();
}


#pragma mark -


/*************** CREATE PLAYER SPARKLES ********************/

void CreatePlayerSparkles(ObjNode *theNode)
{
short	i;

			/* CREATE CHEST LIGHTS */

	i = theNode->Sparkles[PLAYER_SPARKLE_CHEST] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i == -1)
		return;

	gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
	gSparkles[i].where.x = theNode->Coord.x;
	gSparkles[i].where.y = theNode->Coord.y + 100.0f;
	gSparkles[i].where.z = theNode->Coord.z;

	gSparkles[i].aim.x =
	gSparkles[i].aim.y =
	gSparkles[i].aim.z = 1;

	gSparkles[i].color.r = 1;
	gSparkles[i].color.g = 1;
	gSparkles[i].color.b = .3;
	gSparkles[i].color.a = .7;

	gSparkles[i].scale = 80.0f;
	gSparkles[i].separation = 30.0f;

	gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark4;
}

/***************** CREATE PLAYER JUMPJET SPARKLES **********************/

void CreatePlayerJumpJetSparkles(ObjNode *theNode)
{
short	i;

			/* LEFT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i == -1)
		return;

	gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
	gSparkles[i].where.x = theNode->Coord.x;
	gSparkles[i].where.y = theNode->Coord.y;;
	gSparkles[i].where.z = theNode->Coord.z;

	gSparkles[i].aim.x =
	gSparkles[i].aim.y =
	gSparkles[i].aim.z = 1;

	gSparkles[i].color.r = 1;
	gSparkles[i].color.g = 1;
	gSparkles[i].color.b = 1;
	gSparkles[i].color.a = 1;

	gSparkles[i].scale = 300.0f  * gPlayerInfo.scaleRatio;
	gSparkles[i].separation = 10.0f;

	gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;


			/* RIGHT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT] = GetFreeSparkle(theNode);				// get free sparkle slot
	if (i == -1)
		return;

	gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;
	gSparkles[i].where.x = theNode->Coord.x;
	gSparkles[i].where.y = theNode->Coord.y;;
	gSparkles[i].where.z = theNode->Coord.z;

	gSparkles[i].aim.x =
	gSparkles[i].aim.y =
	gSparkles[i].aim.z = 1;

	gSparkles[i].color.r = 1;
	gSparkles[i].color.g = 1;
	gSparkles[i].color.b = 1;
	gSparkles[i].color.a = 1;

	gSparkles[i].scale = 300.0f  * gPlayerInfo.scaleRatio;
	gSparkles[i].separation = 10.0f;

	gSparkles[i].textureNum = PARTICLE_SObjType_WhiteSpark3;

}



/*************** UPDATE PLAYER SPARKLES ********************/

void UpdatePlayerSparkles(ObjNode *theNode)
{
short			i;
OGLMatrix4x4	m;
float			fps = gFramesPerSecondFrac;

		/*****************************/
		/* UPDATE THE CHEST SPARKLES */
		/*****************************/

	i = theNode->Sparkles[PLAYER_SPARKLE_CHEST];								// get sparkle index
	if (i != -1)
	{
		static const OGLPoint3D	p = {0,0,0};

				/* GET MATRIX FOR JOINT */

		FindJointFullMatrix(theNode, PLAYER_JOINT_TORSO, &m);


				/* CALC COORD */

		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);


		gPlayerSparkleColor += fps * 10.0f;
		gSparkles[i].color.r = (1.0f + sin(gPlayerSparkleColor)) * .5f;
		gSparkles[i].scale = 80.0f * gPlayerInfo.scaleRatio;
	}


			/***************************/
			/* UPDATE JUMP-JET SPARKLE */
			/***************************/

				/* LEFT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT];								// get sparkle index
	if (i != -1)
	{
		static const OGLPoint3D	p = {0,-30,-20};


		FindJointFullMatrix(theNode, PLAYER_JOINT_LEFTFOOT, &m);
		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * .6f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[PLAYER_SPARKLE_LEFTFOOT] = -1;
		}
	}


				/* RIGHT FOOT */

	i = theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT];								// get sparkle index
	if (i != -1)
	{
		static const OGLPoint3D	p = {0,-30,-20};

		FindJointFullMatrix(theNode, PLAYER_JOINT_RIGHTFOOT, &m);
		OGLPoint3D_Transform(&p, &m, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * .6f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[PLAYER_SPARKLE_RIGHTFOOT] = -1;
		}
	}


			/*************************/
			/* UPDATE MUZZLE SPARKLE */
			/*************************/

	i = theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH];								// get sparkle index
	if (i != -1)
	{
		FindCoordOnJoint(theNode, PLAYER_JOINT_LEFTHAND, &gPlayerMuzzleTipOff, &gSparkles[i].where);

		gSparkles[i].color.a -= fps * 4.0f;						// fade out
		if (gSparkles[i].color.a <= 0.0f)
		{
			DeleteSparkle(i);
			theNode->Sparkles[PLAYER_SPARKLE_MUZZLEFLASH] = -1;
		}
	}


}


#pragma mark -


/***************** ADD RUNWAY LIGHTS ***********************/

Boolean AddRunwayLights(TerrainItemEntryType *itemPtr, long  x, long z)
{
uint8_t	flicker = itemPtr->parm[3] & 1;						// see if flicker
ObjNode	*newObj;
short	i,numSparkles,j,t;
float	r,dx,dz,x2,z2;
static const short textureTable[] =
{
	PARTICLE_SObjType_WhiteSpark,
	PARTICLE_SObjType_WhiteSpark2,
	PARTICLE_SObjType_WhiteSpark3,
	PARTICLE_SObjType_WhiteSpark4,
	PARTICLE_SObjType_RedGlint,
	PARTICLE_SObjType_RedSpark,
	PARTICLE_SObjType_BlueSpark,
	PARTICLE_SObjType_GreenSpark,
	PARTICLE_SObjType_BlueGlint
};

	t = itemPtr->parm[2];									// get texture #
	t = textureTable[t];

	numSparkles = itemPtr->parm[1];
	if (numSparkles > MAX_NODE_SPARKLES)
		DoFatalAlert("AddRunwayLights: # lights > MAX_NODE_SPARKLES");
	if (numSparkles == 0)
		numSparkles = 1;

			/*********************/
			/* MAKE EVENT OBJECT */
			/*********************/

	gNewObjectDefinition.genre		= EVENT_GENRE;
	gNewObjectDefinition.coord.x 	= x;
	gNewObjectDefinition.coord.z 	= z;
	gNewObjectDefinition.coord.y 	= GetTerrainY(x,z);
	gNewObjectDefinition.flags 		= 0;
	gNewObjectDefinition.slot 		= SLOT_OF_DUMB+20;
	gNewObjectDefinition.moveCall 	= MoveStaticObject;
	newObj = MakeNewObject(&gNewObjectDefinition);

	newObj->TerrainItemPtr = itemPtr;								// keep ptr to item list


			/*********************/
			/* MAKE THE SPARKLES */
			/*********************/

	r = (float)itemPtr->parm[0] * (PI/8);

	dx = -sin(r) * (TERRAIN_POLYGON_SIZE/2);
	dz = -cos(r) * (TERRAIN_POLYGON_SIZE/2);
	x2 = x;
	z2 = z;

	for (j = 0; j < numSparkles; j++)
	{
		i = newObj->Sparkles[j] = GetFreeSparkle(newObj);				// get free sparkle slot
		if (i != -1)
		{
			if (flicker)
				gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL|SPARKLE_FLAG_FLICKER;
			else
				gSparkles[i].flags = SPARKLE_FLAG_OMNIDIRECTIONAL;

			gSparkles[i].where.x = x2;
			gSparkles[i].where.z = z2;
			gSparkles[i].where.y = GetTerrainY(x2,z2) + 20.0f;

			gSparkles[i].color.r = 1;
			gSparkles[i].color.g = 1;
			gSparkles[i].color.b = 1;
			gSparkles[i].color.a = 1;

			gSparkles[i].scale = 60.0f;

			gSparkles[i].separation = 40.0f;

			gSparkles[i].textureNum = t;
		}


		x2 += dx;
		z2 += dz;
	}



	return(true);													// item was added
}

