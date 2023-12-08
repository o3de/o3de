/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/QuadComponentMode.h>

#include "ComponentModes/QuadViewportEdit.h"
#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>
#include <AzToolsFramework/Manipulators/QuadManipulatorRequestBus.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>

namespace AzToolsFramework
{

    void InstallQuadViewportEditFunctions(QuadViewportEdit* quadViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        auto getQuadHeight = [entityComponentIdPair]()
        {
            float height = 1.0f;
            QuadManipulatorRequestBus::EventResult(height, entityComponentIdPair, &QuadManipulatorRequestBus::Events::GetHeight);
            return height;
        };
        auto getQuadWidth = [entityComponentIdPair]()
        {
            float width = 1.0f;
            QuadManipulatorRequestBus::EventResult(width, entityComponentIdPair, &QuadManipulatorRequestBus::Events::GetWidth);
            return width;
        };
        auto setQuadHeight = [entityComponentIdPair](float height)
        {
            QuadManipulatorRequestBus::Event(entityComponentIdPair, &QuadManipulatorRequestBus::Events::SetHeight, height);
        };
        auto setQuadWidth = [entityComponentIdPair](float width)
        {
            QuadManipulatorRequestBus::Event(entityComponentIdPair, &QuadManipulatorRequestBus::Events::SetWidth, width);
        };
        quadViewportEdit->InstallSetQuadWidth(AZStd::move(setQuadWidth));
        quadViewportEdit->InstallSetQuadHeight(AZStd::move(setQuadHeight));
        quadViewportEdit->InstallGetQuadWidth(AZStd::move(getQuadWidth));
        quadViewportEdit->InstallGetQuadHeight(AZStd::move(getQuadHeight));
    }

    AZ_CLASS_ALLOCATOR_IMPL(QuadComponentMode, AZ::SystemAllocator)

    void QuadComponentMode::Reflect(AZ::ReflectContext* context)
    {
        ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<QuadComponentMode>(context);
    }

    QuadComponentMode::QuadComponentMode(const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType)
        : BaseShapeComponentMode(entityComponentIdPair, componentType, false)
    {
        auto quadViewportEdit = AZStd::make_unique<QuadViewportEdit>();
        InstallBaseShapeViewportEditFunctions(quadViewportEdit.get(), m_entityComponentIdPair);
        InstallQuadViewportEditFunctions(quadViewportEdit.get(), m_entityComponentIdPair);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)] = AZStd::move(quadViewportEdit);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->Setup(g_mainManipulatorManagerId);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->AddEntityComponentIdPair(
            m_entityComponentIdPair);
        ShapeComponentModeRequestBus::Handler::BusConnect(m_entityComponentIdPair);
        AzFramework::EntityDebugDisplayEventBus::Handler::BusConnect(m_entityComponentIdPair.GetEntityId());
    }

    QuadComponentMode::~QuadComponentMode()
    {
        AzFramework::EntityDebugDisplayEventBus::Handler::BusDisconnect();
        ShapeComponentModeRequestBus::Handler::BusDisconnect();
    }

    void QuadComponentMode::DisplayEntityViewport(
        const AzFramework::ViewportInfo& viewportInfo, [[maybe_unused]] AzFramework::DebugDisplayRequests& debugDisplay)
    {
        const AzFramework::CameraState cameraState = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)]->OnCameraStateChanged(cameraState);
    }

    AZStd::string QuadComponentMode::GetComponentModeName() const
    {
        return "Quad Edit Mode";
    }

    AZ::Uuid QuadComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<QuadComponentMode>();
    }

    void QuadComponentMode::RegisterActions()
    {
        BaseShapeComponentMode::RegisterActions("quad");
    }

    void QuadComponentMode::BindActionsToModes()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        BaseShapeComponentMode::BindActionsToModes("quad", serializeContext->FindClassData(azrtti_typeid<QuadComponentMode>())->m_name);
    }

    void QuadComponentMode::BindActionsToMenus()
    {
        BaseShapeComponentMode::BindActionsToMenus("quad");
    }
}; // namespace AzToolsFramework
