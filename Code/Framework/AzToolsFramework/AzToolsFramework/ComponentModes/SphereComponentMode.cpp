/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/SphereComponentMode.h>

#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>
#include <AzToolsFramework/Manipulators/RadiusManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    void InstallSphereViewportEditFunctions(SphereViewportEdit* sphereViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        auto getSphereRadius = [entityComponentIdPair]()
        {
            float radius = 0.5f;
            RadiusManipulatorRequestBus::EventResult(radius, entityComponentIdPair, &RadiusManipulatorRequestBus::Events::GetRadius);
            return radius;
        };
        auto setSphereRadius = [entityComponentIdPair](float radius)
        {
            RadiusManipulatorRequestBus::Event(entityComponentIdPair, &RadiusManipulatorRequestBus::Events::SetRadius, radius);
        };
        sphereViewportEdit->InstallGetSphereRadius(AZStd::move(getSphereRadius));
        sphereViewportEdit->InstallSetSphereRadius(AZStd::move(setSphereRadius));
    }

    AZ_CLASS_ALLOCATOR_IMPL(SphereComponentMode, AZ::SystemAllocator)

        void SphereComponentMode::Reflect(AZ::ReflectContext* context)
    {
        ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<SphereComponentMode>(context);
    }

    SphereComponentMode::SphereComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : BaseShapeComponentMode(entityComponentIdPair, componentType, allowAsymmetricalEditing)
    {
        auto sphereViewportEdit = AZStd::make_unique<SphereViewportEdit>();
        InstallBaseShapeViewportEditFunctions(sphereViewportEdit.get(), m_entityComponentIdPair);
        InstallSphereViewportEditFunctions(sphereViewportEdit.get(), m_entityComponentIdPair);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)] = AZStd::move(sphereViewportEdit);

        if (m_allowAsymmetricalEditing)
        {
            auto shapeTranslationOffsetViewportEdit = AZStd::make_unique<ShapeTranslationOffsetViewportEdit>();
            InstallBaseShapeViewportEditFunctions(shapeTranslationOffsetViewportEdit.get(), m_entityComponentIdPair);
            m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::TranslationOffset)] =
                AZStd::move(shapeTranslationOffsetViewportEdit);
            SetupCluster();
            SetShapeSubMode(ShapeComponentModeRequests::SubMode::Dimensions);
        }
        else
        {
            m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->Setup(g_mainManipulatorManagerId);
            m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->AddEntityComponentIdPair(
                m_entityComponentIdPair);
        }
        ShapeComponentModeRequestBus::Handler::BusConnect(m_entityComponentIdPair);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
    }

    SphereComponentMode::~SphereComponentMode()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        ShapeComponentModeRequestBus::Handler::BusDisconnect();
    }

    void SphereComponentMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->OnCameraStateChanged(cameraState);
    }

    AZStd::string SphereComponentMode::GetComponentModeName() const
    {
        return "Sphere Edit Mode";
    }

    AZ::Uuid SphereComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<SphereComponentMode>();
    }

    void SphereComponentMode::RegisterActions()
    {
        BaseShapeComponentMode::RegisterActions("sphere");
    }

    void SphereComponentMode::BindActionsToModes()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        BaseShapeComponentMode::BindActionsToModes(
            "sphere", serializeContext->FindClassData(azrtti_typeid<SphereComponentMode>())->m_name);
    }

    void SphereComponentMode::BindActionsToMenus()
    {
        BaseShapeComponentMode::BindActionsToMenus("sphere");
    }
} // namespace AzToolsFramework
