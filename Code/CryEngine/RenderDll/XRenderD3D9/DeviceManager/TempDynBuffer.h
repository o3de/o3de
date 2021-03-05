/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEMANAGER_TEMPDYNBUFFER_H
#define CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEMANAGER_TEMPDYNBUFFER_H
#pragma once

#include <RenderDll/Common/DevBuffer.h>

namespace TempDynBuffer
{
    enum State
    {
        Default,
        Allocated,
        Locked,
        Filled
    };

    struct ValidatorNull
    {
        void Check(State) {}
        void Check2(State, State) {}
        void Set(State) {}
    };

    struct ValidatorDbgBreak
    {
        ValidatorDbgBreak()
            : m_state(Default) {}

        void Check(State expectedState)
        {
            if (m_state != expectedState)
            {
                __debugbreak();
            }
        }

        void Check2(State expectedState0, State expectedState1)
        {
            if (m_state != expectedState0 && m_state != expectedState1)
            {
                __debugbreak();
            }
        }

        void Set(State newState)
        {
            m_state = newState;
        }

        State m_state;
    };

#if !defined(_RELEASE)
    typedef ValidatorDbgBreak ValidatorDefault;
#else
    typedef ValidatorNull ValidatorDefault;
#endif

    //////////////////////////////////////////////////////////////////////////
    template<typename T, BUFFER_BIND_TYPE bindType, class Validator>
    class TempDynBufferBase
    {
    public:
        static const buffer_handle_t invalidHandle = ~0u;

    protected:
        TempDynBufferBase(IRenderer* renderer)
            : m_handle(invalidHandle)
            , m_renderer(renderer)
            , m_numElements(0)
            , m_DevBufMan(renderer->GetDeviceBufferManager())
            , m_validator() {}
        ~TempDynBufferBase() { m_validator.Check(Default); }

        void Allocate(size_t numElements) { AllocateInternal(numElements, sizeof(T)); }
        bool IsAllocated() const { return m_handle != invalidHandle; }
        void Release();

        void Update(const T* pData) { UpdateInternal(pData, sizeof(T)); }
        T* Lock();
        void Unlock();

    protected:
        void AllocateInternal(size_t numElements, size_t elementSize);
        void UpdateInternal(const void* pData, size_t elementSize);

    protected:
        IRenderer* m_renderer;
        size_t m_handle;
        size_t m_numElements;
        CGuardedDeviceBufferManager m_DevBufMan;
        Validator m_validator;
    };

    template<typename T, BUFFER_BIND_TYPE bindType, class Validator>
    void TempDynBufferBase<T, bindType, Validator>::AllocateInternal(size_t numElements, size_t elementSize)
    {
        m_validator.Check(Default);
        BUFFER_USAGE usageType = m_renderer->IsVideoThreadModeEnabled()
            ? BU_WHEN_LOADINGTHREAD_ACTIVE // use a separate pool for everything in video rendering mode
            : BU_TRANSIENT_RT; // default to transient_RT
        m_handle = m_DevBufMan.Create(bindType, usageType, numElements * elementSize);
        IF (m_handle != invalidHandle, 1)
        {
            m_numElements = numElements;
            m_validator.Set(Allocated);
        }
    }

    template<typename T, BUFFER_BIND_TYPE bindType, class Validator>
    void TempDynBufferBase<T, bindType, Validator>::Release()
    {
        IF (m_handle != invalidHandle, 1)
        {
            m_validator.Check2(Allocated, Filled);
            m_DevBufMan.Destroy(m_handle);
            m_handle = invalidHandle;
            m_numElements = 0;
        }
        m_validator.Set(Default);
    }

    template<typename T, BUFFER_BIND_TYPE bindType, class Validator>
    void TempDynBufferBase<T, bindType, Validator>::UpdateInternal(const void* pData, size_t elementSize)
    {
        m_validator.Check(Allocated);
        m_DevBufMan.UpdateBuffer(m_handle, pData, m_numElements * elementSize);
        m_validator.Set(Filled);
    }

