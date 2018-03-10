/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2014-2016 Robert Beckebans
Copyright (C) 2014-2016 Kot in Action Creative Artel

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

#pragma hdrstop
#include "precompiled.h"
#include "../RenderCommon.h"

// RB begin
#if defined(_WIN32)

// Vista OpenGL wrapper check
#include "../sys/win32/win_local.h"
#endif
// RB end

// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#define BUGFIXEDSCREENSHOTRESOLUTION 1
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
#include "../framework/Common_local.h"
#endif

// DeviceContext bypasses RenderSystem to work directly with this
idGuiModel * tr_guiModel;

// functions that are not called every frame
glconfig_t	glConfig;

const char* fileExten[3] = { "tga", "png", "jpg" };
const char* envDirection[6] = { "_px", "_nx", "_py", "_ny", "_pz", "_nz" };
const char* skyDirection[6] = { "_forward", "_back", "_left", "_right", "_up", "_down" };


/*
========================
glBindMultiTextureEXT

As of 2011/09/16 the Intel drivers for "Sandy Bridge" and "Ivy Bridge" integrated graphics do not support this extension.
========================
*/
/*
void APIENTRY glBindMultiTextureEXT( GLenum texunit, GLenum target, GLuint texture )
{
	glActiveTextureARB( texunit );
	glBindTexture( target, texture );
}
*/

/*
=================
R_CheckExtension
=================
*/
// RB begin
static bool R_CheckExtension( const char* name )
// RB end
{
	if( !strstr( glConfig.extensions_string, name ) )
	{
		common->Printf( "X..%s not found\n", name );
		return false;
	}
	
	common->Printf( "...using %s\n", name );
	return true;
}


/*
========================
DebugCallback

For ARB_debug_output
========================
*/
// RB: added const to userParam
static void CALLBACK DebugCallback( unsigned int source, unsigned int type,
									unsigned int id, unsigned int severity, int length, const char* message, const void* userParam )
{
	// it probably isn't safe to do an idLib::Printf at this point
	
	// RB: printf should be thread safe on Linux
#if defined(_WIN32)
	OutputDebugString( message );
	OutputDebugString( "\n" );
#else
	printf( "%s\n", message );
#endif
	// RB end
}

/*
=================
R_CheckExtension
=================
*/
bool R_CheckExtension( char* name )
{
	if( !strstr( glConfig.extensions_string, name ) )
	{
		common->Printf( "X..%s not found\n", name );
		return false;
	}
	
	common->Printf( "...using %s\n", name );
	return true;
}

/*
==================
R_CheckPortableExtensions
==================
*/
// RB: replaced QGL with GLEW
static void R_CheckPortableExtensions()
{
	glConfig.glVersion = atof( glConfig.version_string );
	const char* badVideoCard = idLocalization::GetString( "#str_06780" );
	if( glConfig.glVersion < 2.0f )
	{
		idLib::FatalError( "%s", badVideoCard );
	}
	
	if( idStr::Icmpn( glConfig.renderer_string, "ATI ", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "AMD ", 4 ) == 0 )
	{
		glConfig.vendor = VENDOR_AMD;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "NVIDIA", 6 ) == 0 )
	{
		glConfig.vendor = VENDOR_NVIDIA;
	}
	else if( idStr::Icmpn( glConfig.renderer_string, "Intel", 5 ) == 0 )
	{
		glConfig.vendor = VENDOR_INTEL;
	}
	
	// RB: Mesa support
	if( idStr::Icmpn( glConfig.renderer_string, "Mesa", 4 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "X.org", 5 ) == 0 || idStr::Icmpn( glConfig.renderer_string, "Gallium", 7 ) == 0 ||
			strcmp( glConfig.vendor_string, "X.Org" ) == 0 ||
			idStr::Icmpn( glConfig.renderer_string, "llvmpipe", 8 ) == 0 )
	{
		if( glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE )
		{
			glConfig.driverType = GLDRV_OPENGL_MESA_CORE_PROFILE;
		}
		else
		{
			glConfig.driverType = GLDRV_OPENGL_MESA;
		}
	}
	// RB end
	
	// GL_ARB_multitexture
	if( glConfig.driverType != GLDRV_OPENGL3X )
	{
		glConfig.multitextureAvailable = true;
	}
	else
	{
		glConfig.multitextureAvailable = GLEW_ARB_multitexture != 0;
	}
	
	// GL_EXT_direct_state_access
	glConfig.directStateAccess = GLEW_EXT_direct_state_access != 0;
	
	
	// GL_ARB_texture_compression + GL_S3_s3tc
	// DRI drivers may have GL_ARB_texture_compression but no GL_EXT_texture_compression_s3tc
	if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	{
		glConfig.textureCompressionAvailable = true;
	}
	else
	{
		glConfig.textureCompressionAvailable = GLEW_ARB_texture_compression != 0 && GLEW_EXT_texture_compression_s3tc != 0;
	}
	// GL_EXT_texture_filter_anisotropic
	glConfig.anisotropicFilterAvailable = GLEW_EXT_texture_filter_anisotropic != 0;
	if( glConfig.anisotropicFilterAvailable )
	{
		glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &glConfig.maxTextureAnisotropy );
		common->Printf( "   maxTextureAnisotropy: %f\n", glConfig.maxTextureAnisotropy );
	}
	else
	{
		glConfig.maxTextureAnisotropy = 1;
	}
	
	// GL_EXT_texture_lod_bias
	// The actual extension is broken as specificed, storing the state in the texture unit instead
	// of the texture object.  The behavior in GL 1.4 is the behavior we use.
	glConfig.textureLODBiasAvailable = ( glConfig.glVersion >= 1.4 || GLEW_EXT_texture_lod_bias != 0 );
	if( glConfig.textureLODBiasAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_texture_lod_bias" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_EXT_texture_lod_bias" );
	}
	
	// GL_ARB_seamless_cube_map
	glConfig.seamlessCubeMapAvailable = GLEW_ARB_seamless_cube_map != 0;
	r_useSeamlessCubeMap.SetModified();		// the CheckCvars() next frame will enable / disable it
	
	// GL_ARB_framebuffer_sRGB
	glConfig.sRGBFramebufferAvailable = GLEW_ARB_framebuffer_sRGB != 0;
	r_useSRGB.SetModified();		// the CheckCvars() next frame will enable / disable it
	
	// GL_ARB_vertex_buffer_object
	if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	{
		glConfig.vertexBufferObjectAvailable = true;
	}
	else
	{
		glConfig.vertexBufferObjectAvailable = GLEW_ARB_vertex_buffer_object != 0;
	}
	
	// GL_ARB_map_buffer_range, map a section of a buffer object's data store
	//if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	//{
	//    glConfig.mapBufferRangeAvailable = true;
	//}
	//else
	{
		glConfig.mapBufferRangeAvailable = GLEW_ARB_map_buffer_range != 0;
	}
	
	// GL_ARB_vertex_array_object
	//if( glConfig.driverType == GLDRV_OPENGL_MESA_CORE_PROFILE )
	//{
	//    glConfig.vertexArrayObjectAvailable = true;
	//}
	//else
	{
		glConfig.vertexArrayObjectAvailable = GLEW_ARB_vertex_array_object != 0;
	}
	
	// GL_ARB_draw_elements_base_vertex
	glConfig.drawElementsBaseVertexAvailable = GLEW_ARB_draw_elements_base_vertex != 0;
	
	// GL_ARB_vertex_program / GL_ARB_fragment_program
	glConfig.fragmentProgramAvailable = GLEW_ARB_fragment_program != 0;
	//if( glConfig.fragmentProgramAvailable )
	{
		glGetIntegerv( GL_MAX_TEXTURE_COORDS, ( GLint* )&glConfig.maxTextureCoords );
		glGetIntegerv( GL_MAX_TEXTURE_IMAGE_UNITS, ( GLint* )&glConfig.maxTextureImageUnits );
	}
	
	// GLSL, core in OpenGL > 2.0
	glConfig.glslAvailable = ( glConfig.glVersion >= 2.0f );
	
	// GL_ARB_uniform_buffer_object
	glConfig.uniformBufferAvailable = GLEW_ARB_uniform_buffer_object != 0;
	if( glConfig.uniformBufferAvailable )
	{
		glGetIntegerv( GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, ( GLint* )&glConfig.uniformBufferOffsetAlignment );
		if( glConfig.uniformBufferOffsetAlignment < 256 )
		{
			glConfig.uniformBufferOffsetAlignment = 256;
		}
	}
	// RB: make GPU skinning optional for weak OpenGL drivers
	glConfig.gpuSkinningAvailable = glConfig.uniformBufferAvailable && ( glConfig.driverType == GLDRV_OPENGL3X || glConfig.driverType == GLDRV_OPENGL32_CORE_PROFILE || glConfig.driverType == GLDRV_OPENGL32_COMPATIBILITY_PROFILE );
	
	// ATI_separate_stencil / OpenGL 2.0 separate stencil
	glConfig.twoSidedStencilAvailable = ( glConfig.glVersion >= 2.0f ) || GLEW_ATI_separate_stencil != 0;
	
	// GL_EXT_depth_bounds_test
	glConfig.depthBoundsTestAvailable = GLEW_EXT_depth_bounds_test != 0;
	
	// GL_ARB_sync
	glConfig.syncAvailable = GLEW_ARB_sync &&
							 // as of 5/24/2012 (driver version 15.26.12.64.2761) sync objects
							 // do not appear to work for the Intel HD 4000 graphics
							 ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() );
							 
	// GL_ARB_occlusion_query
	glConfig.occlusionQueryAvailable = GLEW_ARB_occlusion_query != 0;
	
	// GL_ARB_timer_query
	glConfig.timerQueryAvailable = ( GLEW_ARB_timer_query != 0 || GLEW_EXT_timer_query != 0 ) && ( glConfig.vendor != VENDOR_INTEL || r_skipIntelWorkarounds.GetBool() ) && glConfig.driverType != GLDRV_OPENGL_MESA;
	
	// GREMEDY_string_marker
	glConfig.gremedyStringMarkerAvailable = GLEW_GREMEDY_string_marker != 0;
	if( glConfig.gremedyStringMarkerAvailable )
	{
		common->Printf( "...using %s\n", "GL_GREMEDY_string_marker" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_GREMEDY_string_marker" );
	}
	
	// GL_ARB_framebuffer_object
	glConfig.framebufferObjectAvailable = GLEW_ARB_framebuffer_object != 0;
	if( glConfig.framebufferObjectAvailable )
	{
		glGetIntegerv( GL_MAX_RENDERBUFFER_SIZE, &glConfig.maxRenderbufferSize );
		glGetIntegerv( GL_MAX_COLOR_ATTACHMENTS, &glConfig.maxColorAttachments );
		
		common->Printf( "...using %s\n", "GL_ARB_framebuffer_object" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_ARB_framebuffer_object" );
	}
	
	// GL_EXT_framebuffer_blit
	glConfig.framebufferBlitAvailable = GLEW_EXT_framebuffer_blit != 0;
	if( glConfig.framebufferBlitAvailable )
	{
		common->Printf( "...using %s\n", "GL_EXT_framebuffer_blit" );
	}
	else
	{
		common->Printf( "X..%s not found\n", "GL_EXT_framebuffer_blit" );
	}
	
	// GL_ARB_debug_output
	glConfig.debugOutputAvailable = GLEW_ARB_debug_output != 0;
	if( glConfig.debugOutputAvailable )
	{
		if( r_debugContext.GetInteger() >= 1 )
		{
			glDebugMessageCallbackARB( ( GLDEBUGPROCARB ) DebugCallback, NULL );
		}
		if( r_debugContext.GetInteger() >= 2 )
		{
			// force everything to happen in the main thread instead of in a separate driver thread
			glEnable( GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB );
		}
		if( r_debugContext.GetInteger() >= 3 )
		{
			// enable all the low priority messages
			glDebugMessageControlARB( GL_DONT_CARE,
									  GL_DONT_CARE,
									  GL_DEBUG_SEVERITY_LOW_ARB,
									  0, NULL, true );
		}
	}
	
	// GL_ARB_multitexture
	if( !glConfig.multitextureAvailable )
	{
		idLib::Error( "GL_ARB_multitexture not available" );
	}
	// GL_ARB_texture_compression + GL_EXT_texture_compression_s3tc
	if( !glConfig.textureCompressionAvailable )
	{
		idLib::Error( "GL_ARB_texture_compression or GL_EXT_texture_compression_s3tc not available" );
	}
	// GL_ARB_vertex_buffer_object
	if( !glConfig.vertexBufferObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_buffer_object not available" );
	}
	// GL_ARB_map_buffer_range
	if( !glConfig.mapBufferRangeAvailable )
	{
		idLib::Error( "GL_ARB_map_buffer_range not available" );
	}
	// GL_ARB_vertex_array_object
	if( !glConfig.vertexArrayObjectAvailable )
	{
		idLib::Error( "GL_ARB_vertex_array_object not available" );
	}
	// GL_ARB_draw_elements_base_vertex
	if( !glConfig.drawElementsBaseVertexAvailable )
	{
		idLib::Error( "GL_ARB_draw_elements_base_vertex not available" );
	}
	// GL_ARB_vertex_program / GL_ARB_fragment_program
	//if( !glConfig.fragmentProgramAvailable )
	//{
	//	idLib::Warning( "GL_ARB_fragment_program not available" );
	//}
	// GLSL
	if( !glConfig.glslAvailable )
	{
		idLib::Error( "GLSL not available" );
	}
	// GL_ARB_uniform_buffer_object
	if( !glConfig.uniformBufferAvailable )
	{
		idLib::Error( "GL_ARB_uniform_buffer_object not available" );
	}
	// GL_EXT_stencil_two_side
	if( !glConfig.twoSidedStencilAvailable )
	{
		idLib::Error( "GL_ATI_separate_stencil not available" );
	}
	
	// generate one global Vertex Array Object (VAO)
	glGenVertexArrays( 1, &glConfig.global_vao );
	glBindVertexArray( glConfig.global_vao );
}
// RB end



static bool r_initialized = false;

/*
=============================
R_IsInitialized
=============================
*/
bool R_IsInitialized()
{
	return r_initialized;
}

idStr extensions_string;

/*
==================
R_InitOpenGL

This function is responsible for initializing a valid OpenGL subsystem
for rendering.  This is done by calling the system specific GLimp_Init,
which gives us a working OGL subsystem, then setting all necessary openGL
state, including images, vertex programs, and display lists.

Changes to the vertex cache size or smp state require a vid_restart.

If R_IsInitialized() is false, no rendering can take place, but
all renderSystem functions will still operate properly, notably the material
and model information functions.
==================
*/
void R_InitOpenGL()
{

	common->Printf( "----- R_InitOpenGL -----\n" );
	
	if( R_IsInitialized() )
	{
		common->FatalError( "R_InitOpenGL called while active" );
	}
	
	// DG: make sure SDL has setup video so getting supported modes in R_SetNewMode() works
	GLimp_PreInit();
	// DG end
	
	R_SetNewMode( true );
	
	
	// input and sound systems need to be tied to the new window
	Sys_InitInput();
	
	// get our config strings
	glConfig.vendor_string = ( const char* )glGetString( GL_VENDOR );
	glConfig.renderer_string = ( const char* )glGetString( GL_RENDERER );
	glConfig.version_string = ( const char* )glGetString( GL_VERSION );
	glConfig.shading_language_string = ( const char* )glGetString( GL_SHADING_LANGUAGE_VERSION );
	glConfig.extensions_string = ( const char* )glGetString( GL_EXTENSIONS );
	
	if( glConfig.extensions_string == NULL )
	{
		// As of OpenGL 3.2, glGetStringi is required to obtain the available extensions
		//glGetStringi = ( PFNGLGETSTRINGIPROC )GLimp_ExtensionPointer( "glGetStringi" );
		
		// Build the extensions string
		GLint numExtensions;
		glGetIntegerv( GL_NUM_EXTENSIONS, &numExtensions );
		extensions_string.Clear();
		for( int i = 0; i < numExtensions; i++ )
		{
			extensions_string.Append( ( const char* )glGetStringi( GL_EXTENSIONS, i ) );
			// the now deprecated glGetString method usaed to create a single string with each extension separated by a space
			if( i < numExtensions - 1 )
			{
				extensions_string.Append( ' ' );
			}
		}
		glConfig.extensions_string = extensions_string.c_str();
	}
	
	
	float glVersion = atof( glConfig.version_string );
	float glslVersion = atof( glConfig.shading_language_string );
	idLib::Printf( "OpenGL Version   : %3.1f\n", glVersion );
	idLib::Printf( "OpenGL Vendor    : %s\n", glConfig.vendor_string );
	idLib::Printf( "OpenGL Renderer  : %s\n", glConfig.renderer_string );
	idLib::Printf( "OpenGL GLSL      : %3.1f\n", glslVersion );
	idLib::Printf( "OpenGL Extensions: %s\n", glConfig.extensions_string );
	
	// OpenGL driver constants
	GLint temp;
	glGetIntegerv( GL_MAX_TEXTURE_SIZE, &temp );
	glConfig.maxTextureSize = temp;
	
	// stubbed or broken drivers may have reported 0...
	if( glConfig.maxTextureSize <= 0 )
	{
		glConfig.maxTextureSize = 256;
	}
	
	r_initialized = true;
	
	// recheck all the extensions (FIXME: this might be dangerous)
	R_CheckPortableExtensions();
	
	renderProgManager.Init();
	
	r_initialized = true;
	
	// allocate the vertex array range or vertex objects
	vertexCache.Init(glConfig.uniformBufferOffsetAlignment);
	
	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();
	
	// Reset our gamma
	R_SetColorMappings();
	
	// RB begin
#if defined(_WIN32)
	static bool glCheck = false;
	if( !glCheck && win32.osversion.dwMajorVersion == 6 )
	{
		glCheck = true;
		if( !idStr::Icmp( glConfig.vendor_string, "Microsoft" ) && idStr::FindText( glConfig.renderer_string, "OpenGL-D3D" ) != -1 )
		{
			if( cvarSystem->GetCVarBool( "r_fullscreen" ) )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_NOW, "vid_restart partial windowed\n" );
				Sys_GrabMouseCursor( false );
			}
			int ret = MessageBox( NULL, "Please install OpenGL drivers from your graphics hardware vendor to run " GAME_NAME ".\nYour OpenGL functionality is limited.",
								  "Insufficient OpenGL capabilities", MB_OKCANCEL | MB_ICONWARNING | MB_TASKMODAL );
			if( ret == IDCANCEL )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "quit\n" );
				cmdSystem->ExecuteCommandBuffer();
			}
			if( cvarSystem->GetCVarBool( "r_fullscreen" ) )
			{
				cmdSystem->BufferCommandText( CMD_EXEC_APPEND, "vid_restart\n" );
			}
		}
	}
