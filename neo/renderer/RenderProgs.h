/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/
#ifndef __RENDERPROGS_H__
#define __RENDERPROGS_H__

static const int PC_ATTRIB_INDEX_VERTEX		= 0;
static const int PC_ATTRIB_INDEX_NORMAL		= 2;
static const int PC_ATTRIB_INDEX_COLOR		= 3;
static const int PC_ATTRIB_INDEX_COLOR2		= 4;
static const int PC_ATTRIB_INDEX_ST			= 8;
static const int PC_ATTRIB_INDEX_TANGENT	= 9;

#if defined( ID_VULKAN )
static const int MAX_DESC_SETS				= 16384;
#endif

// This enum list corresponds to the global constant register indecies as defined in global.inc for all
// shaders.  We used a shared pool to keeps things simple.  If something changes here then it also
// needs to change in global.inc and vice versa
enum renderParm_t
{
	// For backwards compatibility, do not change the order of the first 17 items
	RENDERPARM_SCREENCORRECTIONFACTOR = 0,
	RENDERPARM_WINDOWCOORD,
	RENDERPARM_DIFFUSEMODIFIER,
	RENDERPARM_SPECULARMODIFIER,
	
	RENDERPARM_LOCALLIGHTORIGIN,
	RENDERPARM_LOCALVIEWORIGIN,
	
	RENDERPARM_LIGHTPROJECTION_S,
	RENDERPARM_LIGHTPROJECTION_T,
	RENDERPARM_LIGHTPROJECTION_Q,
	RENDERPARM_LIGHTFALLOFF_S,
	
	RENDERPARM_BUMPMATRIX_S,
	RENDERPARM_BUMPMATRIX_T,
	
	RENDERPARM_DIFFUSEMATRIX_S,
	RENDERPARM_DIFFUSEMATRIX_T,
	
	RENDERPARM_SPECULARMATRIX_S,
	RENDERPARM_SPECULARMATRIX_T,
	
	RENDERPARM_VERTEXCOLOR_MODULATE,
	RENDERPARM_VERTEXCOLOR_ADD,
	
	// The following are new and can be in any order
	
	RENDERPARM_COLOR,
	RENDERPARM_VIEWORIGIN,
	RENDERPARM_GLOBALEYEPOS,
	
	RENDERPARM_MVPMATRIX_X,
	RENDERPARM_MVPMATRIX_Y,
	RENDERPARM_MVPMATRIX_Z,
	RENDERPARM_MVPMATRIX_W,
	
	RENDERPARM_MODELMATRIX_X,
	RENDERPARM_MODELMATRIX_Y,
	RENDERPARM_MODELMATRIX_Z,
	RENDERPARM_MODELMATRIX_W,
	
	RENDERPARM_PROJMATRIX_X,
	RENDERPARM_PROJMATRIX_Y,
	RENDERPARM_PROJMATRIX_Z,
	RENDERPARM_PROJMATRIX_W,
	
	RENDERPARM_MODELVIEWMATRIX_X,
	RENDERPARM_MODELVIEWMATRIX_Y,
	RENDERPARM_MODELVIEWMATRIX_Z,
	RENDERPARM_MODELVIEWMATRIX_W,
	
	RENDERPARM_TEXTUREMATRIX_S,
	RENDERPARM_TEXTUREMATRIX_T,
	
	RENDERPARM_TEXGEN_0_S,
	RENDERPARM_TEXGEN_0_T,
	RENDERPARM_TEXGEN_0_Q,
	RENDERPARM_TEXGEN_0_ENABLED,
	
	RENDERPARM_TEXGEN_1_S,
	RENDERPARM_TEXGEN_1_T,
	RENDERPARM_TEXGEN_1_Q,
	RENDERPARM_TEXGEN_1_ENABLED,
	
	RENDERPARM_WOBBLESKY_X,
	RENDERPARM_WOBBLESKY_Y,
	RENDERPARM_WOBBLESKY_Z,
	
