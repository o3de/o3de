/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/BoxComponentMode.h>

#include <AzToolsFramework/ComponentModes/ShapeTranslationOffsetViewportEdit.h>
#include <AzToolsFramework/Manipulators/BoxManipulatorRequestBus.h>

namespace AzToolsFramework
{
    void InstallBoxViewportEditFunctions(BoxViewportEdit* boxViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        auto getLocalTransform = [entityComponentIdPair]()
        {
            AZ::Transform boxLocalTransform = AZ::Transform::CreateIdentity();
            BoxManipulatorRequestBus::EventResult(
                boxLocalTransform, entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetCurrentLocalTransform);
            return boxLocalTransform;
        };
        auto getBoxDimensions = [entityComponentIdPair]()
        {
            AZ::Vector3 boxDimensions = AZ::Vector3::CreateOne();
            BoxManipulatorRequestBus::EventResult(boxDimensions, entityComponentIdPair, &BoxManipulatorRequestBus::Events::GetDimensions);
            return boxDimensions;
        };
        auto setBoxDimensions = [entityComponentIdPair](const AZ::Vector3& boxDimensions)
        {
            BoxManipulatorRequestBus::Event(entityComponentIdPair, &BoxManipulatorRequestBus::Events::SetDimensions, boxDimensions);
        };
        boxViewportEdit->InstallGetBoxDimensions(AZStd::move(getBoxDimensions));
        boxViewportEdit->InstallSetBoxDimensions(AZStd::move(setBoxDimensions));
    }

    AZ_CLASS_ALLOCATOR_IMPL(BoxComponentMode, AZ::SystemAllocator)

    void BoxComponentMode::Reflect(AZ::ReflectContext* context)
    {
        ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<BoxComponentMode>(context);
    }

    BoxComponentMode::BoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : BaseShapeComponentMode(entityComponentIdPair, componentType, allowAsymmetricalEditing)
    {
        auto boxViewportEdit = AZStd::make_unique<BoxViewportEdit>(m_allowAsymmetricalEditing);
        InstallBaseShapeViewportEditFunctions(boxViewportEdit.get(), m_entityComponentIdPair);
        InstallBoxViewportEditFunctions(boxViewportEdit.get(), m_entityComponentIdPair);
        m_subModes[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)] = AZStd::move(boxViewportEdit);
            
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
    }

    BoxComponentMode::~BoxComponentMode()
    {
        ShapeComponentModeRequestBus::Handler::BusDisconnect();
    }

    AZStd::string BoxComponentMode::GetComponentModeName() const
    {
        return "Box Edit Mode";
    }

    AZ::Uuid BoxComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<BoxComponentMode>();
    }

    void BoxComponentMode::RegisterActions()
    {
        BaseShapeComponentMode::RegisterActions("box");
    }

    void BoxComponentMode::BindActionsToModes()
    {
        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        BaseShapeComponentMode::BindActionsToModes("box", serializeContext->FindClassData(azrtti_typeid<BoxComponentMode>())->m_name);
    }

    void BoxComponentMode::BindActionsToMenus()
    {
        BaseShapeComponentMode::BindActionsToMenus("box");
    }
} // namespace AzToolsFramework