    template<typename T, BUFFER_BIND_TYPE bindType, class Validator>
    T* TempDynBufferBase<T, bindType, Validator>::Lock()
    {
        m_validator.Check(Allocated);
        T* p = (T*) m_DevBufMan.BeginWrite(m_handle);
        m_validator.Set(Locked);
        return p;
    }

    template<typename T, BUFFER_BIND_TYPE bindType, class Validator>
    void TempDynBufferBase<T, bindType, Validator>::Unlock()
    {
        m_validator.Check(Locked);
        m_DevBufMan.EndReadWrite(m_handle);
        m_validator.Set(Filled);
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T, class Validator = ValidatorDefault>
    class TempDynVBBase
        : public TempDynBufferBase<T, BBT_VERTEX_BUFFER, Validator>
    {
    public:
        typedef TempDynBufferBase<T, BBT_VERTEX_BUFFER, Validator> Base;

    public:
        using Base::IsAllocated;
        using Base::Release;

        using Base::Update;
        using Base::Lock;
        using Base::Unlock;

    protected:
        TempDynVBBase(IRenderer* renderer)
            : TempDynBufferBase<T, BBT_VERTEX_BUFFER, Validator>(renderer),
            m_renderer(renderer){}
        ~TempDynVBBase() {}

        void BindInternal(uint32 streamID, size_t stride);

    protected:
        IRenderer* m_renderer = nullptr;
        using Base::m_handle;
        using Base::m_validator;
    };

    template<typename T, class Validator>
    void TempDynVBBase<T, Validator>::BindInternal(uint32 streamID, size_t stride)
    {
        m_validator.Check(Filled);
        size_t bufferOffset = ~0;
        D3DBuffer* pVB = this->m_DevBufMan.GetD3D(m_handle, &bufferOffset);
        m_renderer->FX_SetVStream(streamID, pVB, bufferOffset, stride);
    }

    //////////////////////////////////////////////////////////////////////////
    template<typename T, class Validator = ValidatorDefault>
    class TempDynVB
        : public TempDynVBBase<T, Validator>
    {
        friend class TempDynVBAny;

    public:
        typedef TempDynVBBase<T, Validator> Base;

    public:
        static void CreateFillAndBind(const T* pData, size_t numElements, uint32 streamID) { CreateFillAndBindInternal(pData, numElements, streamID, sizeof(T)); }

    public:
        TempDynVB(IRenderer* renderer)
            : TempDynVBBase<T, Validator>(renderer)
            , m_renderer(renderer){}

        using Base::Allocate;
        void Bind(uint32 streamID) { BindInternal(streamID, sizeof(T)); }

    protected:
        static void CreateFillAndBindInternal(const void* pData, size_t numElements, uint32 streamID, size_t stride);

    protected:
        IRenderer* m_renderer = nullptr;
        using Base::BindInternal;

    protected:
        TempDynVB(const TempDynVB&);
        TempDynVB& operator=(const TempDynVB&);
    };

    template<typename T, class Validator>
    void TempDynVB<T, Validator>::CreateFillAndBindInternal(const void* pData, size_t numElements, uint32 streamID, size_t stride)
    {
        TempDynVB<T, Validator> vb(gRenDev);
        vb.AllocateInternal(numElements, stride);
        vb.UpdateInternal(pData, stride);
        vb.BindInternal(streamID, stride);
        vb.Release();
    }

    //////////////////////////////////////////////////////////////////////////
    template<class Validator = ValidatorDefault>
    class TempDynInstVB
        : public TempDynVBBase<void, Validator>
    {
    public:
        typedef TempDynVBBase<void, Validator> Base;

    public:
        TempDynInstVB(IRenderer* renderer)
            : TempDynVBBase<void, Validator>(renderer),
            m_renderer(renderer)
        {}

        void Allocate(size_t numElements, size_t elementSize) { AllocateInternal(numElements, elementSize); }
        void Bind(uint32 streamID, uint32 stride) { BindInternal(streamID, stride); }

    protected:
        using Base::AllocateInternal;
        using Base::BindInternal;

    protected:
        IRenderer* m_renderer = nullptr;

        TempDynInstVB(const TempDynInstVB&);
        TempDynInstVB& operator=(const TempDynInstVB&);
    };

    //////////////////////////////////////////////////////////////////////////
    class TempDynVBAny
    {
    public:
        static void CreateFillAndBind(const void* pData, size_t numElements, uint32 streamID, size_t stride) { TempDynVB<void>::CreateFillAndBindInternal(pData, numElements, streamID, stride); }

    private:
        IRenderer* m_renderer = nullptr;
        TempDynVBAny(IRenderer* renderer);
    };

    //////////////////////////////////////////////////////////////////////////
    template <RenderIndexType idxType>
    struct MapIndexType {};

    template <>
    struct MapIndexType<Index16>
    {
        typedef uint16 Type;
        enum
        {
            Size = sizeof(Type)
        };
    };

    template <>
    struct MapIndexType<Index32>
    {
        typedef uint32 Type;
        enum
        {
            Size = sizeof(Type)
        };
    };

    template<RenderIndexType idxType, class Validator = ValidatorDefault>
    class TempDynIB
        : public TempDynBufferBase<typename MapIndexType<idxType>::Type, BBT_INDEX_BUFFER, Validator>
    {
    public:
        typedef typename MapIndexType<idxType>::Type IndexDataType;
        typedef TempDynBufferBase<IndexDataType, BBT_INDEX_BUFFER, Validator> Base;

    public:
        static void CreateFillAndBind(const IndexDataType* pData, size_t numElements);

    public:
        TempDynIB(IRenderer* renderer)
            : TempDynBufferBase<IndexDataType, BBT_INDEX_BUFFER, Validator>(renderer),
            m_renderer(renderer)
        {}

        void Allocate(size_t numElements);
        using Base::IsAllocated;
        using Base::Release;

        using Base::Update;
        using Base::Lock;
        using Base::Unlock;

        void Bind();

    protected:
        IRenderer* m_renderer = nullptr;

        TempDynIB(const TempDynIB&);
        TempDynIB& operator=(const TempDynIB&);

    protected:
        using Base::m_handle;
        using Base::m_validator;
    };

    template<RenderIndexType idxType, class Validator>
    void TempDynIB<idxType, Validator>::Allocate(size_t numElements)
    {
        Base::Allocate(numElements);
    }

    template<RenderIndexType idxType, class Validator>
    void TempDynIB<idxType, Validator>::Bind()
    {
        m_validator.Check(Filled);
        size_t bufferOffset = ~0;
        D3DBuffer* pIB = this->m_DevBufMan.GetD3D(m_handle, &bufferOffset);
        m_renderer->FX_SetIStream(pIB, bufferOffset, idxType);
    }

    template<RenderIndexType idxType, class Validator>
    void TempDynIB<idxType, Validator>::CreateFillAndBind(const IndexDataType* pData, size_t numElements)
    {
        TempDynIB<idxType, Validator> ib(gcpRendD3D);
        ib.Allocate(numElements);
        ib.Update(pData);
        ib.Bind();
        ib.Release();
    }
} // namespace TempDynBuffer

using TempDynBuffer::TempDynVB;
using TempDynBuffer::TempDynVBAny;
typedef TempDynBuffer::TempDynInstVB<> TempDynInstVB;

typedef TempDynBuffer::TempDynIB<Index16> TempDynIB16;
typedef TempDynBuffer::TempDynIB<Index32> TempDynIB32;

#endif // CRYINCLUDE_CRYENGINE_RENDERDLL_XRENDERD3D9_DEVICEMANAGER_TEMPDYNBUFFER_H