#endif
	// RB end
}



/*
=====================
R_ReloadSurface_f

Reload the material displayed by r_showSurfaceInfo
=====================
*/
static void R_ReloadSurface_f( const idCmdArgs& args )
{
	modelTrace_t mt;
	idVec3 start, end;
	
	// start far enough away that we don't hit the player model
	start = tr.primaryView->renderView.vieworg + tr.primaryView->renderView.viewaxis[0] * 16;
	end = start + tr.primaryView->renderView.viewaxis[0] * 1000.0f;
	if( !tr.primaryWorld->Trace( mt, start, end, 0.0f, false ) )
	{
		return;
	}
	
	common->Printf( "Reloading %s\n", mt.material->GetName() );
	
	// reload the decl
	mt.material->base->Reload();
	
	// reload any images used by the decl
	mt.material->ReloadImages( false );
}

/*
==============
R_ListModes_f
==============
*/
static void R_ListModes_f( const idCmdArgs& args )
{
	for( int displayNum = 0 ; ; displayNum++ )
	{
		idList<vidMode_t> modeList;
		if( !R_GetModeListForDisplay( displayNum, modeList ) )
		{
			break;
		}
		for( int i = 0; i < modeList.Num() ; i++ )
		{
			common->Printf( "Monitor %i, mode %3i: %4i x %4i @ %ihz\n", displayNum + 1, i, modeList[i].width, modeList[i].height, modeList[i].displayHz );
		}
	}
}

/*
=============
R_TestImage_f

Display the given image centered on the screen.
testimage <number>
testimage <filename>
=============
*/
void R_TestImage_f( const idCmdArgs& args )
{
	int imageNum;
	
	if( tr.testVideo )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;
	
	if( args.Argc() != 2 )
	{
		return;
	}
	
	if( idStr::IsNumeric( args.Argv( 1 ) ) )
	{
		imageNum = atoi( args.Argv( 1 ) );
		if( imageNum >= 0 && imageNum < globalImages->images.Num() )
		{
			tr.testImage = globalImages->images[imageNum];
		}
	}
	else
	{
		tr.testImage = globalImages->ImageFromFile( args.Argv( 1 ), TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	}
}

/*
=============
R_TestVideo_f

Plays the cinematic file in a testImage
=============
*/
void R_TestVideo_f( const idCmdArgs& args )
{
	if( tr.testVideo )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
	}
	tr.testImage = NULL;
	
	if( args.Argc() < 2 )
	{
		return;
	}
	
	tr.testImage = globalImages->ImageFromFile( "_scratch", TF_DEFAULT, TR_REPEAT, TD_DEFAULT );
	tr.testVideo = idCinematic::Alloc();
	tr.testVideo->InitFromFile( args.Argv( 1 ), true );
	
	cinData_t	cin;
	cin = tr.testVideo->ImageForTime( 0 );
	if( cin.imageY == NULL )
	{
		delete tr.testVideo;
		tr.testVideo = NULL;
		tr.testImage = NULL;
		return;
	}
	
	common->Printf( "%i x %i images\n", cin.imageWidth, cin.imageHeight );
	
	int	len = tr.testVideo->AnimationLength();
	common->Printf( "%5.1f seconds of video\n", len * 0.001 );
	
	tr.testVideoStartTime = tr.primaryRenderView.time[1];
	
	// try to play the matching wav file
	idStr	wavString = args.Argv( ( args.Argc() == 2 ) ? 1 : 2 );
	wavString.StripFileExtension();
	wavString = wavString + ".wav";
	common->SW()->PlayShaderDirectly( wavString.c_str() );
}

