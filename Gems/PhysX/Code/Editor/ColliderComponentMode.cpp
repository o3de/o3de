/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ColliderComponentMode.h"
#include "ColliderAssetScaleMode.h"
#include "ColliderBoxMode.h"
#include "ColliderCapsuleMode.h"
#include "ColliderOffsetMode.h"
#include "ColliderRotationMode.h"
#include "ColliderSphereMode.h"

#include <Editor/Source/ComponentModes/PhysXSubComponentModeBase.h>
#include <PhysX/EditorColliderComponentRequestBus.h>

#include <AzFramework/Physics/ShapeConfiguration.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>
#include <AzToolsFramework/ComponentModes/BoxViewportEdit.h>

namespace PhysX
{
    namespace
    {
        //! Uri's for shortcut actions.
        const AZ::Crc32 SetDimensionsSubModeActionUri = AZ_CRC("com.o3de.action.physx.setdimensionssubmode", 0x77b70dd6);
        const AZ::Crc32 SetOffsetSubModeActionUri = AZ_CRC("com.o3de.action.physx.setoffsetsubmode", 0xc06132e5);
        const AZ::Crc32 SetRotationSubModeActionUri = AZ_CRC("com.o3de.action.physx.setrotationsubmode", 0xc4225918);
        const AZ::Crc32 ResetSubModeActionUri = AZ_CRC("com.o3de.action.physx.resetsubmode", 0xb70b120e);
    } // namespace

    AZ_CLASS_ALLOCATOR_IMPL(ColliderComponentMode, AZ::SystemAllocator, 0);

