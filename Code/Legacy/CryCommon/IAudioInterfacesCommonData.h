/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/base.h>
#include <AzCore/Math/Matrix3x4.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>


#define AUDIO_BIT(x)     (1 << (x))
#define AUDIO_TRIGGER_IMPL_ID_NUM_RESERVED 100// IDs below that value are used for the CATLTriggerImpl_Internal


namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    typedef AZ::u64 TATLIDType;
    typedef AZ::u32 TATLEnumFlagsType;
    typedef TATLIDType TAudioObjectID;
    typedef TATLIDType TAudioControlID;
    typedef TATLIDType TAudioSwitchStateID;
    typedef TATLIDType TAudioEnvironmentID;
    typedef TATLIDType TAudioPreloadRequestID;
    typedef TATLIDType TAudioEventID;
    typedef TATLIDType TAudioFileEntryID;
    typedef TATLIDType TAudioTriggerImplID;
    typedef TATLIDType TAudioTriggerInstanceID;
    typedef TATLIDType TAudioProxyID;
    typedef TATLIDType TAudioSourceId;
    typedef TATLIDType TAudioFileId;
    typedef TATLIDType TAudioFileCollectionId;
    typedef TATLIDType TAudioFileLanguageId;

#define INVALID_AUDIO_OBJECT_ID (static_cast<Audio::TAudioObjectID>(0))
#define GLOBAL_AUDIO_OBJECT_ID (static_cast<Audio::TAudioObjectID>(1))
#define INVALID_AUDIO_CONTROL_ID (static_cast<Audio::TAudioControlID>(0))
#define INVALID_AUDIO_SWITCH_STATE_ID (static_cast<Audio::TAudioSwitchStateID>(0))
#define INVALID_AUDIO_ENVIRONMENT_ID (static_cast<Audio::TAudioEnvironmentID>(0))
#define INVALID_AUDIO_PRELOAD_REQUEST_ID (static_cast<Audio::TAudioPreloadRequestID>(0))
#define INVALID_AUDIO_EVENT_ID (static_cast<Audio::TAudioEventID>(0))
#define INVALID_AUDIO_FILE_ENTRY_ID (static_cast<Audio::TAudioFileEntryID>(0))
#define INVALID_AUDIO_TRIGGER_IMPL_ID (static_cast<Audio::TAudioTriggerImplID>(0))
#define INVALID_AUDIO_TRIGGER_INSTANCE_ID (static_cast<Audio::TAudioTriggerInstanceID>(0))
#define INVALID_AUDIO_ENUM_FLAG_TYPE (static_cast<Audio::TATLEnumFlagsType>(0))
#define ALL_AUDIO_REQUEST_SPECIFIC_TYPE_FLAGS (static_cast<Audio::TATLEnumFlagsType>(-1))
#define INVALID_AUDIO_PROXY_ID (static_cast<Audio::TAudioProxyID>(0))
#define DEFAULT_AUDIO_PROXY_ID (static_cast<Audio::TAudioProxyID>(1))
#define INVALID_AUDIO_SOURCE_ID (static_cast<Audio::TAudioSourceId>(0))
#define INVALID_AUDIO_FILE_ID (static_cast<Audio::TAudioFileId>(0))
#define INVALID_AUDIO_FILE_COLLECTION_ID (static_cast<Audio::TAudioFileCollectionId>(0))
#define INVALID_AUDIO_FILE_LANGUAGE_ID (static_cast<Audio::TAudioFileLanguageId>(0))

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    // <title EAudioRequestStatus>
    // Summary:
    //      An enum that lists possible statuses of an AudioRequest.
    //      Used as a return type for many function used by the AudioSystem internally,
    //      and also for most of the IAudioSystemImplementation calls.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestStatus : TATLEnumFlagsType
    {
        eARS_NONE = 0,
        eARS_SUCCESS = 1,
        eARS_PARTIAL_SUCCESS = 2,
        eARS_FAILURE = 3,
        eARS_PENDING = 4,
        eARS_FAILURE_INVALID_OBJECT_ID = 5,
        eARS_FAILURE_INVALID_CONTROL_ID = 6,
        eARS_FAILURE_INVALID_REQUEST = 7,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! Converts a boolean value to an EAudioRequestStatus.
    //! @param result The boolean value to convert.
    //! @return eARS_SUCCESS if result is true, eARS_FAILURE otherwise.
    inline EAudioRequestStatus BoolToARS(bool result)
    {
        return result ? eARS_SUCCESS : eARS_FAILURE;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SATLWorldPosition
    {
        SATLWorldPosition()
            : m_transform(AZ::Matrix3x4::CreateIdentity())
        {}

        SATLWorldPosition(const AZ::Vector3& rPos)
            : m_transform(AZ::Matrix3x4::CreateIdentity())
        {
            m_transform.SetTranslation(rPos);
        }

        SATLWorldPosition(const AZ::Transform& rTransform)
            : m_transform(AZ::Matrix3x4::CreateFromTransform(rTransform))
        {}

        SATLWorldPosition(const AZ::Matrix3x4& rTransform)
            : m_transform(rTransform)
        {
        }

        inline AZ::Vector3 GetPositionVec() const
        {
            return m_transform.GetTranslation();
        }

        inline AZ::Vector3 GetUpVec() const
        {
            return m_transform.GetBasisZ();
        }

        inline AZ::Vector3 GetForwardVec() const
        {
            return m_transform.GetBasisY();
        }

        inline AZ::Vector3 GetRightVec() const
        {
            return m_transform.GetBasisX();
        }

        inline void NormalizeForwardVec()
        {
            auto forward = GetForwardVec();
            if (forward.IsZero())
            {
                m_transform.SetBasisY(AZ::Vector3::CreateAxisY());
            }
            else
            {
                forward.Normalize();
                m_transform.SetBasisY(forward);
            }
        }

        inline void NormalizeUpVec()
        {
            auto up = GetUpVec();
            if (up.IsZero())
            {
                m_transform.SetBasisZ(AZ::Vector3::CreateAxisZ());
            }
            else
            {
                up.Normalize();
                m_transform.SetBasisZ(up);
            }
        }

    private:
        AZ::Matrix3x4 m_transform;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioAssetType : TATLEnumFlagsType
    {
        eAAT_STREAM = 1,
        eAAT_SOURCE = 2,
        eAAT_NONE = 3,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioCodecType : TATLEnumFlagsType
    {
        eACT_PCM        = 1,
        eACT_ADPCM      = 2,
        eACT_XMA        = 3,
        eACT_VORBIS     = 4,
        eACT_XWMA       = 5,
        eACT_AAC        = 6,
        eACT_STREAM_PCM = 7,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestFlags : TATLEnumFlagsType
    {
        eARF_NONE                   = 0,                // assumes Lowest priority
        eARF_PRIORITY_NORMAL        = AUDIO_BIT(0),    // will be processed if no high priority requests are pending
        eARF_PRIORITY_HIGH          = AUDIO_BIT(1),    // will be processed first
        eARF_EXECUTE_BLOCKING       = AUDIO_BIT(2),    // blocks main thread until the request has been fully handled
        eARF_SYNC_CALLBACK          = AUDIO_BIT(3),    // callback (ATL's NotifyListener) will happen on the main thread
        eARF_SYNC_FINISHED_CALLBACK = AUDIO_BIT(4),    // "finished trigger instance" callback will happen on the main thread
        eARF_THREAD_SAFE_PUSH       = AUDIO_BIT(5),    // use when pushing a request from a non-main thread, e.g. AK::EventManager or AudioThread
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestType : TATLEnumFlagsType
    {
        eART_NONE                           = 0,
        eART_AUDIO_MANAGER_REQUEST          = 1,
        eART_AUDIO_CALLBACK_MANAGER_REQUEST = 2,
        eART_AUDIO_OBJECT_REQUEST           = 3,
        eART_AUDIO_LISTENER_REQUEST         = 4,
        eART_AUDIO_ALL_REQUESTS             = static_cast<TATLEnumFlagsType>(-1),
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioRequestResult : TATLEnumFlagsType
    {
        eARR_NONE       = 0,
        eARR_SUCCESS    = 1,
        eARR_FAILURE    = 2,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioEventState : TATLEnumFlagsType
    {
        eAES_NONE               = 0,
        eAES_PLAYING            = 1,
        eAES_PLAYING_DELAYED    = 2,
        eAES_LOADING            = 3,
        eAES_UNLOADING          = 4,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class ObstructionType : TATLEnumFlagsType
    {
        Ignore = 0,
        SingleRay,
        MultiRay,
        Count,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class PanningMode
    {
        Speakers,
        Headphones,
    };


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class AudioInputSourceType
    {
        Unsupported,    // Unsupported type
        //OggFile,      // Audio Input from an Ogg file
        //OpusFile,     // Audio Input from an Opus file
        PcmFile,        // Audio Input from a raw PCM file
        WavFile,        // Audio Input from a Wav file
        Microphone,     // Audio Input from a Microphone
        Synthesis,      // Audio Input that is synthesized (user-provided systhesis function)
        ExternalStream, // Audio Input from a stream source (video stream, network stream, etc)
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class AudioInputSampleType
    {
        Unsupported,    // Unsupported type
        Int,            // Integer type, probably don't need to differentiate signed vs unsigned
        Float,          // Floating poitn type
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class MultiPositionBehaviorType
    {
        Separate,       // Sound positions are treated separately as individual point sources, i.e. like torches along a wall.
        Blended,        // Sound positions are blended together as a 'spread out' sound, i.e. like a river.
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    using MultiPositionVec = AZStd::vector<AZ::Vector3>;

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct MultiPositionParams
    {
        MultiPositionVec m_positions;
        MultiPositionBehaviorType m_type;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioInputConfig
    {
        SAudioInputConfig()
            : m_sourceId(INVALID_AUDIO_SOURCE_ID)
            , m_sampleRate(0)
            , m_numChannels(0)
            , m_bitsPerSample(0)
            , m_bufferSize(0)
            , m_sampleType(AudioInputSampleType::Unsupported)
            , m_autoUnloadFile(false)
        {}

        SAudioInputConfig(AudioInputSourceType sourceType, const char* filename, bool autoUnloadFile = true)
            : m_sourceId(INVALID_AUDIO_SOURCE_ID)
            , m_sampleRate(0)
            , m_numChannels(0)
            , m_bitsPerSample(0)
            , m_bufferSize(0)
            , m_sourceType(sourceType)
            , m_sampleType(AudioInputSampleType::Unsupported)
            , m_sourceFilename(filename)
            , m_autoUnloadFile(autoUnloadFile)
        {}

        SAudioInputConfig(
            AudioInputSourceType sourceType,
            AZ::u32 sampleRate,
            AZ::u32 numChannels,
            AZ::u32 bitsPerSample,
            AudioInputSampleType sampleType)
            : m_sourceId(INVALID_AUDIO_SOURCE_ID)
            , m_sampleRate(sampleRate)
            , m_numChannels(numChannels)
            , m_bitsPerSample(bitsPerSample)
            , m_sourceType(sourceType)
            , m_sampleType(sampleType)
            , m_autoUnloadFile(false)
        {}

        void SetBufferSizeFromFrameCount(AZ::u32 frameCount)
        {
            m_bufferSize = (m_numChannels * frameCount * (m_bitsPerSample >> 3));
        }

        AZ::u32 GetSampleCountFromBufferSize() const
        {
            AZ_Assert(m_bitsPerSample >= 8, "Bits Per Sample is set too low!\n");
            return m_bufferSize / (m_bitsPerSample >> 3);
        }

        TAudioSourceId m_sourceId;          // This is set later after the source is created
        AZ::u32 m_sampleRate;               // 44100, 48000, ...
        AZ::u32 m_numChannels;              // 1 = Mono, 2 = Stereo
        AZ::u32 m_bitsPerSample;            // e.g. 16, 32
        AZ::u32 m_bufferSize;               // Size in bytes
        AudioInputSourceType m_sourceType;  // File, Synthesis, Microphone, ...
        AudioInputSampleType m_sampleType;  // Int, Float
        AZStd::string m_sourceFilename;
        bool m_autoUnloadFile;              // For file types, specifies whether file should unload after playback finishes
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioStreamData
    {
        AudioStreamData()
        {}

        AudioStreamData(AZ::u8* buffer, AZStd::size_t dataSize)
            : m_data(buffer)
            , m_sizeBytes(dataSize)
        {}

        AZ::u8* m_data = nullptr;           // points to start of raw data
        union {
            AZStd::size_t m_sizeBytes = 0;  // in bytes
            AZStd::size_t m_offsetBytes;    // if using this structure as a read/write bookmark, use this alias
        };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct AudioStreamMultiTrackData
    {
        AudioStreamMultiTrackData()
        {
            m_data[0] = m_data[1] = m_data[2] = m_data[3] = m_data[4] = m_data[5] = nullptr;
        }

        const void* m_data[6];              // 6 channels max
        union {
            AZStd::size_t m_sizeBytes = 0;  // size in bytes of each track
            AZStd::size_t m_offsetBytes;    // if using this structure as a read/write bookmark, use this alias
        };
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioRequestDataBase
    {
        explicit SAudioRequestDataBase(const EAudioRequestType eType = eART_NONE)
            : eRequestType(eType)
        {}

        virtual ~SAudioRequestDataBase() {}

        const EAudioRequestType eRequestType;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioSourceInfo
    {
        TAudioSourceId m_sourceId;
        TAudioFileId m_fileId;
        TAudioFileCollectionId m_languageId;
        TAudioFileLanguageId m_collectionId;
        EAudioCodecType m_codecType;

        SAudioSourceInfo()
            : m_sourceId(INVALID_AUDIO_SOURCE_ID)
            , m_fileId(INVALID_AUDIO_FILE_ID)
            , m_languageId(INVALID_AUDIO_FILE_LANGUAGE_ID)
            , m_collectionId(INVALID_AUDIO_FILE_COLLECTION_ID)
            , m_codecType(eACT_STREAM_PCM)
        {}

        SAudioSourceInfo(TAudioSourceId sourceId)
            : m_sourceId(sourceId)
            , m_fileId(INVALID_AUDIO_FILE_ID)
            , m_languageId(INVALID_AUDIO_FILE_LANGUAGE_ID)
            , m_collectionId(INVALID_AUDIO_FILE_COLLECTION_ID)
            , m_codecType(eACT_STREAM_PCM)
        {}

        SAudioSourceInfo(
            TAudioSourceId sourceId,
            TAudioFileId fileId,
            TAudioFileLanguageId languageId,
            TAudioFileCollectionId collectionId,
            EAudioCodecType codecType)
            : m_sourceId(sourceId)
            , m_fileId(fileId)
            , m_languageId(languageId)
            , m_collectionId(collectionId)
            , m_codecType(codecType)
        {}
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioCallBackInfos
    {
        struct UserData
        {
            UserData(void* userData)
                : m_pUserData(userData)
            {}

            UserData(uint64_t userData)
                : m_userData(userData)
            {}

            UserData& operator=(void* userData)
            {
                m_pUserData = userData;
                return *this;
            }

            UserData& operator=(uint64_t userData)
            {
                m_userData = userData;
                return *this;
            }

            operator void*() const
            {
                return m_pUserData;
            }

            operator uint64_t() const
            {
                return m_userData;
            }

            bool operator==(void* const userData)
            {
                return this->m_pUserData == userData;
            }

            bool operator==(uint64_t userData)
            {
                return this->m_userData == userData;
            }

            union
            {
                void* m_pUserData;
                uint64_t m_userData;
            };
        };

        SAudioCallBackInfos(const SAudioCallBackInfos& rOther)
            : pObjectToNotify(rOther.pObjectToNotify)
            , pUserData(rOther.pUserData)
            , pUserDataOwner(rOther.pUserDataOwner)
            , nRequestFlags(rOther.nRequestFlags)
        {
        }

        explicit SAudioCallBackInfos(
            void* const pPassedObjectToNotify = nullptr,
            UserData pPassedUserData = nullptr,
            void* const pPassedUserDataOwner = nullptr,
            const TATLEnumFlagsType nPassedRequestFlags = eARF_PRIORITY_NORMAL)
            : pObjectToNotify(pPassedObjectToNotify)
            , pUserData(pPassedUserData)
            , pUserDataOwner(pPassedUserDataOwner)
            , nRequestFlags(nPassedRequestFlags)
        {
        }

        static const SAudioCallBackInfos& GetEmptyObject()
        {
            static SAudioCallBackInfos emptyInstance;
            return emptyInstance;
        }

        void* const pObjectToNotify;
        UserData pUserData;
        void* const pUserDataOwner;
        const TATLEnumFlagsType nRequestFlags;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioRequest
    {
        SAudioRequest()
            : nFlags(eARF_NONE)
            , nAudioObjectID(INVALID_AUDIO_OBJECT_ID)
            , pOwner(nullptr)
            , pUserData(nullptr)
            , pUserDataOwner(nullptr)
            , pData(nullptr)
        {}

        ~SAudioRequest() {}

        SAudioRequest(const SAudioRequest& other) = delete;
        SAudioRequest& operator=(const SAudioRequest& other) = delete;

        TATLEnumFlagsType nFlags;
        TAudioObjectID nAudioObjectID;
        void* pOwner;
        void* pUserData;
        void* pUserDataOwner;
        SAudioRequestDataBase* pData;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    struct SAudioRequestInfo
    {
        explicit SAudioRequestInfo(
            const EAudioRequestResult ePassedResult,
            void* const pPassedOwner,
            void* const pPassedUserData,
            void* const pPassedUserDataOwner,
            const EAudioRequestType ePassedAudioRequestType,
            const TATLEnumFlagsType nPassedSpecificAudioRequest,
            const TAudioControlID nPassedAudioControlID,
            const TAudioObjectID nPassedAudioObjectID,
            const TAudioEventID passedAudioEventID)
            : eResult(ePassedResult)
            , pOwner(pPassedOwner)
            , pUserData(pPassedUserData)
            , pUserDataOwner(pPassedUserDataOwner)
            , eAudioRequestType(ePassedAudioRequestType)
            , nSpecificAudioRequest(nPassedSpecificAudioRequest)
            , nAudioControlID(nPassedAudioControlID)
            , nAudioObjectID(nPassedAudioObjectID)
            , audioEventID(passedAudioEventID)
        {}

        const EAudioRequestResult eResult;
        void* const pOwner;
        void* const pUserData;
        void* const pUserDataOwner;
        const EAudioRequestType eAudioRequestType;
        const TATLEnumFlagsType nSpecificAudioRequest;
        const TAudioControlID nAudioControlID;
        const TAudioObjectID nAudioObjectID;
        const TAudioEventID audioEventID;
    };

} // namespace Audio


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Audio::MultiPositionBehaviorType, "{96851568-74F9-4EEC-9195-82DCF701EEEF}");
    AZ_TYPE_INFO_SPECIALIZE(Audio::ObstructionType, "{8C056768-40E2-4B2D-AF01-9F7A6817BAAA}");
} // namespace AZ

