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
#ifndef INCLUDE_NETWORKGRIDMATECOMMON_HEADER
#define INCLUDE_NETWORKGRIDMATECOMMON_HEADER

#pragma once

#include <I3DEngine.h>
#include <physinterface.h>
#include <ILevelSystem.h>
#include <SimpleSerialize.h>

#include <GridMate/Serialize/Buffer.h>
#include <GridMate/Session/Session.h>

struct ILevelSystem;

namespace GridMate
{
    class EntityReplica;

    class IGridMate;
    class GridSession;
    class GridMember;

    inline ILevelSystem* GetLevelSystem()
    {
        if (gEnv->pSystem)
        {
            return gEnv->pSystem->GetILevelSystem();
        }

        return nullptr;
    }

    /*!
     * Buffer types for serialization.
     */
    typedef GridMate::ReadBuffer                ReadBufferType;
    typedef GridMate::WriteBufferStaticInPlace                        WriteBufferType;

    /*!
     * Generic marshalable byte buffer.
     * This is currently used to own memory used by RMIs and NetSerialize. To reduce allocations,
     * the buffer attempts to use internal memory, allocating from the heap only as necessary.
     * Many of the classes owning this structure allocate often, such as each RMI invocation.
     * If this proves a problem, we can pool the invocation wrappers.
     */
    template<size_t BaseSize, typename BufferSizeType = uint16>
    class FlexibleBuffer
    {
    public:
        typedef BufferSizeType SizeType;

        FlexibleBuffer(const char* sourceBuffer = nullptr, SizeType sourceBufferSize = 0)
            : m_buffer  (nullptr)
            , m_size    (0)
        {
            Set(sourceBuffer, sourceBufferSize);
        }

        ~FlexibleBuffer()
        {
            Free();
        }

        void Set(const char* sourceBuffer = nullptr, SizeType sourceBufferSize = 0)
        {
            Free();

            if (0 != sourceBufferSize)
            {
                if (sourceBufferSize > BaseSize)
                {
                    m_buffer = new char[ sourceBufferSize ];
                }
                else
                {
                    m_buffer = m_baseBuffer;
                }

                if (sourceBuffer)
                {
                    memcpy(m_buffer, sourceBuffer, sourceBufferSize);
                }

                m_size = sourceBufferSize;
            }
            else
            {
                m_buffer = nullptr;
                m_size = 0;
            }
        }

        void Free()
        {
            if (m_buffer != m_baseBuffer)
            {
                delete[] m_buffer;
            }

            m_buffer = nullptr;
            m_size = 0;
        }

        ReadBufferType GetReadBuffer() const
        {
            return ReadBufferType(EndianType::BigEndian, m_buffer, m_size);
        }

        WriteBufferType GetWriteBuffer()
        {
            return WriteBufferType(m_buffer, m_size);
        }

        char* GetData() const { return m_buffer; }
        SizeType GetSize() const { return m_size; }

        class Marshaler
        {
        public:

            void Marshal(GridMate::WriteBuffer& wb, const FlexibleBuffer<BaseSize, BufferSizeType>& b)
            {
                wb.Write(b.m_size);

                if (0 != b.m_size)
                {
                    wb.WriteRaw(b.m_buffer, b.m_size);
                }
            }

            void Unmarshal(FlexibleBuffer<BaseSize, BufferSizeType>& b, GridMate::ReadBuffer& rb)
            {
                b.Free();

                rb.Read(b.m_size);

                if (0 != b.m_size)
                {
                    b.m_buffer = new char[ b.m_size ];
                    rb.ReadRaw(b.m_buffer, b.m_size);
                }
            }
        };

        char*       m_buffer;
        SizeType    m_size;

        char        m_baseBuffer[BaseSize];
    };

    /*!
     * Smart-ptr managed version of FlexibleBuffer.
     */
    template<size_t BaseSize, typename BufferSizeType = uint16>
    class ManagedFlexibleBuffer
        : public _i_multithread_reference_target_t
        , public FlexibleBuffer<BaseSize, BufferSizeType>
    {
    public:

        typedef _smart_ptr<ManagedFlexibleBuffer<BaseSize> > Ptr;

        typedef FlexibleBuffer<BaseSize> Parent;
        ManagedFlexibleBuffer()
            : Parent() {}
        ManagedFlexibleBuffer(BufferSizeType sourceSize)
            : Parent(nullptr, sourceSize) {}
        ManagedFlexibleBuffer(const char* sourceBuffer, BufferSizeType sourceSize)
            : Parent(sourceBuffer, sourceSize) {}

        /// Marshaler to handle sending buffers via smart pointer.
        class PtrMarshaler
        {
        public:

            typedef ManagedFlexibleBuffer<BaseSize, BufferSizeType> BufferType;

            void Marshal(GridMate::WriteBuffer& wb, const typename BufferType::Ptr& b)
            {
                const typename BufferType::SizeType size = b.get() ? b->m_size : 0;
                wb.Write(size);

                if (0 != size)
                {
                    wb.WriteRaw(b->m_buffer, size);
                }
            }

            void Unmarshal(typename BufferType::Ptr& b, GridMate::ReadBuffer& rb)
            {
                b = new BufferType();

                rb.Read(b->m_size);

                if (0 != b->m_size)
                {
                    b->m_buffer = new char[ b->m_size ];
                    rb.ReadRaw(b->m_buffer, b->m_size);
                }
            }
        };
    };
} // namespace GridMate

#endif // INCLUDE_NETWORKGRIDMATECOMMON_HEADER
