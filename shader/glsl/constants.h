#ifndef  _CONSTANTS_H_
#define  _CONSTANTS_H_

// texture
#define TEXTURE_DEFAULT 					0x00000000
#define TEXTURE_ADD 						0x00000001
#define TEXTURE_MUL 						0x00000002

// cullMask
#define RAY_FIRST_PERSON_VISIBLE 			0x00000001
#define RAY_MIRROR_VISIBLE 					0x00000002
#define RAY_FIRST_PERSON_MIRROR_VISIBLE 	0x00000003
#define RAY_SKY_VISIBLE 					0x00000004

#define MATERIAL_KIND_MASK          		0x0000000f
#define MATERIAL_KIND_INVALID       		0x00000000
#define MATERIAL_KIND_REGULAR       		0x00000001
#define MATERIAL_KIND_LAVA        			0x00000002
#define MATERIAL_KIND_SLIME        			0x00000003
#define MATERIAL_KIND_WATER        			0x00000004
#define MATERIAL_KIND_FOG        			0x00000005
#define MATERIAL_KIND_GLASS       			0x00000006
#define MATERIAL_KIND_BULLET       			0x00000007

#define MATERIAL_FLAG_MASK          	    0x00000ff0
#define MATERIAL_FLAG_OPAQUE 			    0x00000010
#define MATERIAL_FLAG_TRANSPARENT  		    0x00000020
#define MATERIAL_FLAG_SEE_THROUGH  		    0x00000040
#define MATERIAL_FLAG_MIRROR 			    0x00000080
#define MATERIAL_FLAG_NEEDSCOLOR 		    0x00000100
#define MATERIAL_FLAG_PARTICLE 			    0x00000200

//
#define SBT_RGEN_PRIMARY_RAYS               0x00000000
#define SBT_RMISS_PATH_TRACER               0x00000001
#define SBT_RCHIT_OPAQUE                    0x00000002
#define SBT_RAHIT_PARTICLE                  0x00000003

// instance data type
#define S_TYPE_OPAQUE 						0x00000001u
#define S_TYPE_MIRROR 						0x00000002u
#define S_TYPE_NEEDSCOLOR 					0x00000004u
#define S_TYPE_PARTICLE 					0x00000008u


/*
#define MATERIAL_FLAG_LIGHT          0x08000000
#define MATERIAL_FLAG_CORRECT_ALBEDO 0x04000000
#define MATERIAL_FLAG_HANDEDNESS     0x02000000
#define MATERIAL_FLAG_WEAPON         0x01000000
#define MATERIAL_FLAG_WARP           0x00800000
#define MATERIAL_FLAG_FLOWING        0x00400000
#define MATERIAL_FLAG_DOUBLE_SIDED   0x00200000
#define MATERIAL_FLAG_SHELL_RED      0x00100000
#define MATERIAL_FLAG_SHELL_GREEN    0x00080000
#define MATERIAL_FLAG_SHELL_BLUE     0x00040000


#define MATERIAL_LIGHT_STYLE_MASK    0x0003f000
#define MATERIAL_LIGHT_STYLE_SHIFT   12
#define MATERIAL_INDEX_MASK          0x00000fff
*/
#endif /*_CONSTANTS_H_*/