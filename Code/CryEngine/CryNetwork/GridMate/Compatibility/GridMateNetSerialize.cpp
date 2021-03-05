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
#include "CryNetwork_precompiled.h"

/*!
 * Basic marshalers and structures to support backward-compatibility with CrEngine's
 * vanilla aspect serialization mechanisms are implemented in this file.
 *
 * Aspect serialization is implemented as behavior of EntityReplica, for simplicity.
 * Most of the behavior is located in EntityReplica::UpdateAspects().
 * Removal of the shim will also require removing those components of EntityReplica.
 *
 * Aspect profiles are also supported through EntityReplica, as are client-delegated
 * aspects.
 */

#include "GridMateNetSerialize.h"
#include "../NetworkGridmateDebug.h"

namespace GridMate
{
    namespace NetSerialize
    {
        //-----------------------------------------------------------------------------
        uint32 HashBuffer(const char* buffer, size_t size)
        {
            if (size > 0)
            {
                // \todo - This is a very slow per-byte hash.
                return AZStd::hash_range(&buffer[0], &buffer[size]);
            }

            return 0;
        }

        //-----------------------------------------------------------------------------
        NetworkAspectType s_globallyDelegatableAspects = 0;

        void SetDelegatableAspects(NetworkAspectType aspects)
        {
            s_globallyDelegatableAspects = aspects;
        }

        NetworkAspectType GetDelegatableAspectMask()
        {
            return s_globallyDelegatableAspects;
        }

        //-----------------------------------------------------------------------------
        AspectSerializeState::AspectSerializeState()
            : m_hash(0)
            , m_writtenSizeBytes(0)
            , m_serializeToken(0)
        {
        }

        //-----------------------------------------------------------------------------
        bool AspectSerializeState::operator == (const AspectSerializeState& rhs) const
        {
            return (GetHash() == rhs.GetHash() && m_serializeToken == rhs.m_serializeToken);
        }

        //-----------------------------------------------------------------------------
        bool AspectSerializeState::UpdateHash(uint32 hash, uint32 bytesWritten)
        {
            bool changed = false;

            if (hash != m_hash)
            {
                ++m_serializeToken;
                changed = true;
            }

            m_hash = hash;
            m_writtenSizeBytes = bytesWritten;

            return changed;
        }

        //-----------------------------------------------------------------------------
        uint32 AspectSerializeState::GetHash() const
        {
            return m_hash;
        }

        //-----------------------------------------------------------------------------
        AspectSerializeState::Marshaler::Marshaler()
            : m_storage(nullptr)
            , m_isEnabled(false)
            , m_waitingForDispatch(false)
            , m_debugName(nullptr)
            , m_debugIndex(0)
        {
        }

        //-----------------------------------------------------------------------------
        bool AspectSerializeState::Marshaler::AllocateAspectSerializationBuffer(uint32 size)
        {
            m_storage.reset(size ? new AspectBuffer(size) : nullptr);

            if (size)
            {
                GM_DEBUG_TRACE("Allocated buffer of size %u bytes for aspect buffer.", size);
            }

            return (m_storage->GetData() != nullptr);
        }

        //-----------------------------------------------------------------------------
        void AspectSerializeState::Marshaler::DeallocateAspectSerializationBuffer()
        {
            m_storage.reset();
        }

        //-----------------------------------------------------------------------------
        ReadBufferType AspectSerializeState::Marshaler::GetReadBuffer() const
        {
            if (m_storage)
            {
                return ReadBufferType(GridMate::EndianType::BigEndian, m_storage->GetData(), m_storage->GetSize());
            }
            else
            {
                return ReadBufferType(GridMate::EndianType::BigEndian, nullptr, 0);
            }
        }

        //-----------------------------------------------------------------------------
        WriteBufferType AspectSerializeState::Marshaler::GetWriteBuffer()
        {
            if (m_storage)
            {
                return WriteBufferType(GridMate::EndianType::BigEndian, m_storage->GetData(), m_storage->GetSize());
            }
            else
            {
                return WriteBufferType(GridMate::EndianType::BigEndian, nullptr, 0);
            }
        }

        //-----------------------------------------------------------------------------
        void AspectSerializeState::Marshaler::Marshal(GridMate::WriteBuffer& wb, const AspectSerializeState& s)
        {
            wb.Write(s.m_serializeToken);

            const uint16 writtenSizeBytes = m_storage ? s.m_writtenSizeBytes : 0;
            wb.Write(writtenSizeBytes);

            GM_ASSERT_TRACE(writtenSizeBytes == 0 || writtenSizeBytes <= m_storage->GetSize(),
                "Claims %u bytes written, but aspect buffer is only %u bytes in size.",
                writtenSizeBytes, m_storage->GetSize());

            if (writtenSizeBytes > 0)
            {
                wb.WriteRaw(m_storage->GetData(), writtenSizeBytes);
            }
        }

        //-----------------------------------------------------------------------------
        void AspectSerializeState::Marshaler::Unmarshal(AspectSerializeState& s, GridMate::ReadBuffer& rb)
        {
            rb.Read(s.m_serializeToken);
            rb.Read(s.m_writtenSizeBytes);

            if (s.m_writtenSizeBytes > 0)
            {
                if (s.m_writtenSizeBytes > GetStorageSize())
                {
                    AllocateAspectSerializationBuffer(s.m_writtenSizeBytes);
                }

                GM_ASSERT_TRACE(m_storage.get() && m_storage->GetData(),
                    "NetSerializeUnmarshal: Buffer is not prepared for aspect.");

                rb.ReadRaw(m_storage->GetData(), s.m_writtenSizeBytes);
            }
        }
    } // namespace NetSerialize
} // namespace GridMate
