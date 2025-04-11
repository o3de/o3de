/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EMotionFXConfig.h"
#include "Motion.h"
#include <MCore/Source/IDGenerator.h>
#include "Pose.h"
#include "MotionInstance.h"
#include "Actor.h"
#include "ActorInstance.h"
#include "MotionManager.h"
#include "EventManager.h"
#include "EventHandler.h"
#include "MotionEventTable.h"
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/MotionData/RootMotionExtractionData.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Motion, MotionAllocator)

    Motion::Motion(const char* name)
        : MCore::RefCounted()
    {
        m_id = aznumeric_caster(MCore::GetIDGenerator().GenerateID());
        m_eventTable = AZStd::make_unique<MotionEventTable>();
        m_unitType = GetEMotionFX().GetUnitType();
        m_fileUnitType = m_unitType;
        m_extractionFlags = static_cast<EMotionExtractionFlags>(0);

        if (name)
        {
            SetName(name);
        }

#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime       = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // automatically register the motion
        GetMotionManager().AddMotion(this);
    }

    Motion::~Motion()
    {
        // trigger the OnDeleteMotion event
        GetEventManager().OnDeleteMotion(this);

        // automatically unregister the motion
        if (m_autoUnregister)
        {
            GetMotionManager().RemoveMotion(this, false);
        }

        delete m_motionData;
    }


    // set the name of the motion
    void Motion::SetName(const char* name)
    {
        // calculate the ID
        m_nameId = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // set the filename of the motion
    void Motion::SetFileName(const char* filename)
    {
        m_fileName = filename;
    }


    // adjust the dirty flag
    void Motion::SetDirtyFlag(bool dirty)
    {
        m_dirtyFlag = dirty;
    }


    // adjust the auto unregistering from the motion manager on delete
    void Motion::SetAutoUnregister(bool enabled)
    {
        m_autoUnregister = enabled;
    }


    // do we auto unregister from the motion manager on delete?
    bool Motion::GetAutoUnregister() const
    {
        return m_autoUnregister;
    }


    void Motion::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        m_isOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool Motion::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return m_isOwnedByRuntime;
#else
        return true;
#endif
    }


    const char* Motion::GetName() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId).c_str();
    }


    const AZStd::string& Motion::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(m_nameId);
    }


    void Motion::SetMotionFPS(float motionFPS)
    {
        m_motionFps = motionFPS;
    }


    float Motion::GetMotionFPS() const
    {
        return m_motionFps;
    }


    void Motion::SetDefaultPlayBackInfo(const PlayBackInfo& playBackInfo)
    {
        m_defaultPlayBackInfo = playBackInfo;
    }


    PlayBackInfo* Motion::GetDefaultPlayBackInfo()
    {
        return &m_defaultPlayBackInfo;
    }

    const PlayBackInfo* Motion::GetDefaultPlayBackInfo() const
    {
        return &m_defaultPlayBackInfo;
    }


    bool Motion::GetDirtyFlag() const
    {
        return m_dirtyFlag;
    }


    void Motion::SetMotionExtractionFlags(EMotionExtractionFlags flags)
    {
        m_extractionFlags = flags;
    }


    EMotionExtractionFlags Motion::GetMotionExtractionFlags() const
    {
        return m_extractionFlags;
    }


    void Motion::SetCustomData(void* dataPointer)
    {
        m_customData = dataPointer;
    }


    void* Motion::GetCustomData() const
    {
        return m_customData;
    }


    MotionEventTable* Motion::GetEventTable() const
    {
        return m_eventTable.get();
    }

    void Motion::SetEventTable(AZStd::unique_ptr<MotionEventTable> eventTable)
    {
        m_eventTable = AZStd::move(eventTable);
    }

    void Motion::SetID(uint32 id)
    {
        m_id = id;
    }

    const char* Motion::GetFileName() const
    {
        return m_fileName.c_str();
    }


    const AZStd::string& Motion::GetFileNameString() const
    {
        return m_fileName;
    }


    uint32 Motion::GetID() const
    {
        return m_id;
    }


    void Motion::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        m_unitType = unitType;
    }


    MCore::Distance::EUnitType Motion::GetUnitType() const
    {
        return m_unitType;
    }


    void Motion::SetFileUnitType(MCore::Distance::EUnitType unitType)
    {
        m_fileUnitType = unitType;
    }


    MCore::Distance::EUnitType Motion::GetFileUnitType() const
    {
        return m_fileUnitType;
    }

    void Motion::Scale(float scaleFactor)
    {
        AZ_Assert(m_motionData, "Expecting motion data");
        m_motionData->Scale(scaleFactor);
        GetEventManager().OnScaleMotionData(this, scaleFactor);
    }

    // scale everything to the given unit type
    void Motion::ScaleToUnitType(MCore::Distance::EUnitType targetUnitType)
    {
        if (m_unitType == targetUnitType)
        {
            return;
        }

        // calculate the scale factor and scale
        const float scaleFactor = static_cast<float>(MCore::Distance::GetConversionFactor(m_unitType, targetUnitType));
        Scale(scaleFactor);

        // update the unit type
        m_unitType = targetUnitType;
    }

    void Motion::UpdateDuration()
    {
        AZ_Assert(m_motionData, "Expecting motion data");
        m_motionData->UpdateDuration();
    }

    float Motion::GetDuration() const
    {
        AZ_Assert(m_motionData, "Expecting motion data");
        return m_motionData->GetDuration();
    }

    void Motion::CalcNodeTransform(const MotionInstance* instance, Transform* outTransform, [[maybe_unused]] Actor* actor, Node* node, float timeValue, bool enableRetargeting)
    {
        AZ_Assert(m_motionData, "Expecting motion data");

        MotionDataSampleSettings sampleSettings;
        sampleSettings.m_actorInstance = instance->GetActorInstance();
        sampleSettings.m_inPlace = instance->GetIsInPlace();
        sampleSettings.m_mirror = instance->GetMirrorMotion();
        sampleSettings.m_retarget = enableRetargeting;
        sampleSettings.m_sampleTime = timeValue;
        sampleSettings.m_inputPose = sampleSettings.m_actorInstance->GetTransformData()->GetBindPose();

        *outTransform = m_motionData->SampleJointTransform(sampleSettings, node->GetNodeIndex());
    }

    void Motion::Update(const Pose* inputPose, Pose* outputPose, MotionInstance* instance)
    {
        AZ_Assert(m_motionData, "Expecting motion data");

        MotionDataSampleSettings sampleSettings;
        sampleSettings.m_actorInstance = instance->GetActorInstance();
        sampleSettings.m_inPlace = instance->GetIsInPlace();
        sampleSettings.m_mirror = instance->GetMirrorMotion();
        sampleSettings.m_retarget = instance->GetRetargetingEnabled();
        sampleSettings.m_sampleTime = instance->GetCurrentTime();
        sampleSettings.m_inputPose = inputPose ? inputPose : sampleSettings.m_actorInstance->GetTransformData()->GetBindPose();

        m_motionData->SamplePose(sampleSettings, outputPose);
    }

    void Motion::SamplePose(Pose* outputPose, const MotionDataSampleSettings& sampleSettings)
    {
        AZ_Assert(m_motionData, "Expecting motion data");
        m_motionData->SamplePose(sampleSettings, outputPose);
    }

    const MotionData* Motion::GetMotionData() const
    {
        return m_motionData;
    }

    MotionData* Motion::GetMotionData()
    {
        return m_motionData;
    }

    void Motion::SetMotionData(MotionData* motionData, bool delOldFromMem)
    {
        if (delOldFromMem)
        {
            delete m_motionData;
        }
        m_motionData = motionData;
    }

    void Motion::SetRootMotionExtractionData(AZStd::shared_ptr<RootMotionExtractionData> data)
    {
        m_rootMotionExtractionData = AZStd::move(data);
    }

    const AZStd::shared_ptr<RootMotionExtractionData>& Motion::GetRootMotionExtractionData() const
    {
        return m_rootMotionExtractionData;
    }
} // namespace EMotionFX
