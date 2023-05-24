/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/CapsuleComponentMode.h>

#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>
#include <AzToolsFramework/Manipulators/CapsuleManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/RadiusManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{
    void InstallCapsuleViewportEditFunctions(CapsuleViewportEdit* capsuleViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        auto getCapsuleHeight = [entityComponentIdPair]()
        {
            float height = 1.0f;
            CapsuleManipulatorRequestBus::EventResult(height, entityComponentIdPair, &CapsuleManipulatorRequestBus::Events::GetHeight);
            return height;
        };
        auto getCapsuleRadius = [entityComponentIdPair]()
        {
            float radius = 0.25f;
            RadiusManipulatorRequestBus::EventResult(radius, entityComponentIdPair, &RadiusManipulatorRequestBus::Events::GetRadius);
            return radius;
        };
        auto setCapsuleHeight = [entityComponentIdPair](float height)
        {
            CapsuleManipulatorRequestBus::Event(entityComponentIdPair, &CapsuleManipulatorRequestBus::Events::SetHeight, height);
        };
        auto setCapsuleRadius = [entityComponentIdPair](float radius)
        {
            RadiusManipulatorRequestBus::Event(entityComponentIdPair, &RadiusManipulatorRequestBus::Events::SetRadius, radius);
        };
        capsuleViewportEdit->InstallGetCapsuleHeight(AZStd::move(getCapsuleHeight));
        capsuleViewportEdit->InstallGetCapsuleRadius(AZStd::move(getCapsuleRadius));
        capsuleViewportEdit->InstallSetCapsuleHeight(AZStd::move(setCapsuleHeight));
        capsuleViewportEdit->InstallSetCapsuleRadius(AZStd::move(setCapsuleRadius));
    }

    AZ_CLASS_ALLOCATOR_IMPL(CapsuleComponentMode, AZ::SystemAllocator)

    void CapsuleComponentMode::Reflect(AZ::ReflectContext* context)
    {
        ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<CapsuleComponentMode>(context);
    }

    CapsuleComponentMode::CapsuleComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : BaseShapeComponentMode(entityComponentIdPair, componentType, allowAsymmetricalEditing)
    {
        auto capsuleViewportEdit = AZStd::make_unique<CapsuleViewportEdit>(m_allowAsymmetricalEditing);
        InstallBaseShapeViewportEditFunctions(capsuleViewportEdit.get(), m_entityComponentIdPair);
        InstallCapsuleViewportEditFunctions(capsuleViewportEdit.get(), m_entityComponentIdPair);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)] = AZStd::move(capsuleViewportEdit);

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

    CapsuleComponentMode::~CapsuleComponentMode()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        ShapeComponentModeRequestBus::Handler::BusDisconnect();
    }

    void CapsuleComponentMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->OnCameraStateChanged(cameraState);
    }

    AZStd::string CapsuleComponentMode::GetComponentModeName() const
    {
        return "Capsule Edit Mode";
    }

    AZ::Uuid CapsuleComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<CapsuleComponentMode>();
    }

    void CapsuleComponentMode::RegisterActions()
    {
        BaseShapeComponentMode::RegisterActions("capsule");
    }

    void CapsuleComponentMode::BindActionsToModes()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        BaseShapeComponentMode::BindActionsToModes(
            "capsule", serializeContext->FindClassData(azrtti_typeid<CapsuleComponentMode>())->m_name);
    }

    void CapsuleComponentMode::BindActionsToMenus()
    {
        BaseShapeComponentMode::BindActionsToMenus("capsule");
    }
} // namespace AzToolsFramework