static int R_QsortSurfaceAreas( const void* a, const void* b )
{
	const idMaterial*	ea, *eb;
	int	ac, bc;
	
	ea = *( idMaterial** )a;
	if( !ea->EverReferenced() )
	{
		ac = 0;
	}
	else
	{
		ac = ea->GetSurfaceArea();
	}
	eb = *( idMaterial** )b;
	if( !eb->EverReferenced() )
	{
		bc = 0;
	}
	else
	{
		bc = eb->GetSurfaceArea();
	}
	
	if( ac < bc )
	{
		return -1;
	}
	if( ac > bc )
	{
		return 1;
	}
	
	return idStr::Icmp( ea->GetName(), eb->GetName() );
}


/*
===================
R_ReportSurfaceAreas_f

Prints a list of the materials sorted by surface area
===================
*/
#pragma warning( disable: 6385 ) // This is simply to get pass a false defect for /analyze -- if you can figure out a better way, please let Shawn know...
void R_ReportSurfaceAreas_f( const idCmdArgs& args )
{
	unsigned int		i;
	idMaterial**	list;
	
	const unsigned int count = declManager->GetNumDecls( DECL_MATERIAL );
	if( count == 0 )
	{
		return;
	}
	
	list = ( idMaterial** )_alloca( count * sizeof( *list ) );
	
	for( i = 0 ; i < count ; i++ )
	{
		list[i] = ( idMaterial* )declManager->DeclByIndex( DECL_MATERIAL, i, false );
	}
	
	qsort( list, count, sizeof( list[0] ), R_QsortSurfaceAreas );
	
	// skip over ones with 0 area
	for( i = 0 ; i < count ; i++ )
	{
		if( list[i]->GetSurfaceArea() > 0 )
		{
			break;
		}
	}
	
	for( ; i < count ; i++ )
	{
		// report size in "editor blocks"
		int	blocks = list[i]->GetSurfaceArea() / 4096.0;
		common->Printf( "%7i %s\n", blocks, list[i]->GetName() );
	}
}
#pragma warning( default: 6385 )


/*
==============================================================================

						SCREEN SHOTS

==============================================================================
*/

/*
====================
R_ReadTiledPixels

NO LONGER SUPPORTED (FIXME: make standard case work)

Used to allow the rendering of an image larger than the actual window by
tiling it into window-sized chunks and rendering each chunk separately

If ref isn't specified, the full session UpdateScreen will be done.
====================
*/
void R_ReadTiledPixels( int width, int height, byte* buffer, renderView_t* ref = NULL )
{
	// include extra space for OpenGL padding to word boundaries
	int sysWidth = renderSystem->GetWidth();
	int sysHeight = renderSystem->GetHeight();
	byte* temp = ( byte* )R_StaticAlloc( ( sysWidth + 3 ) * sysHeight * 3 );
	
	// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	if( sysWidth > width )
		sysWidth = width;
		
	if( sysHeight > height )
		sysHeight = height;
		
	// make sure the game / draw thread has completed
	//commonLocal.WaitGameThread();
	
	// discard anything currently on the list
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
	
	int originalNativeWidth = glConfig.nativeScreenWidth;
	int originalNativeHeight = glConfig.nativeScreenHeight;
	glConfig.nativeScreenWidth = sysWidth;
	glConfig.nativeScreenHeight = sysHeight;
#endif
	
	// disable scissor, so we don't need to adjust all those rects
	r_useScissor.SetBool( false );
	
	for( int xo = 0 ; xo < width ; xo += sysWidth )
	{
		for( int yo = 0 ; yo < height ; yo += sysHeight )
		{
			// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
			// discard anything currently on the list
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
			if( ref )
			{
				// ref is only used by envShot, Event_camShot, etc to grab screenshots of things in the world,
				// so this omits the hud and other effects
				tr.primaryWorld->RenderScene( ref );
			}
			else
			{
				// build all the draw commands without running a new game tic
				commonLocal.Draw();
			}
			// this should exit right after vsync, with the GPU idle and ready to draw
			const emptyCommand_t* cmd = tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
			
			// get the GPU busy with new commands
			tr.RenderCommandBuffers( cmd );
			
			// discard anything currently on the list (this triggers SwapBuffers)
			tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
#else
			// foresthale 2014-03-01: note: ref is always NULL in every call path to this function
			if( ref )
			{
				// discard anything currently on the list
				tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
			
				// build commands to render the scene
				tr.primaryWorld->RenderScene( ref );
			
				// finish off these commands
				const emptyCommand_t* cmd = tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
			
				// issue the commands to the GPU
				tr.RenderCommandBuffers( cmd );
			}
			else
			{
				const bool captureToImage = false;
				common->UpdateScreen( captureToImage, false );
			}
#endif
			
			int w = sysWidth;
			if( xo + w > width )
			{
				w = width - xo;
			}
			int h = sysHeight;
			if( yo + h > height )
			{
				h = height - yo;
			}
			
			glReadBuffer( GL_FRONT );
			glReadPixels( 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, temp );
			
			int	row = ( w * 3 + 3 ) & ~3;		// OpenGL pads to dword boundaries
			
			for( int y = 0 ; y < h ; y++ )
			{
				memcpy( buffer + ( ( yo + y )* width + xo ) * 3,
						temp + y * row, w * 3 );
			}
		}
	}
	
	// foresthale 2014-03-01: fixed custom screenshot resolution by doing a more direct render path
#ifdef BUGFIXEDSCREENSHOTRESOLUTION
	// discard anything currently on the list
	tr.SwapCommandBuffers( NULL, NULL, NULL, NULL );
	
	glConfig.nativeScreenWidth = originalNativeWidth;
	glConfig.nativeScreenHeight = originalNativeHeight;
#endif
	
	r_useScissor.SetBool( true );
	
	R_StaticFree( temp );
}


/*
==================
TakeScreenshot

Move to tr_imagefiles.c...

Downsample is the number of steps to mipmap the image before saving it
If ref == NULL, common->UpdateScreen will be used
==================
*/
// RB: changed .tga to .png
void idRenderSystemLocal::TakeScreenshot( int width, int height, const char* fileName, int blends, renderView_t* ref, int exten )
{
	byte*		buffer;
	int			i, j, c, temp;
	idStr finalFileName;
	
	finalFileName.Format( "%s.%s", fileName, fileExten[exten] );
	
	takingScreenshot = true;
	
	int pix = width * height;
	const int bufferSize = pix * 3 + 18;
	
	if( exten == PNG )
	{
		buffer = ( byte* )R_StaticAlloc( pix * 3 );
	}
	else if( exten == TGA )
	{
		buffer = ( byte* )R_StaticAlloc( bufferSize );
		memset( buffer, 0, bufferSize );
	}
	
	if( blends <= 1 )
	{
		if( exten == PNG )
		{
			R_ReadTiledPixels( width, height, buffer, ref );
		}
		else if( exten == TGA )
		{
			R_ReadTiledPixels( width, height, buffer + 18, ref );
		}
	}
	else
	{
		unsigned short* shortBuffer = ( unsigned short* )R_StaticAlloc( pix * 2 * 3 );
		memset( shortBuffer, 0, pix * 2 * 3 );
		
		// enable anti-aliasing jitter
		r_jitter.SetBool( true );
		
		for( i = 0 ; i < blends ; i++ )
		{
			if( exten == PNG )
			{
				R_ReadTiledPixels( width, height, buffer, ref );
			}
			else if( exten == TGA )
			{
				R_ReadTiledPixels( width, height, buffer + 18, ref );
			}
			
			for( j = 0 ; j < pix * 3 ; j++ )
			{
				if( exten == PNG )
				{
					shortBuffer[j] += buffer[j];
				}
				else if( exten == TGA )
				{
					shortBuffer[j] += buffer[18 + j];
				}
			}
		}
		
		// divide back to bytes
		for( i = 0 ; i < pix * 3 ; i++ )
		{
			if( exten == PNG )
			{
				buffer[i] = shortBuffer[i] / blends;
			}
			else if( exten == TGA )
			{
				buffer[18 + i] = shortBuffer[i] / blends;
			}
		}
		
		R_StaticFree( shortBuffer );
		r_jitter.SetBool( false );
	}
	if( exten == PNG )
	{
		R_WritePNG( finalFileName, buffer, 3, width, height, false, "fs_basepath" );
	}
	else
	{
		// fill in the header (this is vertically flipped, which qglReadPixels emits)
		buffer[2] = 2;	// uncompressed type
		buffer[12] = width & 255;
		buffer[13] = width >> 8;
		buffer[14] = height & 255;
		buffer[15] = height >> 8;
		buffer[16] = 24;	// pixel size
		
		// swap rgb to bgr
		c = 18 + width * height * 3;
		
		for( i = 18 ; i < c ; i += 3 )
		{
			temp = buffer[i];
			buffer[i] = buffer[i + 2];
			buffer[i + 2] = temp;
		}
		
		fileSystem->WriteFile( finalFileName, buffer, c, "fs_basepath" );
	}
	
	R_StaticFree( buffer );
	
	takingScreenshot = false;
}

/*
==================
R_ScreenshotFilename

Returns a filename with digits appended
if we have saved a previous screenshot, don't scan
from the beginning, because recording demo avis can involve
thousands of shots
==================
*/
void R_ScreenshotFilename( int& lastNumber, const char* base, idStr& fileName )
{
	bool restrict = cvarSystem->GetCVarBool( "fs_restrict" );
	cvarSystem->SetCVarBool( "fs_restrict", false );
	
	lastNumber++;
	if( lastNumber > 99999 )
	{
		lastNumber = 99999;
	}
	for( ; lastNumber < 99999 ; lastNumber++ )
	{
	
		// RB: added date to screenshot name
#if 0
		int	frac = lastNumber;
		int	a, b, c, d, e;
		
		a = frac / 10000;
		frac -= a * 10000;
		b = frac / 1000;
		frac -= b * 1000;
		c = frac / 100;
		frac -= c * 100;
		d = frac / 10;
		frac -= d * 10;
		e = frac;
		
		sprintf( fileName, "%s%i%i%i%i%i.png", base, a, b, c, d, e );
#else
		time_t aclock;
		time( &aclock );
		struct tm* t = localtime( &aclock );
		
		sprintf( fileName, "%s%s-%04d%02d%02d-%02d%02d%02d-%03d", base, "rbdoom-3-bfg",
				 1900 + t->tm_year, 1 + t->tm_mon, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, lastNumber );
#endif
		// RB end
		if( lastNumber == 99999 )
		{
			break;
		}
		int len = fileSystem->ReadFile( fileName, NULL, NULL );
		if( len <= 0 )
		{
			break;
		}
		// check again...
	}
	cvarSystem->SetCVarBool( "fs_restrict", restrict );
}