	RENDERPARM_OVERBRIGHT,
	RENDERPARM_ENABLE_SKINNING,
	RENDERPARM_ALPHA_TEST,
	
	// RB begin
	RENDERPARM_AMBIENT_COLOR,
	
	RENDERPARM_GLOBALLIGHTORIGIN,
	RENDERPARM_JITTERTEXSCALE,
	RENDERPARM_JITTERTEXOFFSET,
	RENDERPARM_CASCADEDISTANCES,
	
	RENDERPARM_SHADOW_MATRIX_0_X,	// rpShadowMatrices[6 * 4]
	RENDERPARM_SHADOW_MATRIX_0_Y,
	RENDERPARM_SHADOW_MATRIX_0_Z,
	RENDERPARM_SHADOW_MATRIX_0_W,
	
	RENDERPARM_SHADOW_MATRIX_1_X,
	RENDERPARM_SHADOW_MATRIX_1_Y,
	RENDERPARM_SHADOW_MATRIX_1_Z,
	RENDERPARM_SHADOW_MATRIX_1_W,
	
	RENDERPARM_SHADOW_MATRIX_2_X,
	RENDERPARM_SHADOW_MATRIX_2_Y,
	RENDERPARM_SHADOW_MATRIX_2_Z,
	RENDERPARM_SHADOW_MATRIX_2_W,
	
	RENDERPARM_SHADOW_MATRIX_3_X,
	RENDERPARM_SHADOW_MATRIX_3_Y,
	RENDERPARM_SHADOW_MATRIX_3_Z,
	RENDERPARM_SHADOW_MATRIX_3_W,
	
	RENDERPARM_SHADOW_MATRIX_4_X,
	RENDERPARM_SHADOW_MATRIX_4_Y,
	RENDERPARM_SHADOW_MATRIX_4_Z,
	RENDERPARM_SHADOW_MATRIX_4_W,
	
	RENDERPARM_SHADOW_MATRIX_5_X,
	RENDERPARM_SHADOW_MATRIX_5_Y,
	RENDERPARM_SHADOW_MATRIX_5_Z,
	RENDERPARM_SHADOW_MATRIX_5_W,
	// RB end
	
	RENDERPARM_USER0,
	RENDERPARM_USER1,
	RENDERPARM_USER2,
	RENDERPARM_USER3,
	RENDERPARM_USER4,
	RENDERPARM_USER5,
	RENDERPARM_USER6,
	RENDERPARM_USER7,

	RENDERPARM_TOTAL
};

extern const char * GLSLParmNames[];

enum rpStage_t 
{
	SHADER_STAGE_VERTEX		= BIT( 0 ),
	SHADER_STAGE_FRAGMENT	= BIT( 1 ),
	SHADER_STAGE_ALL		= SHADER_STAGE_VERTEX | SHADER_STAGE_FRAGMENT
};

enum shaderFeature_t
{
	USE_GPU_SKINNING,
	LIGHT_POINT,
	LIGHT_PARALLEL,
	BRIGHTPASS,
	HDR_DEBUG,
	USE_SRGB,
	
	MAX_SHADER_MACRO_NAMES,
};

#if defined(ID_VULKAN)
enum rpBinding_t {
	BINDING_TYPE_UNIFORM_BUFFER,
	BINDING_TYPE_SAMPLER,
	BINDING_TYPE_MAX
};

struct shader_t {
	shader_t() : module( VK_NULL_HANDLE ) {}

	idStr					name;
	rpStage_t				stage;
	VkShaderModule			module;
	idList< rpBinding_t >	bindings;
	idList< int >			parmIndices;
};

struct renderProg_t {
	renderProg_t() :
					usesJoints( false ),
					optionalSkinning( false ),
					vertexShaderIndex( -1 ),
					fragmentShaderIndex( -1 ),
					vertexLayoutType( LAYOUT_DRAW_VERT ),
					pipelineLayout( VK_NULL_HANDLE ),
					descriptorSetLayout( VK_NULL_HANDLE ) {}

	struct pipelineState_t {
		pipelineState_t() : 
					stateBits( 0 ),
					pipeline( VK_NULL_HANDLE ) {
		}

