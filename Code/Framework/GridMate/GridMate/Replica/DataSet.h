/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#ifndef GM_DATASET_H
#define GM_DATASET_H

#include <AzCore/std/functional.h>
#include <GridMate/Containers/vector.h>

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>

#include <GridMate/Replica/Replica.h>
#include <GridMate/Replica/ReplicaChunk.h>
#include <GridMate/Replica/ReplicaCommon.h>
#include <GridMate/Replica/Throttles.h>
#include <GridMate/Replica/ReplicaTarget.h>

#include <GridMate/Serialize/Buffer.h>
#include <AzCore/std/containers/ring_buffer.h>


namespace AzFramework
{
    class NetworkContext;
}

namespace GridMate
{
    /**
     * \brief Default DataSet callbacks traits.
     */
    struct DataSetDefaultTraits
    {
        /**
         * \brief Should a change in DataSet value invoke a callback on a primary replica chunk? 
         * 
         * By default, DataSet::BindInterface<C, &C::Callback> only invokes on client/non-authoritative replica chunks.
         * This switch enables the callback on server/authoritative replica chunks.
         * Warning: this change should not be enabled on existing Open 3D Engine components as they were not written with this option in mind.
         * 
         * New user custom replica chunk will work just fine.
         */
        static const bool s_invokeAuthoritativeCallback = false;
    };

    /**
     * \brief Turns on DataSet callbacks to be invoked on the primary replica as well as client replicas.
     */
    struct DataSetInvokeEverywhereTraits : DataSetDefaultTraits
    {
        static const bool s_invokeAuthoritativeCallback = true;
    };

    class ReplicaChunkDescriptor;
    class ReplicaChunkBase;
    class ReplicaMarshalTaskBase;
    class ReplicaPeer;
    class ReplicaTarget;

    /**
    * DataSetBase
    * Base type for all replica datasets
    */
    class DataSetBase
    {
        friend ReplicaChunkDescriptor;
        friend ReplicaChunkBase;
        friend AzFramework::NetworkContext;

    public:
        void SetMaxIdleTime(float dt) { m_maxIdleTicks = dt; }
        float GetMaxIdleTime() const { return m_maxIdleTicks; }
        bool CanSet() const;
        bool IsDefaultValue() const { return m_isDefaultValue; }
        void MarkAsDefaultValue() { m_isDefaultValue = true; }
        void MarkAsNonDefaultValue() { m_isDefaultValue = false; }
        /**
        * Returns the last updated network time of the DataSet.
        */
        unsigned int GetLastUpdateTime() const { return m_lastUpdateTime; }

        ReplicaChunkBase* GetReplicaChunkBase() const { return m_replicaChunk; }

        AZ::u64 GetRevision() const { return m_revision; }

        using DispatchCallback = AZStd::function<void(const TimeContext& tc)>;

        /**
         * \brief Delta compressed DataSets use an intermediary to catch dispatches of changed DataSets in their logic
         * \param callback to a custom object when a DataSet changes
         */
        void SetDispatchOverride(DispatchCallback callback) { m_override = callback; }

        /**
         * \brief Delta compressed fields override a dispatch
         * \return not-null if this DataSet is used for Delta Compression
         */
        const DispatchCallback& GetDispatchOverride() const { return m_override; }

    protected:
        explicit DataSetBase(const char* debugName);
        virtual ~DataSetBase() = default;

        virtual PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) = 0;
        virtual void Unmarshal(UnmarshalContext& mc) = 0;
        virtual void ResetDirty() = 0;
        virtual void SetDirty();
        virtual void DispatchChangedEvent(const TimeContext& tc) { (void)tc; }
        ReadBuffer GetMarshalData() const;

        float               m_maxIdleTicks;             //Note: used only if ACK feedback disabled
        WriteBufferDynamic  m_streamCache;
        ReplicaChunkBase*   m_replicaChunk;             ///< raw pointer, assuming datasets do not exists without replica chunk
        unsigned int        m_lastUpdateTime;

        bool                m_isDefaultValue;
        AZ::u64             m_revision;          ///< Latest revision number; 0 means unset

