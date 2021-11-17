/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Editor/Source/ComponentModes/Joints/JointsComponentMode.h>

#include <AzCore/Memory/SystemAllocator.h>
#include <Editor/EditorJointConfiguration.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeAngleCone.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeAnglePair.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeLinearFloat.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeRotation.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeSnapPosition.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeSnapRotation.h>
#include <Editor/Source/ComponentModes/Joints/JointsSubComponentModeTranslate.h>

namespace PhysX
{
    AZ_CLASS_ALLOCATOR_IMPL(JointsComponentMode, AZ::SystemAllocator, 0);

    namespace SubModeData
    {
        const AZ::Crc32 SwitchToTranslationSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtotranslationsubmode");
        static const char* TranslationTitle = "Switch to Position Mode";
        static const char* TranslationToolTip = "Position Mode - Change the position of the joint.";

        const AZ::Crc32 SwitchToRotationSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtorotationsubmode");
        static const char* RotationTitle = "Switch to Rotation Mode";
        static const char* RotationToolTip = "Rotation Mode- Change the rotation of the joint.";

        const AZ::Crc32 SwitchToMaxForceSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtomaxforce");
        static const char* MaxForceTitle = "Switch to Max Force Mode";
        static const char* MaxForceToolTip = "Max Force Mode - Change the maximum force allowed before the joint breaks.";

        const AZ::Crc32 SwitchToMaxTorqueSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtomaxtorque");
        static const char* MaxTorqueTitle = "Switch to Max Torque Mode";
        static const char* MaxTorqueToolTip = "Max Torque Mode - Change the maximum torque allowed before the joint breaks.";

        const AZ::Crc32 SwitchToDampingSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtodamping");
        static const char* DampingTitle = "Switch to Damping Mode";
        static const char* DampingToolTip = "Damping Mode - Change the damping strength of the joint when beyond the limit.";

        const AZ::Crc32 SwitchToStiffnessSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtostiffness");
        static const char* StiffnessTitle = "Switch to Stiffness Mode";
        static const char* StiffnessToolTip = "Stiffness Mode - Change the stiffness strength of the joint when beyond the limit.";

        const AZ::Crc32 SwitchToTwistLimitsSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtotwistlimits");
        static const char* TwistLimitsTitle = "Switch to Twist Limits Mode";
        static const char* TwistLimitsToolTip = "Twist Limits Mode - Change the limits of the joint.";

        const AZ::Crc32 SwitchToSwingLimitsSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtoswinglimits");
        static const char* SwingLimitsTitle = "Switch to Swing Limits Mode";
        static const char* SwingLimitsToolTip = "Swing Limits Mode - Change the limits of the joint.";

        const AZ::Crc32 SwitchToSnapPositionSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtosnapposition");
        static const char* SnapPositionTitle = "Switch to Snap Position Mode";
        static const char* SnapPositionToolTip = "Snap Position Mode - Snap the position of the joint to another Entity.";

        const AZ::Crc32 SwitchToSnapRotationSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.switchtosnaprotation");
        static const char* SnapRotationTitle = "Switch to Snap Rotation Mode";
        static const char* SnapRotationToolTip = "Snap Rotation Mode - Snap the rotation of the joint toward another Entity.";

        const AZ::Crc32 ResetSubMode = AZ_CRC_CE("com.o3de.action.physx.joints.resetsubmode");
        static const char* ResetTitle = "Reset Current Mode";
        static const char* ResetToolTip = "Reset changes made during this mode edit.";

    } // namespace ActionData

    namespace Internal
    {
        static AzToolsFramework::ViewportUi::ButtonId RegisterClusterButton(
            AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName, const char* tooltip)
        {
            AzToolsFramework::ViewportUi::ButtonId buttonId;
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                buttonId, AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
                AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, clusterId, buttonId, tooltip);

            return buttonId;
        }

