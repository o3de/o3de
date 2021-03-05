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
#ifndef INCLUDE_GRIDMATENETSERIALIZE_HEADER
#define INCLUDE_GRIDMATENETSERIALIZE_HEADER

#pragma once

#include "ProjectDefines.h"

#include "../NetworkGridMateCommon.h"
#include "../NetworkGridmateDebug.h"
#include "../NetworkGridmateMarshaling.h"

#include <GridMate/Serialize/DataMarshal.h>

#include <INetwork.h>

namespace GridMate
{
    namespace NetSerialize
    {
        //! Callback for LegacySerializeProvider
        using AcquireSerializeCallback = AZStd::function<void(ISerialize*)>;


        /*
        * This interface is to allow setting custom serializers for legacy aspects and rmis serialization
        */
        class ILegacySerializeProvider : public AZ::EBusTraits
        {
        public:
            // Called when need serializer for the legacy aspect, passing WriteBuffer to serialize aspect to and a callback that will return prepared serializer
            virtual void AcquireSerializer(WriteBuffer& wb, AcquireSerializeCallback callback) = 0;

            // Called when need deserializer for the legacy aspect, passing ReadBuffer to deserialize aspect to and a callback that will return prepared deserializer
            virtual void AcquireDeserializer(ReadBuffer& rb, AcquireSerializeCallback callback) = 0;
        };

        enum
        {
            kNumAspectSlots = 26
        };

        void SetDelegatableAspects(NetworkAspectType aspects);
        NetworkAspectType GetDelegatableAspectMask();

        /*!
         * Utility function for hashing an arbitrary byte buffer.
         * This is currently used to detect changes in aspect data.
         */
        uint32 HashBuffer(const char* buffer, size_t size);

        /*!
         * Wraps data and state management to facilitate the Aspect serialization model
         * used by CryEngine.
         * We pipe this data through the EntityReplica.
         */
        class AspectSerializeState
        {
        public:
            friend class AspectSerializeStateMarshaler;

            /*!
             * Marshaler for a given aspects serialization state.
             */
            class Marshaler
            {
            public:
                Marshaler();

                //! An aspect is active if we unmarshaled a non-zero data requirement for it, per the server.
                bool IsActive() const { return GetStorageSize() > 0; }

                //! Is a dispatch-to-GameObject (via netSerialize()) pending?
                bool IsWaitingForDispatch() const { return m_waitingForDispatch; }
                //! New data has come in, mark for dispatch.
                void MarkWaitingForDispatch() { m_waitingForDispatch = true; }
                //! Clear pending dispatch.
                void MarkDispatchComplete() { m_waitingForDispatch = false; }

                //! Allocate space for this aspect to serialize data.
                //! A size of zero is effectively ignored, and the aspect remains inactive.
                bool AllocateAspectSerializationBuffer(uint32 size);
                void DeallocateAspectSerializationBuffer();

                uint32 GetStorageSize() const { return m_storage ? m_storage->GetSize() : 0; }

                ReadBufferType GetReadBuffer() const;
                WriteBufferType GetWriteBuffer();

                void Marshal(GridMate::WriteBuffer& wb, const AspectSerializeState& s);
                void Unmarshal(AspectSerializeState& s, GridMate::ReadBuffer& rb);                

            private:
                bool m_waitingForDispatch;          //! Set if the aspect was recently unmarshaled, so we know to
                                                    //! dispatch changes to the game.
                bool m_isEnabled;

                typedef ManagedFlexibleBuffer<256> AspectBuffer;
                AspectBuffer::Ptr  m_storage;       //! Contents and size of the aspect buffer.

            public:
                const char* m_debugName;
                size_t m_debugIndex;
            };

            AspectSerializeState();

            uint32 GetWrittenSize() const { return m_writtenSizeBytes; }

            //! Returns true if the data has changed and needs a resend (hash mismatch).
            bool operator == (const AspectSerializeState& rhs) const;

            //! Accessors for serialization buffer hash.
            bool UpdateHash(uint32 hash, uint32 bytesWritten);
            uint32 GetHash() const;

        private:

            uint32      m_hash;                 //! A hash of the serialization buffer's current contents.
            uint16      m_writtenSizeBytes;     //! Current size of data in the aspect's buffer.

            uint8       m_serializeToken;       //! Increments (wrapping okay) each time contents change,
                                                //! so the remote side knows when to dispatch.
        };