    ColliderComponentMode::ColliderComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, AZ::Uuid componentType)
        : AzToolsFramework::ComponentModeFramework::EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        CreateSubModes();
        CreateSubModeSelectionCluster();
        ColliderComponentModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        ColliderComponentModeUiRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    ColliderComponentMode::~ColliderComponentMode()
    {
        ColliderComponentModeUiRequestBus::Handler::BusDisconnect();
        ColliderComponentModeRequestBus::Handler::BusDisconnect();

        RemoveSubModeSelectionCluster();
        m_subModes[m_subMode]->Teardown(GetEntityComponentIdPair());
    }

    void ColliderComponentMode::Refresh()
    {
        m_subModes[m_subMode]->Refresh(GetEntityComponentIdPair());
    }

    AZStd::vector<AzToolsFramework::ActionOverride> ColliderComponentMode::PopulateActionsImpl()
    {
        AzToolsFramework::ActionOverride setOffsetModeAction;
        setOffsetModeAction.SetUri(SetOffsetSubModeActionUri);
        setOffsetModeAction.SetKeySequence(QKeySequence(Qt::Key_1));
        setOffsetModeAction.SetTitle("Set Offset Mode");
        setOffsetModeAction.SetTip("Set offset mode");
        setOffsetModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setOffsetModeAction.SetCallback(
            [this]()
            {
                SetCurrentMode(SubMode::Offset);
            });

        AzToolsFramework::ActionOverride setRotationModeAction;
        setRotationModeAction.SetUri(SetRotationSubModeActionUri);
        setRotationModeAction.SetKeySequence(QKeySequence(Qt::Key_2));
        setRotationModeAction.SetTitle("Set Rotation Mode");
        setRotationModeAction.SetTip("Set rotation mode");
        setRotationModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setRotationModeAction.SetCallback(
            [this]()
            {
                SetCurrentMode(SubMode::Rotation);
            });

        AzToolsFramework::ActionOverride setDimensionsModeAction;
        setDimensionsModeAction.SetUri(SetDimensionsSubModeActionUri);
        setDimensionsModeAction.SetKeySequence(QKeySequence(Qt::Key_3));
        setDimensionsModeAction.SetTitle("Set Resize Mode");
        setDimensionsModeAction.SetTip("Set resize mode");
        setDimensionsModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setDimensionsModeAction.SetCallback(
            [this]()
            {
                SetCurrentMode(SubMode::Dimensions);
            });

        AzToolsFramework::ActionOverride resetModeAction;
        resetModeAction.SetUri(ResetSubModeActionUri);
        resetModeAction.SetKeySequence(QKeySequence(Qt::Key_R));
        resetModeAction.SetTitle("Reset Current Mode");
        resetModeAction.SetTip("Reset current mode");
        resetModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        resetModeAction.SetCallback(
            [this]()
            {
                ResetCurrentMode();
            });

        return { setDimensionsModeAction, setOffsetModeAction, setRotationModeAction, resetModeAction };
    }

    void ColliderComponentMode::CreateSubModes()
    {
        Physics::ShapeType shapeType = Physics::ShapeType::Box;
        EditorColliderComponentRequestBus::EventResult(
            shapeType, GetEntityComponentIdPair(), &EditorColliderComponentRequests::GetShapeType);

        switch (shapeType)
        {
        case Physics::ShapeType::Box:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderBoxMode>();
            break;
        case Physics::ShapeType::Sphere:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderSphereMode>();
            break;
        case Physics::ShapeType::Capsule:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderCapsuleMode>();
            break;
        case Physics::ShapeType::PhysicsAsset:
            m_subModes[SubMode::Dimensions] = AZStd::make_unique<ColliderAssetScaleMode>();
            break;
        }
        m_subModes[SubMode::Offset] = AZStd::make_unique<ColliderOffsetMode>();
        m_subModes[SubMode::Rotation] = AZStd::make_unique<ColliderRotationMode>();
        m_subModes[m_subMode]->Setup(GetEntityComponentIdPair());
    }

    bool ColliderComponentMode::HandleMouseInteraction(const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Wheel &&
            mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
        {
            const int direction = MouseWheelDelta(mouseInteraction) > 0.0f ? -1 : 1;
            AZ::u32 currentModeIndex = static_cast<AZ::u32>(m_subMode);
            AZ::u32 numSubModes = static_cast<AZ::u32>(SubMode::NumModes);
            AZ::u32 nextModeIndex = (currentModeIndex + numSubModes + direction) % m_subModes.size();
            SubMode nextMode = static_cast<SubMode>(nextModeIndex);
            SetCurrentMode(nextMode);
            return true;
        }
        return false;
    }

    ColliderComponentMode::SubMode ColliderComponentMode::GetCurrentMode()
    {
        return m_subMode;
    }

    void ColliderComponentMode::SetCurrentMode(SubMode newMode)
    {
        AZ_Assert(m_subModes.find(newMode) != m_subModes.end(), "Submode not found:%d", newMode);
        m_subModes[m_subMode]->Teardown(GetEntityComponentIdPair());
        m_subMode = newMode;
        m_subModes[m_subMode]->Setup(GetEntityComponentIdPair());

        const auto modeIndex = static_cast<size_t>(newMode);
        AZ_Assert(modeIndex < m_buttonIds.size(), "Invalid mode index %i.", modeIndex);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton, m_modeSelectionClusterId,
            m_buttonIds[modeIndex]);
    }

    AzToolsFramework::ViewportUi::ClusterId ColliderComponentMode::GetClusterId() const
    {
        return m_modeSelectionClusterId;
    }

    AzToolsFramework::ViewportUi::ButtonId ColliderComponentMode::GetOffsetButtonId() const
    {
        return m_buttonIds[static_cast<size_t>(SubMode::Offset)];
    }

    AzToolsFramework::ViewportUi::ButtonId ColliderComponentMode::GetRotationButtonId() const
    {
        return m_buttonIds[static_cast<size_t>(SubMode::Rotation)];
    }

    AzToolsFramework::ViewportUi::ButtonId ColliderComponentMode::GetDimensionsButtonId() const
    {
        return m_buttonIds[static_cast<size_t>(SubMode::Dimensions)];
    }

    AZStd::string ColliderComponentMode::GetComponentModeName() const
    {
        return "Collider Edit Mode";
    }

    void RefreshUI()
    {
        /// The reason this is in a free function is because ColliderComponentMode
        /// privately inherits from ToolsApplicationNotificationBus. Trying to invoke
        /// the bus inside the class scope causes the compiler to complain it's not accessible
        /// to due private inheritence.
        /// Using the global namespace operator :: should have fixed that, except there
        /// is a bug in the microsoft compiler meaning it doesn't work. So this is a work around.
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplay, AzToolsFramework::Refresh_Values);
    }

    void ColliderComponentMode::ResetCurrentMode()
    {
        m_subModes[m_subMode]->ResetValues(GetEntityComponentIdPair());
        m_subModes[m_subMode]->Refresh(GetEntityComponentIdPair());
        RefreshUI();
    }

    AZStd::vector<AzToolsFramework::ViewportUi::ClusterId> ColliderComponentMode::PopulateViewportUiImpl()
    {
        return AZStd::vector<AzToolsFramework::ViewportUi::ClusterId>{ m_modeSelectionClusterId };
    }

    static AzToolsFramework::ViewportUi::ButtonId RegisterClusterButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, const char* iconName)
    {
        AzToolsFramework::ViewportUi::ButtonId buttonId;
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId, AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton, clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        return buttonId;
    }

    void ColliderComponentMode::RemoveSubModeSelectionCluster()
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId, &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_modeSelectionClusterId);
    }

    void ColliderComponentMode::CreateSubModeSelectionCluster()
    {
        // create the cluster for changing transform mode
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::EventResult(
            m_modeSelectionClusterId, AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::CreateCluster, AzToolsFramework::ViewportUi::Alignment::TopLeft);

        // create and register the buttons
        m_buttonIds.resize(static_cast<size_t>(SubMode::NumModes));
        m_buttonIds[static_cast<size_t>(SubMode::Offset)] = RegisterClusterButton(m_modeSelectionClusterId, "Move");
        m_buttonIds[static_cast<size_t>(SubMode::Rotation)] = RegisterClusterButton(m_modeSelectionClusterId, "Rotate");
        m_buttonIds[static_cast<size_t>(SubMode::Dimensions)] = RegisterClusterButton(m_modeSelectionClusterId, "Scale");

        SetCurrentMode(SubMode::Offset);

        const auto onButtonClicked = [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::Offset)])
            {
                SetCurrentMode(SubMode::Offset);
            }
            else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::Rotation)])
            {
                SetCurrentMode(SubMode::Rotation);
            }
            else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::Dimensions)])
            {
                SetCurrentMode(SubMode::Dimensions);
            }
            else
            {
                AZ_Error("PhysX Collider Component Mode", false, "Unrecognized button ID.");
            }
        };

        m_modeSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(onButtonClicked);
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler, m_modeSelectionClusterId,
            m_modeSelectionHandler);
    }
} // namespace PhysX