        DispatchCallback m_override; // Used by delta compressed DataSets to combine dispatch callbacks
    };

    // This shim is here to temporarily allow support for Pointer type unmarshalling
    // (current approach of detecting differences doesn't readily allow for this behavior)
    // This is mainly to support ScriptProperties.
    class MarshalerShim
    {
    public:
        template<class MarshalerType, class DataType>
        static bool Unmarshal(UnmarshalContext& mc, MarshalerType& marshaler, DataType& sourceValue, AZStd::false_type /*isPointerType*/)
        {
            DataType value;

            // Expects a return of whether or not the value was actually read.
            if (mc.m_iBuf->Read(value, marshaler))
            {
                if (!(value == sourceValue))
                {
                    sourceValue = value;
                    return true;
                }
            }

            return false;
        }

        template<class MarshalerType, class DataType>
        static bool Unmarshal(UnmarshalContext& mc, MarshalerType& marshaler, DataType& sourceValue, AZStd::true_type /*isPointerType*/)
        {
            // Expects a return of whether or not the value was changed.
            return marshaler.UnmarshalToPointer(sourceValue, (*mc.m_iBuf));
        }
    };

    /**
        Declares a networked DataSet of type DataType. Optionally pass in a marshaler that
        can write the data to a stream. Otherwise the DataSet will expect to find a
        ForwardMarshaler specialized on type DataType. Optionally pass in a throttler
        that can decide when the data has changed enough to send to the downstream proxies.
    **/
    template<typename DataType, typename MarshalerType = Marshaler<DataType>, typename ThrottlerType = BasicThrottle<DataType>>
    class DataSet
        : public DataSetBase
    {
    public:
        template<class C, void (C::* FuncPtr)(const DataType&, const TimeContext&), typename CallbackTraits = DataSetDefaultTraits>
        class BindInterface;

        template<class C, void (C::* FuncPtr)(const DataType&, const TimeContext&)>
        class BindOverrideInterface;

        struct StampedBuffer
        {
            AZ::u64                                 m_stamp;  ///< Counter stamp
            AZStd::shared_ptr<WriteBufferDynamic>   m_buffer; ///< Marshalled value
        };
        //AZStd::ring_buffer<StampedBuffer>           m_values;       ///< History of values

        /**
            Constructs a DataSet.
        **/
        DataSet(const char* debugName, const DataType& value = DataType(), const MarshalerType& marshaler = MarshalerType(), const ThrottlerType& throttler = ThrottlerType())
            : DataSetBase(debugName)
            , m_value(value)
            , m_throttler(throttler)
            , m_marshaler(marshaler)
            , m_idleTicks(-1.f)
        {
            m_throttler.UpdateBaseline(value);
        }

        /**
            Modify the DataSet. Call this on the Primary node to change the data,
            which will be propagated to all proxies.
        **/
        void Set(const DataType& v)
        {
            if (CanSet())
            {
                m_value = v;
                m_isDefaultValue = false;
                SetDirty();
            }
        }

        /**
            Modify the DataSet. Call this on the Primary node to change the data,
            which will be propagated to all proxies.
        **/
        void Set(DataType&& v)
        {
            if (CanSet())
            {
                m_value = v;
                m_isDefaultValue = false;
                SetDirty();
            }
        }

        /**
            Modify the DataSet. Call this on the Primary node to change the data,
            which will be propagated to all proxies.
        **/
        template <class ... Args>
        void SetEmplace(Args&& ... args)
        {
            if (CanSet())
            {
                m_value = DataType(AZStd::forward<Args>(args) ...);
                m_isDefaultValue = false;
                SetDirty();
            }
        }

        /**
            Modify the DataSet directly without copying it. Call this on the Primary node,
            passing in a function object that takes the value by reference, optionally
            modifies the data, and returns true if the data was changed.
        **/
        template<typename FuncPtr>
        bool Modify(FuncPtr func)
        {
            static_assert(AZStd::is_same<decltype(func(m_value)), bool>::value, "Function object must return dirty status");

            bool dirty = false;
            if (CanSet())
            {
                dirty = func(m_value);
                if (dirty)
                {
                    m_isDefaultValue = false;
                    SetDirty();
                }
            }
            return dirty;
        }

        /**
            Returns the current value of the DataSet.
        **/
        const DataType& Get() const { return m_value; }

        /**
            Returns the marshaler instance.
        **/
        MarshalerType& GetMarshaler() { return m_marshaler; }

        /**
            Returns the throttler instance.
        **/
        ThrottlerType& GetThrottler() { return m_throttler; }

        //@{
        /**
            Returns equality for values of the same type
        **/
        bool operator==(const DataType& other) const { return m_value == other; }
        bool operator==(const DataSet& other) const { return m_value == other.m_value; }

        bool operator!=(const DataType& other) const { return !(m_value == other); }
        bool operator!=(const DataSet& other) const { return !(m_value == other.m_value); }
        //@}

    protected:
        DataSet(const DataSet& rhs) = delete;
        DataSet& operator=(const DataSet&) = delete;

        void SetDirty() override
        {
            if (!IsWithinToleranceThreshold())
            {
                DataSetBase::SetDirty();
            }
        }

        PrepareDataResult PrepareData(EndianType endianType, AZ::u32 marshalFlags) override
        {
            PrepareDataResult pdr(false, false, false, false);

            if (ReplicaTarget::IsAckEnabled())
            {
                if (!IsWithinToleranceThreshold())
                {
                    m_isDefaultValue = false;
                    pdr.m_isDownstreamUnreliableDirty = true;

                    m_throttler.UpdateBaseline(m_value);
                    m_streamCache.Clear();
                    m_streamCache.SetEndianType(endianType);
                    m_streamCache.Write(m_value, m_marshaler);

                    if (m_replicaChunk && m_replicaChunk->m_replica)    //If this data set is attached to a replica
                    {
                        auto revision = m_replicaChunk->m_replica->GetRevision() + 1;
                        //m_values.push_back(StampedBuffer{ revision, AZStd::make_shared<WriteBufferDynamic>(m_streamCache) });
                        AZ_Assert(m_replicaChunk->m_revision <= revision, "Replica Chunk out of sync with replica chnk %d replica+1 %d"
                            , m_replicaChunk->m_revision, revision);
                        m_replicaChunk->m_revision = m_revision = revision;
                    }
                }
                else if ((marshalFlags & ReplicaMarshalFlags::ForceDirty)
                    || (marshalFlags & ReplicaMarshalFlags::OmitUnmodified)
                    || (m_isDefaultValue && m_streamCache.Size() == 0)
                    )
                {
                    /*
                     * If the dataset is not dirty but the current operation is forcing dirty then
                     * we need to prepare the stream cache by writing the current value in,
                     * otherwise, the marshalling logic will send nothing or an out of date value.
                     *
                     * This can occur with NewOwner command, for example.
                     */

                    m_streamCache.Clear();
                    m_streamCache.SetEndianType(endianType);
                    m_streamCache.Write(m_value, m_marshaler);
                }
            }
            else
            {
                bool isDirty = false;
                if (!IsWithinToleranceThreshold())
                {
                    m_isDefaultValue = false;

                    isDirty = true;
                    m_idleTicks = 0.f;
                }
                else if (m_idleTicks < m_maxIdleTicks)
                {
                    /*
                    * This logic sends updates unreliable for some time and then sends reliable update at the end.
                    * However, this is not necessary in the case of a value that is still a default one,
                    * since the new proxy event (that occurs prior) is always sent reliably.
                    */
                    if (!m_isDefaultValue)
                    {
                        isDirty = true;
                    }

                    m_idleTicks += 1.f;
                }

                if (isDirty)
                {
                    m_throttler.UpdateBaseline(m_value);
                    m_streamCache.Clear();
                    m_streamCache.SetEndianType(endianType);
                    m_streamCache.Write(m_value, m_marshaler);

                    if (m_idleTicks >= m_maxIdleTicks)
                    {
                        pdr.m_isDownstreamReliableDirty = true;
                    }
                    else
                    {
                        pdr.m_isDownstreamUnreliableDirty = true;
                    }
                }
                else if ((marshalFlags & ReplicaMarshalFlags::ForceDirty) || (marshalFlags & ReplicaMarshalFlags::OmitUnmodified))
                {
                    /*
                    * If the dataset is not dirty but the current operation is forcing dirty then
                    * we need to prepare the stream cache by writing the current value in,
                    * otherwise, the marshalling logic will send nothing or an out of date value.
                    *
                    * This can occur with NewOwner command, for example.
                    */

                    m_streamCache.Clear();
                    m_streamCache.SetEndianType(endianType);
                    m_streamCache.Write(m_value, m_marshaler);
                }
            }

            return pdr;
        }

        void Unmarshal(UnmarshalContext& mc) override
        {
            if (MarshalerShim::Unmarshal(mc, m_marshaler, m_value, typename AZStd::is_pointer<DataType>::type()))
            {
                m_lastUpdateTime = mc.m_timestamp;
                m_replicaChunk->AddDataSetEvent(this);
            }
        }

        void ResetDirty() override
        {
            m_idleTicks = m_maxIdleTicks;
        }

        bool IsWithinToleranceThreshold()
        {
            return m_throttler.WithinThreshold(m_value);
        }

        void DispatchChangedEvent(const TimeContext& tc) override
        {
            if (m_override)
            {
                /*
                 * m_override is a AZStd::function<>, so its use can incur a cost.
                 * However, its use is limited to Delta Compressed DataSets.
                 * This @DispatchChangedEvent is only called on pure DataSet<>,
                 * as opposed DataSet<>::BindInterface<> for regular DataSet usage.
                 * And in the cases when m_override wasn't specified,
                 * it's a simple "if (false)" pass-through.
                 */
                m_override(tc);
            }
        }

        DataType m_value;
        ThrottlerType m_throttler;
        MarshalerType m_marshaler;
        float m_idleTicks;
    };
    //-----------------------------------------------------------------------------

    /**
        Declares a DataSet with an event handler that is called when the DataSet is changed.
        Use BindInterface<Class, FuncPtr> to dispatch to a method on the ReplicaChunk's
        ReplicaChunkInterface event handler instance.
    **/
    template<typename DataType, typename MarshalerType, typename ThrottlerType>
    template<class C, void (C::* FuncPtr)(const DataType&, const TimeContext&), typename CallbackTraits>
    class DataSet<DataType, MarshalerType, ThrottlerType>::BindInterface
        : public DataSet<DataType, MarshalerType, ThrottlerType>
    {
    public:
        BindInterface(const char* debugName,
            const DataType& value = DataType(),
            const MarshalerType& marshaler = MarshalerType(),
            const ThrottlerType& throttler = ThrottlerType())
            : DataSet(debugName,
                value,
                marshaler,
                throttler)
        { }

        void SetDirty() override
        {
            DataSet::SetDirty();

            if (CallbackTraits::s_invokeAuthoritativeCallback)
            {
                DispatchChangedEvent({});
            }
        }

    protected:
        void DispatchChangedEvent(const TimeContext& tc) override
        {
            C* c = m_replicaChunk ? static_cast<C*>(m_replicaChunk->GetHandler()) : nullptr;
            if (c)
            {
                TimeContext changeTime;
                changeTime.m_realTime = m_lastUpdateTime;
                changeTime.m_localTime = m_lastUpdateTime - (tc.m_realTime - tc.m_localTime);

                (*c.*FuncPtr)(m_value, changeTime);
            }
        }
    };

    /**
    * CtorContextBase
    */
    class CtorContextBase
    {
        friend class CtorDataSetBase;

        static CtorContextBase* s_pCur;

    protected:
        //-----------------------------------------------------------------------------
        class CtorDataSetBase
        {
        public:
            CtorDataSetBase();
            virtual ~CtorDataSetBase() { }

            virtual void Marshal(WriteBuffer& wb) = 0;
            virtual void Unmarshal(ReadBuffer& rb) = 0;
        };
        //-----------------------------------------------------------------------------
        template<typename DataType, typename MarshalerType = Marshaler<DataType> >
        class CtorDataSet
            : public CtorDataSetBase
        {
            MarshalerType m_marshaler;
            DataType m_value;

        public:
            CtorDataSet(const MarshalerType& marshaler = MarshalerType())
                : m_marshaler(marshaler) { }

            void Set(const DataType& val) { m_value = val; }
            const DataType& Get() const { return m_value; }

            void Marshal(WriteBuffer& wb) override
            {
                wb.Write(m_value, m_marshaler);
            }

            void Unmarshal(ReadBuffer& rb) override
            {
                rb.Read(m_value, m_marshaler);
            }
        };
        //-----------------------------------------------------------------------------

    private:
        typedef vector<CtorDataSetBase*> MembersArrayType;
        MembersArrayType m_members;

    public:
        CtorContextBase();

        void Marshal(WriteBuffer& wb);
        void Unmarshal(ReadBuffer& rb);
    };
    //-----------------------------------------------------------------------------
} // namespace GridMate

#endif // GM_DATASET_H
