/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 
Copyright (C) 2016-2017 Dustin Land

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
#include "../RenderCommon.h"
#include "../Font.h"

//idRenderSystemLocal	tr;
//idRenderSystem * renderSystem = &tr;

// DeviceContext bypasses RenderSystem to work directly with this
idGuiModel * tr_guiModel;

//#define TRACK_FRAME_ALLOCS

//static const unsigned int FRAME_ALLOC_ALIGNMENT = 128;
//static const unsigned int MAX_FRAME_MEMORY = 64 * 1024 * 1024;	// larger so that we can noclip on PC for dev purposes

#if defined( TRACK_FRAME_ALLOCS )
idSysInterlockedInteger frameAllocTypeCount[ FRAME_ALLOC_MAX ];
int frameHighWaterTypeCount[ FRAME_ALLOC_MAX ];
#endif

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

/*
=============
R_MakeFullScreenTris
=============
*/
static srfTriangles_t * R_MakeFullScreenTris() {
	// copy verts and indexes
	srfTriangles_t * tri = (srfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (idDrawVert *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	idDrawVert * verts = tri->verts;

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

	for ( int i = 0 ; i < 4 ; i++ ) {
		verts[i].SetColor( 0xffffffff );
	}

	return tri;
}

/*
=============
R_MakeZeroOneCubeTris
=============
*/
static srfTriangles_t * R_MakeZeroOneCubeTris() {
	srfTriangles_t * tri = (srfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numVerts = 8;
	tri->numIndexes = 36;

	const int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	const int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	const int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	const int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (idDrawVert *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

	idDrawVert * verts = tri->verts;

	const float low = 0.0f;
	const float high = 1.0f;

	idVec3 center( 0.0f );
	idVec3 mx(  low, 0.0f, 0.0f );
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
	tri->indexes[ 0*3+0] = 2;
	tri->indexes[ 0*3+1] = 3;
	tri->indexes[ 0*3+2] = 0;
	tri->indexes[ 1*3+0] = 1;
	tri->indexes[ 1*3+1] = 2;
	tri->indexes[ 1*3+2] = 0;
	// back
	tri->indexes[ 2*3+0] = 5;
	tri->indexes[ 2*3+1] = 1;
	tri->indexes[ 2*3+2] = 0;
	tri->indexes[ 3*3+0] = 4;
	tri->indexes[ 3*3+1] = 5;
	tri->indexes[ 3*3+2] = 0;
	// left
	tri->indexes[ 4*3+0] = 7;
	tri->indexes[ 4*3+1] = 4;
	tri->indexes[ 4*3+2] = 0;
	tri->indexes[ 5*3+0] = 3;
	tri->indexes[ 5*3+1] = 7;
	tri->indexes[ 5*3+2] = 0;
	// right
	tri->indexes[ 6*3+0] = 1;
	tri->indexes[ 6*3+1] = 5;
	tri->indexes[ 6*3+2] = 6;
	tri->indexes[ 7*3+0] = 2;
	tri->indexes[ 7*3+1] = 1;
	tri->indexes[ 7*3+2] = 6;
	// front
	tri->indexes[ 8*3+0] = 3;
	tri->indexes[ 8*3+1] = 2;
	tri->indexes[ 8*3+2] = 6;
	tri->indexes[ 9*3+0] = 7;
	tri->indexes[ 9*3+1] = 3;
	tri->indexes[ 9*3+2] = 6;
	// top
	tri->indexes[10*3+0] = 4;
	tri->indexes[10*3+1] = 7;
	tri->indexes[10*3+2] = 6;
	tri->indexes[11*3+0] = 5;
	tri->indexes[11*3+1] = 4;
	tri->indexes[11*3+2] = 6;

	for ( int i = 0 ; i < 4 ; i++ ) {
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
srfTriangles_t* R_MakeTestImageTriangles() {
	srfTriangles_t * tri = (srfTriangles_t *)Mem_ClearedAlloc( sizeof( *tri ), TAG_RENDER_TOOLS );

	tri->numIndexes = 6;
	tri->numVerts = 4;

	int indexSize = tri->numIndexes * sizeof( tri->indexes[0] );
	int allocatedIndexBytes = ALIGN( indexSize, 16 );
	tri->indexes = (triIndex_t *)Mem_Alloc( allocatedIndexBytes, TAG_RENDER_TOOLS );

	int vertexSize = tri->numVerts * sizeof( tri->verts[0] );
	int allocatedVertexBytes =  ALIGN( vertexSize, 16 );
	tri->verts = (idDrawVert *)Mem_ClearedAlloc( allocatedVertexBytes, TAG_RENDER_TOOLS );

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

	for ( int i = 0; i < 4; i++ ) {
		tempVerts[i].SetColor( 0xFFFFFFFF );
	}
	return tri;
}

/*
=============================
SetNewMode

r_fullScreen -1		borderless window at exact desktop coordinates
r_fullScreen 0		bordered window at exact desktop coordinates
r_fullScreen 1		fullscreen on monitor 1 at r_vidMode
r_fullScreen 2		fullscreen on monitor 2 at r_vidMode
...

r_vidMode -1		use r_customWidth / r_customHeight, even if they don't appear on the mode list
r_vidMode 0			use first mode returned by EnumDisplaySettings()
r_vidMode 1			use second mode returned by EnumDisplaySettings()
...

r_displayRefresh 0	don't specify refresh
r_displayRefresh 70	specify 70 hz, etc
=============================
*/
bool R_GetModeListForDisplay( const int requestedDisplayNum, idList<vidMode_t> & modeList );
bool SetScreenParms( gfxImpParms_t parms );
static void SetNewMode() {
	// try up to three different configurations

	for ( int i = 0 ; i < 3 ; i++ ) {
		gfxImpParms_t parms;

		if ( r_fullscreen.GetInteger() <= 0 ) {
			// use explicit position / size for window
			parms.x = r_windowX.GetInteger();
			parms.y = r_windowY.GetInteger();
			parms.width = r_windowWidth.GetInteger();
			parms.height = r_windowHeight.GetInteger();
			// may still be -1 to force a borderless window
			parms.fullScreen = r_fullscreen.GetInteger();
			parms.displayHz = 0;		// ignored
		} else {
			// get the mode list for this monitor
			idList<vidMode_t> modeList;
			if ( !R_GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList ) ) {
				idLib::Printf( "r_fullscreen reset from %i to 1 because mode list failed.", r_fullscreen.GetInteger() );
				r_fullscreen.SetInteger( 1 );
				R_GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList );
			}
			if ( modeList.Num() < 1 ) {
				idLib::Printf( "Going to safe mode because mode list failed." );
				goto safeMode;
			}

			parms.x = 0;		// ignored
			parms.y = 0;		// ignored
			parms.fullScreen = r_fullscreen.GetInteger();

			// set the parameters we are trying
			if ( r_vidMode.GetInteger() < 0 ) {
				// try forcing a specific mode, even if it isn't on the list
				parms.width = r_customWidth.GetInteger();
				parms.height = r_customHeight.GetInteger();
				parms.displayHz = r_displayRefresh.GetInteger();
			} else {
				if ( r_vidMode.GetInteger() > modeList.Num() ) {
					idLib::Printf( "r_vidMode reset from %i to 0.\n", r_vidMode.GetInteger() );
					r_vidMode.SetInteger( 0 );
				}

				parms.width = modeList[ r_vidMode.GetInteger() ].width;
				parms.height = modeList[ r_vidMode.GetInteger() ].height;
				parms.displayHz = modeList[ r_vidMode.GetInteger() ].displayHz;
			}
		}

		parms.multiSamples = r_multiSamples.GetInteger();

		// rebuild the window
		if ( SetScreenParms( parms ) ) {
			// it worked
			break;
		}

		if ( i == 2 ) {
			idLib::FatalError( "Unable to initialize new mode." );
		}

		if ( i == 0 ) {
			// same settings, no stereo
			continue;
		}

safeMode:
		// if we failed, set everything back to "safe mode"
		// and try again
		r_vidMode.SetInteger( 0 );
		r_fullscreen.SetInteger( 1 );
		r_displayRefresh.SetInteger( 0 );
		r_multiSamples.SetInteger( 0 );
	}
}

/*
=============
idRenderSystemLocal::idRenderSystemLocal
=============
*/
idRenderSystemLocal::idRenderSystemLocal() :
	m_bInitialized( false ),
	unitSquareTriangles( NULL ),
	zeroOneCubeTriangles( NULL ),
	testImageTriangles( NULL )
	//m_frameData( NULL ),
	//m_smpFrame( 0 ) 
{
	Clear();
}

/*
=============
idRenderSystemLocal::~idRenderSystemLocal
=============
*/
idRenderSystemLocal::~idRenderSystemLocal() {

}

/*
===============
idRenderSystemLocal::Init
===============
*/
void idRenderSystemLocal::Init() {	
	if ( m_bInitialized ) {
		idLib::Warning( "RenderSystem already initialized." );
		return;
	}

	idLib::Printf( "------- Initializing renderSystem --------\n" );

	// allocate the frame data, which may be more if smp is enabled
	R_InitFrameData();

	// Start Renderer Backend ( API specific )
	backend.Init();

	// clear all our internal state
	viewCount = 1;		// so cleared structures never match viewCount
	// we used to memset tr, but now that it is a class, we can't, so
	// there may be other state we need to reset

	guiModel = new (TAG_RENDER) idGuiModel;
	guiModel->Clear();
	tr_guiModel = guiModel;	// for DeviceContext fast path

	globalImages->Init();

	idCinematic::InitCinematic();

	InitMaterials();

	renderModelManager->Init();

	// make sure the m_unitSquareTriangles data is current in the vertex / index cache
	if ( unitSquareTriangles == NULL ) 
	{
		unitSquareTriangles = R_MakeFullScreenTris();
	}
	// make sure the zeroOneCubeTriangles data is current in the vertex / index cache
	if ( zeroOneCubeTriangles == NULL ) 
	{
		zeroOneCubeTriangles = R_MakeZeroOneCubeTris();
	}
	// make sure the testImageTriangles data is current in the vertex / index cache
	if ( testImageTriangles == NULL )  
	{
		testImageTriangles = R_MakeTestImageTriangles();
	}

	frontEndJobList = parallelJobManager->AllocJobList( JOBLIST_RENDERER_FRONTEND, JOBLIST_PRIORITY_MEDIUM, 2048, 0, NULL );

	m_bInitialized = true;

	// make sure the command buffers are ready to accept the first screen update
	SwapCommandBuffers( NULL, NULL, NULL, NULL );

	idLib::Printf( "renderSystem initialized.\n" );
	idLib::Printf( "--------------------------------------\n" );
}

/*
===============
idRenderSystemLocal::Shutdown
===============
*/
void RB_ShutdownDebugTools();
void idRenderSystemLocal::Shutdown() {	
	idLib::Printf( "idRenderSystem::Shutdown()\n" );

	fonts.DeleteContents();

	if ( m_bInitialized ) {
		globalImages->PurgeAllImages();
	}

	renderModelManager->Shutdown();

	idCinematic::ShutdownCinematic();

	globalImages->Shutdown();

	// free frame memory
	///ShutdownFrameData();

	UnbindBufferObjects();

	// free the vertex cache, which should have nothing allocated now
	vertexCache.Shutdown();

	RB_ShutdownDebugTools();

	delete guiModel;
	guiModel = NULL;

	parallelJobManager->FreeJobList( frontEndJobList );

	backend.Shutdown();

	Clear();

	// free the context and close the window
	//ShutdownFrameData();
	
	m_bInitialized = false;
}

/*
=================
idRenderSystemLocal::VidRestart
=================
*/
void idRenderSystemLocal::VidRestart() {
	if ( !m_bInitialized ) {
		return;
	}

	// set the mode without re-initializing the context
	SetNewMode();

#if 0
	// this could take a while, so give them the cursor back ASAP
	Sys_GrabMouseCursor( false );

	// dump ambient caches
	renderModelManager->FreeModelVertexCaches();

	// free any current world interaction surfaces and vertex caches
	FreeWorldDerivedData();

	// make sure the defered frees are actually freed
	ToggleSmpFrame();
	ToggleSmpFrame();

	// free the vertex caches so they will be regenerated again
	vertexCache.PurgeAll();

	// sound and input are tied to the window we are about to destroy

	// free all of our texture numbers
	Sys_ShutdownInput();
	globalImages->PurgeAllImages();
	// free the context and close the window
	Shutdown();

	// create the new context and vertex cache
	Init();

	// regenerate all images
	globalImages->ReloadImages( true );

	// make sure the regeneration doesn't use anything no longer valid
	viewCount++;
	viewDef = NULL;
#endif
}

/*
=================
idRenderSystemLocal::InitMaterials
=================
*/
void idRenderSystemLocal::InitMaterials() {
	defaultMaterial = declManager->FindMaterial( "_default", false );
	if ( !defaultMaterial ) {
		common->FatalError( "_default material not found" );
	}
	defaultPointLight = declManager->FindMaterial( "lights/defaultPointLight" );
	defaultProjectedLight = declManager->FindMaterial( "lights/defaultProjectedLight" );
	whiteMaterial = declManager->FindMaterial( "_white" );
	charSetMaterial = declManager->FindMaterial( "textures/bigchars" );
}

/*
===============
idRenderSystemLocal::Clear
===============
*/
void idRenderSystemLocal::Clear() {
	frameCount = 0;
	viewCount = 0;
	worlds.Clear();
	primaryWorld = NULL;
	memset( &primaryRenderView, 0, sizeof( primaryRenderView ) );
	primaryView = NULL;
	defaultMaterial = NULL;
	viewDef = NULL;
	memset( &pc, 0, sizeof( pc ) );
	memset( renderCrops, 0, sizeof( renderCrops ) );
	currentRenderCrop = 0;
	currentColorNativeBytesOrder = 0xFFFFFFFF;
	currentGLState = 0;
	guiRecursionLevel = 0;
	guiModel = NULL;
	takingScreenshot = false;
	//memset( &m_smpFrameData, 0, sizeof( m_frameData ) );

	if ( unitSquareTriangles != NULL ) {
		Mem_Free( unitSquareTriangles );
		unitSquareTriangles = NULL;
	}

	if ( zeroOneCubeTriangles != NULL ) {
		Mem_Free( zeroOneCubeTriangles );
		zeroOneCubeTriangles = NULL;
	}

	if ( testImageTriangles != NULL ) {
		Mem_Free( testImageTriangles );
		testImageTriangles = NULL;
	}

	frontEndJobList = NULL;
}

/*
========================
idRenderSystemLocal::BeginLevelLoad
========================
*/
void idRenderSystemLocal::BeginLevelLoad() {
	globalImages->BeginLevelLoad();
	renderModelManager->BeginLevelLoad();

	// Re-Initialize the Default Materials if needed. 
	InitMaterials();
}

/*
========================
idRenderSystemLocal::LoadLevelImages
========================
*/
void idRenderSystemLocal::LoadLevelImages() {
	globalImages->LoadLevelImages( false );
}

/*
========================
idRenderSystemLocal::Preload
========================
*/
void idRenderSystemLocal::Preload( const idPreloadManifest &manifest, const char *mapName ) {
	globalImages->Preload( manifest, true );
	uiManager->Preload( mapName );
	renderModelManager->Preload( manifest );
}

/*
========================
idRenderSystemLocal::EndLevelLoad
========================
*/
void idRenderSystemLocal::EndLevelLoad() {
	renderModelManager->EndLevelLoad();
	globalImages->EndLevelLoad();
}

/*
============
idRenderSystemLocal::RegisterFont
============
*/
idFont * idRenderSystemLocal::RegisterFont( const char * fontName ) {

	idStrStatic< MAX_OSPATH > baseFontName = fontName;
	baseFontName.Replace( "fonts/", "" );
	for ( int i = 0; i < fonts.Num(); i++ ) {
		if ( idStr::Icmp( fonts[i]->GetName(), baseFontName ) == 0 ) {
			fonts[i]->Touch();
			return fonts[i];
		}
	}
	idFont * newFont = new (TAG_FONT) idFont( baseFontName );
	fonts.Append( newFont );
	return newFont;
}

/*
========================
idRenderSystemLocal::IsFullScreen
========================
*/
bool idRenderSystemLocal::IsFullScreen() const {
	return true;//win32.isFullscreen != 0;
}

/*
========================
idRenderSystemLocal::GetWidth
========================
*/
int idRenderSystemLocal::GetWidth() const {
	return 1280;//win32.nativeScreenWidth;
}

/*
========================
idRenderSystemLocal::GetHeight
========================
*/
int idRenderSystemLocal::GetHeight() const {
	return 720;//win32.nativeScreenHeight;
}

/*
========================
idRenderSystemLocal::GetPixelAspect
========================
*/
float idRenderSystemLocal::GetPixelAspect() const {
	return 1;//win32.pixelAspect;
}

/*
========================
idRenderSystemLocal::FrameAlloc

This data will be automatically freed when the
current frame's back end completes.

This should only be called by the front end.  The
back end shouldn't need to allocate memory.

All temporary data, like dynamic tesselations
and local spaces are allocated here.

All memory is cache-line-cleared for the best performance.
========================
*/
// void * idRenderSystemLocal::FrameAlloc( int bytes, frameAllocType_t type ) {
// #if defined( TRACK_FRAME_ALLOCS )
// 	m_frameData->frameMemoryUsed.Add( bytes );
// 	frameAllocTypeCount[ type ].Add( bytes );
// #endif

// 	bytes = ( bytes + FRAME_ALLOC_ALIGNMENT - 1 ) & ~ ( FRAME_ALLOC_ALIGNMENT - 1 );

// 	// thread safe add
// 	int end = m_frameData->frameMemoryAllocated.Add( bytes );
// 	if ( end > MAX_FRAME_MEMORY ) {
// 		idLib::Error( "idRenderSystemLocal::FrameAlloc ran out of memory. bytes = %d, end = %d, highWaterAllocated = %d\n", bytes, end, m_frameData->highWaterAllocated );
// 	}

// 	byte * ptr = m_frameData->frameMemory + end - bytes;

// 	// cache line clear the memory
// 	for ( int offset = 0; offset < bytes; offset += CACHE_LINE_SIZE ) {
// 		ZeroCacheLine( ptr, offset );
// 	}

// 	return ptr;
// }

/*
============
idRenderSystemLocal::ClearedFrameAlloc
============
*/
// void * idRenderSystemLocal::ClearedFrameAlloc( int bytes, frameAllocType_t type ) {
// 	// NOTE: every allocation is cache line cleared
// 	return FrameAlloc( bytes, type );
// }

/*
============
idRenderSystemLocal::ToggleSmpFrame
============
*/
// void idRenderSystemLocal::ToggleSmpFrame() {
// 	// update the highwater mark
// 	if ( m_frameData->frameMemoryAllocated.GetValue() > m_frameData->highWaterAllocated ) {
// 		m_frameData->highWaterAllocated = m_frameData->frameMemoryAllocated.GetValue();
// #if defined( TRACK_FRAME_ALLOCS )
// 		m_frameData->highWaterUsed = m_frameData->frameMemoryUsed.GetValue();
// 		for ( int i = 0; i < FRAME_ALLOC_MAX; i++ ) {
// 			frameHighWaterTypeCount[i] = frameAllocTypeCount[i].GetValue();
// 		}
// #endif
// 	}

// 	// switch to the next frame
// 	m_smpFrame++;
// 	m_frameData = &m_smpFrameData[ m_smpFrame % NUM_FRAME_DATA ];

// 	// reset the memory allocation
// 	const uint32 bytesNeededForAlignment = FRAME_ALLOC_ALIGNMENT - ( (uint64)m_frameData->frameMemory & ( FRAME_ALLOC_ALIGNMENT - 1 ) );
// 	m_frameData->frameMemoryAllocated.SetValue( bytesNeededForAlignment );
// 	m_frameData->frameMemoryUsed.SetValue( 0 );

// #if defined( TRACK_FRAME_ALLOCS )
// 	for ( int i = 0; i < FRAME_ALLOC_MAX; i++ ) {
// 		frameAllocTypeCount[i].SetValue( 0 );
// 	}
// #endif

// 	// clear the command chain
// 	m_frameData->renderCommandIndex = 0;
// 	m_frameData->renderCommands.Zero();
// }

/*
============
idRenderSystemLocal::InitFrameData
============
*/
// void idRenderSystemLocal::InitFrameData() {
// 	ShutdownFrameData();

// 	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
// 		m_smpFrameData[ i ].frameMemory = (byte *) Mem_Alloc16( MAX_FRAME_MEMORY, TAG_RENDER );
// 	}

// 	// must be set before ToggleSmpFrame()
// 	//m_frameData = &m_smpFrameData[ 0 ];

// 	ToggleSmpFrame();
// }

/*
============
idRenderSystemLocal::ShutdownFrameData
============
*/
// void idRenderSystemLocal::ShutdownFrameData() {
// 	m_frameData = NULL;
// 	for ( int i = 0; i < NUM_FRAME_DATA; ++i ) {
// 		Mem_Free16( m_smpFrameData[ i ].frameMemory );
// 		m_smpFrameData[ i ].frameMemory = NULL;
// 	}
// }

/*
=============
idRenderSystemLocal::AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
// void idRenderSystemLocal::AddDrawViewCmd( viewDef_t *parms, bool guiOnly ) {
// 	renderCommand_t & cmd = m_frameData->renderCommands[ m_frameData->renderCommandIndex++ ];
// 	cmd.op = RC_DRAW_VIEW;
// 	cmd.viewDef = parms;

// 	pc.c_numViews++;

// 	// report statistics about this view
// 	if ( r_showSurfaces.GetBool() ) {
// 		idLib::Printf( "view:%p surfs:%i\n", parms, parms->numDrawSurfs );
// 	}
// }

/*
=============
idRenderSystemLocal::EmitFullscreenGuis
=============
*/
void idRenderSystemLocal::EmitFullscreenGui() {
	idLib::Printf("EmitFullscreenGui NI");
	// viewDef_t * guiViewDef = guiModel->EmitFullScreen();
	// if ( guiViewDef ) {
	// 	// add the command to draw this view
	// 	AddDrawViewCmd( guiViewDef, true );
	// }
	// guiModel->Clear();
}

/*
=============
idRenderSystemLocal::SetColor
=============
*/
// void idRenderSystemLocal::SetColor( const idVec4 & rgba ) {
// 	currentColorNativeBytesOrder = LittleLong( PackColor( rgba ) );
// }

/*
=============
idRenderSystemLocal::GetColor
=============
*/
// uint32 idRenderSystemLocal::GetColor() {
// 	return LittleLong( currentColorNativeBytesOrder );
// }

/*
=============
idRenderSystemLocal::SetGLState
=============
*/
// void idRenderSystemLocal::SetGLState( const uint64 glState ) {
// 	currentGLState = glState;
// }

/*
=============
idRenderSystemLocal::DrawFilled
=============
*/
// void idRenderSystemLocal::DrawFilled( const idVec4 & color, float x, float y, float w, float h ) {
// 	SetColor( color );
// 	DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, whiteMaterial );
// }

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
// void idRenderSystemLocal::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial *material ) {
// 	DrawStretchPic( idVec4( x, y, s1, t1 ), idVec4( x+w, y, s2, t1 ), idVec4( x+w, y+h, s2, t2 ), idVec4( x, y+h, s1, t2 ), material );
// }

/*
=============
idRenderSystemLocal::DrawStretchFX
=============
*/
// void idRenderSystemLocal::DrawStretchFX( 
// 	float x, float y, 
// 	float w, float h, 
// 	float s1, float t1, 
// 	float s2, float t2, 
// 	const idMaterial *material ) {

// 	DrawStretchPic( 
// 		idVec4( x, y, s1, t1 ),		// TL
// 		idVec4( x+w, y, s2, t1 ),	// TR
// 		idVec4( x+w, y+h, s2, t2 ), // BR
// 		idVec4( x, y+h, s1, t2 ),	// BL
// 		material );
// }

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
//static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
// void idRenderSystemLocal::DrawStretchPic( const idVec4 & topLeft, const idVec4 & topRight, const idVec4 & bottomRight, const idVec4 & bottomLeft, const idMaterial * material ) {
// 	if ( !m_bInitialized ) {
// 		return;
// 	}
// 	if ( material == NULL ) {
// 		return;
// 	}

// 	idDrawVert * verts = guiModel->AllocTris( 4, quadPicIndexes, 6, material, currentGLState,
// 	STEREO_DEPTH_TYPE_NONE );
// 	if ( verts == NULL ) {
// 		return;
// 	}

// 	ALIGNTYPE16 idDrawVert localVerts[4];

// 	localVerts[0].Clear();
// 	localVerts[0].xyz[0] = topLeft.x;
// 	localVerts[0].xyz[1] = topLeft.y;
// 	localVerts[0].SetTexCoord( topLeft.z, topLeft.w );
// 	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[0].ClearColor2();

// 	localVerts[1].Clear();
// 	localVerts[1].xyz[0] = topRight.x;
// 	localVerts[1].xyz[1] = topRight.y;
// 	localVerts[1].SetTexCoord( topRight.z, topRight.w );
// 	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[1].ClearColor2();

// 	localVerts[2].Clear();
// 	localVerts[2].xyz[0] = bottomRight.x;
// 	localVerts[2].xyz[1] = bottomRight.y;
// 	localVerts[2].SetTexCoord( bottomRight.z, bottomRight.w );
// 	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[2].ClearColor2();

// 	localVerts[3].Clear();
// 	localVerts[3].xyz[0] = bottomLeft.x;
// 	localVerts[3].xyz[1] = bottomLeft.y;
// 	localVerts[3].SetTexCoord( bottomLeft.z, bottomLeft.w );
// 	localVerts[3].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[3].ClearColor2();

// 	WriteDrawVerts16( verts, localVerts, 4 );
// }

/*
=============
idRenderSystemLocal::DrawStretchTri
=============
*/
// void idRenderSystemLocal::DrawStretchTri( const idVec2 & p1, const idVec2 & p2, const idVec2 & p3, const idVec2 & t1, const idVec2 & t2, const idVec2 & t3, const idMaterial *material ) {
// 	if ( !m_bInitialized ) {
// 		return;
// 	}
// 	if ( material == NULL ) {
// 		return;
// 	}

// 	triIndex_t tempIndexes[3] = { 1, 0, 2 };

// 	idDrawVert * verts = guiModel->AllocTris( 3, tempIndexes, 3, material, currentGLState,
// 	STEREO_DEPTH_TYPE_NONE );
// 	if ( verts == NULL ) {
// 		return;
// 	}

// 	ALIGNTYPE16 idDrawVert localVerts[3];

// 	localVerts[0].Clear();
// 	localVerts[0].xyz[0] = p1.x;
// 	localVerts[0].xyz[1] = p1.y;
// 	localVerts[0].SetTexCoord( t1 );
// 	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[0].ClearColor2();

// 	localVerts[1].Clear();
// 	localVerts[1].xyz[0] = p2.x;
// 	localVerts[1].xyz[1] = p2.y;
// 	localVerts[1].SetTexCoord( t2 );
// 	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[1].ClearColor2();

// 	localVerts[2].Clear();
// 	localVerts[2].xyz[0] = p3.x;
// 	localVerts[2].xyz[1] = p3.y;
// 	localVerts[2].SetTexCoord( t3 );
// 	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
// 	localVerts[2].ClearColor2();

// 	WriteDrawVerts16( verts, localVerts, 3 );
// }

/*
=============
idRenderSystemLocal::AllocTris
=============
*/
// idDrawVert * idRenderSystemLocal::AllocTris( int numVerts, const triIndex_t * indexes, int numIndexes, const idMaterial * material,
// 	const stereoDepthType_t stereoType ) {
// 	return guiModel->AllocTris( numVerts, indexes, numIndexes, material, currentGLState, 
// 		stereoType );
// }

/*
=====================
idRenderSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
=====================
*/
// void idRenderSystemLocal::DrawSmallChar( int x, int y, int ch ) {
// 	int row, col;
// 	float frow, fcol;
// 	float size;

// 	ch &= 255;

// 	if ( ch == ' ' ) {
// 		return;
// 	}

// 	if ( y < -SMALLCHAR_HEIGHT ) {
// 		return;
// 	}

// 	row = ch >> 4;
// 	col = ch & 15;

// 	frow = row * 0.0625f;
// 	fcol = col * 0.0625f;
// 	size = 0.0625f;

// 	DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
// 					   fcol, frow, 
// 					   fcol + size, frow + size, 
// 					   charSetMaterial );
// }

/*
==================
idRenderSystemLocal::DrawSmallStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
// void idRenderSystemLocal::DrawSmallStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor ) {
// 	idVec4		color;
// 	const unsigned char	*s;
// 	int			xx;

// 	// draw the colored text
// 	s = (const unsigned char*)string;
// 	xx = x;
// 	SetColor( setColor );
// 	while ( *s ) {
// 		if ( idStr::IsColor( (const char*)s ) ) {
// 			if ( !forceColor ) {
// 				if ( *(s+1) == C_COLOR_DEFAULT ) {
// 					SetColor( setColor );
// 				} else {
// 					color = idStr::ColorForIndex( *(s+1) );
// 					color[3] = setColor[3];
// 					SetColor( color );
// 				}
// 			}
// 			s += 2;
// 			continue;
// 		}
// 		DrawSmallChar( xx, y, *s );
// 		xx += SMALLCHAR_WIDTH;
// 		s++;
// 	}
// 	SetColor( colorWhite );
// }

/*
=====================
idRenderSystemLocal::DrawBigChar
=====================
*/
// void idRenderSystemLocal::DrawBigChar( int x, int y, int ch ) {
// 	int row, col;
// 	float frow, fcol;
// 	float size;

// 	ch &= 255;

// 	if ( ch == ' ' ) {
// 		return;
// 	}

// 	if ( y < -BIGCHAR_HEIGHT ) {
// 		return;
// 	}

// 	row = ch >> 4;
// 	col = ch & 15;

// 	frow = row * 0.0625f;
// 	fcol = col * 0.0625f;
// 	size = 0.0625f;

// 	DrawStretchPic( x, y, BIGCHAR_WIDTH, BIGCHAR_HEIGHT,
// 					   fcol, frow, 
// 					   fcol + size, frow + size, 
// 					   charSetMaterial );
// }

/*
==================
idRenderSystemLocal::DrawBigStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
// void idRenderSystemLocal::DrawBigStringExt( int x, int y, const char *string, const idVec4 &setColor, bool forceColor ) {
// 	idVec4		color;
// 	const char	*s;
// 	int			xx;

// 	// draw the colored text
// 	s = string;
// 	xx = x;
// 	SetColor( setColor );
// 	while ( *s ) {
// 		if ( idStr::IsColor( s ) ) {
// 			if ( !forceColor ) {
// 				if ( *(s+1) == C_COLOR_DEFAULT ) {
// 					SetColor( setColor );
// 				} else {
// 					color = idStr::ColorForIndex( *(s+1) );
// 					color[3] = setColor[3];
// 					SetColor( color );
// 				}
// 			}
// 			s += 2;
// 			continue;
// 		}
// 		DrawBigChar( xx, y, *s );
// 		xx += BIGCHAR_WIDTH;
// 		s++;
// 	}
// 	SetColor( colorWhite );
// }

/*
====================
idRenderSystemLocal::SwapCommandBuffers

Performs final closeout of any gui models being defined.

Waits for the previous GPU rendering to complete and vsync.

Returns the head of the linked command list that was just closed off.

Returns timing information from the previous frame.

After this is called, new command buffers can be built up in parallel
with the rendering of the closed off command buffers by RenderCommandBuffers()
====================
*/
// void idRenderSystemLocal::SwapCommandBuffers( uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec )  {
// 	SwapCommandBuffers_FinishRendering( frontEndMicroSec, backEndMicroSec, shadowMicroSec, gpuMicroSec );
// 	SwapCommandBuffers_FinishCommandBuffers();
// }

/*
=====================
idRenderSystemLocal::SwapAndRenderCommandBuffers
=====================
*/
// void idRenderSystemLocal::SwapAndRenderCommandBuffers( frameTiming_t * frameTiming ) {
// 	SwapCommandBuffers( frameTiming );
// 	RenderCommandBuffers();
// }

/*
=====================
idRenderSystemLocal::SwapCommandBuffers_FinishRendering
=====================
*/
// void idRenderSystemLocal::SwapCommandBuffers_FinishRendering(  uint64* frontEndMicroSec, uint64* backEndMicroSec, uint64* shadowMicroSec, uint64* gpuMicroSec  )  {
// 	SCOPED_PROFILE_EVENT( "SwapCommandBuffers_FinishRendering" );

// 	if ( !m_bInitialized ) {
// 		return;
// 	}

// 	// wait for our fence to hit, which means the swap has actually happened
// 	// We must do this before clearing any resources the GPU may be using
// 	backend.BlockingSwapBuffers();

// 	//------------------------------

// 	// save out timing information
// 	// if ( frameTiming != NULL ) {
// 	// 	frameTiming->frontendTime= pc.frontEndMicroSec;
// 	// 	frameTiming->backendTime = backend.m_pc.totalMicroSec;
// 	// 	frameTiming->shadowTime = backend.m_pc.shadowMicroSec;
// 	// 	frameTiming->depthTime = backend.m_pc.depthMicroSec;
// 	// 	frameTiming->interactionTime = backend.m_pc.interactionMicroSec;
// 	// 	frameTiming->shaderTime = backend.m_pc.shaderPassMicroSec;
// 	// 	frameTiming->gpuTime = backend.m_pc.gpuMicroSec;
// 	// }

// 	// print any other statistics and clear all of them
// 	PrintPerformanceCounters();
// }

/*
=====================
idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers
=====================
*/
// void R_InitDrawSurfFromTri( drawSurf_t & ds, srfTriangles_t & tri );
// void idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers() {
// 	if ( !m_bInitialized ) {
// 		return;
// 	}

// 	// close any gui drawing
// 	EmitFullscreenGui();

// 	// unmap the buffer objects so they can be used by the GPU
// 	vertexCache.BeginBackEnd();

// 	// copy the code-used drawsurfs that were
// 	// allocated at the start of the buffer memory to the backend referenced locations
// 	backend.m_unitSquareSurface = m_unitSquareSurface;
// 	backend.m_zeroOneCubeSurface = m_zeroOneCubeSurface;
// 	backend.m_testImageSurface = m_testImageSurface;

// 	// use the other buffers next frame, because another CPU
// 	// may still be rendering into the current buffers
// 	ToggleSmpFrame();

// 	// prepare the new command buffer
// 	guiModel->BeginFrame();

// 	//------------------------------
// 	// Make sure that geometry used by code is present in the buffer cache.
// 	// These use frame buffer cache (not static) because they may be used during
// 	// map loads.
// 	//
// 	// It is important to do this first, so if the buffers overflow during
// 	// scene generation, the basic surfaces needed for drawing the buffers will
// 	// always be present.
// 	//------------------------------
// 	R_InitDrawSurfFromTri( m_unitSquareSurface, *m_unitSquareTriangles );
// 	R_InitDrawSurfFromTri( m_zeroOneCubeSurface, *zeroOneCubeTriangles );
// 	R_InitDrawSurfFromTri( m_testImageSurface, *testImageTriangles );

// 	// Reset render crop to be the full screen
// 	renderCrops[0].x1 = 0;
// 	renderCrops[0].y1 = 0;
// 	renderCrops[0].x2 = GetWidth() - 1;
// 	renderCrops[0].y2 = GetHeight() - 1;
// 	currentRenderCrop = 0;

// 	// this is the ONLY place this is modified
// 	frameCount++;

// 	// just in case we did a idLib::Error while this
// 	// was set
// 	guiRecursionLevel = 0;

// 	// the old command buffer can now be rendered, while the new one can
// 	// be built in parallel
// }

/*
====================
idRenderSystemLocal::RenderCommandBuffers
====================
*/
// void idRenderSystemLocal::RenderCommandBuffers() {
// 	// Use the previous smp frame data as the current is being written to.
// 	idFrameData & frameData = m_smpFrameData[ ( m_smpFrame - 1 ) % NUM_FRAME_DATA ];

// 	// if there isn't a draw view command, do nothing to avoid swapping a bad frame
// 	if ( frameData.renderCommandIndex == 0 ) {
// 		return;
// 	}
// 	if ( frameData.renderCommands[ 0 ].op == RC_NOP ) {
// 		return;
// 	}

// 	backend.ExecuteBackEndCommands( frameData.renderCommandIndex, frameData.renderCommands );

// 	// pass in null for now - we may need to do some map specific hackery in the future
// 	resolutionScale.InitForMap( NULL );
// }

/*
=====================
idRenderSystemLocal::GetCroppedViewport

Returns the current cropped pixel coordinates
=====================
*/
// void idRenderSystemLocal::GetCroppedViewport( idScreenRect * viewport ) {
// 	*viewport = renderCrops[ currentRenderCrop ];
// }

/*
========================
idRenderSystemLocal::PerformResolutionScaling

The 3D rendering size can be smaller than the full window resolution to reduce
fill rate requirements while still allowing the GUIs to be full resolution.
In split screen mode the rendering size is also smaller.
========================
*/
// void idRenderSystemLocal::PerformResolutionScaling( int& newWidth, int& newHeight ) {
// 	float xScale = 1.0f;
// 	float yScale = 1.0f;
// 	resolutionScale.ResetToFullResolution();
// 	resolutionScale.GetCurrentResolutionScale( xScale, yScale );

// 	newWidth = idMath::Ftoi( GetWidth() * xScale );
// 	newHeight = idMath::Ftoi( GetHeight() * yScale );
// }

/*
================
idRenderSystemLocal::CropRenderSize
================
*/
// void idRenderSystemLocal::CropRenderSize( int width, int height ) {
// 	if ( !m_bInitialized ) {
// 		return;
// 	}

// 	// close any gui drawing before changing the size
// 	EmitFullscreenGui();

// 	if ( width < 1 || height < 1 ) {
// 		idLib::Error( "CropRenderSize: bad sizes" );
// 	}

// 	idScreenRect & previous = renderCrops[ currentRenderCrop ];

// 	currentRenderCrop++;

// 	idScreenRect & current = renderCrops[ currentRenderCrop ];

// 	current.x1 = previous.x1;
// 	current.x2 = previous.x1 + width - 1;
// 	current.y1 = previous.y2 - height + 1;
// 	current.y2 = previous.y2;
// }

/*
================
idRenderSystemLocal::UnCrop
================
*/
// void idRenderSystemLocal::UnCrop() {
// 	if ( !m_bInitialized ) {
// 		return;
// 	}

// 	if ( currentRenderCrop < 1 ) {
// 		idLib::Error( "idRenderSystemLocal::UnCrop: currentRenderCrop < 1" );
// 	}

// 	// close any gui drawing
// 	EmitFullscreenGui();

// 	currentRenderCrop--;
// }

/*
================
idRenderSystemLocal::CaptureRenderToImage
================
*/
void idRenderSystemLocal::CaptureRenderToImage( const char *imageName, bool clearColorAfterCopy ) {
	idLib::Printf("CaptureRenderToImage NI");
	// if ( !m_bInitialized ) {
	// 	return;
	// }
	// EmitFullscreenGui();

	// idImage	* image = globalImages->GetImage( imageName );
	// if ( image == NULL ) {
	// 	image = globalImages->AllocImage( imageName );
	// }

	// idScreenRect & rc = renderCrops[ currentRenderCrop ];

	// renderCommand_t & cmd = m_frameData->renderCommands[ m_frameData->renderCommandIndex++ ];
	// cmd.op = RC_COPY_RENDER;
	// cmd.x = rc.x1;
	// cmd.y = rc.y1;
	// cmd.imageWidth = rc.GetWidth();
	// cmd.imageHeight = rc.GetHeight();
	// cmd.image = image;
	// cmd.clearColorAfterCopy = clearColorAfterCopy;

	// guiModel->Clear();
}

/*
==============
idRenderSystemLocal::CaptureRenderToFile
==============
*/
void idRenderSystemLocal::CaptureRenderToFile( const char *fileName, bool fixAlpha ) {
	if ( !m_bInitialized ) {
		return;
	}

	// TODO: Refactor for both APIs
#if 0
	idScreenRect & rc = renderCrops[ currentRenderCrop ];

	EmitFullscreenGui();

	RenderCommandBuffers( m_frameData->cmdHead );

	qglReadBuffer( GL_BACK );

	// include extra space for OpenGL padding to word boundaries
	int	c = ( rc.GetWidth() + 3 ) * rc.GetHeight();
	byte *data = (byte *)R_StaticAlloc( c * 3 );
	
	qglReadPixels( rc.x1, rc.y1, rc.GetWidth(), rc.GetHeight(), GL_RGB, GL_UNSIGNED_BYTE, data ); 

	byte *data2 = (byte *)R_StaticAlloc( c * 4 );

	for ( int i = 0 ; i < c ; i++ ) {
		data2[ i * 4 ] = data[ i * 3 ];
		data2[ i * 4 + 1 ] = data[ i * 3 + 1 ];
		data2[ i * 4 + 2 ] = data[ i * 3 + 2 ];
		data2[ i * 4 + 3 ] = 0xff;
	}

	R_WriteTGA( fileName, data2, rc.GetWidth(), rc.GetHeight(), true );

	R_StaticFree( data );
	R_StaticFree( data2 );
#endif
}


/*
==============
idRenderSystemLocal::AllocRenderWorld
==============
*/
// idRenderWorld *idRenderSystemLocal::AllocRenderWorld() {
// 	idRenderWorld *rw;
// 	rw = new (TAG_RENDER) idRenderWorld;
// 	worlds.Append( rw );
// 	return rw;
// }

/*
==============
idRenderSystemLocal::ReCreateWorldReferences

ReloadModels and RegenerateWorld call this
==============
*/
// void idRenderSystemLocal::ReCreateWorldReferences() {
// 	// let the interaction generation code know this
// 	// shouldn't be optimized for a particular view
// 	viewDef = NULL;

// 	for ( int i = 0; i < worlds.Num(); ++i ) {
// 		worlds[ i ]->ReCreateReferences();
// 	}
// }

/*
===================
idRenderSystemLocal::FreeWorldDerivedData

ReloadModels and RegenerateWorld call this
===================
*/
// void idRenderSystemLocal::FreeWorldDerivedData() {
// 	for ( int i = 0; i < worlds.Num(); ++i ) {
// 		worlds[ i ]->FreeDerivedData();
// 	}
// }

/*
===================
idRenderSystemLocal::CheckWorldsForEntityDefsUsingModel
===================
*/
// void idRenderSystemLocal::CheckWorldsForEntityDefsUsingModel( idRenderModel * model ) {
// 	for ( int i = 0; i < worlds.Num(); ++i ) {
// 		worlds[ i ]->CheckForEntityDefsUsingModel( model );
// 	}
// }

/*
==============
idRenderSystemLocal::FreeRenderWorld
==============
*/
// void idRenderSystemLocal::FreeRenderWorld( idRenderWorld *rw ) {
// 	if ( primaryWorld == rw ) {
// 		primaryWorld = NULL;
// 	}
// 	worlds.Remove( rw );
// 	delete rw;
// }

/*
=====================
idRenderSystemLocal::PrintPerformanceCounters

This prints both front and back end counters, so it should
only be called when the back end thread is idle.
=====================
*/
// void idRenderSystemLocal::PrintPerformanceCounters() {
// 	if ( r_showPrimitives.GetInteger() != 0 ) {
// 		backEndCounters_t & bc = backend.m_pc;

// 		idLib::Printf( "views:%i draws:%i tris:%i (shdw:%i)\n",
// 			pc.c_numViews,
// 			bc.c_drawElements + bc.c_shadowElements,
// 			( bc.c_drawIndexes + bc.c_shadowIndexes ) / 3,
// 			bc.c_shadowIndexes / 3
// 			);
// 	}

// 	if ( r_showDynamic.GetBool() ) {
// 		idLib::Printf( "callback:%i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i\n",
// 			pc.c_entityDefCallbacks,
// 			pc.c_generateMd5,
// 			pc.c_deformedVerts,
// 			pc.c_deformedIndexes/3,
// 			pc.c_tangentIndexes/3,
// 			pc.c_guiSurfs
// 			); 
// 	}

// 	if ( r_showCull.GetBool() ) {
// 		idLib::Printf( "%i box in %i box out\n",
// 			pc.c_box_cull_in, pc.c_box_cull_out );
// 	}
	
// 	if ( r_showAddModel.GetBool() ) {
// 		idLib::Printf( "callback:%i createInteractions:%i createShadowVolumes:%i\n",
// 			pc.c_entityDefCallbacks, pc.c_createInteractions, pc.c_createShadowVolumes );
// 		idLib::Printf( "viewEntities:%i  shadowEntities:%i  viewLights:%i\n", pc.c_visibleViewEntities,
// 			pc.c_shadowViewEntities, pc.c_viewLights );
// 	}
// 	if ( r_showUpdates.GetBool() ) {
// 		idLib::Printf( "entityUpdates:%i  entityRefs:%i  lightUpdates:%i  lightRefs:%i\n", 
// 			pc.c_entityUpdates, pc.c_entityReferences,
// 			pc.c_lightUpdates, pc.c_lightReferences );
// 	}
// 	// if ( r_showMemory.GetBool() ) {
// 	// 	idLib::Printf( "frameData: %i (%i)\n", m_frameData->frameMemoryAllocated.GetValue(), m_frameData->highWaterAllocated );
// 	// }

// 	memset( &pc, 0, sizeof( pc ) );
// 	memset( &backend.m_pc, 0, sizeof( backend.m_pc ) );
// }

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

void idRenderSystemLocal::InitOpenGL()
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

	r_initialized = true;
}

void idRenderSystemLocal::ShutdownOpenGL()
{
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

int idRenderSystemLocal::GetVirtualWidth() const
{
	if( r_useVirtualScreenResolution.GetBool() )
	{
		return SCREEN_WIDTH;
	}
	return GetWidth();
}

int idRenderSystemLocal::GetVirtualHeight() const
{
	if( r_useVirtualScreenResolution.GetBool() )
	{
		return SCREEN_HEIGHT;
	}
	return GetHeight();
}

float idRenderSystemLocal::GetPhysicalScreenWidthInCentimeters() const
{
	return 0;
}

stereo3DMode_t idRenderSystemLocal::GetStereo3DMode() const
{
	return STEREO3D_OFF;
}

bool idRenderSystemLocal::IsStereoScopicRenderingSupported() const
{
	return false;
}

bool idRenderSystemLocal::IsGpuSkinningSupported() const
{
	return false;
}

stereo3DMode_t idRenderSystemLocal::GetStereoScopicRenderingMode() const
{
	return STEREO3D_OFF;
}

void idRenderSystemLocal::EnableStereoScopicRendering(stereo3DMode_t) const
{
}

bool idRenderSystemLocal::HasQuadBufferSupport() const
{
	return false;
}

void idRenderSystemLocal::BeginAutomaticBackgroundSwaps(autoRenderIconType_t)
{
}

void idRenderSystemLocal::EndAutomaticBackgroundSwaps()
{
}

bool idRenderSystemLocal::AreAutomaticBackgroundSwapsRunning(autoRenderIconType_t*) const
{
	return false;
}

void idRenderSystemLocal::ResetFonts()
{
}

void idRenderSystemLocal::TakeScreenshot(int, int, char const*, int, renderView_s*, int)
{
}

void idRenderSystemLocal::OnFrame()
{
}

/*
========================
idRenderSystemLocal::UpdateStereo3DMode
========================
*/
void idRenderSystemLocal::UpdateStereo3DMode()
{
}