/*
==================
R_BlendedScreenShot

screenshot
screenshot [filename]
screenshot [width] [height]
screenshot [width] [height] [samples]
==================
*/
#define	MAX_BLENDS	256	// to keep the accumulation in shorts
void R_ScreenShot_f( const idCmdArgs& args )
{
	static int lastNumber = 0;
	idStr checkname;
	
	int width = renderSystem->GetWidth();
	int height = renderSystem->GetHeight();
	int	blends = 0;
	
	switch( args.Argc() )
	{
		case 1:
			width = renderSystem->GetWidth();
			height = renderSystem->GetHeight();
			blends = 1;
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;
		case 2:
			width = renderSystem->GetWidth();
			height = renderSystem->GetHeight();
			blends = 1;
			checkname = args.Argv( 1 );
			break;
		case 3:
			width = atoi( args.Argv( 1 ) );
			height = atoi( args.Argv( 2 ) );
			blends = 1;
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;
		case 4:
			width = atoi( args.Argv( 1 ) );
			height = atoi( args.Argv( 2 ) );
			blends = atoi( args.Argv( 3 ) );
			if( blends < 1 )
			{
				blends = 1;
			}
			if( blends > MAX_BLENDS )
			{
				blends = MAX_BLENDS;
			}
			R_ScreenshotFilename( lastNumber, "screenshots/", checkname );
			break;
		default:
			common->Printf( "usage: screenshot\n       screenshot <filename>\n       screenshot <width> <height>\n       screenshot <width> <height> <blends>\n" );
			return;
	}
	
	// put the console away
	console->Close();
	
	tr.TakeScreenshot( width, height, checkname, blends, NULL, PNG );
	
	common->Printf( "Wrote %s\n", checkname.c_str() );
}

/*
===============
R_StencilShot
Save out a screenshot showing the stencil buffer expanded by 16x range
===============
*/
void R_StencilShot()
{
	int			i, c;
	
	int	width = tr.GetWidth();
	int	height = tr.GetHeight();
	
	int	pix = width * height;
	
	c = pix * 3 + 18;
	idTempArray< byte > buffer( c );
	memset( buffer.Ptr(), 0, 18 );
	
	idTempArray< byte > byteBuffer( pix );
	
	glReadPixels( 0, 0, width, height, GL_STENCIL_INDEX , GL_UNSIGNED_BYTE, byteBuffer.Ptr() );
	
	for( i = 0 ; i < pix ; i++ )
	{
		buffer[18 + i * 3] =
			buffer[18 + i * 3 + 1] =
				//		buffer[18+i*3+2] = ( byteBuffer[i] & 15 ) * 16;
				buffer[18 + i * 3 + 2] = byteBuffer[i];
	}
	
	// fill in the header (this is vertically flipped, which glReadPixels emits)
	buffer[2] = 2;		// uncompressed type
	buffer[12] = width & 255;
	buffer[13] = width >> 8;
	buffer[14] = height & 255;
	buffer[15] = height >> 8;
	buffer[16] = 24;	// pixel size
	
	fileSystem->WriteFile( "screenshots/stencilShot.tga", buffer.Ptr(), c, "fs_savepath" );
}

/*
==================
R_EnvShot_f

envshot <basename>

Saves out env/<basename>_ft.tga, etc
==================
*/
void R_EnvShot_f( const idCmdArgs& args )
{
	idStr		fullname;
	const char*	baseName;
	int			i;
	idMat3		axis[6], oldAxis;
	renderView_t	ref;
	viewDef_t	primary;
	int			blends;
	const char*  extension;
	int			size;
	int         res_w, res_h, old_fov_x, old_fov_y;
	
	res_w = renderSystem->GetWidth();
	res_h = renderSystem->GetHeight();
	
	if( args.Argc() != 2 && args.Argc() != 3 && args.Argc() != 4 )
	{
		common->Printf( "USAGE: envshot <basename> [size] [blends]\n" );
		return;
	}
	baseName = args.Argv( 1 );
	
	blends = 1;
	if( args.Argc() == 4 )
	{
		size = atoi( args.Argv( 2 ) );
		blends = atoi( args.Argv( 3 ) );
	}
	else if( args.Argc() == 3 )
	{
		size = atoi( args.Argv( 2 ) );
		blends = 1;
	}
	else
	{
		size = 256;
		blends = 1;
	}
	
	if( !tr.primaryView )
	{
		common->Printf( "No primary view.\n" );
		return;
	}
	
	primary = *tr.primaryView;
	
	memset( &axis, 0, sizeof( axis ) );
	
	// +X
	axis[0][0][0] = 1;
	axis[0][1][2] = 1;
	axis[0][2][1] = 1;
	
	// -X
	axis[1][0][0] = -1;
	axis[1][1][2] = -1;
	axis[1][2][1] = 1;
	
	// +Y
	axis[2][0][1] = 1;
	axis[2][1][0] = -1;
	axis[2][2][2] = -1;
	
	// -Y
	axis[3][0][1] = -1;
	axis[3][1][0] = -1;
	axis[3][2][2] = 1;
	
	// +Z
	axis[4][0][2] = 1;
	axis[4][1][0] = -1;
	axis[4][2][1] = 1;
	
	// -Z
	axis[5][0][2] = -1;
	axis[5][1][0] = 1;
	axis[5][2][1] = 1;
	
	// let's get the game window to a "size" resolution
	if( ( res_w != size ) || ( res_h != size ) )
	{
		cvarSystem->SetCVarInteger( "r_windowWidth", size );
		cvarSystem->SetCVarInteger( "r_windowHeight", size );
		R_SetNewMode( false ); // the same as "vid_restart"
	} // FIXME that's a hack!!
	
	// so we return to that axis and fov after the fact.
	oldAxis = primary.renderView.viewaxis;
	old_fov_x = primary.renderView.fov_x;
	old_fov_y = primary.renderView.fov_y;
	
	for( i = 0 ; i < 6 ; i++ )
	{
	
		ref = primary.renderView;
		
		extension = envDirection[ i ];
		
		ref.fov_x = ref.fov_y = 90;
		ref.viewaxis = axis[i];
		fullname.Format( "env/%s%s", baseName, extension );
		
		tr.TakeScreenshot( size, size, fullname, blends, &ref, TGA );
	}
	
	// restore the original resolution, axis and fov
	ref.viewaxis = oldAxis;
	ref.fov_x = old_fov_x;
	ref.fov_y = old_fov_y;
	cvarSystem->SetCVarInteger( "r_windowWidth", res_w );
	cvarSystem->SetCVarInteger( "r_windowHeight", res_h );
	R_SetNewMode( false ); // the same as "vid_restart"
	
	common->Printf( "Wrote a env set with the name %s\n", baseName );
}

//============================================================================

static idMat3		cubeAxis[6];


/*
==================
R_SampleCubeMap
==================
*/
void R_SampleCubeMap( const idVec3& dir, int size, byte* buffers[6], byte result[4] )
{
	float	adir[3];
	int		axis, x, y;
	
	adir[0] = fabs( dir[0] );
	adir[1] = fabs( dir[1] );
	adir[2] = fabs( dir[2] );
	
	if( dir[0] >= adir[1] && dir[0] >= adir[2] )
	{
		axis = 0;
	}
	else if( -dir[0] >= adir[1] && -dir[0] >= adir[2] )
	{
		axis = 1;
	}
	else if( dir[1] >= adir[0] && dir[1] >= adir[2] )
	{
		axis = 2;
	}
	else if( -dir[1] >= adir[0] && -dir[1] >= adir[2] )
	{
		axis = 3;
	}
	else if( dir[2] >= adir[1] && dir[2] >= adir[2] )
	{
		axis = 4;
	}
	else
	{
		axis = 5;
	}
	
	float	fx = ( dir * cubeAxis[axis][1] ) / ( dir * cubeAxis[axis][0] );
	float	fy = ( dir * cubeAxis[axis][2] ) / ( dir * cubeAxis[axis][0] );
	
	fx = -fx;
	fy = -fy;
	x = size * 0.5 * ( fx + 1 );
	y = size * 0.5 * ( fy + 1 );
	if( x < 0 )
	{
		x = 0;
	}
	else if( x >= size )
	{
		x = size - 1;
	}
	if( y < 0 )
	{
		y = 0;
	}
	else if( y >= size )
	{
		y = size - 1;
	}
	
	result[0] = buffers[axis][( y * size + x ) * 4 + 0];
	result[1] = buffers[axis][( y * size + x ) * 4 + 1];
	result[2] = buffers[axis][( y * size + x ) * 4 + 2];
	result[3] = buffers[axis][( y * size + x ) * 4 + 3];
}

class CommandlineProgressBar
{
private:
	size_t tics = 0;
	size_t nextTicCount = 0;
	int	count = 0;
	int expectedCount = 0;
	
public:
	CommandlineProgressBar( int _expectedCount )
	{
		expectedCount = _expectedCount;
		
		common->Printf( "0%%  10   20   30   40   50   60   70   80   90   100%%\n" );
		common->Printf( "|----|----|----|----|----|----|----|----|----|----|\n" );
		
		common->UpdateScreen( false );
	}
	
	void Increment()
	{
		if( ( count + 1 ) >= nextTicCount )
		{
			size_t ticsNeeded = ( size_t )( ( ( double )( count + 1 ) / expectedCount ) * 50.0 );
			
			do
			{
				common->Printf( "*" );
			}
			while( ++tics < ticsNeeded );
			
			nextTicCount = ( size_t )( ( tics / 50.0 ) * expectedCount );
			if( count == ( expectedCount - 1 ) )
			{
				if( tics < 51 )
				{
					common->Printf( "*" );
				}
				common->Printf( "\n" );
			}
			
			common->UpdateScreen( false );
		}
		
		count++;
	}
};


// http://holger.dammertz.org/stuff/notes_HammersleyOnHemisphere.html

// To implement the Hammersley point set we only need an efficent way to implement the Van der Corput radical inverse phi2(i).
// Since it is in base 2 we can use some basic bit operations to achieve this.
// The brilliant book Hacker's Delight [warren01] provides us a a simple way to reverse the bits in a given 32bit integer. Using this, the following code then implements phi2(i)

/*
GLSL version

float radicalInverse_VdC( uint bits )
{
	bits = (bits << 16u) | (bits >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
*/

// RB: radical inverse implementation from the Mitsuba PBR system

// Van der Corput radical inverse in base 2 with single precision
inline float RadicalInverse_VdC( uint32_t n, uint32_t scramble = 0U )
{
	/* Efficiently reverse the bits in 'n' using binary operations */
#if (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 2))) || defined(__clang__)
	n = __builtin_bswap32( n );
#else
	n = ( n << 16 ) | ( n >> 16 );
	n = ( ( n & 0x00ff00ff ) << 8 ) | ( ( n & 0xff00ff00 ) >> 8 );
#endif
	n = ( ( n & 0x0f0f0f0f ) << 4 ) | ( ( n & 0xf0f0f0f0 ) >> 4 );
	n = ( ( n & 0x33333333 ) << 2 ) | ( ( n & 0xcccccccc ) >> 2 );
	n = ( ( n & 0x55555555 ) << 1 ) | ( ( n & 0xaaaaaaaa ) >> 1 );
	
	// Account for the available precision and scramble
	n = ( n >> ( 32 - 24 ) ) ^ ( scramble & ~ -( 1 << 24 ) );
	
	return ( float ) n / ( float )( 1U << 24 );
}

// The ith point xi is then computed by
inline idVec2 Hammersley2D( uint i, uint N )
{
	return idVec2( float( i ) / float( N ), RadicalInverse_VdC( i ) );
}