        /*!
         * Implementation of CryEngine serializer that marshals data into a GridMate write buffer.
         * This is used when reading state from a game object, RMI param gathering, etc.
         */
        class EntityNetSerializerCollectState
            : public CSimpleSerializeImpl < false, eST_Network >
        {
        public:

            EntityNetSerializerCollectState(GridMate::WriteBuffer& wb)
                : m_wb(wb)
            {
            }

            template <class T>
            void Value(const char* name, T& value, uint32 policy)
            {
                (void)name;
                (void)policy;

                GridMate::Marshaler<T>().Marshal(m_wb, value);
            }

            void Value(const char* name, EntityId& value, uint32 policy)
            {
                (void)name;

                EntityId serializedId = value;

                switch (policy)
                {
                case 'eid':
                {
                    // Entity Ids don't match across machines, so nodes need to convert
                    // back to the server's Id before sending.
                    serializedId = gEnv->pNetwork->LocalEntityIdToServerEntityId(value);

                    GM_ASSERT_TRACE(value == kInvalidEntityId || serializedId != kInvalidEntityId,
                        "Failed to map local entity Id %u to a server entity Id. "
                        "Make sure the entity whose Id is being serialized was spawned as a networked entity.",
                        value);
                }
                break;
                }

                GridMate::Marshaler<EntityId>().Marshal(m_wb, serializedId);
            }

            template <class T>
            void Value(const char* name, T& value)
            {
                Value(name, value, 0);
            }

            void Value(const char* name, SSerializeString& value, uint32 policy)
            {
                (void)name;
                (void)policy;

                typedef ::string TempString;
                TempString s = value.c_str();
                GridMate::Marshaler<TempString>().Marshal(m_wb, value);
            }

            bool BeginGroup(const char* szName)
            {
                (void)szName;
                return true;
            }

            bool BeginOptionalGroup(const char* szName, bool cond)
            {
                (void)szName;
                GridMate::Marshaler<bool>().Marshal(m_wb, cond);
                return cond;
            }

            void EndGroup()
            {
            }

            uint32 CalculateHash() const
            {
                return HashBuffer(m_wb.Get(), m_wb.Size());
            }

            GridMate::WriteBuffer&    m_wb;
        };

        /*!
         * Implementation of CryEngine serializer that unmarshals data from a GridMate read buffer.
         * This is used when writing state to a game objec, invoking RMIs, etc.
         */
        class EntityNetSerializerDispatchState
            : public CSimpleSerializeImpl < true, eST_Network >
        {
        public:
            EntityNetSerializerDispatchState()
                : m_rb(EndianType::BigEndian)
            {
            }

            EntityNetSerializerDispatchState(const GridMate::ReadBuffer& rb)
                : m_rb(rb)
            {
            }

            template <class T>
            void Value(const char* name, T& value, uint32 policy)
            {
                (void)name;
                (void)policy;
                GridMate::Marshaler<T>().Unmarshal(value, m_rb);
            }

            template <class T>
            void Value(const char* name, T& value)
            {
                Value(name, value, 0);
            }

            void Value(const char* name, EntityId& value, uint32 policy)
            {
                (void)name;
                GridMate::Marshaler<EntityId>().Unmarshal(value, m_rb);

                switch (policy)
                {
                case 'eid':
                {
                    // Entity Ids don't match across machines, so nodes need to convert
                    // back to the server's Id before sending.
                    EntityId mappedValue = gEnv->pNetwork->ServerEntityIdToLocalEntityId(value, true);
                    AZ_Warning("CryNetworkShim", value == kInvalidEntityId || mappedValue != kInvalidEntityId, "Failed to map server entity id 0x%x to local entity id", value);
                    value = mappedValue;
                }
                break;
                }
            }

            void Value(const char* name, SSerializeString& value, uint32 policy)
            {
                (void)name;
                (void)policy;

                typedef ::string TempString;
                TempString s = value.c_str();
                CryStringMarshaler().Unmarshal(s, m_rb);
                value = s;
            }

            bool BeginGroup(const char* szName)
            {
                (void)szName;
                return true;
            }

            bool BeginOptionalGroup(const char* szName, bool cond)
            {
                (void)szName;
                GridMate::Marshaler<bool>().Unmarshal(cond, m_rb);

                return cond;
            }

            void EndGroup()
            {
            }

            GridMate::ReadBuffer    m_rb;
        };
    } // namespace NetSerialize
} // namespace GridMate

#endif // INCLUDE_GRIDMATENETSERIALIZE_HEADER