        void RefreshUI()
        {
            // The reason this is in a free function is because ColliderComponentMode
            // privately inherits from ToolsApplicationNotificationBus. Trying to invoke
            // the bus inside the class scope causes the compiler to complain it's not accessible
            // to due private inheritance.
            // Using the global namespace operator :: should have fixed that, except there
            // is a bug in the Microsoft compiler meaning it doesn't work. So this is a work around.
            AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
                &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
        }
    } // namespace Internal

    JointsComponentMode::JointsComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        m_modeSelectionClusterIds.assign(static_cast<int>(ClusterGroups::GroupCount), AzToolsFramework::ViewportUi::InvalidClusterId);
        SetupSubModes(entityComponentIdPair);

        EditorJointRequestBus::Event(
            entityComponentIdPair, &EditorJointRequests::SetBoolValue, JointsComponentModeCommon::ParamaterNames::ComponentMode, true);
    }

    JointsComponentMode::~JointsComponentMode()
    {
        EditorJointRequestBus::Event(
            GetEntityComponentIdPair(), &EditorJointRequests::SetBoolValue, JointsComponentModeCommon::ParamaterNames::ComponentMode,
            false);

        TeardownSubModes();
        m_subModes[m_subMode]->Teardown(GetEntityComponentIdPair());
    }

    void JointsComponentMode::Refresh()
    {
        m_subModes[m_subMode]->Refresh(GetEntityComponentIdPair());
    }

    AZStd::vector<AzToolsFramework::ActionOverride> JointsComponentMode::PopulateActionsImpl()
    {
        const AZ::EntityComponentIdPair entityComponentIdPair = GetEntityComponentIdPair();

        AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> subModesState;
        EditorJointRequestBus::EventResult(subModesState, entityComponentIdPair, &EditorJointRequests::GetSubComponentModesState);

        auto makeActionOverride = [](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Crc32& actionUri,
                                     const QString& title, const QString& tip,const AZStd::function<void()>&& callback)
        {
            AzToolsFramework::ActionOverride actionOverride;
            actionOverride.SetTitle(title);
            actionOverride.SetTip(tip);
            actionOverride.SetUri(actionUri);
            actionOverride.SetEntityComponentIdPair(entityComponentIdPair);
            actionOverride.SetCallback(callback);
            return actionOverride;
        };

        AZStd::vector<AzToolsFramework::ActionOverride> actions;

        // translation action
        {
            AzToolsFramework::ActionOverride translateAction = makeActionOverride(
                entityComponentIdPair, SubModeData::SwitchToTranslationSubMode,
                SubModeData::TranslationTitle, SubModeData::TranslationToolTip,
                [this]()
                {
                    SetCurrentMode(
                        JointsComponentModeCommon::SubComponentModes::ModeType::Translation,
                        m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Translation]);
                });
            translateAction.SetKeySequence(QKeySequence(Qt::Key_1));
            actions.emplace_back(translateAction);
        }

        // rotation action
        {
            AzToolsFramework::ActionOverride rotationAction = makeActionOverride(
                entityComponentIdPair, SubModeData::SwitchToRotationSubMode,
                SubModeData::RotationTitle, SubModeData::RotationToolTip,
                [this]()
                {
                    SetCurrentMode(
                        JointsComponentModeCommon::SubComponentModes::ModeType::Rotation,
                        m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Rotation]);
                });
            rotationAction.SetKeySequence(QKeySequence(Qt::Key_2));
            actions.emplace_back(rotationAction);
        }

        //setup action for other enabled options
        for (auto [modeType, parameterString] : subModesState)
        {
            switch (modeType)
            {
            case JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToMaxForceSubMode,
                        SubModeData::MaxForceTitle, SubModeData::MaxForceToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToMaxTorqueSubMode,
                        SubModeData::MaxTorqueTitle, SubModeData::MaxTorqueToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::Damping:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToDampingSubMode,
                        SubModeData::DampingTitle, SubModeData::DampingToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::Damping,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Damping]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToStiffnessSubMode,
                        SubModeData::StiffnessTitle, SubModeData::StiffnessToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToTwistLimitsSubMode,
                        SubModeData::TwistLimitsTitle, SubModeData::TwistLimitsToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToSwingLimitsSubMode,
                        SubModeData::SwingLimitsTitle, SubModeData::SwingLimitsToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToSnapPositionSubMode,
                        SubModeData::SnapPositionTitle, SubModeData::SnapPositionToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition]);
                        }));
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation:
                {
                    actions.emplace_back(makeActionOverride(
                        entityComponentIdPair, SubModeData::SwitchToSnapRotationSubMode,
                        SubModeData::SnapRotationTitle, SubModeData::SnapRotationToolTip,
                        [this]()
                        {
                            SetCurrentMode(
                                JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation,
                                m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation]);
                        }));
                }
                break;
            }
        }

        // reset values
        {
            AzToolsFramework::ActionOverride resetValuesAction = makeActionOverride(
                entityComponentIdPair, SubModeData::ResetSubMode,
                SubModeData::ResetTitle, SubModeData::ResetToolTip,
                [this]()
                {
                    ResetCurrentMode();
                });
            resetValuesAction.SetKeySequence(QKeySequence(Qt::Key_R));
            actions.emplace_back(resetValuesAction);
        }

        return actions;
    }

    AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> JointsComponentMode::PopulateViewportUiImpl()
    {
        AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> ids;
        ids.reserve(m_modeSelectionClusterIds.size());
        for (auto clusterid : m_modeSelectionClusterIds)
        {
            if (clusterid != AzToolsFramework::ViewportUi::InvalidClusterId)
            {
                ids.emplace_back(clusterid);
            }
        }
        return ids;
    }

    void JointsComponentMode::SetCurrentMode(JointsComponentModeCommon::SubComponentModes::ModeType newMode, ButtonData& buttonData)
    {
        
        if (auto subMode = m_subModes.find(newMode);
            subMode != m_subModes.end())
        {
            const AZ::EntityComponentIdPair entityComponentIdPair = GetEntityComponentIdPair();
            m_subModes[m_subMode]->Teardown(entityComponentIdPair);
            m_subMode = newMode;
            subMode->second->Setup(entityComponentIdPair);

            // if this button is on a different cluster. clear the active state.
            if (m_activeButton.m_clusterId != buttonData.m_clusterId)
            {
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                    AzToolsFramework::ViewportUi::DefaultViewportId,
                    &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_activeButton.m_clusterId);
            }
            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                AzToolsFramework::ViewportUi::DefaultViewportId,
                &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, buttonData.m_clusterId,
                buttonData.m_buttonId);
            m_activeButton = buttonData;
        }
        else
        {
            AZ_Assert(false, "PhysXJoints Uninitialized joint component mode selected.");
        }
    }

    bool JointsComponentMode::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        // Propagate mouse interaction to sub-component mode.
        if (m_subModes[m_subMode])
        {
            m_subModes[m_subMode]->HandleMouseInteraction(mouseInteraction);
        }

        return false;
    }

    void JointsComponentMode::SetupSubModes(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        //retrieve the enabled sub components from the entity
        AZStd::vector<JointsComponentModeCommon::SubModeParamaterState> subModesState;
        EditorJointRequestBus::EventResult(subModesState, entityComponentIdPair, &EditorJointRequests::GetSubComponentModesState);

        //group 1 is always available so create it
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_modeSelectionClusterIds[static_cast<int>(ClusterGroups::Group1)], AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster, AzToolsFramework::ViewportUi::Alignment::TopLeft);

        //check if groups 2 and/or 3 need to be created
        for (auto [modeType, _] : subModesState)
        {
            const AzToolsFramework::ViewportUi::ClusterId group2Id = GetClusterId(ClusterGroups::Group2);
            const AzToolsFramework::ViewportUi::ClusterId group3Id = GetClusterId(ClusterGroups::Group3);
            switch (modeType)
            {
            case JointsComponentModeCommon::SubComponentModes::ModeType::Damping:
            case JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness:
            case JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits:
            case JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits:
                {
                    if (group2Id == AzToolsFramework::ViewportUi::InvalidClusterId)
                    {
                        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                            m_modeSelectionClusterIds[static_cast<int>(ClusterGroups::Group2)],
                            AzToolsFramework::ViewportUi::DefaultViewportId,
                            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
                            AzToolsFramework::ViewportUi::Alignment::TopLeft);
                    }
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce:
            case JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque:
                {
                    if (group3Id == AzToolsFramework::ViewportUi::InvalidClusterId)
                    {
                        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
                            m_modeSelectionClusterIds[static_cast<int>(ClusterGroups::Group3)],
                            AzToolsFramework::ViewportUi::DefaultViewportId,
                            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
                            AzToolsFramework::ViewportUi::Alignment::TopLeft);
                    }
                }
                break;
            default:
                AZ_Error("Joints", false, "Joints component mode cluster UI setup found unknown sub mode.");
                break;
            }
            //if both are created - break;
            if (group2Id != AzToolsFramework::ViewportUi::InvalidClusterId && group3Id != AzToolsFramework::ViewportUi::InvalidClusterId)
            {
                break;
            }
        }

        const AzToolsFramework::ViewportUi::ClusterId group1ClusterId = GetClusterId(ClusterGroups::Group1);
        const AzToolsFramework::ViewportUi::ClusterId group2ClusterId = GetClusterId(ClusterGroups::Group2);
        const AzToolsFramework::ViewportUi::ClusterId group3ClusterId = GetClusterId(ClusterGroups::Group3);

        //translation and rotation are enabled for all joints in group 1
        m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::Translation] =
            AZStd::make_unique<JointsSubComponentModeTranslation>();
        m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Translation] =
            ButtonData{ group1ClusterId, Internal::RegisterClusterButton(group1ClusterId, "Move", SubModeData::TranslationToolTip) };
        
        m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::Rotation] = AZStd::make_unique<JointsSubComponentModeRotation>();
        m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Rotation] =
            ButtonData{ group1ClusterId, Internal::RegisterClusterButton(group1ClusterId, "Rotate", SubModeData::RotationTitle) };

        //some constants for some modes
        const float ExponentBreakage = 1.0f;
        const float ExponentSpring = 2.0f;

        //setup the remaining modes if they're in the enabled list.
        for (auto [modeType, parameterString] : subModesState)
        {
            switch (modeType)
            {
            case JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce] =
                        AZStd::make_unique<JointsSubComponentModeLinearFloat>(
                            parameterString, ExponentBreakage, EditorJointConfig::s_breakageMax, EditorJointConfig::s_breakageMin);

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group3ClusterId, "joints/MaxForce", SubModeData::MaxForceToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::MaxForce] =
                        ButtonData{ group3ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque] =
                        AZStd::make_unique<JointsSubComponentModeLinearFloat>(
                            parameterString, ExponentBreakage, EditorJointConfig::s_breakageMax, EditorJointConfig::s_breakageMin);

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group3ClusterId, "joints/MaxTorque", SubModeData::MaxTorqueToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::MaxTorque] =
                        ButtonData{ group3ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::Damping:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::Damping] =
                        AZStd::make_unique<JointsSubComponentModeLinearFloat>(
                            parameterString, ExponentSpring, EditorJointLimitBase::s_springMax, EditorJointLimitBase::s_springMin);

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group2ClusterId, "joints/Damping", SubModeData::DampingToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Damping] = ButtonData{ group2ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness] =
                        AZStd::make_unique<JointsSubComponentModeLinearFloat>(
                            parameterString, ExponentSpring, EditorJointLimitBase::s_springMax, EditorJointLimitBase::s_springMin);

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group2ClusterId, "joints/Stiffness", SubModeData::StiffnessToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Stiffness] =
                        ButtonData{ group2ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits] =
                        AZStd::make_unique<JointsSubComponentModeAnglePair>(
                            parameterString,
                            AZ::Vector3::CreateAxisX(), // PhysX revolute joints uses the x-axis by default
                            EditorJointLimitPairConfig::s_angleMax, EditorJointLimitPairConfig::s_angleMin);

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group2ClusterId, "joints/TwistLimits", SubModeData::TwistLimitsToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::TwistLimits] =
                        ButtonData{ group2ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits] =
                        AZStd::make_unique<JointsSubComponentModeAngleCone>(
                            parameterString, EditorJointLimitPairConfig::s_angleMax, EditorJointLimitPairConfig::s_angleMin);

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group2ClusterId, "joints/SwingLimits", SubModeData::SwingLimitsToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::SwingLimits] =
                        ButtonData{ group2ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition] =
                        AZStd::make_unique<JointsSubComponentModeSnapPosition>();

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group1ClusterId, "joints/SnapPosition", SubModeData::SnapPositionToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::SnapPosition] =
                        ButtonData{ group1ClusterId, buttonId };
                }
                break;
            case JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation:
                {
                    m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation] =
                        AZStd::make_unique<JointsSubComponentModeSnapRotation>();

                    const AzToolsFramework::ViewportUi::ButtonId buttonId =
                        Internal::RegisterClusterButton(group1ClusterId, "joints/SnapRotation", SubModeData::SnapRotationToolTip);
                    m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::SnapRotation] =
                        ButtonData{ group1ClusterId, buttonId };
                }
                break;
            default:
                AZ_Error("Joints", false, "Joints component mode cluster button setup found unknown sub mode.");
                break;
            }
        }

        //register click handler for the buttons
        m_modeSelectionHandlers.push_back(AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                for (auto itr : m_buttonData)
                {
                    if (itr.second.m_clusterId == GetClusterId(ClusterGroups::Group1) && itr.second.m_buttonId == buttonId)
                    {
                        SetCurrentMode(itr.first, itr.second);
                        break;
                    }
                }
            }));
        m_modeSelectionHandlers.push_back(AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                for (auto itr : m_buttonData)
                {
                    if (itr.second.m_clusterId == GetClusterId(ClusterGroups::Group2) && itr.second.m_buttonId == buttonId)
                    {
                        SetCurrentMode(itr.first, itr.second);
                        break;
                    }
                }
            }));
        m_modeSelectionHandlers.push_back(AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                for (auto itr : m_buttonData)
                {
                    if (itr.second.m_clusterId == GetClusterId(ClusterGroups::Group3) && itr.second.m_buttonId == buttonId)
                    {
                        SetCurrentMode(itr.first, itr.second);
                        break;
                    }
                }
            }));

        for (int i = 0; i < static_cast<int>(ClusterGroups::GroupCount); i++)
        {
            if (m_modeSelectionClusterIds[i] != AzToolsFramework::ViewportUi::InvalidClusterId)
            {
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                    AzToolsFramework::ViewportUi::DefaultViewportId,
                    &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
                    m_modeSelectionClusterIds[i], m_modeSelectionHandlers[i]);
            }
        }
        
        // set the translate as enabled by default.
        SetCurrentMode(
            JointsComponentModeCommon::SubComponentModes::ModeType::Translation,
            m_buttonData[JointsComponentModeCommon::SubComponentModes::ModeType::Translation]);

        m_subModes[JointsComponentModeCommon::SubComponentModes::ModeType::Translation]->Setup(GetEntityComponentIdPair());
        m_subMode = JointsComponentModeCommon::SubComponentModes::ModeType::Translation;
    }

    void JointsComponentMode::ResetCurrentMode()
    {
        const AZ::EntityComponentIdPair entityComponentIdPair = GetEntityComponentIdPair();
        m_subModes[m_subMode]->ResetValues(entityComponentIdPair);
        m_subModes[m_subMode]->Refresh(entityComponentIdPair);

        Internal::RefreshUI();
    }

    void JointsComponentMode::TeardownSubModes()
    {
        for (auto clusterid : m_modeSelectionClusterIds)
        {
            if (clusterid != AzToolsFramework::ViewportUi::InvalidClusterId)
            {
                AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                    AzToolsFramework::ViewportUi::DefaultViewportId,
                    &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, clusterid);
            }
        }
        m_modeSelectionClusterIds.assign(static_cast<int>(ClusterGroups::GroupCount), AzToolsFramework::ViewportUi::InvalidClusterId);
    }

    AzToolsFramework::ViewportUi::ClusterId JointsComponentMode::GetClusterId(ClusterGroups group)
    {
        return m_modeSelectionClusterIds[static_cast<int>(group)];
    }
} // namespace PhysX