idVec3 ImportanceSampleGGX( const idVec2& Xi, float roughness, const idVec3& N )
{
	float a = roughness * roughness;
	
	// cosinus distributed direction (Z-up or tangent space) from the hammersley point xi
	float Phi = 2 * idMath::PI * Xi.x;
	float cosTheta = sqrt( ( 1 - Xi.y ) / ( 1 + ( a * a - 1 ) * Xi.y ) );
	float sinTheta = sqrt( 1 - cosTheta * cosTheta );
	
	idVec3 H;
	H.x = sinTheta * cos( Phi );
	H.y = sinTheta * sin( Phi );
	H.z = cosTheta;
	
	// rotate from tangent space to world space along N
	idVec3 upVector = abs( N.z ) < 0.999f ? idVec3( 0, 0, 1 ) : idVec3( 1, 0, 0 );
	idVec3 tangentX = upVector.Cross( N );
	tangentX.Normalize();
	idVec3 tangentY = N.Cross( tangentX );
	
	return tangentX * H.x + tangentY * H.y + N * H.z;
}

/*
==================
R_MakeAmbientMap_f

R_MakeAmbientMap_f <basename> [size]

Saves out env/<basename>_amb_ft.tga, etc
==================
*/
void R_MakeAmbientMap_f( const idCmdArgs& args )
{
	idStr fullname;
	const char*	baseName;
	int			i;
	renderView_t	ref;
	viewDef_t	primary;
	int			downSample;
	int			outSize;
	byte*		buffers[6];
	int			width = 0, height = 0;
	
	if( args.Argc() != 2 && args.Argc() != 3 )
	{
		common->Printf( "USAGE: ambientshot <basename> [size]\n" );
		return;
	}
	baseName = args.Argv( 1 );
	
	downSample = 0;
	if( args.Argc() == 3 )
	{
		outSize = atoi( args.Argv( 2 ) );
	}
	else
	{
		outSize = 32;
	}
	
	memset( &cubeAxis, 0, sizeof( cubeAxis ) );
	cubeAxis[0][0][0] = 1;
	cubeAxis[0][1][2] = 1;
	cubeAxis[0][2][1] = 1;
	
	cubeAxis[1][0][0] = -1;
	cubeAxis[1][1][2] = -1;
	cubeAxis[1][2][1] = 1;
	
	cubeAxis[2][0][1] = 1;
	cubeAxis[2][1][0] = -1;
	cubeAxis[2][2][2] = -1;
	
	cubeAxis[3][0][1] = -1;
	cubeAxis[3][1][0] = -1;
	cubeAxis[3][2][2] = 1;
	
	cubeAxis[4][0][2] = 1;
	cubeAxis[4][1][0] = -1;
	cubeAxis[4][2][1] = 1;
	
	cubeAxis[5][0][2] = -1;
	cubeAxis[5][1][0] = 1;
	cubeAxis[5][2][1] = 1;
	
	// read all of the images
	for( i = 0 ; i < 6 ; i++ )
	{
		fullname.Format( "env/%s%s.%s", baseName, envDirection[i], fileExten[TGA] );
		common->Printf( "loading %s\n", fullname.c_str() );
		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );
		R_LoadImage( fullname, &buffers[i], &width, &height, NULL, true );
		if( !buffers[i] )
		{
			common->Printf( "failed.\n" );
			for( i-- ; i >= 0 ; i-- )
			{
				Mem_Free( buffers[i] );
			}
			return;
		}
	}
	
	bool pacifier = true;
	
	// resample with hemispherical blending
	int	samples = 1000;
	
	byte*	outBuffer = ( byte* )_alloca( outSize * outSize * 4 );
	
	for( int map = 0 ; map < 2 ; map++ )
	{
		CommandlineProgressBar progressBar( outSize * outSize * 6 );
		
		int	start = Sys_Milliseconds();
		
		for( i = 0 ; i < 6 ; i++ )
		{
			for( int x = 0 ; x < outSize ; x++ )
			{
				for( int y = 0 ; y < outSize ; y++ )
				{
					idVec3	dir;
					float	total[3];
					
					dir = cubeAxis[i][0] + -( -1 + 2.0 * x / ( outSize - 1 ) ) * cubeAxis[i][1] + -( -1 + 2.0 * y / ( outSize - 1 ) ) * cubeAxis[i][2];
					dir.Normalize();
					total[0] = total[1] = total[2] = 0;
					
					float roughness = map ? 0.1 : 0.95;		// small for specular, almost hemisphere for ambient
					
					for( int s = 0 ; s < samples ; s++ )
					{
						idVec2 Xi = Hammersley2D( s, samples );
						idVec3 test = ImportanceSampleGGX( Xi, roughness, dir );
						
						byte	result[4];
						//test = dir;
						R_SampleCubeMap( test, width, buffers, result );
						total[0] += result[0];
						total[1] += result[1];
						total[2] += result[2];
					}
					outBuffer[( y * outSize + x ) * 4 + 0] = total[0] / samples;
					outBuffer[( y * outSize + x ) * 4 + 1] = total[1] / samples;
					outBuffer[( y * outSize + x ) * 4 + 2] = total[2] / samples;
					outBuffer[( y * outSize + x ) * 4 + 3] = 255;
					
					progressBar.Increment();
				}
			}
			
			if( map == 0 )
			{
				fullname.Format( "env/%s_amb%s.%s", baseName, envDirection[i], fileExten[PNG] );
			}
			else
			{
				fullname.Format( "env/%s_spec%s.%s", baseName, envDirection[i], fileExten[PNG] );
			}
			//common->Printf( "writing %s\n", fullname.c_str() );
			
			const bool captureToImage = false;
			common->UpdateScreen( captureToImage );
			
			//R_WriteTGA( fullname, outBuffer, outSize, outSize, false, "fs_basepath" );
			R_WritePNG( fullname, outBuffer, 4, outSize, outSize, true, "fs_basepath" );
		}
		
		int	end = Sys_Milliseconds();
		
		if( map == 0 )
		{
			common->Printf( "env/%s_amb convolved  in %5.1f seconds\n\n", baseName, ( end - start ) * 0.001f );
		}
		else
		{
			common->Printf( "env/%s_spec convolved  in %5.1f seconds\n\n", baseName, ( end - start ) * 0.001f );
		}
	}
	
	for( i = 0 ; i < 6 ; i++ )
	{
		if( buffers[i] )
		{
			Mem_Free( buffers[i] );
		}
	}
}

void R_TransformCubemap( const char* orgDirection[6], const char* orgDir, const char* destDirection[6], const char* destDir, const char* baseName )
{
	idStr fullname;
	int			i;
	bool        errorInOriginalImages = false;
	byte*		buffers[6];
	int			width = 0, height = 0;
	
	for( i = 0 ; i < 6 ; i++ )
	{
		// read every image images
		fullname.Format( "%s/%s%s.%s", orgDir, baseName, orgDirection[i], fileExten [TGA] );
		common->Printf( "loading %s\n", fullname.c_str() );
		const bool captureToImage = false;
		common->UpdateScreen( captureToImage );
		R_LoadImage( fullname, &buffers[i], &width, &height, NULL, true );
		
		//check if the buffer is troublesome
		if( !buffers[i] )
		{
			common->Printf( "failed.\n" );
			errorInOriginalImages = true;
		}
		else if( width != height )
		{
			common->Printf( "wrong size pal!\n\n\nget your shit together and set the size according to your images!\n\n\ninept programmers are inept!\n" );
			errorInOriginalImages = true; // yeah, but don't just choke on a joke!
		}
		else
		{
			errorInOriginalImages = false;
		}
		
		if( errorInOriginalImages )
		{
			errorInOriginalImages = false;
			for( i-- ; i >= 0 ; i-- )
			{
				Mem_Free( buffers[i] ); // clean up every buffer from this stage down
			}
			
			return;
		}
		
		// apply rotations and flips
		R_ApplyCubeMapTransforms( i, buffers[i], width );
		
		//save the images with the appropiate skybox naming convention
		fullname.Format( "%s/%s/%s%s.%s", destDir, baseName, baseName, destDirection[i], fileExten [TGA] );
		common->Printf( "writing %s\n", fullname.c_str() );
		common->UpdateScreen( false );
		R_WriteTGA( fullname, buffers[i], width, width, false, "fs_basepath" );
	}
	
	for( i = 0 ; i < 6 ; i++ )
	{
		if( buffers[i] )
		{
			Mem_Free( buffers[i] );
		}
	}
}

/*
==================
R_TransformEnvToSkybox_f

R_TransformEnvToSkybox_f <basename>

transforms env textures (of the type px, py, pz, nx, ny, nz)
to skybox textures ( forward, back, left, right, up, down)
==================
*/
void R_TransformEnvToSkybox_f( const idCmdArgs& args )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "USAGE: envToSky <basename>\n" );
		return;
	}
	
	R_TransformCubemap( envDirection, "env", skyDirection, "skybox", args.Argv( 1 ) );
}

/*
==================
R_TransformSkyboxToEnv_f

R_TransformSkyboxToEnv_f <basename>

transforms skybox textures ( forward, back, left, right, up, down)
to env textures (of the type px, py, pz, nx, ny, nz)
==================
*/

void R_TransformSkyboxToEnv_f( const idCmdArgs& args )
{

	if( args.Argc() != 2 )
	{
		common->Printf( "USAGE: skyToEnv <basename>\n" );
		return;
	}
	
	R_TransformCubemap( skyDirection, "skybox", envDirection, "env", args.Argv( 1 ) );
}

//============================================================================


/*
===============
R_SetColorMappings
===============
*/
void R_SetColorMappings()
{
	float b = r_brightness.GetFloat();
	float invg = 1.0f / r_gamma.GetFloat();
	
	float j = 0.0f;
	for( int i = 0; i < 256; i++, j += b )
	{
		int inf = idMath::Ftoi( 0xffff * pow( j / 255.0f, invg ) + 0.5f );
		tr.gammaTable[i] = idMath::ClampInt( 0, 0xFFFF, inf );
	}
	
	GLimp_SetGamma( tr.gammaTable, tr.gammaTable, tr.gammaTable );
}

