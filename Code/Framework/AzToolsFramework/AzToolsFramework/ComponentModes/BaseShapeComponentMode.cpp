/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/ComponentModes/BaseShapeComponentMode.h>

#include <AzCore/Component/NonUniformScaleBus.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/Manipulators/ShapeManipulatorRequestBus.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>

namespace AzToolsFramework
{
    namespace BaseShapeComponentModeIdentifiers
    {
        //! Uri's for shortcut actions.
        const AZ::Crc32 SetDimensionsSubModeActionUri = AZ_CRC_CE("org.o3de.action.shape.setdimensionssubmode");
        const AZ::Crc32 SetTranslationOffsetSubModeActionUri = AZ_CRC_CE("org.o3de.action.shape.settranslationoffsetsubmode");
        const AZ::Crc32 ResetSubModeActionUri = AZ_CRC_CE("org.o3de.action.shape.resetsubmode");
    } // namespace BaseShapeComponentModeIdentifiers

    void InstallBaseShapeViewportEditFunctions(
        BaseShapeViewportEdit* baseShapeViewportEdit, const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        auto getManipulatorSpace = [entityComponentIdPair]()
        {
            AZ::Transform manipulatorSpace = AZ::Transform::CreateIdentity();
            ShapeManipulatorRequestBus::EventResult(
                manipulatorSpace, entityComponentIdPair, &ShapeManipulatorRequestBus::Events::GetManipulatorSpace);
            return manipulatorSpace;
        };
        auto getNonUniformScale = [entityComponentIdPair]()
        {
            AZ::Vector3 nonUniformScale = AZ::Vector3::CreateOne();
            AZ::NonUniformScaleRequestBus::EventResult(
                nonUniformScale, entityComponentIdPair.GetEntityId(), &AZ::NonUniformScaleRequestBus::Events::GetScale);
            return nonUniformScale;
        };
        auto getTranslationOffset = [entityComponentIdPair]()
        {
            AZ::Vector3 translationOffset = AZ::Vector3::CreateZero();
            ShapeManipulatorRequestBus::EventResult(
                translationOffset, entityComponentIdPair, &ShapeManipulatorRequestBus::Events::GetTranslationOffset);
            return translationOffset;
        };
        auto getRotationOffset = [entityComponentIdPair]()
        {
            AZ::Quaternion rotationOffset = AZ::Quaternion::CreateIdentity();
            ShapeManipulatorRequestBus::EventResult(
                rotationOffset, entityComponentIdPair, &ShapeManipulatorRequestBus::Events::GetRotationOffset);
            return rotationOffset;
        };
        auto setTranslationOffset = [entityComponentIdPair](const AZ::Vector3& translationOffset)
        {
            ShapeManipulatorRequestBus::Event(
                entityComponentIdPair, &ShapeManipulatorRequestBus::Events::SetTranslationOffset, translationOffset);
        };
        baseShapeViewportEdit->InstallGetManipulatorSpace(AZStd::move(getManipulatorSpace));
        baseShapeViewportEdit->InstallGetNonUniformScale(AZStd::move(getNonUniformScale));
        baseShapeViewportEdit->InstallGetTranslationOffset(AZStd::move(getTranslationOffset));
        baseShapeViewportEdit->InstallGetRotationOffset(AZStd::move(getRotationOffset));
        baseShapeViewportEdit->InstallSetTranslationOffset(AZStd::move(setTranslationOffset));
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

    AZ_CLASS_ALLOCATOR_IMPL(BaseShapeComponentMode, AZ::SystemAllocator)

    BaseShapeComponentMode::BaseShapeComponentMode(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid componentType, bool allowAsymmetricalEditing)
        : EditorBaseComponentMode::EditorBaseComponentMode(entityComponentIdPair, componentType)
        , m_entityComponentIdPair(entityComponentIdPair)
        , m_allowAsymmetricalEditing(allowAsymmetricalEditing)
    {
    }

    BaseShapeComponentMode::~BaseShapeComponentMode()
    {
        ShapeComponentModeRequestBus::Handler::BusDisconnect();
        m_subModes[static_cast<AZ::u32>(m_subMode)]->Teardown();
        if (m_allowAsymmetricalEditing)
        {
            ViewportUi::ViewportUiRequestBus::Event(
                ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::RemoveCluster, m_clusterId);
            m_clusterId = ViewportUi::InvalidClusterId;
        }
    }

    void BaseShapeComponentMode::Refresh()
    {
        m_subModes[static_cast<AZ::u32>(m_subMode)]->UpdateManipulators();
    }

    void BaseShapeComponentMode::SetupCluster()
    {
        ViewportUi::ViewportUiRequestBus::EventResult(
            m_clusterId,
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::CreateCluster,
            ViewportUi::Alignment::TopLeft);

        m_buttonIds[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)] =
            RegisterClusterButton(ViewportUi::DefaultViewportId, m_clusterId, "Scale", DimensionsTooltip);
        m_buttonIds[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::TranslationOffset)] =
            RegisterClusterButton(ViewportUi::DefaultViewportId, m_clusterId, "Move", TranslationOffsetTooltip);

