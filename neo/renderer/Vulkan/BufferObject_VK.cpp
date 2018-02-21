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

#include "precompiled.h"
#include "../RenderCommon.h"
#include "../BufferObject.h"
#include "Staging_VK.h"

extern idCVar r_showBuffers;

/*
========================
UnbindBufferObjects
========================
*/
void UnbindBufferObjects() {

}

/*
================================================================================================

idVertexBuffer

================================================================================================
*/

/*
========================
idVertexBuffer::idVertexBuffer
========================
*/
idVertexBuffer::idVertexBuffer() {
	SetUnmapped();
}

/*
========================
idVertexBuffer::AllocBufferObject
========================
*/
bool idVertexBuffer::AllocBufferObject( const void * data, int allocSize, bufferUsageType_t usage ) {
	assert( m_apiObject == VK_NULL_HANDLE );
	assert_16_byte_aligned( data );

	if ( allocSize <= 0 ) {
		idLib::Error( "idVertexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	m_size = allocSize;
	m_usage = usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.size = numBytes;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if ( m_usage == BU_STATIC ) {
		bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

#if defined( ID_USE_AMD_ALLOCATOR )
	VmaMemoryRequirements vmaReq = {};
	if ( m_usage == BU_STATIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	} else if ( m_usage == BU_DYNAMIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaReq.flags = VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT;
	}

	ID_VK_CHECK( vmaCreateBuffer( vmaAllocator, &bufferCreateInfo, &vmaReq, &m_apiObject, &m_vmaAllocation, &m_allocation ) );

#else
	VkResult ret = vkCreateBuffer( vkcontext.device, &bufferCreateInfo, NULL, &m_apiObject );
	assert( ret == VK_SUCCESS );

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( vkcontext.device, m_apiObject, &memoryRequirements );

	vulkanMemoryUsage_t memUsage = ( m_usage == BU_STATIC ) ? VULKAN_MEMORY_USAGE_GPU_ONLY : VULKAN_MEMORY_USAGE_CPU_TO_GPU;

	m_allocation = vulkanAllocator.Allocate( 
		memoryRequirements.size, 
		memoryRequirements.alignment, 
		memoryRequirements.memoryTypeBits, 
		memUsage,
		VULKAN_ALLOCATION_TYPE_BUFFER );

	ID_VK_CHECK( vkBindBufferMemory( vkcontext.device, m_apiObject, m_allocation.deviceMemory, m_allocation.offset ) );
#endif

	if ( r_showBuffers.GetBool() ) {
		idLib::Printf( "vertex buffer alloc %p, (%i bytes)\n", this, GetSize() );
	}

	// copy the data
	if ( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
idVertexBuffer::FreeBufferObject
========================
*/
void idVertexBuffer::FreeBufferObject() {
	if ( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if ( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if ( m_apiObject == VK_NULL_HANDLE ) {
		return;
	}

	if ( r_showBuffers.GetBool() ) {
		idLib::Printf( "vertex buffer free %p, (%i bytes)\n", this, GetSize() );
	}

	if ( m_apiObject != VK_NULL_HANDLE ) {
#if defined( ID_USE_AMD_ALLOCATOR )
		vmaDestroyBuffer( vmaAllocator, m_apiObject, m_vmaAllocation );
		m_apiObject = VK_NULL_HANDLE;
		m_allocation = VmaAllocationInfo();
		m_vmaAllocation = NULL;
#else
		vulkanAllocator.Free( m_allocation );
		vkDestroyBuffer( vkcontext.device, m_apiObject, NULL );
		m_apiObject = VK_NULL_HANDLE;
		m_allocation = vulkanAllocation_t();
#endif
	}

	ClearWithoutFreeing();
}

/*
========================
idVertexBuffer::Update
========================
*/
void idVertexBuffer::Update( const void * data, int size, int offset ) const {
	assert( m_apiObject != VK_NULL_HANDLE );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( size > GetSize() ) {
		idLib::FatalError( "idVertexBuffer::Update: size overrun, %i > %i\n", size, GetSize() );
	}
	
	if ( m_usage == BU_DYNAMIC ) {
		CopyBuffer( 
#if defined( ID_USE_AMD_ALLOCATOR )
			(byte *)m_allocation.pMappedData + GetOffset() + offset, 
#else
			m_allocation.data + GetOffset() + offset,
#endif
			(const byte *)data, size );
	} else {
		VkBuffer stageBuffer;
		VkCommandBuffer commandBuffer;
		int stageOffset = 0;
		byte * stageData = stagingManager.Stage( size, 1, commandBuffer, stageBuffer, stageOffset );

		memcpy( stageData, data, size );

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = stageOffset;
		bufferCopy.dstOffset = GetOffset() + offset;
		bufferCopy.size = size;

		vkCmdCopyBuffer( commandBuffer, stageBuffer, m_apiObject, 1, &bufferCopy );
	}
}

/*
========================
idVertexBuffer::MapBuffer
========================
*/
void * idVertexBuffer::MapBuffer( bufferMapType_t mapType ) const {
	assert( m_apiObject != VK_NULL_HANDLE );

	if ( m_usage == BU_STATIC ) {
		idLib::FatalError( "idVertexBuffer::MapBuffer: Cannot map a buffer marked as BU_STATIC." );
	}

#if defined( ID_USE_AMD_ALLOCATOR )
	void * buffer = (byte *)m_allocation.pMappedData + GetOffset();
#else
	void * buffer = m_allocation.data + GetOffset();
#endif

	SetMapped();

	if ( buffer == NULL ) {
		idLib::FatalError( "idVertexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idVertexBuffer::UnmapBuffer
========================
*/
void idVertexBuffer::UnmapBuffer() const {
	assert( m_apiObject != VK_NULL_HANDLE );

	if ( m_usage == BU_STATIC ) {
		idLib::FatalError( "idVertexBuffer::UnmapBuffer: Cannot unmap a buffer marked as BU_STATIC." );
	}

	SetUnmapped();
}

/*
========================
idVertexBuffer::ClearWithoutFreeing
========================
*/
void idVertexBuffer::ClearWithoutFreeing() {
	m_size = 0;
	m_offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	m_apiObject = VK_NULL_HANDLE;
#if defined( ID_USE_AMD_ALLOCATOR )
	m_allocation = VmaAllocationInfo();
	m_vmaAllocation = NULL;
#else
	m_allocation.deviceMemory = VK_NULL_HANDLE;
#endif
}

/*
================================================================================================

idIndexBuffer

================================================================================================
*/

/*
========================
idIndexBuffer::idIndexBuffer
========================
*/
idIndexBuffer::idIndexBuffer() {
	SetUnmapped();
}

/*
========================
idIndexBuffer::AllocBufferObject
========================
*/
bool idIndexBuffer::AllocBufferObject( const void * data, int allocSize, bufferUsageType_t usage ) {
	assert( m_apiObject == VK_NULL_HANDLE );
	assert_16_byte_aligned( data );

	if ( allocSize <= 0 ) {
		idLib::Error( "idIndexBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	m_size = allocSize;
	m_usage = usage;

	bool allocationFailed = false;

	int numBytes = GetAllocedSize();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.size = numBytes;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	if ( m_usage == BU_STATIC ) {
		bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

#if defined( ID_USE_AMD_ALLOCATOR )
	VmaMemoryRequirements vmaReq = {};
	if ( m_usage == BU_STATIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	} else if ( m_usage == BU_DYNAMIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaReq.flags = VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT;
	}

	ID_VK_CHECK( vmaCreateBuffer( vmaAllocator, &bufferCreateInfo, &vmaReq, &m_apiObject, &m_vmaAllocation, &m_allocation ) );

#else
	VkResult ret = vkCreateBuffer( vkcontext.device, &bufferCreateInfo, NULL, &m_apiObject );
	assert( ret == VK_SUCCESS );

	VkMemoryRequirements memoryRequirements;
	vkGetBufferMemoryRequirements( vkcontext.device, m_apiObject, &memoryRequirements );

	vulkanMemoryUsage_t memUsage = ( m_usage == BU_STATIC ) ? VULKAN_MEMORY_USAGE_GPU_ONLY : VULKAN_MEMORY_USAGE_CPU_TO_GPU;

	m_allocation = vulkanAllocator.Allocate( 
		memoryRequirements.size, 
		memoryRequirements.alignment, 
		memoryRequirements.memoryTypeBits, 
		memUsage,
		VULKAN_ALLOCATION_TYPE_BUFFER );

	ID_VK_CHECK( vkBindBufferMemory( vkcontext.device, m_apiObject, m_allocation.deviceMemory, m_allocation.offset ) );
#endif

	if ( r_showBuffers.GetBool() ) {
		idLib::Printf( "index buffer alloc %p, (%i bytes)\n", this, GetSize() );
	}

	// copy the data
	if ( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
idIndexBuffer::FreeBufferObject
========================
*/
void idIndexBuffer::FreeBufferObject() {
	if ( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if ( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if ( m_apiObject == VK_NULL_HANDLE ) {
		return;
	}

	if ( r_showBuffers.GetBool() ) {
		idLib::Printf( "index buffer free %p, (%i bytes)\n", this, GetSize() );
	}

	if ( m_apiObject != VK_NULL_HANDLE ) {
#if defined( ID_USE_AMD_ALLOCATOR )
		vmaDestroyBuffer( vmaAllocator, m_apiObject, m_vmaAllocation );
		m_apiObject = VK_NULL_HANDLE;
		m_allocation = VmaAllocationInfo();
		m_vmaAllocation = NULL;
#else
		vulkanAllocator.Free( m_allocation );
		vkDestroyBuffer( vkcontext.device, m_apiObject, NULL );
		m_apiObject = VK_NULL_HANDLE;
		m_allocation = vulkanAllocation_t();
#endif
	}

	ClearWithoutFreeing();
}

/*
========================
idIndexBuffer::Update
========================
*/
void idIndexBuffer::Update( const void * data, int size, int offset ) const {
	assert( m_apiObject != VK_NULL_HANDLE );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( size > GetSize() ) {
		idLib::FatalError( "idIndexBuffer::Update: size overrun, %i > %i\n", size, GetSize() );
	}

	if ( m_usage == BU_DYNAMIC ) {
		CopyBuffer( 
#if defined( ID_USE_AMD_ALLOCATOR )
			(byte *)m_allocation.pMappedData + GetOffset() + offset, 
#else
			m_allocation.data + GetOffset() + offset,
#endif
			(const byte *)data, size );
	} else {
		VkBuffer stageBuffer;
		VkCommandBuffer commandBuffer;
		int stageOffset = 0;
		byte * stageData = stagingManager.Stage( size, 1, commandBuffer, stageBuffer, stageOffset );

		memcpy( stageData, data, size );

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = stageOffset;
		bufferCopy.dstOffset = GetOffset() + offset;
		bufferCopy.size = size;

		vkCmdCopyBuffer( commandBuffer, stageBuffer, m_apiObject, 1, &bufferCopy );
	}
}

/*
========================
idIndexBuffer::MapBuffer
========================
*/
void * idIndexBuffer::MapBuffer( bufferMapType_t mapType ) const {
	assert( m_apiObject != VK_NULL_HANDLE );

	if ( m_usage == BU_STATIC ) {
		idLib::FatalError( "idIndexBuffer::MapBuffer: Cannot map a buffer marked as BU_STATIC." );
	}

#if defined( ID_USE_AMD_ALLOCATOR )
	void * buffer = (byte *)m_allocation.pMappedData + GetOffset();
#else
	void * buffer = m_allocation.data + GetOffset();
#endif

	SetMapped();

	if ( buffer == NULL ) {
		idLib::FatalError( "idIndexBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idIndexBuffer::UnmapBuffer
========================
*/
void idIndexBuffer::UnmapBuffer() const {
	assert( m_apiObject != VK_NULL_HANDLE );

	if ( m_usage == BU_STATIC ) {
		idLib::FatalError( "idIndexBuffer::UnmapBuffer: Cannot unmap a buffer marked as BU_STATIC." );
	}

	SetUnmapped();
}

/*
========================
idIndexBuffer::ClearWithoutFreeing
========================
*/
void idIndexBuffer::ClearWithoutFreeing() {
	m_size = 0;
	m_offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	m_apiObject = VK_NULL_HANDLE;
#if defined( ID_USE_AMD_ALLOCATOR )
	m_allocation = VmaAllocationInfo();
	m_vmaAllocation = NULL;
#else
	m_allocation.deviceMemory = VK_NULL_HANDLE;
#endif
}

/*
================================================================================================

idUniformBuffer

================================================================================================
*/

/*
========================
idUniformBuffer::idUniformBuffer
========================
*/
idUniformBuffer::idUniformBuffer() {
	m_usage = BU_DYNAMIC;
	SetUnmapped();
}

/*
========================
idUniformBuffer::AllocBufferObject
========================
*/
bool idUniformBuffer::AllocBufferObject( const void * data, int allocSize, bufferUsageType_t usage ) {
	assert( m_apiObject == VK_NULL_HANDLE );
	assert_16_byte_aligned( data );

	if ( allocSize <= 0 ) {
		idLib::Error( "idUniformBuffer::AllocBufferObject: allocSize = %i", allocSize );
	}

	m_size = allocSize;
	m_usage = usage;

	bool allocationFailed = false;

	const int numBytes = GetAllocedSize();

	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.size = numBytes;
	bufferCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	if ( m_usage == BU_STATIC ) {
		bufferCreateInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}

#if defined( ID_USE_AMD_ALLOCATOR )
	VmaMemoryRequirements vmaReq = {};
	if ( m_usage == BU_STATIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	} else if ( m_usage == BU_DYNAMIC ) {
		vmaReq.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
		vmaReq.flags = VMA_MEMORY_REQUIREMENT_PERSISTENT_MAP_BIT;
	}

	ID_VK_CHECK( vmaCreateBuffer( vmaAllocator, &bufferCreateInfo, &vmaReq, &m_apiObject, &m_vmaAllocation, &m_allocation ) );

#else
	VkResult ret = vkCreateBuffer( vkcontext.device, &bufferCreateInfo, NULL, &m_apiObject );
	assert( ret == VK_SUCCESS );

	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements( vkcontext.device, m_apiObject, &memoryRequirements );

	vulkanMemoryUsage_t memUsage = ( m_usage == BU_STATIC ) ? VULKAN_MEMORY_USAGE_GPU_ONLY : VULKAN_MEMORY_USAGE_CPU_TO_GPU;
	m_allocation = vulkanAllocator.Allocate( 
		memoryRequirements.size, 
		memoryRequirements.alignment, 
		memoryRequirements.memoryTypeBits, 
		memUsage,
		VULKAN_ALLOCATION_TYPE_BUFFER );

	ID_VK_CHECK( vkBindBufferMemory( vkcontext.device, m_apiObject, m_allocation.deviceMemory, m_allocation.offset ) );
#endif

	if ( r_showBuffers.GetBool() ) {
		idLib::Printf( "joint buffer alloc %p, (%i bytes), m_allocation.data %p\n", this, GetSize(), m_allocation.data );
	}

	// copy the data
	if ( data != NULL ) {
		Update( data, allocSize );
	}

	return !allocationFailed;
}

/*
========================
idUniformBuffer::FreeBufferObject
========================
*/
void idUniformBuffer::FreeBufferObject() {
	if ( IsMapped() ) {
		UnmapBuffer();
	}

	// if this is a sub-allocation inside a larger buffer, don't actually free anything.
	if ( OwnsBuffer() == false ) {
		ClearWithoutFreeing();
		return;
	}

	if ( m_apiObject == VK_NULL_HANDLE ) {
		return;
	}

	if ( r_showBuffers.GetBool() ) {
		idLib::Printf( "joint buffer free %p, (%i bytes)\n", this, GetSize() );
	}

	if ( m_apiObject != VK_NULL_HANDLE ) {
#if defined( ID_USE_AMD_ALLOCATOR )
		vmaDestroyBuffer( vmaAllocator, m_apiObject, m_vmaAllocation );
		m_apiObject = VK_NULL_HANDLE;
		m_allocation = VmaAllocationInfo();
		m_vmaAllocation = NULL;
#else
		vulkanAllocator.Free( m_allocation );
		vkDestroyBuffer( vkcontext.device, m_apiObject, NULL );
		m_apiObject = VK_NULL_HANDLE;
		m_allocation = vulkanAllocation_t();
#endif
	}

	ClearWithoutFreeing();
}

/*
========================
idUniformBuffer::Update
========================
*/
void idUniformBuffer::Update( const void * data, int size, int offset ) const {
	assert( m_apiObject != VK_NULL_HANDLE );
	assert_16_byte_aligned( data );
	assert( ( GetOffset() & 15 ) == 0 );

	if ( size > GetSize() ) {
		idLib::FatalError( "idUniformBuffer::Update: size overrun, %i > %i\n", size, m_size );
	}

	if ( m_usage == BU_DYNAMIC ) {
		CopyBuffer( 
#if defined( ID_USE_AMD_ALLOCATOR )
			(byte *)m_allocation.pMappedData + GetOffset() + offset, 
#else
			m_allocation.data + GetOffset() + offset,
#endif
			(const byte *)data, size );
	} else {
		VkBuffer stageBuffer;
		VkCommandBuffer commandBuffer;
		int stageOffset = 0;
		byte * stageData = stagingManager.Stage( size, 1, commandBuffer, stageBuffer, stageOffset );

		memcpy( stageData, data, size );

		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = stageOffset;
		bufferCopy.dstOffset = GetOffset() + offset;
		bufferCopy.size = size;

		vkCmdCopyBuffer( commandBuffer, stageBuffer, m_apiObject, 1, &bufferCopy );
	}
}

/*
========================
idUniformBuffer::MapBuffer
========================
*/
void * idUniformBuffer::MapBuffer( bufferMapType_t mapType ) {
	assert( mapType == BM_WRITE );
	assert( m_apiObject != VK_NULL_HANDLE );

	if ( m_usage == BU_STATIC ) {
		idLib::FatalError( "idUniformBuffer::MapBuffer: Cannot map a buffer marked as BU_STATIC." );
	}

#if defined( ID_USE_AMD_ALLOCATOR )
	void * buffer = (byte *)m_allocation.pMappedData + GetOffset();
#else
	void * buffer = m_allocation.data + GetOffset();
#endif

	SetMapped();

	if ( buffer == NULL ) {
		idLib::FatalError( "idUniformBuffer::MapBuffer: failed" );
	}
	return buffer;
}

/*
========================
idUniformBuffer::UnmapBuffer
========================
*/
void idUniformBuffer::UnmapBuffer() {
	assert( m_apiObject != VK_NULL_HANDLE );

	if ( m_usage == BU_STATIC ) {
		idLib::FatalError( "idUniformBuffer::UnmapBuffer: Cannot unmap a buffer marked as BU_STATIC." );
	}

	SetUnmapped();
}

/*
========================
idUniformBuffer::ClearWithoutFreeing
========================
*/
void idUniformBuffer::ClearWithoutFreeing() {
	m_size = 0;
	m_offsetInOtherBuffer = OWNS_BUFFER_FLAG;
	m_apiObject = VK_NULL_HANDLE;
#if defined( ID_USE_AMD_ALLOCATOR )
	m_allocation = VmaAllocationInfo();
	m_vmaAllocation = NULL;
#else
	m_allocation.deviceMemory = VK_NULL_HANDLE;
#endif
}