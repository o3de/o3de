/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include the required headers
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
#include <EMotionFX/Source/MotionData/MotionData.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/TransformData.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(Motion, MotionAllocator, 0)

    Motion::Motion(const char* name)
        : BaseObject()
    {
        mID = MCore::GetIDGenerator().GenerateID();
        m_eventTable = AZStd::make_unique<MotionEventTable>();
        mUnitType = GetEMotionFX().GetUnitType();
        mFileUnitType = mUnitType;
        mExtractionFlags = static_cast<EMotionExtractionFlags>(0);

        if (name)
        {
            SetName(name);
        }

#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime       = false;
#endif // EMFX_DEVELOPMENT_BUILD

        // automatically register the motion
        GetMotionManager().AddMotion(this);
    }

    Motion::~Motion()
    {
        // trigger the OnDeleteMotion event
        GetEventManager().OnDeleteMotion(this);

        // automatically unregister the motion
        if (mAutoUnregister)
        {
            GetMotionManager().RemoveMotion(this, false);
        }

        delete m_motionData;
    }


    // set the name of the motion
    void Motion::SetName(const char* name)
    {
        // calculate the ID
        mNameID = MCore::GetStringIdPool().GenerateIdForString(name);
    }


    // set the filename of the motion
    void Motion::SetFileName(const char* filename)
    {
        mFileName = filename;
    }


    // adjust the dirty flag
    void Motion::SetDirtyFlag(bool dirty)
    {
        mDirtyFlag = dirty;
    }


    // adjust the auto unregistering from the motion manager on delete
    void Motion::SetAutoUnregister(bool enabled)
    {
        mAutoUnregister = enabled;
    }


    // do we auto unregister from the motion manager on delete?
    bool Motion::GetAutoUnregister() const
    {
        return mAutoUnregister;
    }


    void Motion::SetIsOwnedByRuntime(bool isOwnedByRuntime)
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        mIsOwnedByRuntime = isOwnedByRuntime;
#else
        AZ_UNUSED(isOwnedByRuntime);
#endif
    }


    bool Motion::GetIsOwnedByRuntime() const
    {
#if defined(EMFX_DEVELOPMENT_BUILD)
        return mIsOwnedByRuntime;
#else
        return true;
#endif
    }


    const char* Motion::GetName() const
    {
        return MCore::GetStringIdPool().GetName(mNameID).c_str();
    }


    const AZStd::string& Motion::GetNameString() const
    {
        return MCore::GetStringIdPool().GetName(mNameID);
    }


    void Motion::SetMotionFPS(float motionFPS)
    {
        mMotionFPS = motionFPS;
    }


    float Motion::GetMotionFPS() const
    {
        return mMotionFPS;
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
        return mDirtyFlag;
    }


    void Motion::SetMotionExtractionFlags(EMotionExtractionFlags flags)
    {
        mExtractionFlags = flags;
    }


    EMotionExtractionFlags Motion::GetMotionExtractionFlags() const
    {
        return mExtractionFlags;
    }


    void Motion::SetCustomData(void* dataPointer)
    {
        mCustomData = dataPointer;
    }


    void* Motion::GetCustomData() const
    {
        return mCustomData;
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
        mID = id;
    }

    const char* Motion::GetFileName() const
    {
        return mFileName.c_str();
    }


    const AZStd::string& Motion::GetFileNameString() const
    {
        return mFileName;
    }


    uint32 Motion::GetID() const
    {
        return mID;
    }


    void Motion::SetUnitType(MCore::Distance::EUnitType unitType)
    {
        mUnitType = unitType;
    }


    MCore::Distance::EUnitType Motion::GetUnitType() const
    {
        return mUnitType;
    }


    void Motion::SetFileUnitType(MCore::Distance::EUnitType unitType)
    {
        mFileUnitType = unitType;
    }


    MCore::Distance::EUnitType Motion::GetFileUnitType() const
    {
        return mFileUnitType;
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
        if (mUnitType == targetUnitType)
        {
            return;
        }

        // calculate the scale factor and scale
        const float scaleFactor = static_cast<float>(MCore::Distance::GetConversionFactor(mUnitType, targetUnitType));
        Scale(scaleFactor);

        // update the unit type
        mUnitType = targetUnitType;
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

        MotionData::SampleSettings sampleSettings;
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

        MotionData::SampleSettings sampleSettings;
        sampleSettings.m_actorInstance = instance->GetActorInstance();
        sampleSettings.m_inPlace = instance->GetIsInPlace();
        sampleSettings.m_mirror = instance->GetMirrorMotion();
        sampleSettings.m_retarget = instance->GetRetargetingEnabled();
        sampleSettings.m_sampleTime = instance->GetCurrentTime();
        sampleSettings.m_inputPose = inputPose ? inputPose : sampleSettings.m_actorInstance->GetTransformData()->GetBindPose();
        
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
} // namespace EMotionFX
