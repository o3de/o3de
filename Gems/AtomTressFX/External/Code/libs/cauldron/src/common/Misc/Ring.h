// AMD AMDUtils code
// 
// Copyright(c) 2018 Advanced Micro Devices, Inc.All rights reserved.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#pragma once

// This is the typical ring buffer, it is used by resources that will be reused. 
// For example the command Lists, the 'dynamic' constant buffers, etc..
//
class Ring
{
public:
    void Create(uint32_t TotalSize)
    {
        m_Head = 0;
        m_AllocatedSize = 0;
        m_TotalSize = TotalSize;
    }

    uint32_t GetSize() { return m_AllocatedSize; }
    uint32_t GetHead() { return m_Head; }
    uint32_t GetTail() { return (m_Head + m_AllocatedSize) % m_TotalSize; }

    //helper to avoid allocating chunks that wouldn't fit contiguously in the ring
    uint32_t PaddingToAvoidCrossOver(uint32_t size)
    {
        int tail = GetTail();
        if ((tail + size) > m_TotalSize)
            return (m_TotalSize - tail);
        else
            return 0;
    }

    bool Alloc(uint32_t size, uint32_t *pOut)
    {
        if (m_AllocatedSize + size <= m_TotalSize)
        {
            if (pOut)
                *pOut = GetTail();

            m_AllocatedSize += size;
            return true;
        }

        assert(false);
        return false;
    }

    bool Free(uint32_t size)
    {
        if (m_AllocatedSize > size)
        {
            m_Head = (m_Head + size) % m_TotalSize;
            m_AllocatedSize -= size;
            return true;
        }
        return false;
    }
private:
    uint32_t m_Head;
    uint32_t m_AllocatedSize;
    uint32_t m_TotalSize;
};

// 
// This class can be thought as ring buffer inside a ring buffer. The outer ring is for , 
// the frames and the internal one is for the resources that were allocated for that frame.
// The size of the outer ring is typically the number of back buffers.
//
// When the outer ring is full, for the next allocation it automatically frees the entries 
// of the oldest frame and makes those entries available for the next frame. This happens 
// when you call 'OnBeginFrame()' 
//
class RingWithTabs
{
public:

    void OnCreate(uint32_t numberOfBackBuffers, uint32_t memTotalSize)
    {
        m_backBufferIndex = 0;
        m_numberOfBackBuffers = numberOfBackBuffers;

        //init mem per frame tracker
        m_memAllocatedInFrame = 0;
        for (int i = 0; i < 4; i++)
            m_allocatedMemPerBackBuffer[i] = 0;

        m_mem.Create(memTotalSize);
    }

    void OnDestroy()
    {
        m_mem.Free(m_mem.GetSize());
    }

    bool Alloc(uint32_t size, uint32_t *pOut)
    {
        uint32_t padding = m_mem.PaddingToAvoidCrossOver(size);
        if (padding > 0)
        {
            m_memAllocatedInFrame += padding;

            if (m_mem.Alloc(padding, NULL) == false) //alloc chunk to avoid crossover, ignore offset        
            {
                return false;  //no mem, cannot allocate apdding
            }
        }

        if (m_mem.Alloc(size, pOut) == true)
        {
            m_memAllocatedInFrame += size;
            return true;
        }
        return false;
    }

    void OnBeginFrame()
    {
        m_allocatedMemPerBackBuffer[m_backBufferIndex] = m_memAllocatedInFrame;
        m_memAllocatedInFrame = 0;

        m_backBufferIndex = (m_backBufferIndex + 1) % m_numberOfBackBuffers;

        // free all the entries for the oldest buffer in one go
        uint32_t memToFree = m_allocatedMemPerBackBuffer[m_backBufferIndex];
        m_mem.Free(memToFree);
    }
private:
    //internal ring buffer
    Ring m_mem;

    //this is the external ring buffer (I could have reused the Ring class though)
    uint32_t m_backBufferIndex;
    uint32_t m_numberOfBackBuffers;

    uint32_t m_memAllocatedInFrame;
    uint32_t m_allocatedMemPerBackBuffer[4];
};
