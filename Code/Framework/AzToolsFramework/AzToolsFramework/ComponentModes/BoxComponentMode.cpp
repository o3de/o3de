/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "BoxComponentMode.h"

namespace AzToolsFramework
{
    AZ_CLASS_ALLOCATOR_IMPL(BoxComponentMode, AZ::SystemAllocator, 0)

    void BoxComponentMode::Reflect(AZ::ReflectContext* context)
    {
        ComponentModeFramework::ReflectEditorBaseComponentModeDescendant<BoxComponentMode>(context);
    }

    BoxComponentMode::BoxComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_entityComponentIdPair(entityComponentIdPair)
        , m_allowAsymmetricalEditing(allowAsymmetricalEditing)
    {
        m_subModes[static_cast<size_t>(SubMode::Dimensions)] = AZStd::make_unique<BoxViewportEdit>();
        if (m_allowAsymmetricalEditing)
        {
            m_subModes[static_cast<size_t>(SubMode::TranslationOffset)] = AZStd::make_unique<ShapeTranslationOffsetViewportEdit>();
            SetupCluster();
            SetCurrentMode(SubMode::Dimensions);
        }
        else
        {
            m_subModes[static_cast<size_t>(SubMode::Dimensions)]->Setup(entityComponentIdPair);
        }
    }

    static ViewportUi::ButtonId RegisterClusterButton(
        AZ::s32 viewportId, ViewportUi::ClusterId clusterId, const char* iconName, const char* tooltip)
    {
        ViewportUi::ButtonId buttonId;
        ViewportUi::ViewportUiRequestBus::EventResult(
            buttonId,
            viewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateClusterButton,
            clusterId,
            AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName));

        ViewportUi::ViewportUiRequestBus::Event(
            viewportId, &ViewportUi::ViewportUiRequestBus::Events::SetClusterButtonTooltip, clusterId, buttonId, tooltip);

        return buttonId;
    }


    BoxComponentMode::~BoxComponentMode()
    {
        m_subModes[static_cast<size_t>(m_subMode)]->Teardown();
        if (m_allowAsymmetricalEditing)
        {            
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_clusterId);
            m_clusterId = ViewportUi::InvalidClusterId;
        }
    }

    void BoxComponentMode::Refresh()
    {
        m_subModes[static_cast<size_t>(m_subMode)]->UpdateManipulators();
    }

    AZStd::string BoxComponentMode::GetComponentModeName() const
    {
        return "Box Edit Mode";
    }

    AZ::Uuid BoxComponentMode::GetComponentModeType() const
    {
        return azrtti_typeid<BoxComponentMode>();
    }

    void BoxComponentMode::SetupCluster()
    {
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_clusterId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            ViewportUi::Alignment::TopLeft);

        m_buttonIds[static_cast<size_t>(SubMode::Dimensions)] =
            RegisterClusterButton(ViewportUi::DefaultViewportId, m_clusterId, "Scale", DimensionsTooltip);
        m_buttonIds[static_cast<size_t>(SubMode::TranslationOffset)] =
            RegisterClusterButton(ViewportUi::DefaultViewportId, m_clusterId, "Move", TranslationOffsetTooltip);

        const auto onJointLimitButtonClicked = [this](ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::Dimensions)])
            {
                SetCurrentMode(SubMode::Dimensions);
            }
            else if (buttonId == m_buttonIds[static_cast<size_t>(SubMode::TranslationOffset)])
            {
                SetCurrentMode(SubMode::TranslationOffset);
            }
        };

        m_modeSelectionHandler = AZ::Event<ViewportUi::ButtonId>::Handler(onJointLimitButtonClicked);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_clusterId,
            m_modeSelectionHandler);
    }

    void BoxComponentMode::SetCurrentMode(SubMode mode)
    {
        AZ_Assert(mode < SubMode::NumModes, "Submode not found:%d", static_cast<AZ::u32>(mode));
        m_subModes[static_cast<size_t>(m_subMode)]->Teardown();
        const auto modeIndex = static_cast<size_t>(mode);
        AZ_Assert(modeIndex < m_buttonIds.size(), "Invalid mode index %i.", modeIndex);
        m_subMode = mode;
        m_subModes[modeIndex]->Setup(m_entityComponentIdPair);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_clusterId);

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            m_clusterId,
            m_buttonIds[modeIndex]);
    }
} // namespace AzToolsFramework
