/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.

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
#ifndef __BUFFEROBJECT_H__
#define __BUFFEROBJECT_H__

#if defined( ID_VULKAN )
#include "Vulkan/Allocator_VK.h"
#endif

/*
================================================================================================

	Buffer Objects

================================================================================================
*/

enum bufferMapType_t
{
	BM_READ,			// map for reading
	BM_WRITE			// map for writing
};

enum bufferUsageType_t {
	BU_STATIC,			// GPU R
	BU_DYNAMIC,			// GPU R, CPU R/W
};

// Returns all targets to virtual memory use instead of buffer object use.
// Call this before doing any conventional buffer reads, like screenshots.
void UnbindBufferObjects();

/*
================================================================================================

idBufferObject

================================================================================================
*/

class idBufferObject {
public:
						idBufferObject();

	int					GetSize() const { return ( m_size & ~MAPPED_FLAG ); }
	int					GetAllocedSize() const { return ( ( m_size & ~MAPPED_FLAG ) + 15 ) & ~15; }
	bufferUsageType_t	GetUsage() const { return m_usage; }
	int					GetOffset() const { return ( m_offsetInOtherBuffer & ~OWNS_BUFFER_FLAG ); }

	bool				IsMapped() const { return ( m_size & MAPPED_FLAG ) != 0; }

protected:
	void				SetMapped() const { const_cast< int & >( m_size ) |= MAPPED_FLAG; }
	void				SetUnmapped() const { const_cast< int & >( m_size ) &= ~MAPPED_FLAG; }
	bool				OwnsBuffer() const { return ( ( m_offsetInOtherBuffer & OWNS_BUFFER_FLAG ) != 0 ); }

protected:
	int					m_size;					// size in bytes
	int					m_offsetInOtherBuffer;	// offset in bytes
	bufferUsageType_t	m_usage;

#if defined(ID_VULKAN)
public:
	VkBuffer			GetAPIObject() const { return m_apiObject; }

protected:
	VkBuffer			m_apiObject;

#if defined( ID_USE_AMD_ALLOCATOR )
	VmaAllocation		m_vmaAllocation;
	VmaAllocationInfo	m_allocation;
#else
	vulkanAllocation_t	m_allocation;
#endif

#else
	void* 				apiObject;
#endif /* ID_VULKAN */

	// sizeof() confuses typeinfo...
	static const int	MAPPED_FLAG			= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );
	static const int	OWNS_BUFFER_FLAG	= 1 << ( 4 /* sizeof( int ) */ * 8 - 1 );	
};


/*
================================================
idVertexBuffer
================================================
*/
class idVertexBuffer : public idBufferObject
{
public:
	idVertexBuffer();
	~idVertexBuffer();
	
	// Allocate or free the buffer.
	bool				AllocBufferObject( const void* data, int allocSize, bufferUsageType_t usage );
	void				FreeBufferObject();
	
	// Make this buffer a reference to another buffer.
	void				Reference( const idVertexBuffer& other );
	void				Reference( const idVertexBuffer& other, int refOffset, int refSize );
	
	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int size, int offset = 0 ) const;
	
	void* 				MapBuffer( bufferMapType_t mapType ) const;
	idDrawVert* 		MapVertexBuffer( bufferMapType_t mapType ) const
	{
		return static_cast< idDrawVert* >( MapBuffer( mapType ) );
	}
	void				UnmapBuffer() const;
	
private:
	void				ClearWithoutFreeing();
	
	DISALLOW_COPY_AND_ASSIGN( idVertexBuffer );
};

/*
================================================
idIndexBuffer
================================================
*/
class idIndexBuffer : public idBufferObject
{
public:
	idIndexBuffer();
	~idIndexBuffer();
	
	// Allocate or free the buffer.
	bool				AllocBufferObject( const void* data, int allocSize, bufferUsageType_t usage );
	void				FreeBufferObject();
	
	// Make this buffer a reference to another buffer.
	void				Reference( const idIndexBuffer& other );
	void				Reference( const idIndexBuffer& other, int refOffset, int refSize );
	
	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int size, int offset = 0 ) const;
	
	void* 				MapBuffer( bufferMapType_t mapType ) const;
	triIndex_t* 		MapIndexBuffer( bufferMapType_t mapType ) const
	{
		return static_cast< triIndex_t* >( MapBuffer( mapType ) );
	}
	void				UnmapBuffer() const;
	
private:
	void				ClearWithoutFreeing();
	
	DISALLOW_COPY_AND_ASSIGN( idIndexBuffer );
};


/*
================================================================================================

idUniformBuffer

IMPORTANT NOTICE: on the PC, binding to an offset in uniform buffer objects 
is limited to GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, which is 256 on current nvidia cards,
so joint offsets, which are multiples of 48 bytes, must be in multiples of 16 = 768 bytes.
================================================================================================
*/
class idUniformBuffer : public idBufferObject {
public:
						idUniformBuffer();
						~idUniformBuffer();

	// Allocate or free the buffer.
	bool				AllocBufferObject( const void * data, int allocSize, bufferUsageType_t usage );
	void				FreeBufferObject();

	// Make this buffer a reference to another buffer.
	void				Reference( const idUniformBuffer & other );
	void				Reference( const idUniformBuffer & other, int refOffset, int refSize );

	// Copies data to the buffer. 'size' may be less than the originally allocated size.
	void				Update( const void * data, int size, int offset = 0 ) const;

	void *				MapBuffer( bufferMapType_t mapType );
	void				UnmapBuffer();

private:
	void				ClearWithoutFreeing();

	DISALLOW_COPY_AND_ASSIGN( idUniformBuffer );
};

/*
================================================
idJointBuffer

IMPORTANT NOTICE: on the PC, binding to an offset in uniform buffer objects
is limited to GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, which is 256 on current nvidia cards,
so joint offsets, which are multiples of 48 bytes, must be in multiples of 16 = 768 bytes.
================================================
*/
class idJointBuffer : public idBufferObject
{
public:
	idJointBuffer();
	~idJointBuffer();
	
	// Allocate or free the buffer.
	bool				AllocBufferObject( const float* joints, int numAllocJoints, bufferUsageType_t usage );
	void				FreeBufferObject();
	
	// Make this buffer a reference to another buffer.
	void				Reference( const idJointBuffer& other );
	void				Reference( const idJointBuffer& other, int jointRefOffset, int numRefJoints );
	
	// Copies data to the buffer. 'numJoints' may be less than the originally allocated size.
	void				Update( const float* joints, int numUpdateJoints ) const;
	
	float* 				MapBuffer( bufferMapType_t mapType ) const;
	void				UnmapBuffer() const;
	
	int					GetNumJoints() const
	{
		return GetSize();
	}
	
	void				Swap( idJointBuffer& other );
		
private:
	void				ClearWithoutFreeing();
	
	DISALLOW_COPY_AND_ASSIGN( idJointBuffer );
};

#endif // !__BUFFEROBJECT_H__