/*
================
GfxInfo_f
================
*/
void GfxInfo_f( const idCmdArgs& args )
{
	common->Printf( "CPU: %s\n", Sys_GetProcessorString() );
	
	const char* fsstrings[] =
	{
		"windowed",
		"fullscreen"
	};
	
	common->Printf( "\nGL_VENDOR: %s\n", glConfig.vendor_string );
	common->Printf( "GL_RENDERER: %s\n", glConfig.renderer_string );
	common->Printf( "GL_VERSION: %s\n", glConfig.version_string );
	common->Printf( "GL_EXTENSIONS: %s\n", glConfig.extensions_string );
	if( glConfig.wgl_extensions_string )
	{
		common->Printf( "WGL_EXTENSIONS: %s\n", glConfig.wgl_extensions_string );
	}
	common->Printf( "GL_MAX_TEXTURE_SIZE: %d\n", glConfig.maxTextureSize );
	common->Printf( "GL_MAX_TEXTURE_COORDS_ARB: %d\n", glConfig.maxTextureCoords );
	common->Printf( "GL_MAX_TEXTURE_IMAGE_UNITS_ARB: %d\n", glConfig.maxTextureImageUnits );
	
	// print all the display adapters, monitors, and video modes
	//void DumpAllDisplayDevices();
	//DumpAllDisplayDevices();
	
	common->Printf( "\nPIXELFORMAT: color(%d-bits) Z(%d-bit) stencil(%d-bits)\n", glConfig.colorBits, glConfig.depthBits, glConfig.stencilBits );
	common->Printf( "MODE: %d, %d x %d %s hz:", r_vidMode.GetInteger(), renderSystem->GetWidth(), renderSystem->GetHeight(), fsstrings[r_fullscreen.GetBool()] );
	if( glConfig.displayFrequency )
	{
		common->Printf( "%d\n", glConfig.displayFrequency );
	}
	else
	{
		common->Printf( "N/A\n" );
	}
	
	common->Printf( "-------\n" );
	
	// RB begin
#if defined(_WIN32) && !defined(USE_GLES2)
	// WGL_EXT_swap_interval
	typedef BOOL ( WINAPI * PFNWGLSWAPINTERVALEXTPROC )( int interval );
	extern	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT;
	
	if( r_swapInterval.GetInteger() && wglSwapIntervalEXT != NULL )
	{
		common->Printf( "Forcing swapInterval %i\n", r_swapInterval.GetInteger() );
	}
	else
	{
		common->Printf( "swapInterval not forced\n" );
	}
#endif
	// RB end
	
	if( glConfig.stereoPixelFormatAvailable && glConfig.isStereoPixelFormat )
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel format active\n" );
	}
	else if( glConfig.stereoPixelFormatAvailable )
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel available but not selected\n" );
	}
	else
	{
		idLib::Printf( "OpenGl quad buffer stereo pixel format not available\n" );
	}
	
	idLib::Printf( "Stereo mode: " );
	switch( renderSystem->GetStereo3DMode() )
	{
		case STEREO3D_OFF:
			idLib::Printf( "STEREO3D_OFF\n" );
			break;
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			idLib::Printf( "STEREO3D_SIDE_BY_SIDE_COMPRESSED\n" );
			break;
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
			idLib::Printf( "STEREO3D_TOP_AND_BOTTOM_COMPRESSED\n" );
			break;
		case STEREO3D_SIDE_BY_SIDE:
			idLib::Printf( "STEREO3D_SIDE_BY_SIDE\n" );
			break;
		case STEREO3D_HDMI_720:
			idLib::Printf( "STEREO3D_HDMI_720\n" );
			break;
		case STEREO3D_INTERLACED:
			idLib::Printf( "STEREO3D_INTERLACED\n" );
			break;
		case STEREO3D_QUAD_BUFFER:
			idLib::Printf( "STEREO3D_QUAD_BUFFER\n" );
			break;
		default:
			idLib::Printf( "Unknown (%i)\n", renderSystem->GetStereo3DMode() );
			break;
	}
	
	idLib::Printf( "%i multisamples\n", glConfig.multisamples );
	
	common->Printf( "%5.1f cm screen width (%4.1f\" diagonal)\n",
					glConfig.physicalScreenWidthInCentimeters, glConfig.physicalScreenWidthInCentimeters / 2.54f
					* sqrt( ( float )( 16 * 16 + 9 * 9 ) ) / 16.0f );
	extern idCVar r_forceScreenWidthCentimeters;
	if( r_forceScreenWidthCentimeters.GetFloat() )
	{
		common->Printf( "screen size manually forced to %5.1f cm width (%4.1f\" diagonal)\n",
						renderSystem->GetPhysicalScreenWidthInCentimeters(), renderSystem->GetPhysicalScreenWidthInCentimeters() / 2.54f
						* sqrt( ( float )( 16 * 16 + 9 * 9 ) ) / 16.0f );
	}
	
	if( glConfig.gpuSkinningAvailable )
	{
		common->Printf( S_COLOR_GREEN "GPU skeletal animation available\n" );
	}
	else
	{
		common->Printf( S_COLOR_RED "GPU skeletal animation not available (slower CPU path active)\n" );
	}
}

/*
=================
R_VidRestart_f
=================
*/
void R_VidRestart_f( const idCmdArgs& args )
{
	// if OpenGL isn't started, do nothing
	if( !R_IsInitialized() )
	{
		return;
	}
	
	// set the mode without re-initializing the context
	R_SetNewMode( false );
	
#if 0
	bool full = true;
	bool forceWindow = false;
	for( int i = 1 ; i < args.Argc() ; i++ )
	{
		if( idStr::Icmp( args.Argv( i ), "partial" ) == 0 )
		{
			full = false;
			continue;
		}
		if( idStr::Icmp( args.Argv( i ), "windowed" ) == 0 )
		{
			forceWindow = true;
			continue;
		}
	}
	
	// this could take a while, so give them the cursor back ASAP
	Sys_GrabMouseCursor( false );
	
	// dump ambient caches
	renderModelManager->FreeModelVertexCaches();
	
	// free any current world interaction surfaces and vertex caches
	R_FreeDerivedData();
	
	// make sure the defered frees are actually freed
	R_ToggleSmpFrame();
	R_ToggleSmpFrame();
	
	// free the vertex caches so they will be regenerated again
	vertexCache.PurgeAll();
	
	// sound and input are tied to the window we are about to destroy
	
	if( full )
	{
		// free all of our texture numbers
		Sys_ShutdownInput();
		globalImages->PurgeAllImages();
		// free the context and close the window
		GLimp_Shutdown();
		r_initialized = false;
		
		// create the new context and vertex cache
		bool latch = cvarSystem->GetCVarBool( "r_fullscreen" );
		if( forceWindow )
		{
			cvarSystem->SetCVarBool( "r_fullscreen", false );
		}
		R_InitOpenGL();
		cvarSystem->SetCVarBool( "r_fullscreen", latch );
		
		// regenerate all images
		globalImages->ReloadImages( true );
	}
	else
	{
		glimpParms_t parms;
		parms.width = glConfig.nativeScreenWidth;
		parms.height = glConfig.nativeScreenHeight;
		parms.fullScreen = ( forceWindow ) ? false : r_fullscreen.GetInteger();
		parms.displayHz = r_displayRefresh.GetInteger();
		parms.multiSamples = r_multiSamples.GetInteger();
		parms.stereo = false;
		GLimp_SetScreenParms( parms );
	}
	
	
	
	// make sure the regeneration doesn't use anything no longer valid
	tr.viewCount++;
	tr.viewDef = NULL;
	
	// check for problems
	int err = glGetError();
	if( err != GL_NO_ERROR )
	{
		common->Printf( "glGetError() = 0x%x\n", err );
	}
#endif
	
}

/*
=================
R_InitMaterials
=================
*/
void R_InitMaterials()
{
	tr.defaultMaterial = declManager->FindMaterial( "_default", false );
	if( !tr.defaultMaterial )
	{
		common->FatalError( "_default material not found" );
	}
	tr.defaultPointLight = declManager->FindMaterial( "lights/defaultPointLight" );
	tr.defaultProjectedLight = declManager->FindMaterial( "lights/defaultProjectedLight" );
	tr.whiteMaterial = declManager->FindMaterial( "_white" );
	tr.charSetMaterial = declManager->FindMaterial( "textures/bigchars" );
}


/*
=================
R_SizeUp_f

Keybinding command
=================
*/
static void R_SizeUp_f( const idCmdArgs& args )
{
	if( r_screenFraction.GetInteger() + 10 > 100 )
	{
		r_screenFraction.SetInteger( 100 );
	}
	else
	{
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() + 10 );
	}
}


/*
=================
R_SizeDown_f

Keybinding command
=================
*/
static void R_SizeDown_f( const idCmdArgs& args )
{
	if( r_screenFraction.GetInteger() - 10 < 10 )
	{
		r_screenFraction.SetInteger( 10 );
	}
	else
	{
		r_screenFraction.SetInteger( r_screenFraction.GetInteger() - 10 );
	}
}


/*
===============
TouchGui_f

  this is called from the main thread
===============
*/
void R_TouchGui_f( const idCmdArgs& args )
{
	const char*	gui = args.Argv( 1 );
	
	if( !gui[0] )
	{
		common->Printf( "USAGE: touchGui <guiName>\n" );
		return;
	}
	
	common->Printf( "touchGui %s\n", gui );
	const bool captureToImage = false;
	common->UpdateScreen( captureToImage );
	uiManager->Touch( gui );
}

/*
=================
R_InitCvars
=================
*/
void R_InitCvars()
{
	// update latched cvars here
}

/*
=================
R_InitCommands
=================
*/
void R_InitCommands()
{
	cmdSystem->AddCommand( "sizeUp", R_SizeUp_f, CMD_FL_RENDERER, "makes the rendered view larger" );
	cmdSystem->AddCommand( "sizeDown", R_SizeDown_f, CMD_FL_RENDERER, "makes the rendered view smaller" );
	cmdSystem->AddCommand( "reloadGuis", R_ReloadGuis_f, CMD_FL_RENDERER, "reloads guis" );
	cmdSystem->AddCommand( "listGuis", R_ListGuis_f, CMD_FL_RENDERER, "lists guis" );
	cmdSystem->AddCommand( "touchGui", R_TouchGui_f, CMD_FL_RENDERER, "touches a gui" );
	cmdSystem->AddCommand( "screenshot", R_ScreenShot_f, CMD_FL_RENDERER, "takes a screenshot" );
	cmdSystem->AddCommand( "envshot", R_EnvShot_f, CMD_FL_RENDERER, "takes an environment shot" );
	cmdSystem->AddCommand( "makeAmbientMap", R_MakeAmbientMap_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "makes an ambient map" );
	cmdSystem->AddCommand( "envToSky", R_TransformEnvToSkybox_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "transforms environment textures to sky box textures" );
	cmdSystem->AddCommand( "skyToEnv", R_TransformSkyboxToEnv_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "transforms sky box textures to environment textures" );
	cmdSystem->AddCommand( "gfxInfo", GfxInfo_f, CMD_FL_RENDERER, "show graphics info" );
	cmdSystem->AddCommand( "modulateLights", R_ModulateLights_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "modifies shader parms on all lights" );
	cmdSystem->AddCommand( "testImage", R_TestImage_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given image centered on screen", idCmdSystem::ArgCompletion_ImageName );
	cmdSystem->AddCommand( "testVideo", R_TestVideo_f, CMD_FL_RENDERER | CMD_FL_CHEAT, "displays the given cinematic", idCmdSystem::ArgCompletion_VideoName );
	cmdSystem->AddCommand( "reportSurfaceAreas", R_ReportSurfaceAreas_f, CMD_FL_RENDERER, "lists all used materials sorted by surface area" );
	cmdSystem->AddCommand( "showInteractionMemory", R_ShowInteractionMemory_f, CMD_FL_RENDERER, "shows memory used by interactions" );
	cmdSystem->AddCommand( "vid_restart", R_VidRestart_f, CMD_FL_RENDERER, "restarts renderSystem" );
	cmdSystem->AddCommand( "listRenderEntityDefs", R_ListRenderEntityDefs_f, CMD_FL_RENDERER, "lists the entity defs" );
	cmdSystem->AddCommand( "listRenderLightDefs", R_ListRenderLightDefs_f, CMD_FL_RENDERER, "lists the light defs" );
	cmdSystem->AddCommand( "listModes", R_ListModes_f, CMD_FL_RENDERER, "lists all video modes" );
	cmdSystem->AddCommand( "reloadSurface", R_ReloadSurface_f, CMD_FL_RENDERER, "reloads the decl and images for selected surface" );
}


/*
=============
R_MakeFullScreenTris
=============
*/
static srfTriangles_t* R_MakeFullScreenTris()
{
	// copy verts and indexes
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );
	
	tri->numIndexes = 6;
	tri->numVerts = 4;
	
	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );
	
	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );
	
	idDrawVert* verts = tri->verts;
	
	triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );
	
	verts[0].xyz[0] = -1.0f;
	verts[0].xyz[1] = 1.0f;
	verts[0].SetTexCoord( 0.0f, 1.0f );
	
	verts[1].xyz[0] = 1.0f;
	verts[1].xyz[1] = 1.0f;
	verts[1].SetTexCoord( 1.0f, 1.0f );
	
	verts[2].xyz[0] = 1.0f;
	verts[2].xyz[1] = -1.0f;
	verts[2].SetTexCoord( 1.0f, 0.0f );
	
	verts[3].xyz[0] = -1.0f;
	verts[3].xyz[1] = -1.0f;
	verts[3].SetTexCoord( 0.0f, 0.0f );
	
	for( int i = 0 ; i < 4 ; i++ )
	{
		verts[i].SetColor( 0xffffffff );
	}
	
	
	return tri;
}

