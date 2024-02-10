/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "ComponentModes/CylinderViewportEdit.h"
#include <AzToolsFramework/ComponentModes/CylinderComponentMode.h>

#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>
#include <AzToolsFramework/Manipulators/RadiusManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/CylinderManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>


namespace AzToolsFramework
{


    void InstallCylinderViewportEditFunctions(
        CylinderViewportEdit* cylinderViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair) {
        auto getCylinderHeight = [entityComponentIdPair]()
        {
            float height = 1.0f;
            CylinderManipulatorRequestBus::EventResult(height, entityComponentIdPair, &CylinderManipulatorRequestBus::Events::GetHeight);
            return height;
        };
        auto getCylinderRadius = [entityComponentIdPair]()
        {
            float radius = 0.25f;
            RadiusManipulatorRequestBus::EventResult(radius, entityComponentIdPair, &RadiusManipulatorRequestBus::Events::GetRadius);
            return radius;
        };
        auto setCylinderHeight = [entityComponentIdPair](float height)
        {
            CylinderManipulatorRequestBus::Event(entityComponentIdPair, &CylinderManipulatorRequestBus::Events::SetHeight, height);
        };
        auto setCylinderRadius = [entityComponentIdPair](float radius)
        {
            RadiusManipulatorRequestBus::Event(entityComponentIdPair, &RadiusManipulatorRequestBus::Events::SetRadius, radius);
        };
        cylinderViewportEdit->InstallGetCylinderHeight(AZStd::move(getCylinderHeight));
        cylinderViewportEdit->InstallGetCylinderRadius(AZStd::move(getCylinderRadius));
        cylinderViewportEdit->InstallSetCylinderHeight(AZStd::move(setCylinderHeight));
        cylinderViewportEdit->InstallSetCylinderRadius(AZStd::move(setCylinderRadius));
    }

    AZ_CLASS_ALLOCATOR_IMPL(CylinderComponentMode, AZ::SystemAllocator)

    void CylinderComponentMode::Reflect(AZ::ReflectContext* context)
    {
        ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<CylinderComponentMode>(context);
    }

    CylinderComponentMode::CylinderComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : BaseShapeComponentMode(entityComponentIdPair, componentType, allowAsymmetricalEditing)
    {
        auto cylinderViewportEdit = AZStd::make_unique<CylinderViewportEdit>(m_allowAsymmetricalEditing);
        InstallBaseShapeViewportEditFunctions(cylinderViewportEdit.get(), m_entityComponentIdPair);
        InstallCylinderViewportEditFunctions(cylinderViewportEdit.get(), m_entityComponentIdPair);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)] = AZStd::move(cylinderViewportEdit);

        if (m_allowAsymmetricalEditing)
        {
            auto shapeTranslationOffsetViewportEdit = AZStd::make_unique<ShapeTranslationOffsetViewportEdit>();
            InstallBaseShapeViewportEditFunctions(shapeTranslationOffsetViewportEdit.get(), m_entityComponentIdPair);
            m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::TranslationOffset)] = AZStd::move(shapeTranslationOffsetViewportEdit);
            SetupCluster();
            SetShapeSubMode(ShapeComponentModeRequests::SubMode::Dimensions);
        }
        else
        {
            m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->Setup(g_mainManipulatorManagerId);
            m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->AddEntityComponentIdPair(m_entityComponentIdPair);
        }
        ShapeComponentModeRequestBus::Handler::BusConnect(m_entityComponentIdPair);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
    }

    CylinderComponentMode::~CylinderComponentMode()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        ShapeComponentModeRequestBus::Handler::BusDisconnect();
    }

    void CylinderComponentMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->OnCameraStateChanged(cameraState);
    }

    AZStd::string CylinderComponentMode::GetComponentModeName() const
    {
        return "Cylinder Edit Mode";
    }

    AZ::Uuid CylinderComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<CylinderComponentMode>();
    }

    void CylinderComponentMode::RegisterActions()
    {
        BaseShapeComponentMode::RegisterActions("cylinder");
    }

    void CylinderComponentMode::BindActionsToModes()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        BaseShapeComponentMode::BindActionsToModes("cylinder", serializeContext->FindClassData(azrtti_typeid<CylinderComponentMode>())->m_name);
    }

    void CylinderComponentMode::BindActionsToMenus()
    {
        BaseShapeComponentMode::BindActionsToMenus("cylinder");
    }
}; // namespace AzToolsFramework
