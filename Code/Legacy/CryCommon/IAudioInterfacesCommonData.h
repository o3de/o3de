/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/base.h>
#include <AzCore/Component/EntityId.h>
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
    //      An enum that lists possible statuses of an in-progress AudioRequest.
    //      Used as a return type for many function used by the AudioSystem/ATL internally,
    //      and also for most of the IAudioSystemImplementation calls.
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class EAudioRequestStatus
    {
        None,
        Success,
        Failure,
        PartialSuccess,
        Pending,
        FailureInvalidObjectId,
        FailureInvalidControlId,
        FailureInvalidRequest,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum class EAudioRequestResult
    {
        Success,
        Failure,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //! Converts a boolean value to an EAudioRequestStatus.
    //! @param result The boolean value to convert.
    //! @return Success if result is true, Failure otherwise.
    inline EAudioRequestStatus BoolToARS(bool result)
    {
        return result ? EAudioRequestStatus::Success : EAudioRequestStatus::Failure;
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
        eARF_NONE           = 0,
        eARF_SYNC_CALLBACK  = AUDIO_BIT(0), // Indicates the callback will be called on the audio
                                            // thread immediately after the request has been handled.
                                            // If it's a blocking request, this means the callback is
                                            // executed before main thread is unblocked.
                                            // Care should be taken to avoid any data races.
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EAudioEventState : TATLEnumFlagsType
    {
        eAES_NONE               = 0,
        eAES_PLAYING            = 1,
        eAES_LOADING            = 3,
        eAES_UNLOADING          = 4,
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////
    enum EATLTriggerStatus : TATLEnumFlagsType
    {
        eATS_NONE                       = 0,
        eATS_PLAYING                    = AUDIO_BIT(0),
        eATS_PREPARED                   = AUDIO_BIT(1),
        eATS_LOADING                    = AUDIO_BIT(2),
        eATS_UNLOADING                  = AUDIO_BIT(3),
        eATS_STARTING                   = AUDIO_BIT(4),
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
        SAudioInputConfig() = default;

        SAudioInputConfig(AudioInputSourceType sourceType, const char* filename, bool autoUnloadFile = true)
            : m_sourceType(sourceType)
            , m_sourceFilename(filename)
            , m_autoUnloadFile(autoUnloadFile)
        {}

        SAudioInputConfig(
            AudioInputSourceType sourceType,
            AZ::u32 sampleRate,
            AZ::u32 numChannels,
            AZ::u32 bitsPerSample,
            AudioInputSampleType sampleType)
            : m_sampleRate(sampleRate)
            , m_numChannels(numChannels)
            , m_bitsPerSample(bitsPerSample)
            , m_sourceType(sourceType)
            , m_sampleType(sampleType)
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

        //! Source Id, this is set after the source is created with the manager
        TAudioSourceId m_sourceId{ INVALID_AUDIO_SOURCE_ID };
        //! Sample rate of the source, e.g. 44100, 48000
        AZ::u32 m_sampleRate{ 0 };
        //! Number of channels, e.g. 1 = Mono, 2 = Stereo
        AZ::u32 m_numChannels{ 0 };
        //! Number of bits per sample, e.g. 16, 32
        AZ::u32 m_bitsPerSample{ 0 };
        //! Size of the buffer in bytes
        AZ::u32 m_bufferSize{ 0 };
        //! The type of the source, e.g. File, Synthesis, Microphone
        AudioInputSourceType m_sourceType{ AudioInputSourceType::Unsupported };
        //! The sample format, e.g. Int, Float
        AudioInputSampleType m_sampleType{ AudioInputSampleType::Unsupported };
        //! The filename of the source (if any)
        AZStd::string m_sourceFilename{};
        //! For files, whether the file should unload after playback completes
        bool m_autoUnloadFile{ false };
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
    struct SAudioSourceInfo
    {
        TAudioSourceId m_sourceId{ INVALID_AUDIO_SOURCE_ID };
        TAudioFileId m_fileId{ INVALID_AUDIO_FILE_ID };
        TAudioFileCollectionId m_languageId{ INVALID_AUDIO_FILE_LANGUAGE_ID };
        TAudioFileLanguageId m_collectionId{ INVALID_AUDIO_FILE_COLLECTION_ID };
        EAudioCodecType m_codecType{ eACT_STREAM_PCM };

        SAudioSourceInfo() = default;

        SAudioSourceInfo(TAudioSourceId sourceId)
            : m_sourceId(sourceId)
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
    struct TriggerNotificationIdType
    {
        AZ_TYPE_INFO(TriggerNotificationIdType, "{E355AC15-8C88-4BDD-8CCE-9999EC32F970}");
        uintptr_t m_owner{ 0 };

        TriggerNotificationIdType() = default;
        ~TriggerNotificationIdType() = default;

        TriggerNotificationIdType(void* owner)
            : m_owner(reinterpret_cast<uintptr_t>(owner))
        {
        }
        TriggerNotificationIdType(AZ::EntityId owner)
            : m_owner(static_cast<uintptr_t>(static_cast<AZ::u64>(owner)))
        {
        }

        inline bool operator==(const TriggerNotificationIdType& rhs) const
        {
            return (m_owner == rhs.m_owner);
        }

        inline bool operator!=(const TriggerNotificationIdType& rhs) const
        {
            return !(*this == rhs); 
        }
    };


} // namespace Audio

namespace AZStd
{
    template<>
    struct hash<Audio::TriggerNotificationIdType>
    {
        constexpr size_t operator()(const Audio::TriggerNotificationIdType& id) const
        {
            AZStd::hash<uintptr_t> hasher;
            return hasher(id.m_owner);
        }
    };
} // namespace AZStd

namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(Audio::MultiPositionBehaviorType, "{96851568-74F9-4EEC-9195-82DCF701EEEF}");
    AZ_TYPE_INFO_SPECIALIZE(Audio::ObstructionType, "{8C056768-40E2-4B2D-AF01-9F7A6817BAAA}");
} // namespace AZ

