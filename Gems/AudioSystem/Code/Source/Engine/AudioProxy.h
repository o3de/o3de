/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AudioAllocators.h>
#include <IAudioSystem.h>

#include <AzCore/std/containers/deque.h>

namespace Audio
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    class CAudioProxy
        : public IAudioProxy
    {
    public:
        CAudioProxy();
        ~CAudioProxy() override;

        // IAudioProxy
        void Initialize(const char* const sObjectName, const bool bInitAsync = true) override;
        void Release() override;
        void Reset() override;
        void ExecuteSourceTrigger(
            TAudioControlID nTriggerID,
            const SAudioSourceInfo& rSourceInfo,
            const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override;
        void ExecuteTrigger(const TAudioControlID nTriggerID, const SAudioCallBackInfos& rCallbackInfos = SAudioCallBackInfos::GetEmptyObject()) override;
        void StopAllTriggers() override;
        void StopTrigger(const TAudioControlID nTriggerID) override;
        void SetSwitchState(const TAudioControlID nSwitchID, const TAudioSwitchStateID nStateID) override;
        void SetRtpcValue(const TAudioControlID nRtpcID, const float fValue) override;
        void SetObstructionCalcType(const EAudioObjectObstructionCalcType eObstructionType) override;
        void SetPosition(const SATLWorldPosition& refPosition) override;
        void SetPosition(const AZ::Vector3& refPosition) override;
        void SetMultiplePositions(const MultiPositionParams& params) override;
        void SetEnvironmentAmount(const TAudioEnvironmentID nEnvironmentID, const float fValue) override;
        void SetCurrentEnvironments() override;
        void ResetRtpcValues() override;
        TAudioObjectID GetAudioObjectID() const override
        {
            return m_nAudioObjectID;
        }
        // ~IAudioProxy

        static void OnAudioEvent(const SAudioRequestInfo* const pAudioRequestInfo);

        void ClearEnvironments();
        void ExecuteQueuedCommands();

    private:
        static const size_t sMaxAreas = 10;

        TAudioObjectID m_nAudioObjectID;
        SATLWorldPosition m_oPosition;

        TATLEnumFlagsType m_nFlags;

        ///////////////////////////////////////////////////////////////////////////////////////////////
        enum EQueuedAudioCommandType : TATLEnumFlagsType
        {
            eQACT_NONE                      = 0,
            eQACT_EXECUTE_TRIGGER,
            eQACT_EXECUTE_SOURCE_TRIGGER,
            eQACT_STOP_TRIGGER,
            eQACT_SET_SWITCH_STATE,
            eQACT_SET_RTPC_VALUE,
            eQACT_SET_POSITION,
            eQACT_SET_ENVIRONMENT_AMOUNT,
            eQACT_SET_CURRENT_ENVIRONMENTS,
            eQACT_CLEAR_ENVIRONMENTS,
            eQACT_CLEAR_RTPCS,
            eQACT_RESET,
            eQACT_RELEASE,
            eQACT_INITIALIZE,
            eQACT_STOP_ALL_TRIGGERS,
            eQACT_SET_MULTI_POSITIONS,
        };

        ///////////////////////////////////////////////////////////////////////////////////////////////
        struct SQueuedAudioCommand
        {
            SQueuedAudioCommand(const EQueuedAudioCommandType ePassedType)
                : eType(ePassedType)
                , nTriggerID(INVALID_AUDIO_CONTROL_ID)
                , nSwitchID(INVALID_AUDIO_CONTROL_ID)
                , nStateID(INVALID_AUDIO_SWITCH_STATE_ID)
                , nRtpcID(INVALID_AUDIO_CONTROL_ID)
                , fValue(0.0f)
                , nEnvironmentID(INVALID_AUDIO_ENVIRONMENT_ID)
                , pOwnerOverride(nullptr)
                , pUserData(nullptr)
                , pUserDataOwner(nullptr)
                , nRequestFlags(eARF_NONE)
            {}

            SQueuedAudioCommand& operator=(const SQueuedAudioCommand& refOther)
            {
                eType = refOther.eType;
                nTriggerID = refOther.nTriggerID;
                nSwitchID = refOther.nSwitchID;
                nStateID = refOther.nStateID;
                nRtpcID = refOther.nRtpcID;
                fValue = refOther.fValue;
                nEnvironmentID = refOther.nEnvironmentID;
                pOwnerOverride = refOther.pOwnerOverride;
                pUserData = refOther.pUserData;
                pUserDataOwner = refOther.pUserDataOwner;
                nRequestFlags = refOther.nRequestFlags;
                sValue = refOther.sValue;
                oPosition = refOther.oPosition;
                oMultiPosParams = refOther.oMultiPosParams;
                rSourceInfo = refOther.rSourceInfo;
                return *this;
            }

            EQueuedAudioCommandType eType;
            TAudioControlID nTriggerID;
            TAudioControlID nSwitchID;
            TAudioSwitchStateID nStateID;
            TAudioControlID nRtpcID;
            float fValue;
            TAudioEnvironmentID nEnvironmentID;
            void* pOwnerOverride;
            void* pUserData;
            void* pUserDataOwner;
            TATLEnumFlagsType nRequestFlags;
            AZStd::string sValue;
            SATLWorldPosition oPosition;
            MultiPositionParams oMultiPosParams;
            SAudioSourceInfo rSourceInfo;
        };

        ///////////////////////////////////////////////////////////////////////////////////////////////
        enum EAudioProxyFlags : TATLEnumFlagsType
        {
            eAPF_NONE           = 0,
            eAPF_WAITING_FOR_ID = AUDIO_BIT(0),
        };

        using TQueuedAudioCommands = AZStd::deque<SQueuedAudioCommand, Audio::AudioSystemStdAllocator>;
        TQueuedAudioCommands m_aQueuedAudioCommands;

        void TryAddQueuedCommand(const SQueuedAudioCommand& refCommand);

        //////////////////////////////////////////////////////////////////////////
        struct SFindSetSwitchState
        {
            SFindSetSwitchState(const TAudioControlID passedAudioSwitchID, const TAudioSwitchStateID passedAudioSwitchStateID)
                : audioSwitchID(passedAudioSwitchID)
                , audioSwitchStateID(passedAudioSwitchStateID)
            {}

            bool operator()(SQueuedAudioCommand& refCommand)
            {
                bool bFound = false;

                if (refCommand.eType == eQACT_SET_SWITCH_STATE && refCommand.nSwitchID == audioSwitchID)
                {
                    // Command for this switch exists, just update the state.
                    refCommand.nStateID = audioSwitchStateID;
                    bFound = true;
                }

                return bFound;
            }

        private:

            const TAudioControlID audioSwitchID;
            const TAudioSwitchStateID audioSwitchStateID;
        };

        //////////////////////////////////////////////////////////////////////////
        struct SFindSetRtpcValue
        {
            SFindSetRtpcValue(const TAudioControlID passedAudioRtpcID, const float passedAudioRtpcValue)
                : audioRtpcID(passedAudioRtpcID)
                , audioRtpcValue(passedAudioRtpcValue)
            {}

            bool operator()(SQueuedAudioCommand& refCommand)
            {
                bool bFound = false;

                if (refCommand.eType == eQACT_SET_RTPC_VALUE && refCommand.nRtpcID == audioRtpcID)
                {
                    // Command for this RTPC exists, just update the value.
                    refCommand.fValue = audioRtpcValue;
                    bFound = true;
                }

                return bFound;
            }

        private:

            const TAudioControlID audioRtpcID;
            const float audioRtpcValue;
        };

        //////////////////////////////////////////////////////////////////////////
        struct SFindSetPosition
        {
            SFindSetPosition(const SATLWorldPosition& refPassedPosition)
                : refPosition(refPassedPosition)
            {}

            bool operator()(SQueuedAudioCommand& refCommand)
            {
                bool bFound = false;

                // Single Position cannot be added into queue when a Multi-Position is already present.

                if (refCommand.eType == eQACT_SET_POSITION)
                {
                    // Single Position exists already, update the position.
                    refCommand.oPosition = refPosition;
                    bFound = true;
                }

                return bFound;
            }

        private:

            const SATLWorldPosition& refPosition;
        };

        //////////////////////////////////////////////////////////////////////////
        struct SFindSetMultiplePositions
        {
            SFindSetMultiplePositions(const MultiPositionParams& refPassedParams)
                : refParams(refPassedParams)
            {}

            bool operator()(SQueuedAudioCommand& refCommand)
            {
                bool found = false;

                if (refCommand.eType == eQACT_SET_POSITION)
                {
                    // Multi-Position command replaces an existing Single Position.
                    refCommand.eType = eQACT_SET_MULTI_POSITIONS;
                }

                if (refCommand.eType == eQACT_SET_MULTI_POSITIONS)
                {
                    // Multi-Position exists already, update with the newer position list.
                    refCommand.oMultiPosParams.m_positions.assign(refParams.m_positions.begin(), refParams.m_positions.end());
                    refCommand.oMultiPosParams.m_type = refParams.m_type;
                    found = true;
                }

                return found;
            }

        private:
            const MultiPositionParams& refParams;
        };

        //////////////////////////////////////////////////////////////////////////
        struct SFindSetEnvironmentAmount
        {
            SFindSetEnvironmentAmount(const TAudioEnvironmentID passedAudioEnvironmentID, const float passedAudioEnvironmentValue)
                : audioEnvironmentID(passedAudioEnvironmentID)
                , audioEnvironmentValue(passedAudioEnvironmentValue)
            {}

            bool operator()(SQueuedAudioCommand& refCommand)
            {
                bool bFound = false;

                if (refCommand.eType == eQACT_SET_ENVIRONMENT_AMOUNT && refCommand.nEnvironmentID == audioEnvironmentID)
                {
                    // Command for this environment exists, just update the value.
                    refCommand.fValue = audioEnvironmentValue;
                    bFound = true;
                }

                return bFound;
            }

        private:

            const TAudioEnvironmentID audioEnvironmentID;
            const float audioEnvironmentValue;
        };

        //////////////////////////////////////////////////////////////////////////
        struct SFindCommand
        {
            SFindCommand(const EQueuedAudioCommandType passedAudioQueuedCommandType)
                : audioQueuedCommandType(passedAudioQueuedCommandType)
            {}

            bool operator()(const SQueuedAudioCommand& refCommand)
            {
                return refCommand.eType == audioQueuedCommandType;
            }

        private:

            const EQueuedAudioCommandType audioQueuedCommandType;
        };
    };
} // namespace Audio