		uint64		stateBits;
		VkPipeline	pipeline;
	};

	VkPipeline GetPipeline( uint64 stateBits, VkShaderModule vertexShader, VkShaderModule fragmentShader );

	idStr						name;
	bool						usesJoints;
	bool						optionalSkinning;
	int							vertexShaderIndex;
	int							fragmentShaderIndex;
	vertexLayoutType_t			vertexLayoutType;
	VkPipelineLayout			pipelineLayout;
	VkDescriptorSetLayout		descriptorSetLayout;
	idList< rpBinding_t >		bindings;
	idList< pipelineState_t >	pipelines;
};
#endif

/*
================================================================================================
idRenderProgManager
================================================================================================
*/
class idRenderProgManager
{
public:
	idRenderProgManager();
	~idRenderProgManager();
	
	void	Init();
	void	Shutdown();
	
	const idVec4 & GetRenderParm( renderParm_t rp );
	void	SetRenderParm( renderParm_t rp, const float* value );
	void	SetRenderParms( renderParm_t rp, const float* values, int numValues );
	
	int		FindShader( const char * name, rpStage_t stage );
	int		FindProgram( const char * name, int vIndex, int fIndex );

#if defined(ID_VULKAN)
	void	StartFrame();

	void 	BindProgram(int index);
	const renderProg_t & GetCurrentRenderProg() const 
	{ 
		return m_renderProgs[ currentRenderProgram ]; 
	}
	void	CommitCurrent( uint64 stateBits, VkCommandBuffer commandBuffer );
	
	void	ClearPipelines();
	void	PrintPipelineStates();

		// the joints buffer should only be bound for vertex programs that use joints
	bool		ShaderUsesJoints() const;
	// the rpEnableSkinning render parm should only be set for vertex programs that use it
	bool		ShaderHasOptionalSkinning() const;
	
#else
	// RB: added progIndex to handle many custom renderprogs
	void	BindShader( int progIndex, int vIndex, int fIndex, bool builtin );
	// RB end

	bool		ShaderUsesJoints() const
	{
		return vertexShaders[currentVertexShader].usesJoints;
	}
	// the rpEnableSkinning render parm should only be set for vertex programs that use it
	bool		ShaderHasOptionalSkinning() const
	{
		return vertexShaders[currentVertexShader].optionalSkinning;
	}

#endif

	void	BindShader_GUI( )
	{
		BindShader_Builtin( BUILTIN_GUI );
	}
	
	void	BindShader_Color( )
	{
		BindShader_Builtin( BUILTIN_COLOR );
	}
	
#if !defined(ID_VULKAN)
	// RB begin
	void	BindShader_ColorSkinned( )
	{
		BindShader_Builtin( BUILTIN_COLOR_SKINNED );
	}
	
	void	BindShader_VertexColor( )
	{
		BindShader_Builtin( BUILTIN_VERTEX_COLOR );
	}
	
	void	BindShader_AmbientLighting()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_LIGHTING );
	}
	
	void	BindShader_AmbientLightingSkinned()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_LIGHTING_SKINNED );
	}
	
	void	BindShader_SmallGeometryBuffer()
	{
		BindShader_Builtin( BUILTIN_SMALL_GEOMETRY_BUFFER );
	}
	
	void	BindShader_SmallGeometryBufferSkinned()
	{
		BindShader_Builtin( BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED );
	}
	// RB end
#endif

	void	BindShader_Texture( )
	{
		BindShader_Builtin( BUILTIN_TEXTURED );
	}
	
	void	BindShader_TextureVertexColor()
	{
		BindShader_Builtin( BUILTIN_TEXTURE_VERTEXCOLOR );
	};
	
#if !defined(ID_VULKAN)	
	void	BindShader_TextureVertexColor_sRGB()
	{
		BindShader_Builtin( BUILTIN_TEXTURE_VERTEXCOLOR_SRGB );
	};
