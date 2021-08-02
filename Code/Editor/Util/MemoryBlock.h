/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Memory block helper used with ZLib


#ifndef CRYINCLUDE_EDITOR_UTIL_MEMORYBLOCK_H
#define CRYINCLUDE_EDITOR_UTIL_MEMORYBLOCK_H
#pragma once
#include "RefCountBase.h"

#include "Include/EditorCoreAPI.h"
struct IEditor;
class CArchive;

class EDITOR_CORE_API CMemoryBlock
    : public CRefCountBase
{
public:
    CMemoryBlock();
    CMemoryBlock(const CMemoryBlock& mem);
    ~CMemoryBlock();

    CMemoryBlock& operator=(const CMemoryBlock& mem);

    //! Allocate or reallocate memory for this block.
    //! @param size Amount of memory in bytes to allocate.
    //! @return true if the allocation succeeded.
    bool Allocate(int size, int uncompressedSize = 0);

    //! Frees memory allocated in this block (if owned).
    //! Just clears internal references (if unowned).
    void Free();

    //! Attach memory buffer to this block.
    //! Ownership is not transferred; this buffer will not be deleted by CMemoryBlock
    void Attach(void* buffer, int size, int uncompressedSize = 0);

    //! Detach memory buffer that was previously attached.
    //! Note: Implemented as Free()
    void Detach();

    //! Returns amount of allocated memory in this block.
    int GetSize() const { return m_size; }

    //! Returns amount of allocated memory in this block.
    int GetUncompressedSize() const { return m_uncompressedSize; }

    void* GetBuffer() const { return m_buffer; };

    //! Copy memory range to memory block.
    void Copy(void* src, int size);

    //! Compress this memory block to specified memory block.
    //! @param toBlock target memory block where compressed result will be stored.
    void Compress(CMemoryBlock& toBlock) const;

    //! Uncompress this memory block to specified memory block.
    //! @param toBlock target memory block where compressed result will be stored.
    void Uncompress(CMemoryBlock& toBlock) const;

    //! Serialize memory block to archive.
    void Serialize(CArchive& ar);

    //! Is MemoryBlock is empty.
    bool IsEmpty() const { return m_buffer == 0; }

private:
    void* m_buffer;
    int m_size;
    //! If not 0, memory block is compressed.
    int m_uncompressedSize;
    //! True if memory block owns its memory.
    bool m_owns;
};
#endif // CRYINCLUDE_EDITOR_UTIL_MEMORYBLOCK_H
