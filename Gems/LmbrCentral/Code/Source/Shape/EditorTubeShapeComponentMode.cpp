/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorTubeShapeComponentMode.h"

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Manipulators/LinearManipulator.h>
#include <AzToolsFramework/Manipulators/ManipulatorManager.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <AzToolsFramework/Viewport/ActionBus.h>
#include <LmbrCentral/Shape/SplineComponentBus.h>
#include <LmbrCentral/Shape/EditorTubeShapeComponentBus.h>
#include <LmbrCentral/Shape/TubeShapeComponentBus.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/ViewportConstants.h>

namespace LmbrCentral
{
    AZ_CLASS_ALLOCATOR_IMPL(EditorTubeShapeComponentMode, AZ::SystemAllocator)

    static const AZ::Crc32 s_resetVariableRadii = AZ_CRC_CE("org.o3de.action.tubeshape.reset_radii");
    static const char* const s_resetRadiiTitle = "Reset Radii";
    static const char* const s_resetRadiiDesc = "Reset all variable radius values to the default";

    EditorTubeShapeComponentMode::EditorTubeShapeComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
    {
        m_currentTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            m_currentTransform, entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZ::TransformNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        ShapeComponentNotificationsBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        SplineComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());
        EditorSplineComponentNotificationBus::Handler::BusConnect(entityComponentIdPair.GetEntityId());

        CreateManipulators();