#endif

	void	BindShader_TextureVertexColorSkinned()
	{
		BindShader_Builtin( BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED );
	};
	
	void	BindShader_TextureTexGenVertexColor()
	{
		BindShader_Builtin( BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR );
	};
	
	void	BindShader_Interaction()
	{
		BindShader_Builtin( BUILTIN_INTERACTION );
	}
	
	void	BindShader_InteractionSkinned()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SKINNED );
	}
	
	void	BindShader_InteractionAmbient()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_AMBIENT );
	}
	
	void	BindShader_InteractionAmbientSkinned()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_AMBIENT_SKINNED );
	}
	
#if !defined(ID_VULKAN)
	// RB begin
	void	BindShader_Interaction_ShadowMapping_Spot()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT );
	}
	
	void	BindShader_Interaction_ShadowMapping_Spot_Skinned()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED );
	}
	
	void	BindShader_Interaction_ShadowMapping_Point()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SHADOW_MAPPING_POINT );
	}
	
	void	BindShader_Interaction_ShadowMapping_Point_Skinned()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED );
	}
	
	void	BindShader_Interaction_ShadowMapping_Parallel()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL );
	}
	
	void	BindShader_Interaction_ShadowMapping_Parallel_Skinned()
	{
		BindShader_Builtin( BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED );
	}
	// RB end
#endif

	void	BindShader_SimpleShade()
	{
		BindShader_Builtin( BUILTIN_SIMPLESHADE );
	}
	
	void	BindShader_Environment()
	{
		BindShader_Builtin( BUILTIN_ENVIRONMENT );
	}
	
	void	BindShader_EnvironmentSkinned()
	{
		BindShader_Builtin( BUILTIN_ENVIRONMENT_SKINNED );
	}
	
	void	BindShader_BumpyEnvironment()
	{
		BindShader_Builtin( BUILTIN_BUMPY_ENVIRONMENT );
	}
	
	void	BindShader_BumpyEnvironmentSkinned()
	{
		BindShader_Builtin( BUILTIN_BUMPY_ENVIRONMENT_SKINNED );
	}
	
	void	BindShader_Depth()
	{
		BindShader_Builtin( BUILTIN_DEPTH );
	}
	
	void	BindShader_DepthSkinned()
	{
		BindShader_Builtin( BUILTIN_DEPTH_SKINNED );
	}
	
	void	BindShader_Shadow()
	{
		// RB: no FFP fragment rendering anymore
		//BindShader( -1, builtinShaders[BUILTIN_SHADOW], -1, true );
		
		BindShader_Builtin( BUILTIN_SHADOW );
		// RB end
	}
	
	void	BindShader_ShadowSkinned()
	{
		// RB: no FFP fragment rendering anymore
		//BindShader( -1, builtinShaders[BUILTIN_SHADOW_SKINNED], -1, true );
		
		BindShader_Builtin( BUILTIN_SHADOW_SKINNED );
		// RB end
	}
	
	void	BindShader_ShadowDebug()
	{
		BindShader_Builtin( BUILTIN_SHADOW_DEBUG );
	}
	
	void	BindShader_ShadowDebugSkinned()
	{
		BindShader_Builtin( BUILTIN_SHADOW_DEBUG_SKINNED );
	}
	
	void	BindShader_BlendLight()
	{
		BindShader_Builtin( BUILTIN_BLENDLIGHT );
	}
	
	void	BindShader_Fog()
	{
		BindShader_Builtin( BUILTIN_FOG );
	}
	
	void	BindShader_FogSkinned()
	{
		BindShader_Builtin( BUILTIN_FOG_SKINNED );
	}
	
	void	BindShader_SkyBox()
	{
		BindShader_Builtin( BUILTIN_SKYBOX );
	}
	
	void	BindShader_WobbleSky()
	{
		BindShader_Builtin( BUILTIN_WOBBLESKY );
	}
	