/*
=============
R_MakeZeroOneCubeTris
=============
*/
static srfTriangles_t* R_MakeZeroOneCubeTris()
{
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );
	
	tri->numVerts = 8;
	tri->numIndexes = 36;
	
	const int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	const int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );
	
	const int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	const int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );
	
	idDrawVert* verts = tri->verts;
	
	const float low = 0.0f;
	const float high = 1.0f;
	
	idVec3 center( 0.0f );
	idVec3 mx( low, 0.0f, 0.0f );
	idVec3 px( high, 0.0f, 0.0f );
	idVec3 my( 0.0f,  low, 0.0f );
	idVec3 py( 0.0f, high, 0.0f );
	idVec3 mz( 0.0f, 0.0f,  low );
	idVec3 pz( 0.0f, 0.0f, high );
	
	verts[0].xyz = center + mx + my + mz;
	verts[1].xyz = center + px + my + mz;
	verts[2].xyz = center + px + py + mz;
	verts[3].xyz = center + mx + py + mz;
	verts[4].xyz = center + mx + my + pz;
	verts[5].xyz = center + px + my + pz;
	verts[6].xyz = center + px + py + pz;
	verts[7].xyz = center + mx + py + pz;
	
	// bottom
	tri->indexes[ 0 * 3 + 0] = 2;
	tri->indexes[ 0 * 3 + 1] = 3;
	tri->indexes[ 0 * 3 + 2] = 0;
	tri->indexes[ 1 * 3 + 0] = 1;
	tri->indexes[ 1 * 3 + 1] = 2;
	tri->indexes[ 1 * 3 + 2] = 0;
	// back
	tri->indexes[ 2 * 3 + 0] = 5;
	tri->indexes[ 2 * 3 + 1] = 1;
	tri->indexes[ 2 * 3 + 2] = 0;
	tri->indexes[ 3 * 3 + 0] = 4;
	tri->indexes[ 3 * 3 + 1] = 5;
	tri->indexes[ 3 * 3 + 2] = 0;
	// left
	tri->indexes[ 4 * 3 + 0] = 7;
	tri->indexes[ 4 * 3 + 1] = 4;
	tri->indexes[ 4 * 3 + 2] = 0;
	tri->indexes[ 5 * 3 + 0] = 3;
	tri->indexes[ 5 * 3 + 1] = 7;
	tri->indexes[ 5 * 3 + 2] = 0;
	// right
	tri->indexes[ 6 * 3 + 0] = 1;
	tri->indexes[ 6 * 3 + 1] = 5;
	tri->indexes[ 6 * 3 + 2] = 6;
	tri->indexes[ 7 * 3 + 0] = 2;
	tri->indexes[ 7 * 3 + 1] = 1;
	tri->indexes[ 7 * 3 + 2] = 6;
	// front
	tri->indexes[ 8 * 3 + 0] = 3;
	tri->indexes[ 8 * 3 + 1] = 2;
	tri->indexes[ 8 * 3 + 2] = 6;
	tri->indexes[ 9 * 3 + 0] = 7;
	tri->indexes[ 9 * 3 + 1] = 3;
	tri->indexes[ 9 * 3 + 2] = 6;
	// top
	tri->indexes[10 * 3 + 0] = 4;
	tri->indexes[10 * 3 + 1] = 7;
	tri->indexes[10 * 3 + 2] = 6;
	tri->indexes[11 * 3 + 0] = 5;
	tri->indexes[11 * 3 + 1] = 4;
	tri->indexes[11 * 3 + 2] = 6;
	
	for( int i = 0 ; i < 4 ; i++ )
	{
		verts[i].SetColor( 0xffffffff );
	}
	
	return tri;
}

/*
================
R_MakeTestImageTriangles

Initializes the Test Image Triangles
================
*/
srfTriangles_t* R_MakeTestImageTriangles()
{
	srfTriangles_t* tri = ( srfTriangles_t* )Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );
	
	tri->numIndexes = 6;
	tri->numVerts = 4;
	
	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = ( triIndex_t* )Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );
	
	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = ( idDrawVert* )Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );
	
	ALIGNTYPE16 triIndex_t tempIndexes[6] = { 3, 0, 2, 2, 0, 1 };
	memcpy( tri->indexes, tempIndexes, indexSize );
	
	idDrawVert* tempVerts = tri->verts;
	tempVerts[0].xyz[0] = 0.0f;
	tempVerts[0].xyz[1] = 0.0f;
	tempVerts[0].xyz[2] = 0;
	tempVerts[0].SetTexCoord( 0.0, 0.0f );
	
	tempVerts[1].xyz[0] = 1.0f;
	tempVerts[1].xyz[1] = 0.0f;
	tempVerts[1].xyz[2] = 0;
	tempVerts[1].SetTexCoord( 1.0f, 0.0f );
	
	tempVerts[2].xyz[0] = 1.0f;
	tempVerts[2].xyz[1] = 1.0f;
	tempVerts[2].xyz[2] = 0;
	tempVerts[2].SetTexCoord( 1.0f, 1.0f );
	
	tempVerts[3].xyz[0] = 0.0f;
	tempVerts[3].xyz[1] = 1.0f;
	tempVerts[3].xyz[2] = 0;
	tempVerts[3].SetTexCoord( 0.0f, 1.0f );
	
	for( int i = 0; i < 4; i++ )
	{
		tempVerts[i].SetColor( 0xFFFFFFFF );
	}
	return tri;
}

/*
=============
idRenderSystemLocal::idRenderSystemLocal
=============
*/
idRenderSystemLocal::idRenderSystemLocal() :
	unitSquareTriangles( NULL ),
	zeroOneCubeTriangles( NULL ),
	testImageTriangles( NULL )
{
	Clear();
}

/*
=============
idRenderSystemLocal::~idRenderSystemLocal
=============
*/
idRenderSystemLocal::~idRenderSystemLocal()
{
}