        EditorTubeShapeComponentModeRequestBus::Handler::BusConnect(entityComponentIdPair);
    }

    EditorTubeShapeComponentMode::~EditorTubeShapeComponentMode()
    {
        EditorTubeShapeComponentModeRequestBus::Handler::BusDisconnect();

        DestroyManipulators();

        EditorSplineComponentNotificationBus::Handler::BusDisconnect();
        SplineComponentNotificationBus::Handler::BusDisconnect();
        ShapeComponentNotificationsBus::Handler::BusDisconnect();
        AZ::TransformNotificationBus::Handler::BusDisconnect();
    }

    void EditorTubeShapeComponentMode::Reflect(AZ::ReflectContext* context)
    {
        AzToolsFramework::ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<EditorTubeShapeComponentMode>(context);
    }

    void EditorTubeShapeComponentMode::RegisterActions()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "EditorTubeShapeComponentMode - could not get ActionManagerInterface on RegisterActions.");

        auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(hotKeyManagerInterface, "EditorTubeShapeComponentMode - could not get HotKeyManagerInterface on RegisterActions.");

        // Reset Radii
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.tubeShape.resetRadii";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = s_resetRadiiTitle;
            actionProperties.m_description = s_resetRadiiDesc;
            actionProperties.m_category = "Tube Shape";

            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                    auto componentModeCollectionInterface = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
                    AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");

                    componentModeCollectionInterface->EnumerateActiveComponents(
                        [](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                        {
                            EditorTubeShapeComponentModeRequestBus::Event(
                                entityComponentIdPair, &EditorTubeShapeComponentModeRequests::ResetRadii);
                        }
                    );
                }
            );

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "R");
        }
    }

    void EditorTubeShapeComponentMode::BindActionsToModes()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "EditorTubeShapeComponentMode - could not get ActionManagerInterface on BindActionsToModes.");

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);

        AZStd::string modeIdentifier = AZStd::string::format(
            "o3de.context.mode.%s", serializeContext->FindClassData(azrtti_typeid<EditorTubeShapeComponentMode>())->m_name);

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.tubeShape.resetRadii");
    }

    void EditorTubeShapeComponentMode::BindActionsToMenus()
    {
        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(menuManagerInterface, "EditorTubeShapeComponentMode - could not get MenuManagerInterface on BindActionsToMenus.");

        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.tubeShape.resetRadii", 6000);
    }

    void EditorTubeShapeComponentMode::Refresh()
    {
        ContainerChanged();
    }

    AZStd::string EditorTubeShapeComponentMode::GetComponentModeName() const
    {
        return "Tube Shape Edit Mode";
    }

    AZ::Uuid EditorTubeShapeComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<EditorTubeShapeComponentMode>();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> EditorTubeShapeComponentMode::PopulateActionsImpl()
    {
        return AZStd::vector<AzToolsFramework::ActionOverride>
        {
            AzToolsFramework::ActionOverride()
                .SetUri(s_resetVariableRadii)
                .SetKeySequence(QKeySequence(Qt::Key_R))
                .SetTitle(s_resetRadiiTitle)
                .SetTip(s_resetRadiiDesc)
                .SetEntityComponentIdPair(GetEntityComponentIdPair())
                .SetCallback([this]()
                {
                    ResetRadii();
                })
        };
    }

    void EditorTubeShapeComponentMode::CreateManipulators()
    {
        const AZ::EntityId entityId = GetEntityId();

        bool empty = false;
        SplineComponentRequestBus::EventResult(
            empty, entityId, &SplineComponentRequests::Empty);

        // if we have no vertices, do not attempt to create any manipulators
        if (empty)
        {
            return;
        }

        AZ::SplinePtr spline;
        SplineComponentRequestBus::EventResult(
            spline, entityId, &SplineComponentRequests::GetSpline);

        const auto tubeManipulatorStates = GenerateTubeManipulatorStates(*spline);

        for (size_t manipulatorIndex = 0; manipulatorIndex < tubeManipulatorStates.size(); ++manipulatorIndex)
        {
            const TubeManipulatorState tubeManipulatorState = tubeManipulatorStates[manipulatorIndex];

            const AZ::Vector3 normal = spline->GetNormal(tubeManipulatorState.m_splineAddress);
            const AZ::Vector3 position = spline->GetPosition(tubeManipulatorState.m_splineAddress);

            float radius = 0.0f;
            TubeShapeComponentRequestsBus::EventResult(
                radius, entityId, &TubeShapeComponentRequests::GetTotalRadius, tubeManipulatorState.m_splineAddress);

            auto linearManipulator = AzToolsFramework::LinearManipulator::MakeShared(m_currentTransform);
            linearManipulator->AddEntityComponentIdPair(GetEntityComponentIdPair());
            linearManipulator->SetLocalTransform(AZ::Transform::CreateTranslation(position + normal * radius));
            linearManipulator->SetAxis(normal);

            AzToolsFramework::ManipulatorViews views;
            views.emplace_back(AzToolsFramework::CreateManipulatorViewQuadBillboard(
                AzFramework::ViewportColors::DefaultManipulatorHandleColor, AzFramework::ViewportConstants::DefaultManipulatorHandleSize));
            linearManipulator->SetViews(AZStd::move(views));
            linearManipulator->Register(AzToolsFramework::g_mainManipulatorManagerId);

            struct SharedState
            {
                float m_startingVariableRadius = 0.0f;
                float m_startingFixedRadius = 0.0f;
            };

            auto sharedState = AZStd::make_shared<SharedState>();
            linearManipulator->InstallLeftMouseDownCallback(
                [this, tubeManipulatorState, sharedState]
                (const AzToolsFramework::LinearManipulator::Action& /*action*/) mutable
            {
                const AZ::EntityId entityId = GetEntityId();

                float variableRadius = 0.0f;
                TubeShapeComponentRequestsBus::EventResult(
                    variableRadius, entityId,
                    &TubeShapeComponentRequests::GetVariableRadius, static_cast<int>(tubeManipulatorState.m_vertIndex));

                // the base radius of the tube (when no variable radii are applied)
                float fixedRadius = 0.0f;
                TubeShapeComponentRequestsBus::EventResult(
                    fixedRadius, entityId, &TubeShapeComponentRequests::GetRadius);

                sharedState->m_startingVariableRadius = variableRadius;
                sharedState->m_startingFixedRadius = fixedRadius;
            });

            linearManipulator->InstallMouseMoveCallback(
                [this, tubeManipulatorState, manipulatorIndex, spline, sharedState]
                (const AzToolsFramework::LinearManipulator::Action& action)
            {
                const AZ::EntityId entityId = GetEntityId();
                const float axisDisplacement = action.LocalPositionOffset().Dot(action.m_fixed.m_axis);

                // set clamped variable radius, it can be no more than the inverse
                // of the base/fixed radius of the tube
                TubeShapeComponentRequestsBus::Event(
                    entityId, &TubeShapeComponentRequests::SetVariableRadius, static_cast<int>(tubeManipulatorState.m_vertIndex),
                    AZ::GetMax(sharedState->m_startingVariableRadius + axisDisplacement, -sharedState->m_startingFixedRadius));

                bool found = false;
                AZ::Vector3 localVertexPosition = AZ::Vector3::CreateZero();
                SplineComponentRequestBus::EventResult(
                    found, entityId, &SplineComponentRequests::GetVertex, tubeManipulatorState.m_vertIndex, localVertexPosition);

                const AZ::Vector3 localNormal = spline->GetNormal(tubeManipulatorState.m_splineAddress);
                const AZ::Vector3 manipulatorVector = action.LocalPosition() - localVertexPosition;
                const float manipulatorDot = manipulatorVector.Dot(localNormal);

                // ensure the manipulator position cannot move passed
                // the center point of a tube vertex
                const AZ::Vector3 localPosition = manipulatorDot >= 0.0f
                    ? action.LocalPosition()
                    : localVertexPosition;

                m_radiusManipulators[manipulatorIndex]->SetLocalTransform(
                    AZ::Transform::CreateTranslation(localPosition));
                m_radiusManipulators[manipulatorIndex]->SetBoundsDirty();

                EditorTubeShapeComponentRequestBus::Event(
                    entityId, &EditorTubeShapeComponentRequests::GenerateVertices);
            });

            m_radiusManipulators.emplace_back(std::move(linearManipulator));
        }
    }

    void EditorTubeShapeComponentMode::DestroyManipulators()
    {
        for (auto& linearManipulator : m_radiusManipulators)
        {
            if (linearManipulator)
            {
                linearManipulator->Unregister();
                linearManipulator.reset();
            }
        }

        m_radiusManipulators.clear();
    }

    void EditorTubeShapeComponentMode::ContainerChanged()
    {
        DestroyManipulators();
        CreateManipulators();
    }

    void EditorTubeShapeComponentMode::OnVertexAdded(size_t /*index*/)
    {
        ContainerChanged();
    }

    void EditorTubeShapeComponentMode::OnVertexRemoved(size_t /*index*/)
    {
        ContainerChanged();
    }

    void EditorTubeShapeComponentMode::OnVerticesSet(const AZStd::vector<AZ::Vector3>& /*vertices*/)
    {
        ContainerChanged();
    }

    void EditorTubeShapeComponentMode::OnVerticesCleared()
    {
        ContainerChanged();
    }

    void EditorTubeShapeComponentMode::OnShapeChanged(const ShapeChangeReasons /*changeReason*/)
    {
        RefreshManipulatorsLocal(GetEntityId());
    }

    void EditorTubeShapeComponentMode::OnSplineChanged()
    {
        RefreshManipulatorsLocal(GetEntityId());
    }

    void EditorTubeShapeComponentMode::OnTransformChanged(
        const AZ::Transform& /*local*/, const AZ::Transform& world)
    {
        m_currentTransform = world;
    }

    void EditorTubeShapeComponentMode::OnSplineTypeChanged()
    {
        ContainerChanged();
    }

    void EditorTubeShapeComponentMode::ResetRadii()
    {
        const AZ::EntityId entityId = GetEntityId();

        // ensure we record undo command for reset
        AzToolsFramework::ScopedUndoBatch undoBatch("Reset variable radii");
        AzToolsFramework::ScopedUndoBatch::MarkEntityDirty(entityId);

        TubeShapeComponentRequestsBus::Event(entityId, &TubeShapeComponentRequests::SetAllVariableRadii, 0.0f);

        RefreshManipulatorsLocal(entityId);

        EditorTubeShapeComponentRequestBus::Event(entityId, &EditorTubeShapeComponentRequests::GenerateVertices);

        AzToolsFramework::OnEntityComponentPropertyChanged(GetEntityComponentIdPair());

        // ensure property grid values are refreshed
        AzToolsFramework::ToolsApplicationNotificationBus::Broadcast(
            &AzToolsFramework::ToolsApplicationNotificationBus::Events::InvalidatePropertyDisplayForComponent, 
            GetEntityComponentIdPair(),
            AzToolsFramework::Refresh_Values);
    }

    void EditorTubeShapeComponentMode::OnOpenCloseChanged(const bool /*closed*/)
    {
        ContainerChanged();
    }

    void EditorTubeShapeComponentMode::RefreshManipulatorsLocal(const AZ::EntityId entityId)
    {
        AZ::SplinePtr spline;
        SplineComponentRequestBus::EventResult(
            spline, entityId, &SplineComponentRequests::GetSpline);

        const auto tubeManipulatorStates = GenerateTubeManipulatorStates(*spline);
        AZ_Assert(tubeManipulatorStates.size() == m_radiusManipulators.size(),
            "tubeManipulatorStates.size() (%zu) does not equal m_radiusManipulators.size() (%zu), it's likely CreateManipulators has not been called",
            tubeManipulatorStates.size(), m_radiusManipulators.size());

        for (size_t manipulatorIndex = 0; manipulatorIndex < tubeManipulatorStates.size(); ++manipulatorIndex)
        {
            const TubeManipulatorState& tubeManipulatorState = tubeManipulatorStates[manipulatorIndex];

            const AZ::Vector3 normal = spline->GetNormal(tubeManipulatorState.m_splineAddress);
            const AZ::Vector3 position = spline->GetPosition(tubeManipulatorState.m_splineAddress);

            float radius = 0.0f;
            TubeShapeComponentRequestsBus::EventResult(
                radius, entityId, &TubeShapeComponentRequests::GetTotalRadius, tubeManipulatorState.m_splineAddress);

            m_radiusManipulators[manipulatorIndex]->SetLocalTransform(
                AZ::Transform::CreateTranslation(position + normal * radius));
            m_radiusManipulators[manipulatorIndex]->SetAxis(normal);
            m_radiusManipulators[manipulatorIndex]->SetBoundsDirty();
        }
    }

    AZStd::vector<EditorTubeShapeComponentMode::TubeManipulatorState> GenerateTubeManipulatorStates(const AZ::Spline& spline)
    {
        if (spline.GetVertexCount() == 0)
        {
            return {};
        }

        const auto segmentCount = spline.GetSegmentCount();
        if (segmentCount == 0)
        {
            return { { AZ::SplineAddress(0), 0 } };
        }

        const AZ::u64 startVertex = spline.GetAddressByFraction(0.0f).m_segmentIndex;
        const AZ::u64 endVertex = startVertex + segmentCount + (spline.IsClosed() ? 0 : 1);

        AZStd::vector<EditorTubeShapeComponentMode::TubeManipulatorState> splineAddresses;
        for (AZ::u64 vertIndex = startVertex; vertIndex < endVertex; ++vertIndex)
        {
            if (vertIndex + 1 == endVertex)
            {
                AZ_Assert(vertIndex > 0, "vertexIndex is 0 and not safe to subtract from")
                    splineAddresses.push_back({ AZ::SplineAddress(vertIndex - 1, 1.0f), vertIndex });
            }
            else
            {
                splineAddresses.push_back({ AZ::SplineAddress(vertIndex), vertIndex });
            }
        }

        return splineAddresses;
    }
} // namespace LmbrCentral