#if !defined(ID_VULKAN)
	void	BindShader_StereoDeGhost()
	{
		BindShader_Builtin( BUILTIN_STEREO_DEGHOST );
	}
	
	void	BindShader_StereoWarp()
	{
		BindShader_Builtin( BUILTIN_STEREO_WARP );
	}
	
	void	BindShader_StereoInterlace()
	{
		BindShader_Builtin( BUILTIN_STEREO_INTERLACE );
	}
	
	void	BindShader_PostProcess()
	{
		BindShader_Builtin( BUILTIN_POSTPROCESS );
	}
	
	void	BindShader_Screen()
	{
		BindShader_Builtin( BUILTIN_SCREEN );
	}
	
	void	BindShader_Tonemap()
	{
		BindShader_Builtin( BUILTIN_TONEMAP );
	}
	
	void	BindShader_Brightpass()
	{
		BindShader_Builtin( BUILTIN_BRIGHTPASS );
	}
	
	void	BindShader_HDRGlareChromatic()
	{
		BindShader_Builtin( BUILTIN_HDR_GLARE_CHROMATIC );
	}
	
	void	BindShader_HDRDebug()
	{
		BindShader_Builtin( BUILTIN_HDR_DEBUG );
	}
	
	void	BindShader_SMAA_EdgeDetection()
	{
		BindShader_Builtin( BUILTIN_SMAA_EDGE_DETECTION );
	}
	
	void	BindShader_SMAA_BlendingWeightCalculation()
	{
		BindShader_Builtin( BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION );
	}
	
	void	BindShader_SMAA_NeighborhoodBlending()
	{
		BindShader_Builtin( BUILTIN_SMAA_NEIGHBORHOOD_BLENDING );
	}
	
	void	BindShader_AmbientOcclusion()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION );
	}
	
	void	BindShader_AmbientOcclusionAndOutput()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT );
	}
	
	void	BindShader_AmbientOcclusionBlur()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_BLUR );
	}
	
	void	BindShader_AmbientOcclusionBlurAndOutput()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT );
	}
	
	void	BindShader_AmbientOcclusionMinify()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_MINIFY );
	}
	
	void	BindShader_AmbientOcclusionReconstructCSZ()
	{
		BindShader_Builtin( BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ );
	}
	
	void	BindShader_DeepGBufferRadiosity()
	{
		BindShader_Builtin( BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI );
	}
	
	void	BindShader_DeepGBufferRadiosityBlur()
	{
		BindShader_Builtin( BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR );
	}
	
	void	BindShader_DeepGBufferRadiosityBlurAndOutput()
	{
		BindShader_Builtin( BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT );
	}
#endif

#if 0
	void	BindShader_ZCullReconstruct()
	{
		BindShader_Builtin( BUILTIN_ZCULL_RECONSTRUCT );
	}
#endif
	
	void	BindShader_Bink()
	{
		BindShader_Builtin( BUILTIN_BINK );
	}
	
	void	BindShader_BinkGUI()
	{
		BindShader_Builtin( BUILTIN_BINK_GUI );
	}
	
#if !defined(ID_VULKAN)
	void	BindShader_MotionBlur()
	{
		BindShader_Builtin( BUILTIN_MOTION_BLUR );
	}
	
	void	BindShader_DebugShadowMap()
	{
		BindShader_Builtin( BUILTIN_DEBUG_SHADOWMAP );
	}
	// RB end
#endif
			
	// this should only be called via the reload shader console command
	void		LoadAllShaders();
	void		KillAllShaders();

	// unbind the currently bound render program
	void		Unbind();
	
