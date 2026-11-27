/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <OpenXRVk/OpenXRVkReferenceSpacesInterface.h>
#include <OpenXRVk/OpenXRVkActionsInterface.h>

#include "OpenXRVkBehaviorReflection.h"

namespace OpenXRVk
{
    void PoseWithVelocities::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PoseWithVelocities>()
                ->Version(1)
                ->Field("Pose", &PoseWithVelocities::m_pose)
                ->Field("LinearVelocity", &PoseWithVelocities::m_linearVelocity)
                ->Field("AngularVelocitu", &PoseWithVelocities::m_angularVelocity)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<PoseWithVelocities>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "openxr")
                ->Property("pose", BehaviorValueGetter(&PoseWithVelocities::m_pose), nullptr)
                ->Property("linearVelocity", BehaviorValueGetter(&PoseWithVelocities::m_linearVelocity), nullptr)
                ->Property("angularVelocity", BehaviorValueGetter(&PoseWithVelocities::m_angularVelocity), nullptr)
                ;
        }
    }


    //! This class implements the functions that are exported into the Behavior Context
    //! as a means to provide scripting access to OpenXRReferenceSpacesInterface.
    class OpenXRReferenceSpaces
    {
    public:
        AZ_TYPE_INFO(OpenXRReferenceSpaces, "{A060D5C5-1514-421B-9AAA-1E276BA2E33E}");
        AZ_CLASS_ALLOCATOR(OpenXRReferenceSpaces, AZ::SystemAllocator);

        static constexpr char LogName[] = "OpenXRReferenceSpaces";

        OpenXRReferenceSpaces() = default;
        ~OpenXRReferenceSpaces() = default;

        static AZStd::vector<AZStd::string> GetReferenceSpaceNames()
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                return {};
            }
            return iface->GetReferenceSpaceNames();
        }


        static AZ::Outcome<bool, AZStd::string> AddReferenceSpace(IOpenXRReferenceSpaces::ReferenceSpaceId referenceSpaceType,
            const AZStd::string& spaceName, const AZ::Transform& poseInReferenceSpace)
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__)
                );
            }
            return iface->AddReferenceSpace(referenceSpaceType, spaceName, poseInReferenceSpace);
        }


        static AZ::Outcome<bool, AZStd::string> RemoveReferenceSpace(const AZStd::string& spaceName)
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__)
                );
            }
            return iface->RemoveReferenceSpace(spaceName);
        }


        static AZ::Outcome<AZ::Transform, AZStd::string> GetReferenceSpacePose(const AZStd::string& spaceName, const AZStd::string& baseSpaceName)
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__)
                );
            }
            return iface->GetReferenceSpacePose(spaceName, baseSpaceName);
        }


        static AZ::Outcome<bool, AZStd::string> SetBaseSpaceForViewSpacePose(const AZStd::string& spaceName)
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__)
                );
            }
            return iface->SetBaseSpaceForViewSpacePose(spaceName);
        }


        static const AZStd::string& GetBaseSpaceForViewSpacePose()
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                static const AZStd::string emptyStr;
                return emptyStr;
            }
            return iface->GetBaseSpaceForViewSpacePose();
        }


        static const AZ::Transform& GetViewSpacePose()
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                return AZ::Transform::Identity();
            }
            return iface->GetViewSpacePose();
        }


        static uint32_t GetViewCount()
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                return 0;
            }
            return iface->GetViewCount();
        }


        static const AZ::Transform& GetViewPose(uint32_t eyeIndex)
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                return AZ::Transform::Identity();
            }
            return iface->GetViewPose(eyeIndex);
        }


        static const AZStd::vector<AZ::Transform>& GetViewPoses()
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                static AZStd::vector<AZ::Transform> EmptyList;
                return EmptyList;
            }
            return iface->GetViewPoses();
        }


        static void ForceViewPosesCacheUpdate()
        {
            const auto iface = OpenXRReferenceSpacesInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRReferenceSpacesInterface is not available.", __FUNCTION__);
                return;
            }
            iface->ForceViewPosesCacheUpdate();
        }

    }; // class OpenXRReferenceSpaces


    //! This class implements the functions that are exported into the Behavior Context
    //! as a means to provide scripting access to OpenXRActionsInterface.
    class OpenXRActions
    {
    public:
        AZ_TYPE_INFO(OpenXRActions, "{290DF9D5-4042-4843-BF23-D49F9FAD6D90}");
        AZ_CLASS_ALLOCATOR(OpenXRActions, AZ::SystemAllocator);

        static constexpr char LogName[] = "OpenXRActions";

        OpenXRActions() = default;
        ~OpenXRActions() = default;


        static AZStd::vector<AZStd::string> GetAllActionSets()
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRActionsInterface is not available.", __FUNCTION__);
                return {};
            }
            return iface->GetAllActionSets();
        }


        static AZ::Outcome<bool, AZStd::string> GetActionSetState(const AZStd::string& actionSetName)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRActionsInterface is not available.", __FUNCTION__);
                return {};
            }
            return iface->GetActionSetState(actionSetName);
        }


        static AZ::Outcome<bool, AZStd::string> SetActionSetState(const AZStd::string& actionSetName, bool activate)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRActionsInterface is not available.", __FUNCTION__);
                return {};
            }
            return iface->SetActionSetState(actionSetName, activate);
        }


        //! REMARK: The original idea was to return an:
        //! AZ::Outcome<IOpenXRActions::ActionHandle, AZStd::string>
        //! But when compiling for Android the Behavior Context ->Method() would statically
        //! fail claiming that it was not constructible.
        static IOpenXRActions::ActionHandle GetActionHandle(const AZStd::string& actionSetName, const AZStd::string& actionName)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRActionsInterface is not available.", __FUNCTION__);
                return {};
            }
            return iface->GetActionHandle(actionSetName, actionName);
        }


        static AZ::Outcome<bool, AZStd::string> GetActionStateBoolean(IOpenXRActions::ActionHandle actionHandle)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->GetActionStateBoolean(actionHandle);
        }


        static AZ::Outcome<float, AZStd::string> GetActionStateFloat(IOpenXRActions::ActionHandle actionHandle)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->GetActionStateFloat(actionHandle);
        }


        static AZ::Outcome<AZ::Vector2, AZStd::string> GetActionStateVector2(IOpenXRActions::ActionHandle actionHandle)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->GetActionStateVector2(actionHandle);
        }


        static AZ::Outcome<bool, AZStd::string> SetBaseReferenceSpaceForPoseActions(const AZStd::string& visualizedSpaceName)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->SetBaseReferenceSpaceForPoseActions(visualizedSpaceName);
        }


        static const AZStd::string& GetBaseReferenceSpaceForPoseActions()
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                AZ_Error(LogName, false, "%s: OpenXRActionsInterface is not available.", __FUNCTION__);
                static const AZStd::string emptyStr;
                return emptyStr;
            }
            return iface->GetBaseReferenceSpaceForPoseActions();
        }


        static AZ::Outcome<AZ::Transform, AZStd::string> GetActionStatePose(IOpenXRActions::ActionHandle actionHandle)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->GetActionStatePose(actionHandle);
        }


        static AZ::Outcome<PoseWithVelocities, AZStd::string> GetActionStatePoseWithVelocities(IOpenXRActions::ActionHandle actionHandle)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->GetActionStatePoseWithVelocities(actionHandle);
        }


        static AZ::Outcome<bool, AZStd::string> ApplyHapticVibrationAction(IOpenXRActions::ActionHandle actionHandle, uint64_t durationNanos, float frequencyHz, float amplitude)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->ApplyHapticVibrationAction(actionHandle, durationNanos, frequencyHz, amplitude);
        }


        static AZ::Outcome<bool, AZStd::string> StopHapticVibrationAction(IOpenXRActions::ActionHandle actionHandle)
        {
            const auto iface = OpenXRActionsInterface::Get();
            if (!iface)
            {
                return AZ::Failure(
                    AZStd::string::format("%s: OpenXRActionsInterface is not available.", __FUNCTION__)
                );
            }
            return iface->StopHapticVibrationAction(actionHandle);
        }

    }; // class OpenXRActions


    void OpenXRBehaviorReflect(AZ::BehaviorContext& context)
    {
        IOpenXRActions::ActionHandle::Reflect(&context);
        PoseWithVelocities::Reflect(&context);

        context.Class<OpenXRReferenceSpaces>("OpenXRReferenceSpaces")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, "openxr")
            ->Constant("ReferenceSpaceIdView", BehaviorConstant(IOpenXRReferenceSpaces::ReferenceSpaceIdView))
            ->Constant("ReferenceSpaceIdLocal", BehaviorConstant(IOpenXRReferenceSpaces::ReferenceSpaceIdLocal))
            ->Constant("ReferenceSpaceIdStage", BehaviorConstant(IOpenXRReferenceSpaces::ReferenceSpaceIdStage))
            ->Constant("ReferenceSpaceNameView", BehaviorConstant(AZStd::string(IOpenXRReferenceSpaces::ReferenceSpaceNameView)))
            ->Constant("ReferenceSpaceNameLocal", BehaviorConstant(AZStd::string(IOpenXRReferenceSpaces::ReferenceSpaceNameLocal)))
            ->Constant("ReferenceSpaceNameStage", BehaviorConstant(AZStd::string(IOpenXRReferenceSpaces::ReferenceSpaceNameStage)))
            ->Constant("LeftEyeViewId", BehaviorConstant(IOpenXRReferenceSpaces::LeftEyeView))
            ->Constant("RightEyeViewId", BehaviorConstant(IOpenXRReferenceSpaces::RightEyeView))
            ->Method("GetReferenceSpaceNames", &OpenXRReferenceSpaces::GetReferenceSpaceNames)
            ->Method("AddReferenceSpace", &OpenXRReferenceSpaces::AddReferenceSpace)
            ->Method("RemoveReferenceSpace", &OpenXRReferenceSpaces::RemoveReferenceSpace)
            ->Method("GetReferenceSpacePose", &OpenXRReferenceSpaces::GetReferenceSpacePose)
            ->Method("SetBaseSpaceForViewSpacePose", &OpenXRReferenceSpaces::SetBaseSpaceForViewSpacePose)
            ->Method("GetBaseSpaceForViewSpacePose", &OpenXRReferenceSpaces::GetBaseSpaceForViewSpacePose)
            ->Method("GetViewSpacePose", &OpenXRReferenceSpaces::GetViewSpacePose)
            ->Method("GetViewCount", &OpenXRReferenceSpaces::GetViewCount)
            ->Method("GetViewPose", &OpenXRReferenceSpaces::GetViewPose)
            ->Method("GetViewPoses", &OpenXRReferenceSpaces::GetViewPoses)
            ->Method("ForceViewPosesCacheUpdate", &OpenXRReferenceSpaces::ForceViewPosesCacheUpdate)
             ;

        context.Class<OpenXRActions>("OpenXRActions")
            ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
            ->Attribute(AZ::Script::Attributes::Module, "openxr")
            ->Method("GetAllActionSets", &OpenXRActions::GetAllActionSets)
            ->Method("GetActionSetState", &OpenXRActions::GetActionSetState)
            ->Method("SetActionSetState", &OpenXRActions::SetActionSetState)
            ->Method("GetActionHandle", &OpenXRActions::GetActionHandle)
            ->Method("GetActionStateBoolean", &OpenXRActions::GetActionStateBoolean)
            ->Method("GetActionStateFloat", &OpenXRActions::GetActionStateFloat)
            ->Method("GetActionStateVector2", &OpenXRActions::GetActionStateVector2)
            ->Method("SetBaseReferenceSpaceForPoseActions", &OpenXRActions::SetBaseReferenceSpaceForPoseActions)
            ->Method("GetBaseReferenceSpaceForPoseActions", &OpenXRActions::GetBaseReferenceSpaceForPoseActions)
            ->Method("GetActionStatePose", &OpenXRActions::GetActionStatePose)
            ->Method("GetActionStatePoseWithVelocities", &OpenXRActions::GetActionStatePoseWithVelocities)
            ->Method("ApplyHapticVibrationAction", &OpenXRActions::ApplyHapticVibrationAction)
            ->Method("StopHapticVibrationAction", &OpenXRActions::StopHapticVibrationAction)
            ;
    }

}
