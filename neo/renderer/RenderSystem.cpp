/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2013-2016 Robert Beckebans
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

#include "RenderCommon.h"

idRenderSystemLocal	tr;
idRenderSystem* renderSystem = &tr;


idCVar r_requestStereoPixelFormat( "r_requestStereoPixelFormat", "1", CVAR_RENDERER, "Ask for a stereo GL pixel format on startup" );
idCVar r_debugContext( "r_debugContext", "0", CVAR_RENDERER, "Enable various levels of context debug." );
idCVar r_multiSamples( "r_multiSamples", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "number of antialiasing samples" );
idCVar r_glDriver( "r_glDriver", "", CVAR_RENDERER, "\"opengl32\", etc." );
idCVar r_skipIntelWorkarounds( "r_skipIntelWorkarounds", "0", CVAR_RENDERER | CVAR_BOOL, "skip workarounds for Intel driver bugs" );
// RB: disabled 16x MSAA
idCVar r_antiAliasing( "r_antiAliasing", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, " 0 = None\n 1 = SMAA 1x\n 2 = MSAA 2x\n 3 = MSAA 4x\n 4 = MSAA 8x\n", 0, ANTI_ALIASING_MSAA_8X );
// RB end
idCVar r_vidMode( "r_vidMode", "0", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_INTEGER, "fullscreen video mode number" );
idCVar r_displayRefresh( "r_displayRefresh", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_NOCHEAT, "optional display refresh rate option for vid mode", 0.0f, 240.0f );
#ifdef WIN32
idCVar r_fullscreen( "r_fullscreen", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
#else
// DG: add mode -2 for SDL, also defaulting to windowed mode, as that causes less trouble on linux
idCVar r_fullscreen( "r_fullscreen", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "-2 = use current monitor, -1 = (reserved), 0 = windowed, 1 = full screen on monitor 1, 2 = full screen on monitor 2, etc" );
// DG end
#endif
idCVar r_customWidth( "r_customWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen width. set r_vidMode to -1 to activate" );
idCVar r_customHeight( "r_customHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "custom screen height. set r_vidMode to -1 to activate" );
idCVar r_windowX( "r_windowX", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowY( "r_windowY", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowWidth( "r_windowWidth", "1280", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );
idCVar r_windowHeight( "r_windowHeight", "720", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "Non-fullscreen parameter" );

idCVar r_useViewBypass( "r_useViewBypass", "1", CVAR_RENDERER | CVAR_INTEGER, "bypass a frame of latency to the view" );
idCVar r_useLightPortalFlow( "r_useLightPortalFlow", "1", CVAR_RENDERER | CVAR_BOOL, "use a more precise area reference determination" );
idCVar r_singleTriangle( "r_singleTriangle", "0", CVAR_RENDERER | CVAR_BOOL, "only draw a single triangle per primitive" );
idCVar r_checkBounds( "r_checkBounds", "0", CVAR_RENDERER | CVAR_BOOL, "compare all surface bounds with precalculated ones" );
idCVar r_useConstantMaterials( "r_useConstantMaterials", "1", CVAR_RENDERER | CVAR_BOOL, "use pre-calculated material registers if possible" );
idCVar r_useSilRemap( "r_useSilRemap", "1", CVAR_RENDERER | CVAR_BOOL, "consider verts with the same XYZ, but different ST the same for shadows" );
idCVar r_useNodeCommonChildren( "r_useNodeCommonChildren", "1", CVAR_RENDERER | CVAR_BOOL, "stop pushing reference bounds early when possible" );
idCVar r_useShadowSurfaceScissor( "r_useShadowSurfaceScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor shadows by the scissor rect of the interaction surfaces" );
idCVar r_useCachedDynamicModels( "r_useCachedDynamicModels", "1", CVAR_RENDERER | CVAR_BOOL, "cache snapshots of dynamic models" );
idCVar r_useSeamlessCubeMap( "r_useSeamlessCubeMap", "1", CVAR_RENDERER | CVAR_BOOL, "use ARB_seamless_cube_map if available" );
idCVar r_useSRGB( "r_useSRGB", "0", CVAR_RENDERER | CVAR_INTEGER | CVAR_ARCHIVE, "1 = both texture and framebuffer, 2 = framebuffer only, 3 = texture only" );
idCVar r_maxAnisotropicFiltering( "r_maxAnisotropicFiltering", "8", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "limit aniso filtering" );
idCVar r_useTrilinearFiltering( "r_useTrilinearFiltering", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "Extra quality filtering" );
// RB: not used anymore
idCVar r_lodBias( "r_lodBias", "0.5", CVAR_RENDERER | CVAR_ARCHIVE, "UNUSED: image lod bias" );
// RB end

idCVar r_useStateCaching( "r_useStateCaching", "1", CVAR_RENDERER | CVAR_BOOL, "avoid redundant state changes in GL_*() calls" );

idCVar r_znear( "r_znear", "3", CVAR_RENDERER | CVAR_FLOAT, "near Z clip plane distance", 0.001f, 200.0f );

idCVar r_ignoreGLErrors( "r_ignoreGLErrors", "0", CVAR_RENDERER | CVAR_BOOL, "ignore GL errors" );
idCVar r_swapInterval( "r_swapInterval", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "0 = tear, 1 = swap-tear where available, 2 = always v-sync" );

idCVar r_gamma( "r_gamma", "1.0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 3.0f );
idCVar r_brightness( "r_brightness", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "changes gamma tables", 0.5f, 2.0f );

idCVar r_jitter( "r_jitter", "0", CVAR_RENDERER | CVAR_BOOL, "randomly subpixel jitter the projection matrix" );

idCVar r_skipStaticInteractions( "r_skipStaticInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created at level load" );
idCVar r_skipDynamicInteractions( "r_skipDynamicInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip interactions created after level load" );
idCVar r_skipSuppress( "r_skipSuppress", "0", CVAR_RENDERER | CVAR_BOOL, "ignore the per-view suppressions" );
idCVar r_skipPostProcess( "r_skipPostProcess", "1", CVAR_RENDERER | CVAR_BOOL, "skip all post-process renderings" );
idCVar r_skipInteractions( "r_skipInteractions", "0", CVAR_RENDERER | CVAR_BOOL, "skip all light/surface interaction drawing" );
idCVar r_skipDynamicTextures( "r_skipDynamicTextures", "0", CVAR_RENDERER | CVAR_BOOL, "don't dynamically create textures" );
idCVar r_skipCopyTexture( "r_skipCopyTexture", "0", CVAR_RENDERER | CVAR_BOOL, "do all rendering, but don't actually copyTexSubImage2D" );
idCVar r_skipBackEnd( "r_skipBackEnd", "0", CVAR_RENDERER | CVAR_BOOL, "don't draw anything" );
idCVar r_skipRender( "r_skipRender", "0", CVAR_RENDERER | CVAR_BOOL, "skip 3D rendering, but pass 2D" );
// RB begin
idCVar r_skipRenderContext( "r_skipRenderContext", "0", CVAR_RENDERER | CVAR_BOOL, "DISABLED: NULL the rendering context during backend 3D rendering" );
// RB end
idCVar r_skipTranslucent( "r_skipTranslucent", "0", CVAR_RENDERER | CVAR_BOOL, "skip the translucent interaction rendering" );
idCVar r_skipAmbient( "r_skipAmbient", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all non-interaction drawing" );
idCVar r_skipNewAmbient( "r_skipNewAmbient", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "bypasses all vertex/fragment program ambient drawing" );
idCVar r_skipBlendLights( "r_skipBlendLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all blend lights" );
idCVar r_skipFogLights( "r_skipFogLights", "0", CVAR_RENDERER | CVAR_BOOL, "skip all fog lights" );
idCVar r_skipDeforms( "r_skipDeforms", "0", CVAR_RENDERER | CVAR_BOOL, "leave all deform materials in their original state" );
idCVar r_skipFrontEnd( "r_skipFrontEnd", "0", CVAR_RENDERER | CVAR_BOOL, "bypasses all front end work, but 2D gui rendering still draws" );
idCVar r_skipUpdates( "r_skipUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't accept any entity or light updates, making everything static" );
idCVar r_skipDecals( "r_skipDecals", "0", CVAR_RENDERER | CVAR_BOOL, "skip decal surfaces" );
idCVar r_skipOverlays( "r_skipOverlays", "0", CVAR_RENDERER | CVAR_BOOL, "skip overlay surfaces" );
idCVar r_skipSpecular( "r_skipSpecular", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_CHEAT | CVAR_ARCHIVE, "use black for specular1" );
idCVar r_skipBump( "r_skipBump", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "uses a flat surface instead of the bump map" );
idCVar r_skipDiffuse( "r_skipDiffuse", "0", CVAR_RENDERER | CVAR_BOOL, "use black for diffuse" );
idCVar r_skipSubviews( "r_skipSubviews", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = don't render any gui elements on surfaces" );
idCVar r_skipGuiShaders( "r_skipGuiShaders", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all gui elements on surfaces, 2 = skip drawing but still handle events, 3 = draw but skip events", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_skipParticles( "r_skipParticles", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = skip all particle systems", 0, 1, idCmdSystem::ArgCompletion_Integer<0, 1> );
idCVar r_skipShadows( "r_skipShadows", "0", CVAR_RENDERER | CVAR_BOOL  | CVAR_ARCHIVE, "disable shadows" );

idCVar r_useLightPortalCulling( "r_useLightPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_useLightAreaCulling( "r_useLightAreaCulling", "1", CVAR_RENDERER | CVAR_BOOL, "0 = off, 1 = on" );
idCVar r_useLightScissors( "r_useLightScissors", "3", CVAR_RENDERER | CVAR_INTEGER, "0 = no scissor, 1 = non-clipped scissor, 2 = near-clipped scissor, 3 = fully-clipped scissor", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_useEntityPortalCulling( "r_useEntityPortalCulling", "1", CVAR_RENDERER | CVAR_INTEGER, "0 = none, 1 = cull frustum corners to plane, 2 = exact clip the frustum faces", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_logFile( "r_logFile", "0", CVAR_RENDERER | CVAR_INTEGER, "number of frames to emit GL logs" );
idCVar r_clear( "r_clear", "2", CVAR_RENDERER, "force screen clear every frame, 1 = purple, 2 = black, 'r g b' = custom" );

idCVar r_offsetFactor( "r_offsetfactor", "0", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
// RB: offset factor was 0, and units were -600 which caused some very ugly polygon offsets on Android so I reverted the values to the same as in Q3A
#if defined(__ANDROID__)
idCVar r_offsetUnits( "r_offsetunits", "-2", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
#else
idCVar r_offsetUnits( "r_offsetunits", "-600", CVAR_RENDERER | CVAR_FLOAT, "polygon offset parameter" );
#endif
// RB end

idCVar r_shadowPolygonOffset( "r_shadowPolygonOffset", "-1", CVAR_RENDERER | CVAR_FLOAT, "bias value added to depth test for stencil shadow drawing" );
idCVar r_shadowPolygonFactor( "r_shadowPolygonFactor", "0", CVAR_RENDERER | CVAR_FLOAT, "scale value for stencil shadow drawing" );
idCVar r_subviewOnly( "r_subviewOnly", "0", CVAR_RENDERER | CVAR_BOOL, "1 = don't render main view, allowing subviews to be debugged" );
idCVar r_testGamma( "r_testGamma", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels", 0, 195 );
idCVar r_testGammaBias( "r_testGammaBias", "0", CVAR_RENDERER | CVAR_FLOAT, "if > 0 draw a grid pattern to test gamma levels" );
idCVar r_lightScale( "r_lightScale", "3", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "all light intensities are multiplied by this", 0, 100 );
idCVar r_flareSize( "r_flareSize", "1", CVAR_RENDERER | CVAR_FLOAT, "scale the flare deforms from the material def" );

idCVar r_skipPrelightShadows( "r_skipPrelightShadows", "0", CVAR_RENDERER | CVAR_BOOL, "skip the dmap generated static shadow volumes" );
idCVar r_useScissor( "r_useScissor", "1", CVAR_RENDERER | CVAR_BOOL, "scissor clip as portals and lights are processed" );
idCVar r_useLightDepthBounds( "r_useLightDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on lights to reduce both shadow and interaction fill" );
idCVar r_useShadowDepthBounds( "r_useShadowDepthBounds", "1", CVAR_RENDERER | CVAR_BOOL, "use depth bounds test on individual shadow volumes to reduce shadow fill" );
// RB begin
idCVar r_useHalfLambertLighting( "r_useHalfLambertLighting", "1", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "use Half-Lambert lighting instead of classic Lambert, requires reloadShaders" );
// RB end

idCVar r_screenFraction( "r_screenFraction", "100", CVAR_RENDERER | CVAR_INTEGER, "for testing fill rate, the resolution of the entire screen can be changed" );
idCVar r_usePortals( "r_usePortals", "1", CVAR_RENDERER | CVAR_BOOL, " 1 = use portals to perform area culling, otherwise draw everything" );
idCVar r_singleLight( "r_singleLight", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one light" );
idCVar r_singleEntity( "r_singleEntity", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one entity" );
idCVar r_singleSurface( "r_singleSurface", "-1", CVAR_RENDERER | CVAR_INTEGER, "suppress all but one surface on each entity" );
idCVar r_singleArea( "r_singleArea", "0", CVAR_RENDERER | CVAR_BOOL, "only draw the portal area the view is actually in" );
idCVar r_orderIndexes( "r_orderIndexes", "1", CVAR_RENDERER | CVAR_BOOL, "perform index reorganization to optimize vertex use" );
idCVar r_lightAllBackFaces( "r_lightAllBackFaces", "0", CVAR_RENDERER | CVAR_BOOL, "light all the back faces, even when they would be shadowed" );

// visual debugging info
idCVar r_showPortals( "r_showPortals", "0", CVAR_RENDERER | CVAR_BOOL, "draw portal outlines in color based on passed / not passed" );
idCVar r_showUnsmoothedTangents( "r_showUnsmoothedTangents", "0", CVAR_RENDERER | CVAR_BOOL, "if 1, put all nvidia register combiner programming in display lists" );
idCVar r_showSilhouette( "r_showSilhouette", "0", CVAR_RENDERER | CVAR_BOOL, "highlight edges that are casting shadow planes" );
idCVar r_showVertexColor( "r_showVertexColor", "0", CVAR_RENDERER | CVAR_BOOL, "draws all triangles with the solid vertex color" );
idCVar r_showUpdates( "r_showUpdates", "0", CVAR_RENDERER | CVAR_BOOL, "report entity and light updates and ref counts" );
idCVar r_showDemo( "r_showDemo", "0", CVAR_RENDERER | CVAR_BOOL, "report reads and writes to the demo file" );
idCVar r_showDynamic( "r_showDynamic", "0", CVAR_RENDERER | CVAR_BOOL, "report stats on dynamic surface generation" );
idCVar r_showTrace( "r_showTrace", "0", CVAR_RENDERER | CVAR_INTEGER, "show the intersection of an eye trace with the world", idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_showIntensity( "r_showIntensity", "0", CVAR_RENDERER | CVAR_BOOL, "draw the screen colors based on intensity, red = 0, green = 128, blue = 255" );
idCVar r_showLights( "r_showLights", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = just print volumes numbers, highlighting ones covering the view, 2 = also draw planes of each volume, 3 = also draw edges of each volume", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showShadows( "r_showShadows", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = visualize the stencil shadow volumes, 2 = draw filled in", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showLightScissors( "r_showLightScissors", "0", CVAR_RENDERER | CVAR_BOOL, "show light scissor rectangles" );
idCVar r_showLightCount( "r_showLightCount", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = colors surfaces based on light count, 2 = also count everything through walls, 3 = also print overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showViewEntitys( "r_showViewEntitys", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = displays the bounding boxes of all view models, 2 = print index numbers" );
idCVar r_showTris( "r_showTris", "0", CVAR_RENDERER | CVAR_INTEGER, "enables wireframe rendering of the world, 1 = only draw visible ones, 2 = draw all front facing, 3 = draw all, 4 = draw with alpha", 0, 4, idCmdSystem::ArgCompletion_Integer<0, 4> );
idCVar r_showSurfaceInfo( "r_showSurfaceInfo", "0", CVAR_RENDERER | CVAR_BOOL, "show surface material name under crosshair" );
idCVar r_showNormals( "r_showNormals", "0", CVAR_RENDERER | CVAR_FLOAT, "draws wireframe normals" );
idCVar r_showMemory( "r_showMemory", "0", CVAR_RENDERER | CVAR_BOOL, "print frame memory utilization" );
idCVar r_showCull( "r_showCull", "0", CVAR_RENDERER | CVAR_BOOL, "report sphere and box culling stats" );
idCVar r_showAddModel( "r_showAddModel", "0", CVAR_RENDERER | CVAR_BOOL, "report stats from tr_addModel" );
idCVar r_showDepth( "r_showDepth", "0", CVAR_RENDERER | CVAR_BOOL, "display the contents of the depth buffer and the depth range" );
idCVar r_showSurfaces( "r_showSurfaces", "0", CVAR_RENDERER | CVAR_BOOL, "report surface/light/shadow counts" );
idCVar r_showPrimitives( "r_showPrimitives", "0", CVAR_RENDERER | CVAR_INTEGER, "report drawsurf/index/vertex counts" );
idCVar r_showEdges( "r_showEdges", "0", CVAR_RENDERER | CVAR_BOOL, "draw the sil edges" );
idCVar r_showTexturePolarity( "r_showTexturePolarity", "0", CVAR_RENDERER | CVAR_BOOL, "shade triangles by texture area polarity" );
idCVar r_showTangentSpace( "r_showTangentSpace", "0", CVAR_RENDERER | CVAR_INTEGER, "shade triangles by tangent space, 1 = use 1st tangent vector, 2 = use 2nd tangent vector, 3 = use normal vector", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
idCVar r_showDominantTri( "r_showDominantTri", "0", CVAR_RENDERER | CVAR_BOOL, "draw lines from vertexes to center of dominant triangles" );
idCVar r_showTextureVectors( "r_showTextureVectors", "0", CVAR_RENDERER | CVAR_FLOAT, " if > 0 draw each triangles texture (tangent) vectors" );
idCVar r_showOverDraw( "r_showOverDraw", "0", CVAR_RENDERER | CVAR_INTEGER, "1 = geometry overdraw, 2 = light interaction overdraw, 3 = geometry and light interaction overdraw", 0, 3, idCmdSystem::ArgCompletion_Integer<0, 3> );
// RB begin
idCVar r_showShadowMaps( "r_showShadowMaps", "0", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_showShadowMapLODs( "r_showShadowMapLODs", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
// RB end

idCVar r_useEntityCallbacks( "r_useEntityCallbacks", "1", CVAR_RENDERER | CVAR_BOOL, "if 0, issue the callback immediately at update time, rather than defering" );

idCVar r_showSkel( "r_showSkel", "0", CVAR_RENDERER | CVAR_INTEGER, "draw the skeleton when model animates, 1 = draw model with skeleton, 2 = draw skeleton only", 0, 2, idCmdSystem::ArgCompletion_Integer<0, 2> );
idCVar r_jointNameScale( "r_jointNameScale", "0.02", CVAR_RENDERER | CVAR_FLOAT, "size of joint names when r_showskel is set to 1" );
idCVar r_jointNameOffset( "r_jointNameOffset", "0.5", CVAR_RENDERER | CVAR_FLOAT, "offset of joint names when r_showskel is set to 1" );

idCVar r_debugLineDepthTest( "r_debugLineDepthTest", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "perform depth test on debug lines" );
idCVar r_debugLineWidth( "r_debugLineWidth", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "width of debug lines" );
idCVar r_debugArrowStep( "r_debugArrowStep", "120", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "step size of arrow cone line rotation in degrees", 0, 120 );
idCVar r_debugPolygonFilled( "r_debugPolygonFilled", "1", CVAR_RENDERER | CVAR_BOOL, "draw a filled polygon" );

idCVar r_materialOverride( "r_materialOverride", "", CVAR_RENDERER, "overrides all materials", idCmdSystem::ArgCompletion_Decl<DECL_MATERIAL> );

idCVar r_debugRenderToTexture( "r_debugRenderToTexture", "0", CVAR_RENDERER | CVAR_INTEGER, "" );

idCVar stereoRender_enable( "stereoRender_enable", "0", CVAR_INTEGER | CVAR_ARCHIVE, "1 = side-by-side compressed, 2 = top and bottom compressed, 3 = side-by-side, 4 = 720 frame packed, 5 = interlaced, 6 = OpenGL quad buffer" );
idCVar stereoRender_swapEyes( "stereoRender_swapEyes", "0", CVAR_BOOL | CVAR_ARCHIVE, "reverse eye adjustments" );
idCVar stereoRender_deGhost( "stereoRender_deGhost", "0.05", CVAR_FLOAT | CVAR_ARCHIVE, "subtract from opposite eye to reduce ghosting" );

idCVar r_useVirtualScreenResolution( "r_useVirtualScreenResolution", "0", CVAR_RENDERER | CVAR_BOOL | CVAR_ARCHIVE, "do 2D rendering at 640x480 and stretch to the current resolution" );

// RB: shadow mapping parameters
idCVar r_useShadowMapping( "r_useShadowMapping", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_INTEGER, "use shadow mapping instead of stencil shadows" );
idCVar r_shadowMapFrustumFOV( "r_shadowMapFrustumFOV", "92", CVAR_RENDERER | CVAR_FLOAT, "oversize FOV for point light side matching" );
idCVar r_shadowMapSingleSide( "r_shadowMapSingleSide", "-1", CVAR_RENDERER | CVAR_INTEGER, "only draw a single side (0-5) of point lights" );
idCVar r_shadowMapImageSize( "r_shadowMapImageSize", "1024", CVAR_RENDERER | CVAR_INTEGER, "", 128, 2048 );
idCVar r_shadowMapJitterScale( "r_shadowMapJitterScale", "3", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter offset" );
idCVar r_shadowMapBiasScale( "r_shadowMapBiasScale", "0.0001", CVAR_RENDERER | CVAR_FLOAT, "scale factor for jitter bias" );
idCVar r_shadowMapRandomizeJitter( "r_shadowMapRandomizeJitter", "1", CVAR_RENDERER | CVAR_BOOL, "randomly offset jitter texture each draw" );
idCVar r_shadowMapSamples( "r_shadowMapSamples", "1", CVAR_RENDERER | CVAR_INTEGER, "0, 1, 4, or 16" );
idCVar r_shadowMapSplits( "r_shadowMapSplits", "3", CVAR_RENDERER | CVAR_INTEGER, "number of splits for cascaded shadow mapping with parallel lights", 0, 4 );
idCVar r_shadowMapSplitWeight( "r_shadowMapSplitWeight", "0.9", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodScale( "r_shadowMapLodScale", "1.4", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_shadowMapLodBias( "r_shadowMapLodBias", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_shadowMapPolygonFactor( "r_shadowMapPolygonFactor", "2", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset factor for drawing shadow buffer" );
idCVar r_shadowMapPolygonOffset( "r_shadowMapPolygonOffset", "3000", CVAR_RENDERER | CVAR_FLOAT, "polygonOffset units for drawing shadow buffer" );
idCVar r_shadowMapOccluderFacing( "r_shadowMapOccluderFacing", "2", CVAR_RENDERER | CVAR_INTEGER, "0 = front faces, 1 = back faces, 2 = twosided" );
idCVar r_shadowMapRegularDepthBiasScale( "r_shadowMapRegularDepthBiasScale", "0.999", CVAR_RENDERER | CVAR_FLOAT, "shadowmap bias to fight shadow acne for point and spot lights" );
idCVar r_shadowMapSunDepthBiasScale( "r_shadowMapSunDepthBiasScale", "0.999991", CVAR_RENDERER | CVAR_FLOAT, "shadowmap bias to fight shadow acne for cascaded shadow mapping with parallel lights" );

// RB: HDR parameters
idCVar r_useHDR( "r_useHDR", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use high dynamic range rendering" );
idCVar r_hdrAutoExposure( "r_hdrAutoExposure", "1", CVAR_RENDERER | CVAR_BOOL, "EXPENSIVE: enables adapative HDR tone mapping otherwise the exposure is derived by r_exposure" );
idCVar r_hdrMinLuminance( "r_hdrMinLuminance", "0.005", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrMaxLuminance( "r_hdrMaxLuminance", "300", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrKey( "r_hdrKey", "0.015", CVAR_RENDERER | CVAR_FLOAT, "magic exposure key that works well with Doom 3 maps" );
idCVar r_hdrContrastDynamicThreshold( "r_hdrContrastDynamicThreshold", "2", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is on, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastStaticThreshold( "r_hdrContrastStaticThreshold", "3", CVAR_RENDERER | CVAR_FLOAT, "if auto exposure is off, all pixels brighter than this cause HDR bloom glares" );
idCVar r_hdrContrastOffset( "r_hdrContrastOffset", "100", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_hdrGlarePasses( "r_hdrGlarePasses", "8", CVAR_RENDERER | CVAR_INTEGER, "how many times the bloom blur is rendered offscreen. number should be even" );
idCVar r_hdrDebug( "r_hdrDebug", "0", CVAR_RENDERER | CVAR_FLOAT, "show scene luminance as heat map" );

idCVar r_ldrContrastThreshold( "r_ldrContrastThreshold", "1.1", CVAR_RENDERER | CVAR_FLOAT, "" );
idCVar r_ldrContrastOffset( "r_ldrContrastOffset", "3", CVAR_RENDERER | CVAR_FLOAT, "" );

idCVar r_useFilmicPostProcessEffects( "r_useFilmicPostProcessEffects", "1", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "apply several post process effects to mimic a filmic look" );
idCVar r_forceAmbient( "r_forceAmbient", "0.2", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_FLOAT, "render additional ambient pass to make the game less dark", 0.0f, 0.4f );

idCVar r_useSSGI( "r_useSSGI", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space global illumination and reflections" );
idCVar r_ssgiDebug( "r_ssgiDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssgiFiltering( "r_ssgiFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_useSSAO( "r_useSSAO", "0", CVAR_RENDERER | CVAR_ARCHIVE | CVAR_BOOL, "use screen space ambient occlusion to darken corners" );
idCVar r_ssaoDebug( "r_ssaoDebug", "0", CVAR_RENDERER | CVAR_INTEGER, "" );
idCVar r_ssaoFiltering( "r_ssaoFiltering", "1", CVAR_RENDERER | CVAR_BOOL, "" );
idCVar r_useHierarchicalDepthBuffer( "r_useHierarchicalDepthBuffer", "1", CVAR_RENDERER | CVAR_BOOL, "" );

idCVar r_exposure( "r_exposure", "0.5", CVAR_ARCHIVE | CVAR_RENDERER | CVAR_FLOAT, "HDR exposure or LDR brightness [0.0 .. 1.0]", 0.0f, 1.0f );
// RB end

/*
=============================
R_SetNewMode

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
void R_SetNewMode( const bool fullInit )
{
	// try up to three different configurations
	
	for( int i = 0 ; i < 3 ; i++ )
	{
		if( i == 0 && stereoRender_enable.GetInteger() != STEREO3D_QUAD_BUFFER )
		{
			continue;		// don't even try for a stereo mode
		}
		
		glimpParms_t	parms;
		
		if( r_fullscreen.GetInteger() <= 0 )
		{
			// use explicit position / size for window
			parms.x = r_windowX.GetInteger();
			parms.y = r_windowY.GetInteger();
			parms.width = r_windowWidth.GetInteger();
			parms.height = r_windowHeight.GetInteger();
			// may still be -1 to force a borderless window
			parms.fullScreen = r_fullscreen.GetInteger();
			parms.displayHz = 0;		// ignored
		}
		else
		{
			// get the mode list for this monitor
			idList<vidMode_t> modeList;
			if( !R_GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList ) )
			{
				idLib::Printf( "r_fullscreen reset from %i to 1 because mode list failed.", r_fullscreen.GetInteger() );
				r_fullscreen.SetInteger( 1 );
				R_GetModeListForDisplay( r_fullscreen.GetInteger() - 1, modeList );
			}
			if( modeList.Num() < 1 )
			{
				idLib::Printf( "Going to safe mode because mode list failed." );
				goto safeMode;
			}
			
			parms.x = 0;		// ignored
			parms.y = 0;		// ignored
			parms.fullScreen = r_fullscreen.GetInteger();
			
			// set the parameters we are trying
			if( r_vidMode.GetInteger() < 0 )
			{
				// try forcing a specific mode, even if it isn't on the list
				parms.width = r_customWidth.GetInteger();
				parms.height = r_customHeight.GetInteger();
				parms.displayHz = r_displayRefresh.GetInteger();
			}
			else
			{
				if( r_vidMode.GetInteger() >= modeList.Num() )
				{
					idLib::Printf( "r_vidMode reset from %i to 0.\n", r_vidMode.GetInteger() );
					r_vidMode.SetInteger( 0 );
				}
				
				parms.width = modeList[ r_vidMode.GetInteger() ].width;
				parms.height = modeList[ r_vidMode.GetInteger() ].height;
				parms.displayHz = modeList[ r_vidMode.GetInteger() ].displayHz;
			}
		}
		
		switch( r_antiAliasing.GetInteger() )
		{
			case ANTI_ALIASING_MSAA_2X:
				parms.multiSamples = 2;
				break;
			case ANTI_ALIASING_MSAA_4X:
				parms.multiSamples = 4;
				break;
			case ANTI_ALIASING_MSAA_8X:
				parms.multiSamples = 8;
				break;
				
			default:
				parms.multiSamples = 0;
				break;
		}
		
		if( i == 0 )
		{
			parms.stereo = ( stereoRender_enable.GetInteger() == STEREO3D_QUAD_BUFFER );
		}
		else
		{
			parms.stereo = false;
		}
		
		if( fullInit )
		{
			// create the context as well as setting up the window
			if( GLimp_Init( parms ) )
			{
				// it worked
				break;
			}
		}
		else
		{
			// just rebuild the window
			if( GLimp_SetScreenParms( parms ) )
			{
				// it worked
				break;
			}
		}
		
		if( i == 2 )
		{
			common->FatalError( "Unable to initialize OpenGL" );
		}
		
		if( i == 0 )
		{
			// same settings, no stereo
			continue;
		}
		
safeMode:
		// if we failed, set everything back to "safe mode"
		// and try again
		r_vidMode.SetInteger( 0 );
		r_fullscreen.SetInteger( 1 );
		r_displayRefresh.SetInteger( 0 );
		r_antiAliasing.SetInteger( 0 );
	}
}

/*
=====================
R_PerformanceCounters

This prints both front and back end counters, so it should
only be called when the back end thread is idle.
=====================
*/
void idRenderSystemLocal::PrintPerformanceCounters()
{
	if( r_showPrimitives.GetInteger() != 0 )
	{
		common->Printf( "views:%i draws:%i tris:%i (shdw:%i)\n",
						pc.c_numViews,
						backend.pc.c_drawElements + backend.pc.c_shadowElements,
						( backend.pc.c_drawIndexes + backend.pc.c_shadowIndexes ) / 3,
						backend.pc.c_shadowIndexes / 3
					  );
	}
	
	if( r_showDynamic.GetBool() )
	{
		common->Printf( "callback:%i md5:%i dfrmVerts:%i dfrmTris:%i tangTris:%i guis:%i\n",
						pc.c_entityDefCallbacks,
						pc.c_generateMd5,
						pc.c_deformedVerts,
						pc.c_deformedIndexes / 3,
						pc.c_tangentIndexes / 3,
						pc.c_guiSurfs
					  );
	}
	
	if( r_showCull.GetBool() )
	{
		common->Printf( "%i box in %i box out\n",
						pc.c_box_cull_in, pc.c_box_cull_out );
	}
	
	if( r_showAddModel.GetBool() )
	{
		common->Printf( "callback:%i createInteractions:%i createShadowVolumes:%i\n",
						pc.c_entityDefCallbacks, pc.c_createInteractions, pc.c_createShadowVolumes );
		common->Printf( "viewEntities:%i  shadowEntities:%i  viewLights:%i\n", pc.c_visibleViewEntities,
						pc.c_shadowViewEntities, pc.c_viewLights );
	}
	if( r_showUpdates.GetBool() )
	{
		common->Printf( "entityUpdates:%i  entityRefs:%i  lightUpdates:%i  lightRefs:%i\n",
						pc.c_entityUpdates, pc.c_entityReferences,
						pc.c_lightUpdates, pc.c_lightReferences );
	}
	if( r_showMemory.GetBool() )
	{
		common->Printf( "frameData: %i (%i)\n", frameData->frameMemoryAllocated.GetValue(), frameData->highWaterAllocated );
	}
	
	memset( &pc, 0, sizeof( pc ) );
	memset( &backend.pc, 0, sizeof( backend.pc ) );
}

/*
====================
RenderCommandBuffers
====================
*/
void idRenderSystemLocal::RenderCommandBuffers( const emptyCommand_t* const cmdHead )
{
	// if there isn't a draw view command, do nothing to avoid swapping a bad frame
	bool	hasView = false;
	for( const emptyCommand_t* cmd = cmdHead ; cmd ; cmd = ( const emptyCommand_t* )cmd->next )
	{
		if( cmd->commandId == RC_DRAW_VIEW_3D || cmd->commandId == RC_DRAW_VIEW_GUI )
		{
			hasView = true;
			break;
		}
	}
	if( !hasView )
	{
		return;
	}
	
	// r_skipBackEnd allows the entire time of the back end
	// to be removed from performance measurements, although
	// nothing will be drawn to the screen.  If the prints
	// are going to a file, or r_skipBackEnd is later disabled,
	// usefull data can be received.
	
	// r_skipRender is usually more usefull, because it will still
	// draw 2D graphics
	if( !r_skipBackEnd.GetBool() )
	{
#if !defined(ID_VULKAN) && !defined(USE_GLES2) && !defined(USE_GLES3)
		if( glConfig.timerQueryAvailable )
		{
			if( tr.timerQueryId == 0 )
			{
				glGenQueries( 1, & tr.timerQueryId );
			}
			glBeginQuery( GL_TIME_ELAPSED_EXT, tr.timerQueryId );
			backend.ExecuteBackEndCommands( cmdHead );
			glEndQuery( GL_TIME_ELAPSED_EXT );
			glFlush();
		}
		else
#endif
		{
			backend.ExecuteBackEndCommands( cmdHead );
		}
	}
	
	// pass in null for now - we may need to do some map specific hackery in the future
	resolutionScale.InitForMap( NULL );
}

/*
============
R_GetCommandBuffer

Returns memory for a command buffer (stretchPicCommand_t,
drawSurfsCommand_t, etc) and links it to the end of the
current command chain.
============
*/
void* R_GetCommandBuffer( int bytes )
{
	emptyCommand_t*	cmd;
	
	cmd = ( emptyCommand_t* )R_FrameAlloc( bytes, FRAME_ALLOC_DRAW_COMMAND );
	cmd->next = NULL;
	frameData->cmdTail->next = &cmd->commandId;
	frameData->cmdTail = cmd;
	
	return ( void* )cmd;
}

/*
=================
R_ViewStatistics
=================
*/
static void R_ViewStatistics( viewDef_t* parms )
{
	// report statistics about this view
	if( !r_showSurfaces.GetBool() )
	{
		return;
	}
	common->Printf( "view:%p surfs:%i\n", parms, parms->numDrawSurfs );
}

/*
=============
R_AddDrawViewCmd

This is the main 3D rendering command.  A single scene may
have multiple views if a mirror, portal, or dynamic texture is present.
=============
*/
void	R_AddDrawViewCmd( viewDef_t* parms, bool guiOnly )
{
	drawSurfsCommand_t*	cmd;
	
	cmd = ( drawSurfsCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = ( guiOnly ) ? RC_DRAW_VIEW_GUI : RC_DRAW_VIEW_3D;
	
	cmd->viewDef = parms;
	
	tr.pc.c_numViews++;
	
	R_ViewStatistics( parms );
}

/*
=============
R_AddPostProcess

This issues the command to do a post process after all the views have
been rendered.
=============
*/
void	R_AddDrawPostProcess( viewDef_t* parms )
{
	postProcessCommand_t* cmd = ( postProcessCommand_t* )R_GetCommandBuffer( sizeof( *cmd ) );
	cmd->commandId = RC_POST_PROCESS;
	cmd->viewDef = parms;
}


//=================================================================================




/*
=============
idRenderSystemLocal::idRenderSystemLocal
=============
*/
// idRenderSystemLocal::idRenderSystemLocal() :
// 	unitSquareTriangles( NULL ),
// 	zeroOneCubeTriangles( NULL ),
// 	testImageTriangles( NULL )
// {
// 	Clear();
// }

/*
=============
idRenderSystemLocal::~idRenderSystemLocal
=============
*/
// idRenderSystemLocal::~idRenderSystemLocal()
// {
// }

/*
=============
idRenderSystemLocal::SetColor
=============
*/
void idRenderSystemLocal::SetColor( const idVec4& rgba )
{
	currentColorNativeBytesOrder = LittleLong( PackColor( rgba ) );
}

/*
=============
idRenderSystemLocal::GetColor
=============
*/
uint32 idRenderSystemLocal::GetColor()
{
	return LittleLong( currentColorNativeBytesOrder );
}

/*
=============
idRenderSystemLocal::SetGLState
=============
*/
void idRenderSystemLocal::SetGLState( const uint64 glState )
{
	currentGLState = glState;
}

/*
=============
idRenderSystemLocal::DrawFilled
=============
*/
void idRenderSystemLocal::DrawFilled( const idVec4& color, float x, float y, float w, float h )
{
	SetColor( color );
	DrawStretchPic( x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, whiteMaterial );
}

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
void idRenderSystemLocal::DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, const idMaterial* material )
{
	DrawStretchPic( idVec4( x, y, s1, t1 ), idVec4( x + w, y, s2, t1 ), idVec4( x + w, y + h, s2, t2 ), idVec4( x, y + h, s1, t2 ), material );
}

/*
=============
idRenderSystemLocal::DrawStretchPic
=============
*/
static triIndex_t quadPicIndexes[6] = { 3, 0, 2, 2, 0, 1 };
void idRenderSystemLocal::DrawStretchPic( const idVec4& topLeft, const idVec4& topRight, const idVec4& bottomRight, const idVec4& bottomLeft, const idMaterial* material )
{
	if( !R_IsInitialized() )
	{
		return;
	}
	if( material == NULL )
	{
		return;
	}
	
	idDrawVert* verts = guiModel->AllocTris( 4, quadPicIndexes, 6, material, currentGLState, STEREO_DEPTH_TYPE_NONE );
	if( verts == NULL )
	{
		return;
	}
	
	ALIGNTYPE16 idDrawVert localVerts[4];
	
	localVerts[0].Clear();
	localVerts[0].xyz[0] = topLeft.x;
	localVerts[0].xyz[1] = topLeft.y;
	localVerts[0].SetTexCoord( topLeft.z, topLeft.w );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();
	
	localVerts[1].Clear();
	localVerts[1].xyz[0] = topRight.x;
	localVerts[1].xyz[1] = topRight.y;
	localVerts[1].SetTexCoord( topRight.z, topRight.w );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();
	
	localVerts[2].Clear();
	localVerts[2].xyz[0] = bottomRight.x;
	localVerts[2].xyz[1] = bottomRight.y;
	localVerts[2].SetTexCoord( bottomRight.z, bottomRight.w );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();
	
	localVerts[3].Clear();
	localVerts[3].xyz[0] = bottomLeft.x;
	localVerts[3].xyz[1] = bottomLeft.y;
	localVerts[3].SetTexCoord( bottomLeft.z, bottomLeft.w );
	localVerts[3].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[3].ClearColor2();
	
	WriteDrawVerts16( verts, localVerts, 4 );
}

/*
=============
idRenderSystemLocal::DrawStretchTri
=============
*/
void idRenderSystemLocal::DrawStretchTri( const idVec2& p1, const idVec2& p2, const idVec2& p3, const idVec2& t1, const idVec2& t2, const idVec2& t3, const idMaterial* material )
{
	if( !R_IsInitialized() )
	{
		return;
	}
	if( material == NULL )
	{
		return;
	}
	
	triIndex_t tempIndexes[3] = { 1, 0, 2 };
	
	idDrawVert* verts = guiModel->AllocTris( 3, tempIndexes, 3, material, currentGLState, STEREO_DEPTH_TYPE_NONE );
	if( verts == NULL )
	{
		return;
	}
	
	ALIGNTYPE16 idDrawVert localVerts[3];
	
	localVerts[0].Clear();
	localVerts[0].xyz[0] = p1.x;
	localVerts[0].xyz[1] = p1.y;
	localVerts[0].SetTexCoord( t1 );
	localVerts[0].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[0].ClearColor2();
	
	localVerts[1].Clear();
	localVerts[1].xyz[0] = p2.x;
	localVerts[1].xyz[1] = p2.y;
	localVerts[1].SetTexCoord( t2 );
	localVerts[1].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[1].ClearColor2();
	
	localVerts[2].Clear();
	localVerts[2].xyz[0] = p3.x;
	localVerts[2].xyz[1] = p3.y;
	localVerts[2].SetTexCoord( t3 );
	localVerts[2].SetNativeOrderColor( currentColorNativeBytesOrder );
	localVerts[2].ClearColor2();
	
	WriteDrawVerts16( verts, localVerts, 3 );
}

/*
=============
idRenderSystemLocal::AllocTris
=============
*/
idDrawVert* idRenderSystemLocal::AllocTris( int numVerts, const triIndex_t* indexes, int numIndexes, const idMaterial* material, const stereoDepthType_t stereoType )
{
	return guiModel->AllocTris( numVerts, indexes, numIndexes, material, currentGLState, stereoType );
}

/*
=====================
idRenderSystemLocal::DrawSmallChar

small chars are drawn at native screen resolution
=====================
*/
void idRenderSystemLocal::DrawSmallChar( int x, int y, int ch )
{
	int row, col;
	float frow, fcol;
	float size;
	
	ch &= 255;
	
	if( ch == ' ' )
	{
		return;
	}
	
	if( y < -SMALLCHAR_HEIGHT )
	{
		return;
	}
	
	row = ch >> 4;
	col = ch & 15;
	
	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;
	
	DrawStretchPic( x, y, SMALLCHAR_WIDTH, SMALLCHAR_HEIGHT,
					fcol, frow,
					fcol + size, frow + size,
					charSetMaterial );
}

/*
==================
idRenderSystemLocal::DrawSmallStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawSmallStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor )
{
	idVec4		color;
	const unsigned char*	s;
	int			xx;
	
	// draw the colored text
	s = ( const unsigned char* )string;
	xx = x;
	SetColor( setColor );
	while( *s )
	{
		if( idStr::IsColor( ( const char* )s ) )
		{
			if( !forceColor )
			{
				if( *( s + 1 ) == C_COLOR_DEFAULT )
				{
					SetColor( setColor );
				}
				else
				{
					color = idStr::ColorForIndex( *( s + 1 ) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawSmallChar( xx, y, *s );
		xx += SMALLCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

/*
=====================
idRenderSystemLocal::DrawBigChar
=====================
*/
void idRenderSystemLocal::DrawBigChar( int x, int y, int ch )
{
	int row, col;
	float frow, fcol;
	float size;
	
	ch &= 255;
	
	if( ch == ' ' )
	{
		return;
	}
	
	if( y < -BIGCHAR_HEIGHT )
	{
		return;
	}
	
	row = ch >> 4;
	col = ch & 15;
	
	frow = row * 0.0625f;
	fcol = col * 0.0625f;
	size = 0.0625f;
	
	DrawStretchPic( x, y, BIGCHAR_WIDTH, BIGCHAR_HEIGHT,
					fcol, frow,
					fcol + size, frow + size,
					charSetMaterial );
}

/*
==================
idRenderSystemLocal::DrawBigStringExt

Draws a multi-colored string with a drop shadow, optionally forcing
to a fixed color.

Coordinates are at 640 by 480 virtual resolution
==================
*/
void idRenderSystemLocal::DrawBigStringExt( int x, int y, const char* string, const idVec4& setColor, bool forceColor )
{
	idVec4		color;
	const char*	s;
	int			xx;
	
	// draw the colored text
	s = string;
	xx = x;
	SetColor( setColor );
	while( *s )
	{
		if( idStr::IsColor( s ) )
		{
			if( !forceColor )
			{
				if( *( s + 1 ) == C_COLOR_DEFAULT )
				{
					SetColor( setColor );
				}
				else
				{
					color = idStr::ColorForIndex( *( s + 1 ) );
					color[3] = setColor[3];
					SetColor( color );
				}
			}
			s += 2;
			continue;
		}
		DrawBigChar( xx, y, *s );
		xx += BIGCHAR_WIDTH;
		s++;
	}
	SetColor( colorWhite );
}

//======================================================================================

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
const emptyCommand_t* idRenderSystemLocal::SwapCommandBuffers(
	uint64* frontEndMicroSec,
	uint64* backEndMicroSec,
	uint64* shadowMicroSec,
	uint64* gpuMicroSec )
{

	SwapCommandBuffers_FinishRendering( frontEndMicroSec, backEndMicroSec, shadowMicroSec, gpuMicroSec );
	
	return SwapCommandBuffers_FinishCommandBuffers();
}

/*
=====================
idRenderSystemLocal::SwapCommandBuffers_FinishRendering
=====================
*/
void idRenderSystemLocal::SwapCommandBuffers_FinishRendering(
	uint64* frontEndMicroSec,
	uint64* backEndMicroSec,
	uint64* shadowMicroSec,
	uint64* gpuMicroSec )
{
	SCOPED_PROFILE_EVENT( "SwapCommandBuffers" );
	
	if( gpuMicroSec != NULL )
	{
		*gpuMicroSec = 0;		// until shown otherwise
	}
	
	if( !R_IsInitialized() )
	{
		return;
	}
	

	assert(frameData);	
	// After coming back from an autoswap, we won't have anything to render
	//if( frameData->cmdHead->next != NULL )
	{
		// wait for our fence to hit, which means the swap has actually happened
		// We must do this before clearing any resources the GPU may be using
		backend.BlockingSwapBuffers();
	}
	
	// read back the start and end timer queries from the previous frame
#if !defined(ID_VULKAN)
	if( glConfig.timerQueryAvailable )
	{
		// RB: 64 bit fixes, changed int64 to GLuint64EXT
		GLuint64EXT drawingTimeNanoseconds = 0;
		// RB end
		
		if( tr.timerQueryId != 0 )
		{
			glGetQueryObjectui64vEXT( tr.timerQueryId, GL_QUERY_RESULT, &drawingTimeNanoseconds );
		}
		if( gpuMicroSec != NULL )
		{
			*gpuMicroSec = drawingTimeNanoseconds / 1000;
		}
	}
#endif

	//------------------------------
	
	// save out timing information
	if( frontEndMicroSec != NULL )
	{
		*frontEndMicroSec = pc.frontEndMicroSec;
	}
	if( backEndMicroSec != NULL )
	{
		*backEndMicroSec = backend.pc.totalMicroSec;
	}
	if( shadowMicroSec != NULL )
	{
		*shadowMicroSec = backend.pc.shadowMicroSec;
	}
	
	// print any other statistics and clear all of them
	PrintPerformanceCounters();
	
	// check for dynamic changes that require some initialization
	backend.CheckCVars();
	
#if !defined(ID_VULKAN)
	// RB: resize HDR buffers
	Framebuffer::CheckFramebuffers();
	// RB end
	
	// check for errors
	GL_CheckErrors();
#endif
}

/*
=====================
idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers
=====================
*/
const emptyCommand_t* idRenderSystemLocal::SwapCommandBuffers_FinishCommandBuffers()
{
	if( !R_IsInitialized() )
	{
		return NULL;
	}
	
	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();
	
	// unmap the buffer objects so they can be used by the GPU
	vertexCache.BeginBackEnd();
	
	// save off this command buffer
	const emptyCommand_t* commandBufferHead = frameData->cmdHead;
	
	// copy the code-used drawsurfs that were
	// allocated at the start of the buffer memory to the backEnd referenced locations
	backend.unitSquareSurface = unitSquareSurface_;
	backend.zeroOneCubeSurface = zeroOneCubeSurface_;
	backend.testImageSurface = testImageSurface_;
	
	// use the other buffers next frame, because another CPU
	// may still be rendering into the current buffers
	R_ToggleSmpFrame();
	
	// possibly change the stereo3D mode
	// PC
	UpdateStereo3DMode();
	
	// prepare the new command buffer
	guiModel->BeginFrame();
	
	//------------------------------
	// Make sure that geometry used by code is present in the buffer cache.
	// These use frame buffer cache (not static) because they may be used during
	// map loads.
	//
	// It is important to do this first, so if the buffers overflow during
	// scene generation, the basic surfaces needed for drawing the buffers will
	// always be present.
	//------------------------------
	R_InitDrawSurfFromTri( unitSquareSurface_, *unitSquareTriangles );
	R_InitDrawSurfFromTri( zeroOneCubeSurface_, *zeroOneCubeTriangles );
	R_InitDrawSurfFromTri( testImageSurface_, *testImageTriangles );
	
	// Reset render crop to be the full screen
	renderCrops[0].x1 = 0;
	renderCrops[0].y1 = 0;
	renderCrops[0].x2 = GetWidth() - 1;
	renderCrops[0].y2 = GetHeight() - 1;
	currentRenderCrop = 0;
	
	// this is the ONLY place this is modified
	frameCount++;
	
	// just in case we did a common->Error while this
	// was set
	guiRecursionLevel = 0;
	
	// the first rendering will be used for commands like
	// screenshot, rather than a possible subsequent remote
	// or mirror render
//	primaryWorld = NULL;

	// set the time for shader effects in 2D rendering
	frameShaderTime = Sys_Milliseconds() * 0.001;
	
#if !defined(ID_VULKAN)
	setBufferCommand_t* cmd2 = ( setBufferCommand_t* )R_GetCommandBuffer( sizeof( *cmd2 ) );
	cmd2->commandId = RC_SET_BUFFER;
	cmd2->buffer = ( int )GL_BACK;
#endif

	// the old command buffer can now be rendered, while the new one can
	// be built in parallel
	return commandBufferHead;
}

/*
=====================
idRenderSystemLocal::WriteDemoPics
=====================
*/
void idRenderSystemLocal::WriteDemoPics()
{
	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_GUI_MODEL );
}

/*
=====================
idRenderSystemLocal::WriteEndFrame
=====================
*/
void idRenderSystemLocal::WriteEndFrame()
{
	common->WriteDemo()->WriteInt( DS_RENDER );
	common->WriteDemo()->WriteInt( DC_END_FRAME );
}

/*
=====================
idRenderSystemLocal::DrawDemoPics
=====================
*/
void idRenderSystemLocal::DrawDemoPics()
{
}

/*
=====================
idRenderSystemLocal::GetCroppedViewport

Returns the current cropped pixel coordinates
=====================
*/
void idRenderSystemLocal::GetCroppedViewport( idScreenRect* viewport )
{
	*viewport = renderCrops[currentRenderCrop];
}

/*
========================
idRenderSystemLocal::PerformResolutionScaling

The 3D rendering size can be smaller than the full window resolution to reduce
fill rate requirements while still allowing the GUIs to be full resolution.
In split screen mode the rendering size is also smaller.
========================
*/
void idRenderSystemLocal::PerformResolutionScaling( int& newWidth, int& newHeight )
{

	float xScale = 1.0f;
	float yScale = 1.0f;
	resolutionScale.GetCurrentResolutionScale( xScale, yScale );
	
	newWidth = idMath::Ftoi( GetWidth() * xScale );
	newHeight = idMath::Ftoi( GetHeight() * yScale );
}

/*
================
idRenderSystemLocal::CropRenderSize
================
*/
void idRenderSystemLocal::CropRenderSize( int width, int height )
{
	if( !R_IsInitialized() )
	{
		return;
	}
	
	// close any gui drawing before changing the size
	guiModel->EmitFullScreen();
	guiModel->Clear();
	
	
	if( width < 1 || height < 1 )
	{
		common->Error( "CropRenderSize: bad sizes" );
	}
	
	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_CROP_RENDER );
		common->WriteDemo()->WriteInt( width );
		common->WriteDemo()->WriteInt( height );
		
		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_CROP_RENDER\n" );
		}
	}
	
	idScreenRect& previous = renderCrops[currentRenderCrop];
	
	currentRenderCrop++;
	
	idScreenRect& current = renderCrops[currentRenderCrop];
	
	current.x1 = previous.x1;
	current.x2 = previous.x1 + width - 1;
	current.y1 = previous.y2 - height + 1;
	current.y2 = previous.y2;
}

/*
================
idRenderSystemLocal::UnCrop
================
*/
void idRenderSystemLocal::UnCrop()
{
	if( !R_IsInitialized() )
	{
		return;
	}
	
	if( currentRenderCrop < 1 )
	{
		common->Error( "idRenderSystemLocal::UnCrop: currentRenderCrop < 1" );
	}
	
	// close any gui drawing
	guiModel->EmitFullScreen();
	guiModel->Clear();
	
	currentRenderCrop--;
	
	if( common->WriteDemo() )
	{
		common->WriteDemo()->WriteInt( DS_RENDER );
		common->WriteDemo()->WriteInt( DC_UNCROP_RENDER );
		
		if( r_showDemo.GetBool() )
		{
			common->Printf( "write DC_UNCROP\n" );
		}
	}
}

/*
==============
idRenderSystemLocal::AllocRenderWorld
==============
*/
idRenderWorld* idRenderSystemLocal::AllocRenderWorld()
{
	idRenderWorldLocal* rw;
	rw = new( TAG_RENDER ) idRenderWorldLocal;
	worlds.Append( rw );
	return rw;
}

/*
==============
idRenderSystemLocal::FreeRenderWorld
==============
*/
void idRenderSystemLocal::FreeRenderWorld( idRenderWorld* rw )
{
	if( primaryWorld == rw )
	{
		primaryWorld = NULL;
	}
	worlds.Remove( static_cast<idRenderWorldLocal*>( rw ) );
	delete rw;
}

/*
==============
idRenderSystemLocal::PrintMemInfo
==============
*/
void idRenderSystemLocal::PrintMemInfo( MemInfo_t* mi )
{
	// sum up image totals
	globalImages->PrintMemInfo( mi );
	
	// sum up model totals
	renderModelManager->PrintMemInfo( mi );
	
	// compute render totals
	
}

/*
===============
idRenderSystemLocal::UploadImage
===============
*/
bool idRenderSystemLocal::UploadImage( const char* imageName, const byte* data, int width, int height )
{
	idImage* image = globalImages->GetImage( imageName );
	if( !image )
	{
		return false;
	}
	image->UploadScratch( data, width, height );
	return true;
}