protected:	
	enum rpBuiltIn_t
	{
		BUILTIN_GUI,
		BUILTIN_COLOR,
		// RB begin
#if !defined(ID_VULKAN)
		BUILTIN_COLOR_SKINNED,
		BUILTIN_VERTEX_COLOR,
		BUILTIN_AMBIENT_LIGHTING,
		BUILTIN_AMBIENT_LIGHTING_SKINNED,
		BUILTIN_SMALL_GEOMETRY_BUFFER,
		BUILTIN_SMALL_GEOMETRY_BUFFER_SKINNED,
#endif
		// RB end
		BUILTIN_SIMPLESHADE,
		BUILTIN_TEXTURED,
		BUILTIN_TEXTURE_VERTEXCOLOR,
#if !defined(ID_VULKAN)
		BUILTIN_TEXTURE_VERTEXCOLOR_SRGB,
#endif
		BUILTIN_TEXTURE_VERTEXCOLOR_SKINNED,
		BUILTIN_TEXTURE_TEXGEN_VERTEXCOLOR,
		BUILTIN_INTERACTION,
		BUILTIN_INTERACTION_SKINNED,
		BUILTIN_INTERACTION_AMBIENT,
		BUILTIN_INTERACTION_AMBIENT_SKINNED,
#if !defined(ID_VULKAN)
		// RB begin
		BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT,
		BUILTIN_INTERACTION_SHADOW_MAPPING_SPOT_SKINNED,
		BUILTIN_INTERACTION_SHADOW_MAPPING_POINT,
		BUILTIN_INTERACTION_SHADOW_MAPPING_POINT_SKINNED,
		BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL,
		BUILTIN_INTERACTION_SHADOW_MAPPING_PARALLEL_SKINNED,
#endif
		// RB end
		BUILTIN_ENVIRONMENT,
		BUILTIN_ENVIRONMENT_SKINNED,
		BUILTIN_BUMPY_ENVIRONMENT,
		BUILTIN_BUMPY_ENVIRONMENT_SKINNED,
		
		BUILTIN_DEPTH,
		BUILTIN_DEPTH_SKINNED,
		BUILTIN_SHADOW,
		BUILTIN_SHADOW_SKINNED,
		BUILTIN_SHADOW_DEBUG,
		BUILTIN_SHADOW_DEBUG_SKINNED,
		
		BUILTIN_BLENDLIGHT,
		BUILTIN_FOG,
		BUILTIN_FOG_SKINNED,
		BUILTIN_SKYBOX,
		BUILTIN_WOBBLESKY,
#if !defined(ID_VULKAN)
		BUILTIN_POSTPROCESS,
		// RB begin
		BUILTIN_SCREEN,
		BUILTIN_TONEMAP,
		BUILTIN_BRIGHTPASS,
		BUILTIN_HDR_GLARE_CHROMATIC,
		BUILTIN_HDR_DEBUG,
		
		BUILTIN_SMAA_EDGE_DETECTION,
		BUILTIN_SMAA_BLENDING_WEIGHT_CALCULATION,
		BUILTIN_SMAA_NEIGHBORHOOD_BLENDING,
		
		BUILTIN_AMBIENT_OCCLUSION,
		BUILTIN_AMBIENT_OCCLUSION_AND_OUTPUT,
		BUILTIN_AMBIENT_OCCLUSION_BLUR,
		BUILTIN_AMBIENT_OCCLUSION_BLUR_AND_OUTPUT,
		BUILTIN_AMBIENT_OCCLUSION_MINIFY,
		BUILTIN_AMBIENT_OCCLUSION_RECONSTRUCT_CSZ,
		
		BUILTIN_DEEP_GBUFFER_RADIOSITY_SSGI,
		BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR,
		BUILTIN_DEEP_GBUFFER_RADIOSITY_BLUR_AND_OUTPUT,

		// RB end
		BUILTIN_STEREO_DEGHOST,
		BUILTIN_STEREO_WARP,
		BUILTIN_ZCULL_RECONSTRUCT,
#endif
		BUILTIN_BINK,
		BUILTIN_BINK_GUI,

#if !defined(ID_VULKAN)
		BUILTIN_STEREO_INTERLACE,
		BUILTIN_MOTION_BLUR,
		
		BUILTIN_DEBUG_SHADOWMAP,
#endif
		MAX_BUILTINS
	};

private:
	int builtinShaders[MAX_BUILTINS];
	int	currentRenderProgram;
	idStaticList< idVec4, RENDERPARM_TOTAL > uniforms;

