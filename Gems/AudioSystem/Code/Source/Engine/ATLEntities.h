/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/IO/IStreamer.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#if !defined(AUDIO_RELEASE)
    #include <AzCore/std/chrono/chrono.h>
#endif // !AUDIO_RELEASE

#include <IAudioSystem.h>
#include <ATLCommon.h>
#include <ATLEntityData.h>
#include <ATLUtils.h>
#include <AudioAllocators.h>


namespace Audio
{
    template <typename KeyType, typename ValueType>
    using ATLMapLookupType = AZStd::unordered_map<KeyType, ValueType, AZStd::hash<KeyType>, AZStd::equal_to<KeyType>, AudioSystemStdAllocator>;

    template <typename KeyType>
    using ATLSetLookupType = AZStd::unordered_set<KeyType, AZStd::hash<KeyType>, AZStd::equal_to<KeyType>, AudioSystemStdAllocator>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EATLObjectFlags : TATLEnumFlagsType
    {
        eAOF_NONE = 0,
        eAOF_TRACK_VELOCITY = AUDIO_BIT(0),
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EATLSubsystem : TATLEnumFlagsType
    {
        eAS_NONE = 0,
        eAS_AUDIO_SYSTEM_IMPLEMENTATION,
        eAS_ATL_INTERNAL,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    template <typename IDType>
    class CATLEntity
    {
    public:
        explicit CATLEntity(const IDType nID, const EATLDataScope eDataScope)
            : m_nID(nID)
            , m_eDataScope(eDataScope)
        {}

        virtual ~CATLEntity() = default;

        virtual IDType GetID() const
        {
            return m_nID;
        }

        virtual EATLDataScope GetDataScope() const
        {
            return m_eDataScope;
        }

    protected:
        EATLDataScope m_eDataScope;

    private:
        const IDType m_nID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSoundPropagationData
    {
        SATLSoundPropagationData()
            : fObstruction(0.0f)
            , fOcclusion(0.0f)
        {}

        ~SATLSoundPropagationData() {}

        float fObstruction;
        float fOcclusion;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLListenerObject
        : public CATLEntity<TAudioObjectID>
    {
    public:
        explicit CATLListenerObject(const TAudioObjectID nID, IATLListenerData* const pImplData = nullptr)
            : CATLEntity<TAudioObjectID>(nID, eADS_NONE)
            , m_pImplData(pImplData)
        {}

        ~CATLListenerObject() override {}

        SATLWorldPosition oPosition;
        IATLListenerData* const m_pImplData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLControlImpl
    {
    public:
        CATLControlImpl()
            : m_eReceiver(eAS_NONE)
        {}

        explicit CATLControlImpl(const EATLSubsystem receiver)
            : m_eReceiver(receiver)
        {}

        virtual ~CATLControlImpl() {}

        EATLSubsystem GetReceiver() const
        {
            return m_eReceiver;
        }

    protected:
        EATLSubsystem m_eReceiver;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLTriggerImpl
        : public CATLControlImpl
    {
    public:

        explicit CATLTriggerImpl(
            const TAudioTriggerImplID nID,
            const TAudioControlID nTriggerID,
            const EATLSubsystem eReceiver,
            IATLTriggerImplData* const pImplData = nullptr)
            : CATLControlImpl(eReceiver)
            , m_nATLID(nID)
            , m_nATLTriggerID(nTriggerID)
            , m_pImplData(pImplData)
        {}

        ~CATLTriggerImpl() override {}

        const TAudioTriggerImplID m_nATLID;
        const TAudioControlID m_nATLTriggerID;
        IATLTriggerImplData* const m_pImplData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLTrigger
        : public CATLEntity<TAudioControlID>
    {
    public:
        using TImplPtrVec = AZStd::vector<CATLTriggerImpl*, Audio::AudioSystemStdAllocator>;

        CATLTrigger(const TAudioControlID nID, const EATLDataScope eDataScope, const TImplPtrVec& rImplPtrs)
            : CATLEntity<TAudioControlID>(nID, eDataScope)
            , m_cImplPtrs(rImplPtrs)
        {}

        ~CATLTrigger() override {}

        TImplPtrVec m_cImplPtrs;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLRtpcImpl
        : public CATLControlImpl
    {
    public:
        CATLRtpcImpl(const EATLSubsystem eReceiver, IATLRtpcImplData* const pImplData = nullptr)
            : CATLControlImpl(eReceiver)
            , m_pImplData(pImplData)
        {}

        ~CATLRtpcImpl() override {}

        IATLRtpcImplData* const m_pImplData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLRtpc
        : public CATLEntity<TAudioControlID>
    {
    public:
        using TImplPtrVec = AZStd::vector<CATLRtpcImpl*, Audio::AudioSystemStdAllocator>;

        CATLRtpc(const TAudioControlID nID, const EATLDataScope eDataScope, const TImplPtrVec& cImplPtrs)
            : CATLEntity<TAudioControlID>(nID, eDataScope)
            , m_cImplPtrs(cImplPtrs)
        {}

        ~CATLRtpc() override {}

        TImplPtrVec m_cImplPtrs;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLSwitchStateImpl
        : public CATLControlImpl
    {
    public:
        explicit CATLSwitchStateImpl(const EATLSubsystem eReceiver, IATLSwitchStateImplData* const pImplData = nullptr)
            : CATLControlImpl(eReceiver)
            , m_pImplData(pImplData)
        {}

        ~CATLSwitchStateImpl() override {}

        IATLSwitchStateImplData* const m_pImplData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLSwitchState
    {
    public:
        using TImplPtrVec = AZStd::vector<CATLSwitchStateImpl*, Audio::AudioSystemStdAllocator>;

        CATLSwitchState(
            const TAudioControlID nSwitchID,
            const TAudioSwitchStateID nStateID,
            const TImplPtrVec& cImplPtrs)
            : m_nID(nStateID)
            , m_nSwitchID(nSwitchID)
            , m_cImplPtrs(cImplPtrs)
        {}

        ~CATLSwitchState() {}

        TAudioSwitchStateID GetID() const
        {
            return m_nID;
        }

        TAudioSwitchStateID GetParentID() const
        {
            return m_nSwitchID;
        }

        TImplPtrVec m_cImplPtrs;

    private:
        const TAudioSwitchStateID m_nID;
        const TAudioControlID m_nSwitchID;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLSwitch
        : public CATLEntity<TAudioControlID>
    {
    public:
        explicit CATLSwitch(const TAudioControlID nID, const EATLDataScope eDataScope)
            : CATLEntity<TAudioControlID>(nID, eDataScope)
        {}

        ~CATLSwitch() override {}

        using TStateMap = ATLMapLookupType<TAudioSwitchStateID, CATLSwitchState*>;
        TStateMap cStates;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLEnvironmentImpl
        : public CATLControlImpl
    {
    public:
        explicit CATLEnvironmentImpl(const EATLSubsystem eReceiver, IATLEnvironmentImplData* const pImplData = nullptr)
            : CATLControlImpl(eReceiver)
            , m_pImplData(pImplData)
        {}

        ~CATLEnvironmentImpl() override {}

        IATLEnvironmentImplData* const m_pImplData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLAudioEnvironment
        : public CATLEntity<TAudioEnvironmentID>
    {
    public:
        using TImplPtrVec = AZStd::vector<CATLEnvironmentImpl*, Audio::AudioSystemStdAllocator>;

        explicit CATLAudioEnvironment(const TAudioEnvironmentID nID, const EATLDataScope eDataScope, const TImplPtrVec& rImplPtrs)
            : CATLEntity<TAudioEnvironmentID>(nID, eDataScope)
            , m_cImplPtrs(rImplPtrs)
        {}

        ~CATLAudioEnvironment() override {}

        TImplPtrVec m_cImplPtrs;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLEvent
        : public CATLEntity<TAudioEventID>
    {
    public:
        explicit CATLEvent(const TAudioEventID nID, const EATLSubsystem eSender, IATLEventData* const pImplData)
            : CATLEntity<TAudioEventID>(nID, eADS_NONE)
            , m_nObjectID(INVALID_AUDIO_OBJECT_ID)
            , m_nTriggerID(INVALID_AUDIO_CONTROL_ID)
            , m_nTriggerImplID(INVALID_AUDIO_TRIGGER_IMPL_ID)
            , m_nTriggerInstanceID(INVALID_AUDIO_TRIGGER_INSTANCE_ID)
            , m_audioEventState(eAES_NONE)
            , m_eSender(eSender)
            , m_pImplData(pImplData)
        {}

        ~CATLEvent() override = default;

        CATLEvent(const CATLEvent&) = delete;               // copy protection!
        CATLEvent& operator=(const CATLEvent&) = delete;    // copy protection!

        void SetDataScope(const EATLDataScope eDataScope)
        {
            m_eDataScope = eDataScope;
        }

        bool IsPlaying() const
        {
            return m_audioEventState == eAES_PLAYING;
        }

        void Clear()
        {
            m_eDataScope = eADS_NONE;
            m_nObjectID = INVALID_AUDIO_OBJECT_ID;
            m_nTriggerID = INVALID_AUDIO_CONTROL_ID;
            m_nTriggerImplID = INVALID_AUDIO_TRIGGER_IMPL_ID;
            m_nTriggerInstanceID = INVALID_AUDIO_TRIGGER_INSTANCE_ID;
            m_audioEventState = eAES_NONE;
        }

        TAudioObjectID m_nObjectID;
        TAudioControlID m_nTriggerID;
        TAudioTriggerImplID m_nTriggerImplID;
        TAudioTriggerInstanceID m_nTriggerInstanceID;
        EAudioEventState m_audioEventState;
        const EATLSubsystem m_eSender;
        IATLEventData* m_pImplData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLAudioFileEntry
    {
    public:
        explicit CATLAudioFileEntry(const char* const filePath = nullptr, IATLAudioFileEntryData* const implData = nullptr);

        ~CATLAudioFileEntry();

        AZStd::string m_filePath;

        AZ::IO::FileRequestPtr m_asyncStreamRequest;
        void* m_memoryBlock;
        IATLAudioFileEntryData* m_implData;

        size_t m_fileSize;
        size_t m_memoryBlockAlignment;
        AZ::u32 m_useCount;
        Flags<TATLEnumFlagsType> m_flags;
        EATLDataScope m_dataScope;

#if !defined(AUDIO_RELEASE)
        AZStd::chrono::steady_clock::time_point m_timeCached;
#endif // !AUDIO_RELEASE
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLPreloadRequest
        : public CATLEntity<TAudioPreloadRequestID>
    {
    public:
        using TFileEntryIDs = AZStd::vector<TAudioFileEntryID, Audio::AudioSystemStdAllocator>;

        explicit CATLPreloadRequest(
            const TAudioPreloadRequestID nID,
            const EATLDataScope eDataScope,
            const bool bAutoLoad,
            const TFileEntryIDs& cFileEntryIDs)
            : CATLEntity<TAudioPreloadRequestID>(nID, eDataScope)
            , m_bAutoLoad(bAutoLoad)
            , m_cFileEntryIDs(cFileEntryIDs)
            , m_allLoaded(false)
        {}

        ~CATLPreloadRequest() override {}

        const bool m_bAutoLoad;
        TFileEntryIDs m_cFileEntryIDs;
        bool m_allLoaded;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    using TATLTriggerLookup = ATLMapLookupType<TAudioControlID, CATLTrigger*>;
    using TATLRtpcLookup = ATLMapLookupType<TAudioControlID, CATLRtpc*>;
    using TATLSwitchLookup = ATLMapLookupType<TAudioControlID, CATLSwitch*>;
    using TATLPreloadRequestLookup = ATLMapLookupType<TAudioPreloadRequestID, CATLPreloadRequest*>;
    using TATLEnvironmentLookup = ATLMapLookupType<TAudioEnvironmentID, CATLAudioEnvironment*>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLSwitchStateImplData_internal
        : public IATLSwitchStateImplData
    {
        SATLSwitchStateImplData_internal(const TAudioControlID nSwitchID, const TAudioSwitchStateID nSwitchStateID)
            : nATLInternalSwitchID(nSwitchID)
            , nATLInternalStateID(nSwitchStateID)
        {}

        ~SATLSwitchStateImplData_internal() override {}

        const TAudioControlID nATLInternalSwitchID;
        const TAudioSwitchStateID nATLInternalStateID;
    };

#if !defined(AUDIO_RELEASE)
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CATLDebugNameStore
    {
    public:
        CATLDebugNameStore() = default;
        ~CATLDebugNameStore() = default;

        // The Add* and Remove* functions return true if the storage changed, false otherwise
        bool AddAudioObject(const TAudioObjectID nObjectID, const char* const sName);
        bool AddAudioTrigger(const TAudioControlID nTriggerID, const char* const sName);
        bool AddAudioRtpc(const TAudioControlID nRtpcID, const char* const sName);
        bool AddAudioSwitch(const TAudioControlID nSwitchID, const char* const sName);
        bool AddAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID, const char* const sName);
        bool AddAudioPreloadRequest(const TAudioPreloadRequestID nRequestID, const char* const sName);
        bool AddAudioEnvironment(const TAudioEnvironmentID nEnvironmentID, const char* const sName);

        bool RemoveAudioObject(const TAudioObjectID nObjectID);
        bool RemoveAudioTrigger(const TAudioControlID nTriggerID);
        bool RemoveAudioRtpc(const TAudioControlID nRtpcID);
        bool RemoveAudioSwitch(const TAudioControlID nSwitchID);
        bool RemoveAudioSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID);
        bool RemoveAudioPreloadRequest(const TAudioPreloadRequestID nRequestID);
        bool RemoveAudioEnvironment(const TAudioEnvironmentID nEnvironmentID);

        // The Lookup* functions return nullptr if the content is not found.
        const char* LookupAudioObjectName(const TAudioObjectID nObjectID) const;
        const char* LookupAudioTriggerName(const TAudioControlID nTriggerID) const;
        const char* LookupAudioRtpcName(const TAudioControlID nRtpcID) const;
        const char* LookupAudioSwitchName(const TAudioControlID nSwitchID) const;
        const char* LookupAudioSwitchStateName(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID) const;
        const char* LookupAudioPreloadRequestName(const TAudioPreloadRequestID nRequestID) const;
        const char* LookupAudioEnvironmentName(const TAudioEnvironmentID nEnvironmentID) const;

    private:
        using TAudioObjectMap = ATLMapLookupType<TAudioObjectID, AZStd::string>;
        using TAudioControlMap = ATLMapLookupType<TAudioControlID, AZStd::string>;
        using TAudioSwitchStateMap = ATLMapLookupType<TAudioSwitchStateID, AZStd::string>;
        using TAudioSwitchMap = ATLMapLookupType<TAudioControlID, AZStd::pair<AZStd::string, TAudioSwitchStateMap>>;
        using TAudioPreloadRequestsMap = ATLMapLookupType<TAudioPreloadRequestID, AZStd::string>;
        using TAudioEnvironmentMap = ATLMapLookupType<TAudioEnvironmentID, AZStd::string>;

        TAudioObjectMap m_cATLObjectNames;
        TAudioControlMap m_cATLTriggerNames;
        TAudioControlMap m_cATLRtpcNames;
        TAudioSwitchMap m_cATLSwitchNames;
        TAudioPreloadRequestsMap m_cATLPreloadRequestNames;
        TAudioEnvironmentMap m_cATLEnvironmentNames;
    };
#endif // !AUDIO_RELEASE
} // namespace Audio