/*
===============
idRenderSystemLocal::Init
===============
*/
void idRenderSystemLocal::Init()
{

	common->Printf( "------- Initializing renderSystem --------\n" );
	
	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
	// we used to memset tr, but now that it is a class, we can't, so
	// there may be other state we need to reset
	
	ambientLightVector[0] = 0.5f;
	ambientLightVector[1] = 0.5f - 0.385f;
	ambientLightVector[2] = 0.8925f;
	ambientLightVector[3] = 1.0f;
	
	backend.Init();
	
	R_InitCvars();
	
	R_InitCommands();
	
	guiModel = new( TAG_RENDER ) idGuiModel;
	guiModel->Clear();
	tr_guiModel = guiModel;	// for DeviceContext fast path
	
	UpdateStereo3DMode();
	
	globalImages->Init();
	
	// RB begin
	Framebuffer::Init();
	// RB end
	
	idCinematic::InitCinematic();
	
	// build brightness translation tables
	R_SetColorMappings();
	
	R_InitMaterials();
	
	renderModelManager->Init();
	
	// set the identity space
	identitySpace.modelMatrix[0 * 4 + 0] = 1.0f;
	identitySpace.modelMatrix[1 * 4 + 1] = 1.0f;
	identitySpace.modelMatrix[2 * 4 + 2] = 1.0f;
	
	// make sure the tr.unitSquareTriangles data is current in the vertex / index cache
	if( unitSquareTriangles == NULL )
	{
		unitSquareTriangles = R_MakeFullScreenTris();
	}
	// make sure the tr.zeroOneCubeTriangles data is current in the vertex / index cache
	if( zeroOneCubeTriangles == NULL )
	{
		zeroOneCubeTriangles = R_MakeZeroOneCubeTris();
	}
	// make sure the tr.testImageTriangles data is current in the vertex / index cache
	if( testImageTriangles == NULL )
	{
		testImageTriangles = R_MakeTestImageTriangles();
	}
	
	frontEndJobList = parallelJobManager->AllocJobList( JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 2048, 0, NULL );
	
	// make sure the command buffers are ready to accept the first screen update
	SwapCommandBuffers( NULL, NULL, NULL, NULL );
	
	common->Printf( "renderSystem initialized.\n" );
	common->Printf( "--------------------------------------\n" );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void idRenderSystemLocal::Shutdown()
{
	common->Printf( "idRenderSystem::Shutdown()\n" );
	
	fonts.DeleteContents();
	
	if( R_IsInitialized() )
	{
		globalImages->PurgeAllImages();
	}
	
	renderModelManager->Shutdown();
	
	idCinematic::ShutdownCinematic();
	
	globalImages->Shutdown();
	
	// RB begin
	Framebuffer::Shutdown();
	// RB end
	
	// free frame memory
	R_ShutdownFrameData();
	
	UnbindBufferObjects();
	
	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();
	
	RB_ShutdownDebugTools();
	
	delete guiModel;
	
	parallelJobManager->FreeJobList( frontEndJobList );
	
	Clear();
	
	ShutdownOpenGL();
}

/*
===============
idRenderSystemLocal::Clear
===============
*/
void idRenderSystemLocal::Clear()
{
	//registered = false;
	frameCount = 0;
	viewCount = 0;
	frameShaderTime = 0.0f;
	ambientLightVector.Zero();
	worlds.Clear();
	primaryWorld = NULL;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = NULL;
	defaultMaterial = NULL;
	testImage = NULL;
	ambientCubeImage = NULL;
	viewDef = NULL;
	memset( &pc, 0, sizeof( pc ) );
	memset( &identitySpace, 0, sizeof( identitySpace ) );
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	currentColorNativeBytesOrder = 0xFFFFFFFF;
	currentGLState = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	memset( gammaTable, 0, sizeof( gammaTable ) );
	takingScreenshot = false;
	
	if( unitSquareTriangles != NULL )
	{
		Mem_Free( unitSquareTriangles );
		unitSquareTriangles = NULL;
	}
	
	if( zeroOneCubeTriangles != NULL )
	{
		Mem_Free( zeroOneCubeTriangles );
		zeroOneCubeTriangles = NULL;
	}
	
	if( testImageTriangles != NULL )
	{
		Mem_Free( testImageTriangles );
		testImageTriangles = NULL;
	}
	
	frontEndJobList = NULL;
}


/*
========================
idRenderSystemLocal::ResetGuiModels
========================
*/
void idRenderSystemLocal::ResetGuiModels()
{
	delete guiModel;
	guiModel = new( TAG_RENDER ) idGuiModel;
	guiModel->Clear();
	guiModel->BeginFrame();
	tr_guiModel = guiModel;	// for DeviceContext fast path
}

/*
========================
idRenderSystemLocal::BeginLevelLoad
========================
*/
void idRenderSystemLocal::BeginLevelLoad()
{
	globalImages->BeginLevelLoad();
	renderModelManager->BeginLevelLoad();
	
	// Re-Initialize the Default Materials if needed.
	R_InitMaterials();
}

/*
========================
idRenderSystemLocal::LoadLevelImages
========================
*/
void idRenderSystemLocal::LoadLevelImages()
{
	globalImages->LoadLevelImages( false );
}

/*
========================
idRenderSystemLocal::Preload
========================
*/
void idRenderSystemLocal::Preload( const idPreloadManifest& manifest, const char* mapName )
{
	globalImages->Preload( manifest, true );
	uiManager->Preload( mapName );
	renderModelManager->Preload( manifest );
}

/*
========================
idRenderSystemLocal::EndLevelLoad
========================
*/
void idRenderSystemLocal::EndLevelLoad()
{
	renderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
}

/*
========================
idRenderSystemLocal::BeginAutomaticBackgroundSwaps
========================
*/
void idRenderSystemLocal::BeginAutomaticBackgroundSwaps( autoRenderIconType_t icon )
{
}

/*
========================
idRenderSystemLocal::EndAutomaticBackgroundSwaps
========================
*/
void idRenderSystemLocal::EndAutomaticBackgroundSwaps()
{
}

/*
========================
idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning
========================
*/
bool idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning( autoRenderIconType_t* icon ) const
{
	return false;
}

/*
============
idRenderSystemLocal::RegisterFont
============
*/
idFont* idRenderSystemLocal::RegisterFont( const char* fontName )
{

	idStrStatic< MAX_OSPATH > baseFontName = fontName;
	baseFontName.Replace( "fonts/", "" );
	for( int i = 0; i < fonts.Num(); i++ )
	{
		if( idStr::Icmp( fonts[i]->GetName(), baseFontName ) == 0 )
		{
			fonts[i]->Touch();
			return fonts[i];
		}
	}
	idFont* newFont = new( TAG_FONT ) idFont( baseFontName );
	fonts.Append( newFont );
	return newFont;
}

/*
========================
idRenderSystemLocal::ResetFonts
========================
*/
void idRenderSystemLocal::ResetFonts()
{
	fonts.DeleteContents( true );
}
/*
========================
idRenderSystemLocal::InitOpenGL
========================
*/
void idRenderSystemLocal::InitOpenGL()
{
	// if OpenGL isn't started, start it now
	if( !R_IsInitialized() )
	{
		R_InitOpenGL();
		
		// Reloading images here causes the rendertargets to get deleted. Figure out how to handle this properly on 360
		globalImages->ReloadImages( true );
		
		int err = glGetError();
		if( err != GL_NO_ERROR )
		{
			common->Printf( "glGetError() = 0x%x\n", err );
		}
	}
}

/*
========================
idRenderSystemLocal::ShutdownOpenGL
========================
*/
void idRenderSystemLocal::ShutdownOpenGL()
{
	// free the context and close the window
	R_ShutdownFrameData();
	GLimp_Shutdown();
	r_initialized = false;
}

/*
========================
idRenderSystemLocal::IsOpenGLRunning
========================
*/
bool idRenderSystemLocal::IsOpenGLRunning() const
{
	return R_IsInitialized();
}

/*
========================
idRenderSystemLocal::IsFullScreen
========================
*/
bool idRenderSystemLocal::IsFullScreen() const
{
	return glConfig.isFullscreen != 0;
}

/*
========================
idRenderSystemLocal::GetWidth
========================
*/
int idRenderSystemLocal::GetWidth() const
{
	if( glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE || glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE_COMPRESSED )
	{
		return glConfig.nativeScreenWidth >> 1;
	}
	return glConfig.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetHeight
========================
*/
int idRenderSystemLocal::GetHeight() const
{
	if( glConfig.stereo3Dmode == STEREO3D_HDMI_720 )
	{
		return 720;
	}
	extern idCVar stereoRender_warp;
	if( glConfig.stereo3Dmode == STEREO3D_SIDE_BY_SIDE && stereoRender_warp.GetBool() )
	{
		// for the Rift, render a square aspect view that will be symetric for the optics
		return glConfig.nativeScreenWidth >> 1;
	}
	if( glConfig.stereo3Dmode == STEREO3D_INTERLACED || glConfig.stereo3Dmode == STEREO3D_TOP_AND_BOTTOM_COMPRESSED )
	{
		return glConfig.nativeScreenHeight >> 1;
	}
	return glConfig.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::GetVirtualWidth
========================
*/
int idRenderSystemLocal::GetVirtualWidth() const
{
	if( r_useVirtualScreenResolution.GetBool() )
	{
		return SCREEN_WIDTH;
	}
	return glConfig.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetVirtualHeight
========================
*/
int idRenderSystemLocal::GetVirtualHeight() const
{
	if( r_useVirtualScreenResolution.GetBool() )
	{
		return SCREEN_HEIGHT;
	}
	return glConfig.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::IsGpuSkinningSupported
========================
*/
bool idRenderSystemLocal::IsGpuSkinningSupported() const
{
	return glConfig.gpuSkinningAvailable;
}

/*
========================
idRenderSystemLocal::GetStereo3DMode
========================
*/
stereo3DMode_t idRenderSystemLocal::GetStereo3DMode() const
{
	return glConfig.stereo3Dmode;
}

/*
========================
idRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
bool idRenderSystemLocal::IsStereoScopicRenderingSupported() const
{
	return true;
}

/*
========================
idRenderSystemLocal::HasQuadBufferSupport
========================
*/
bool idRenderSystemLocal::HasQuadBufferSupport() const
{
	return glConfig.stereoPixelFormatAvailable;
}

/*
========================
idRenderSystemLocal::UpdateStereo3DMode
========================
*/
void idRenderSystemLocal::UpdateStereo3DMode()
{
	if( glConfig.nativeScreenWidth == 1280 && glConfig.nativeScreenHeight == 1470 )
	{
		glConfig.stereo3Dmode = STEREO3D_HDMI_720;
	}
	else
	{
		glConfig.stereo3Dmode = GetStereoScopicRenderingMode();
	}
}

/*
========================
idRenderSystemLocal::GetStereoScopicRenderingMode
========================
*/
stereo3DMode_t idRenderSystemLocal::GetStereoScopicRenderingMode() const
{
	return ( !IsStereoScopicRenderingSupported() ) ? STEREO3D_OFF : ( stereo3DMode_t )stereoRender_enable.GetInteger();
}

/*
========================
idRenderSystemLocal::IsStereoScopicRenderingSupported
========================
*/
void idRenderSystemLocal::EnableStereoScopicRendering( const stereo3DMode_t mode ) const
{
	stereoRender_enable.SetInteger( mode );
}

/*
========================
idRenderSystemLocal::GetPixelAspect
========================
*/
float idRenderSystemLocal::GetPixelAspect() const
{
	switch( glConfig.stereo3Dmode )
	{
		case STEREO3D_SIDE_BY_SIDE_COMPRESSED:
			return glConfig.pixelAspect * 2.0f;
		case STEREO3D_TOP_AND_BOTTOM_COMPRESSED:
		case STEREO3D_INTERLACED:
			return glConfig.pixelAspect * 0.5f;
		default:
			return glConfig.pixelAspect;
	}
}

/*
================
idRenderSystemLocal::CaptureRenderToImage
================
*/
void idRenderSystemLocal::CaptureRenderToImage( const char* imageName, bool clearColorAfterCopy )
{
	if( !R_IsInitialized() )
	{
		return;
	}
	guiModel->EmitFullScreen();
	guiModel->Clear();
	
	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_CAPTURE_RENDER );
		common->WriteDemo()->WriteHashString( imageName );
		
		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_CAPTURE_RENDER: %s\n", imageName );
		}
	}
	idImage*	 image = globalImages->GetImage( imageName );
	if( image == NULL )
	{
		image = globalImages->AllocImage( imageName );
	}
	
	idScreenRect& rc = renderCrops[currentRenderCrop];
	
	copyRenderCommand_t* cmd = ( copyRenderCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_COPY_RENDER;
	cmd->x = rc.x1;
	cmd->y = rc.y1;
	cmd->imageWidth = rc.GetWidth();
	cmd->imageHeight = rc.GetHeight();
	cmd->image = image;
	cmd->clearColorAfterCopy = clearColorAfterCopy;
	
	guiModel->Clear();
}

/*
==============
idRenderSystemLocal::CaptureRenderToFile
==============
*/
void idRenderSystemLocal::CaptureRenderToFile( const char* fileName, bool fixAlpha )
{
	if( !R_IsInitialized() )
	{
		return;
	}
	
	idScreenRect& rc = renderCrops[currentRenderCrop];
	
	guiModel->EmitFullScreen();
	guiModel->Clear();
	RenderCommandBuffers( frameData->cmdHead );
	
	glReadBuffer( GL_BACK );
	
	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc.GetWidth() + 3 ) * rc.GetHeight();
	byte* data = ( byte* )R_StaticAlloc( c * 3 );
	
	glReadPixels( rc.x1, rc.y1, rc.GetWidth(), rc.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, data );
	
	byte* data2 = ( byte* )R_StaticAlloc( c * 4 );
	
	for( int i = 0 ; i < c ; i++ )
	{
		data2[ i * 4 ] = data[ i * 3 ];
		data2[ i * 4 + 1 ] = data[ i * 3 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 3 + 2 ];
		data2[ i * 4 + 3 ] = 0xff;
	}
	
	R_WriteTGA( fileName, data2, rc.GetWidth(), rc.GetHeight(), true );
	
	R_StaticFree( data );
	R_StaticFree( data2 );
}
/*
========================
idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters

This is used to calculate stereoscopic screen offset for a given interocular distance.
========================
*/
idCVar	r_forceScreenWidthCentimeters( "r_forceScreenWidthCentimeters", "0", CVAR_RENDERER | CVAR_ARCHIVE, "Override screen width returned by hardware" );
float idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters() const
{
	if( r_forceScreenWidthCentimeters.GetFloat() > 0 )
	{
		return r_forceScreenWidthCentimeters.GetFloat();
	}
	return glConfig.physicalScreenWidthInCentimeters;
}