#if defined(ID_VULKAN)
	void	LoadShader( int index );
	void	LoadShader( shader_t & shader );

	void	AllocParmBlockBuffer( const idList< int > & parmIndices, idUniformBuffer & ubo );

	void BindShader_Builtin( int i )
	{
		BindProgram(i);
	}

	idList< shader_t, TAG_RENDER >	m_shaders;
	idList< renderProg_t, TAG_RENDER > m_renderProgs;

	int					m_counter;
	int					m_currentData;
	int					m_currentDescSet;
	int					m_currentParmBufferOffset;
	VkDescriptorPool	m_descriptorPools[ NUM_FRAME_DATA ];
	VkDescriptorSet		m_descriptorSets[ NUM_FRAME_DATA ][ MAX_DESC_SETS ];

	idUniformBuffer *	m_parmBuffers[ NUM_FRAME_DATA ];

#else
private:
	void BindShader_Builtin( int i )
	{
		BindShader( -1, builtinShaders[i], builtinShaders[i], true );
	}

	static const int	MAX_GLSL_USER_PARMS = 8;

	int		FindVertexShader( const char* name );
	int		FindFragmentShader( const char* name );

	const char*	GetGLSLParmName( int rp ) const;
	int			GetGLSLCurrentProgram() const
	{
		return currentRenderProgram;
	}
	void		SetUniformValue( const renderParm_t rp, const float* value );
	void		CommitUniforms();
	int			FindGLSLProgram( const char* name, int vIndex, int fIndex );
	void		ZeroUniforms();

	void	LoadVertexShader( int index );
	void	LoadFragmentShader( int index );
	
	static const char* GLSLMacroNames[MAX_SHADER_MACRO_NAMES];
	const char*	GetGLSLMacroName( shaderFeature_t sf ) const;
	
	bool	CompileGLSL( GLenum target, const char* name );
	GLuint	LoadGLSLShader( GLenum target, const char* name, const char* nameOutSuffix, uint32 shaderFeatures, bool builtin, idList<int>& uniforms );
	void	LoadGLSLProgram( const int programIndex, const int vertexShaderIndex, const int fragmentShaderIndex );
	
	static const GLuint INVALID_PROGID = 0xFFFFFFFF;
	
	struct vertexShader_t
	{
		vertexShader_t() : progId( INVALID_PROGID ), usesJoints( false ), optionalSkinning( false ), shaderFeatures( 0 ), builtin( false ) {}
		idStr		name;
		idStr		nameOutSuffix;
		GLuint		progId;
		bool		usesJoints;
		bool		optionalSkinning;
		uint32		shaderFeatures;		// RB: Cg compile macros
		bool		builtin;			// RB: part of the core shaders built into the executable
		idList<int>	uniforms;
	};
	struct fragmentShader_t
	{
		fragmentShader_t() : progId( INVALID_PROGID ), shaderFeatures( 0 ), builtin( false ) {}
		idStr		name;
		idStr		nameOutSuffix;
		GLuint		progId;
		uint32		shaderFeatures;
		bool		builtin;
		idList<int>	uniforms;
	};
	
	struct glslUniformLocation_t
	{
		int		parmIndex;
		int  	uniformIndex;
	};

	struct glslProgram_t
	{
		glslProgram_t() :	progId( INVALID_PROGID ),
			vertexShaderIndex( -1 ),
			fragmentShaderIndex( -1 ),
			vertexUniformArray( -1 ),
			fragmentUniformArray( -1 ) {}
		idStr		name;
		GLuint		progId;
		int			vertexShaderIndex;
		int			fragmentShaderIndex;
		GLint		vertexUniformArray;
		GLint		fragmentUniformArray;
		idList<glslUniformLocation_t> uniformLocations;
	};
	idList<glslProgram_t, TAG_RENDER> glslPrograms;
	
	
	int				currentVertexShader;
	int				currentFragmentShader;
	idList<vertexShader_t, TAG_RENDER> vertexShaders;
	idList<fragmentShader_t, TAG_RENDER> fragmentShaders;
#endif /* ID_VULKAN */	
};

extern idRenderProgManager renderProgManager;

#endif