        const auto onButtonClicked = [this](ViewportUi::ButtonId buttonId)
        {
            if (buttonId == m_buttonIds[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::Dimensions)])
            {
                SetShapeSubMode(ShapeComponentModeRequests::SubMode::Dimensions);
            }
            else if (buttonId == m_buttonIds[static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::TranslationOffset)])
            {
                SetShapeSubMode(ShapeComponentModeRequests::SubMode::TranslationOffset);
            }
        };

        m_modeSelectionHandler = AZ::Event<ViewportUi::ButtonId>::Handler(onButtonClicked);
        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_clusterId,
            m_modeSelectionHandler);
    }

    ShapeComponentModeRequests::SubMode BaseShapeComponentMode::GetShapeSubMode() const
    {
        return m_subMode;
    }

    void BaseShapeComponentMode::SetShapeSubMode(ShapeComponentModeRequests::SubMode mode)
    {
        const auto modeIndex = static_cast<AZ::u32>(mode);
        AZ_Assert(modeIndex < m_subModes.size(), "Submode not found:%d", modeIndex);
        m_subModes[static_cast<AZ::u32>(m_subMode)]->Teardown();
        AZ_Assert(modeIndex < m_buttonIds.size(), "Invalid mode index %i.", modeIndex);
        m_subMode = mode;
        m_subModes[modeIndex]->Setup(g_mainManipulatorManagerId);
        m_subModes[modeIndex]->AddEntityComponentIdPair(m_entityComponentIdPair);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId, &ViewportUi::ViewportUiRequestBus::Events::ClearClusterActiveButton, m_clusterId);

        ViewportUi::ViewportUiRequestBus::Event(
            ViewportUi::DefaultViewportId,
            &ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            m_clusterId,
            m_buttonIds[modeIndex]);
    }

    void BaseShapeComponentMode::ResetShapeSubMode()
    {
        const auto modeIndex = static_cast<AZ::u32>(m_subMode);
        m_subModes[modeIndex]->ResetValues();
        m_subModes[modeIndex]->UpdateManipulators();
    }

    bool BaseShapeComponentMode::HandleMouseInteraction(const ViewportInteraction::MouseInteractionEvent& mouseInteraction)
    {
        if (!m_allowAsymmetricalEditing)
        {
            return false;
        }

        if (mouseInteraction.m_mouseEvent == ViewportInteraction::MouseEvent::Wheel &&
            mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
        {
            const int direction = MouseWheelDelta(mouseInteraction) > 0.0f ? -1 : 1;
            const AZ::u32 currentModeIndex = static_cast<AZ::u32>(m_subMode);
            const AZ::u32 numSubModes = static_cast<AZ::u32>(ShapeComponentModeRequests::SubMode::NumModes);
            const AZ::u32 nextModeIndex = (currentModeIndex + numSubModes + direction) % m_subModes.size();
            ShapeComponentModeRequests::SubMode nextMode = static_cast<ShapeComponentModeRequests::SubMode>(nextModeIndex);
            SetShapeSubMode(nextMode);
            return true;
        }
        return false;
    }

    AZStd::vector<ActionOverride> BaseShapeComponentMode::PopulateActionsImpl()
    {
        if (!m_allowAsymmetricalEditing)
        {
            return {};
        }

        ActionOverride setDimensionsModeAction;
        setDimensionsModeAction.SetUri(BaseShapeComponentModeIdentifiers::SetDimensionsSubModeActionUri);
        setDimensionsModeAction.SetKeySequence(QKeySequence(Qt::Key_1));
        setDimensionsModeAction.SetTitle("Set Dimensions Mode");
        setDimensionsModeAction.SetTip("Set dimensions mode");
        setDimensionsModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setDimensionsModeAction.SetCallback(
            [this]()
            {
                SetShapeSubMode(SubMode::Dimensions);
            });

        ActionOverride setTranslationOffsetModeAction;
        setTranslationOffsetModeAction.SetUri(BaseShapeComponentModeIdentifiers::SetTranslationOffsetSubModeActionUri);
        setTranslationOffsetModeAction.SetKeySequence(QKeySequence(Qt::Key_2));
        setTranslationOffsetModeAction.SetTitle("Set Translation Offset Mode");
        setTranslationOffsetModeAction.SetTip("Set translation offset mode");
        setTranslationOffsetModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        setTranslationOffsetModeAction.SetCallback(
            [this]()
            {
                SetShapeSubMode(SubMode::TranslationOffset);
            });

        ActionOverride resetModeAction;
        resetModeAction.SetUri(BaseShapeComponentModeIdentifiers::ResetSubModeActionUri);
        resetModeAction.SetKeySequence(QKeySequence(Qt::Key_R));
        resetModeAction.SetTitle("Reset Current Mode");
        resetModeAction.SetTip("Reset current mode");
        resetModeAction.SetEntityComponentIdPair(GetEntityComponentIdPair());
        resetModeAction.SetCallback(
            [this]()
            {
                ResetShapeSubMode();
            });

        return { setDimensionsModeAction, setTranslationOffsetModeAction, resetModeAction };
    }

    void BaseShapeComponentMode::RegisterActions(const char* shapeName)
    {
        AZStd::string category = AZStd::string::format("%s Component Mode", shapeName);
        AZStd::to_upper(category.begin(), category.begin() + 1);

        auto actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "BaseShapeComponentMode - could not get ActionManagerInterface on RegisterActions.");

        auto hotKeyManagerInterface = AZ::Interface<HotKeyManagerInterface>::Get();
        AZ_Assert(hotKeyManagerInterface, "BaseShapeComponentMode - could not get HotKeyManagerInterface on RegisterActions.");

        // Dimensions sub-mode
        {
            const auto actionIdentifier = AZStd::string::format("o3de.action.%sComponentMode.setDimensionsSubMode", shapeName);
            ActionProperties actionProperties;
            actionProperties.m_name = "Set Dimensions Mode";
            actionProperties.m_description = "Set Dimensions Mode";
            actionProperties.m_category = category;

            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                    auto componentModeCollectionInterface = AZ::Interface<ComponentModeCollectionInterface>::Get();
                    AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");

                    componentModeCollectionInterface->EnumerateActiveComponents(
                        [](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                        {
                            ShapeComponentModeRequestBus::Event(
                                entityComponentIdPair,
                                &ShapeComponentModeRequests::SetShapeSubMode,
                                ShapeComponentModeRequests::SubMode::Dimensions);
                        });
                });

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "1");
        }

        // Translation offset sub-mode
        {
            const auto actionIdentifier = AZStd::string::format("o3de.action.%sComponentMode.setTranslationOffsetSubMode", shapeName);
            ActionProperties actionProperties;
            actionProperties.m_name = "Set Translation Offset Mode";
            actionProperties.m_description = "Set Translation Offset Mode";
            actionProperties.m_category = category;

            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                    auto componentModeCollectionInterface = AZ::Interface<ComponentModeCollectionInterface>::Get();
                    AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");

                    componentModeCollectionInterface->EnumerateActiveComponents(
                        [](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                        {
                            ShapeComponentModeRequestBus::Event(
                                entityComponentIdPair,
                                &ShapeComponentModeRequests::SetShapeSubMode,
                                ShapeComponentModeRequests::SubMode::TranslationOffset);
                        });
                });

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "2");
        }

        // Reset current mode
        {
            const auto actionIdentifier = AZStd::string::format("o3de.action.%sComponentMode.resetCurrentMode", shapeName);
            ActionProperties actionProperties;
            actionProperties.m_name = "Reset Current Mode";
            actionProperties.m_description = "Reset Current Mode";
            actionProperties.m_category = category;

            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier,
                actionIdentifier,
                actionProperties,
                []
                {
                    auto componentModeCollectionInterface = AZ::Interface<ComponentModeCollectionInterface>::Get();
                    AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");

                    componentModeCollectionInterface->EnumerateActiveComponents(
                        [](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                        {
                            ShapeComponentModeRequestBus::Event(entityComponentIdPair, &ShapeComponentModeRequests::ResetShapeSubMode);
                        });
                });

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "R");
        }
    }

    void BaseShapeComponentMode::BindActionsToModes(const char* shapeName, const char* className)
    {
        auto actionManagerInterface = AZ::Interface<ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "BaseShapeComponentMode - could not get ActionManagerInterface on RegisterActions.");

        const AZStd::string modeIdentifier = AZStd::string::format("o3de.context.mode.%s", className);
        const AZStd::string prefix = AZStd::string::format("o3de.action.%sComponentMode", shapeName);

        actionManagerInterface->AssignModeToAction(modeIdentifier, prefix + ".setDimensionsSubMode");
        actionManagerInterface->AssignModeToAction(modeIdentifier, prefix + ".setTranslationOffsetSubMode");
        actionManagerInterface->AssignModeToAction(modeIdentifier, prefix + ".resetCurrentMode");
    }

    void BaseShapeComponentMode::BindActionsToMenus(const char* shapeName)
    {
        auto menuManagerInterface = AZ::Interface<MenuManagerInterface>::Get();
        AZ_Assert(menuManagerInterface, "BaseShapeComponentMode - could not get MenuManagerInterface on BindActionsToMenus.");

        const AZStd::string prefix = AZStd::string::format("o3de.action.%sComponentMode", shapeName);

        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, prefix + ".setDimensionsSubMode", 6000);
        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, prefix + ".setTranslationOffsetSubMode", 6001);
        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, prefix + ".resetCurrentMode", 6002);
    }
} // namespace AzToolsFramework

