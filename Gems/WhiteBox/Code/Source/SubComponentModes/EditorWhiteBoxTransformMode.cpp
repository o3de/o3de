/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxTransformMode.h"
#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "Util/WhiteBoxEditorDrawUtil.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <Manipulators/LinearManipulator.h>
#include <Manipulators/ManipulatorManager.h>
#include <Manipulators/RotationManipulators.h>
#include <Manipulators/ScaleManipulators.h>
#include <Manipulators/TranslationManipulators.h>
#include <Viewport/ViewportSettings.h>
#include <Viewport/WhiteBoxModifierUtil.h>
#include <Viewport/WhiteBoxViewportConstants.h>

#include <QKeySequence>

namespace WhiteBox
{
    AZ_CLASS_ALLOCATOR_IMPL(TransformMode, AZ::SystemAllocator)

    static const AZ::Crc32 SwitchTranslationMode = AZ_CRC_CE("org.o3de.action.whitebox.switch_translation");
    static const AZ::Crc32 SwitchRotationMode = AZ_CRC_CE("org.o3de.action.whitebox.switch_rotation");
    static const AZ::Crc32 SwitchScaleMode = AZ_CRC_CE("org.o3de.action.whitebox.switch_scale");

    // Numeric input action identifiers
    constexpr AZStd::string_view NumericBeginMoveId     = "o3de.action.whiteBoxTransform.numeric.beginMove";
    constexpr AZStd::string_view NumericBeginRotateId   = "o3de.action.whiteBoxTransform.numeric.beginRotate";
    constexpr AZStd::string_view NumericBeginScaleId    = "o3de.action.whiteBoxTransform.numeric.beginScale";
    constexpr AZStd::string_view NumericAxisXId         = "o3de.action.whiteBoxTransform.numeric.axisX";
    constexpr AZStd::string_view NumericAxisYId         = "o3de.action.whiteBoxTransform.numeric.axisY";
    constexpr AZStd::string_view NumericAxisZId         = "o3de.action.whiteBoxTransform.numeric.axisZ";
    constexpr AZStd::string_view NumericConfirmId       = "o3de.action.whiteBoxTransform.numeric.confirm";
    constexpr AZStd::string_view NumericCancelId        = "o3de.action.whiteBoxTransform.numeric.cancel";
    constexpr AZStd::string_view NumericBackspaceId     = "o3de.action.whiteBoxTransform.numeric.backspace";
    constexpr AZStd::string_view NumericDecimalId       = "o3de.action.whiteBoxTransform.numeric.decimal";
    constexpr AZStd::string_view NumericNegateId        = "o3de.action.whiteBoxTransform.numeric.negate";
    constexpr AZStd::string_view NumericOpPlusId        = "o3de.action.whiteBoxTransform.numeric.opPlus";
    constexpr AZStd::string_view NumericOpMultId        = "o3de.action.whiteBoxTransform.numeric.opMult";
    constexpr AZStd::string_view NumericOpDivId         = "o3de.action.whiteBoxTransform.numeric.opDiv";
    // digit 0-9
    constexpr AZStd::string_view NumericDigitIds[10] = {
        "o3de.action.whiteBoxTransform.numeric.digit0",
        "o3de.action.whiteBoxTransform.numeric.digit1",
        "o3de.action.whiteBoxTransform.numeric.digit2",
        "o3de.action.whiteBoxTransform.numeric.digit3",
        "o3de.action.whiteBoxTransform.numeric.digit4",
        "o3de.action.whiteBoxTransform.numeric.digit5",
        "o3de.action.whiteBoxTransform.numeric.digit6",
        "o3de.action.whiteBoxTransform.numeric.digit7",
        "o3de.action.whiteBoxTransform.numeric.digit8",
        "o3de.action.whiteBoxTransform.numeric.digit9",
    };

    const constexpr char* SwitchToTranslationModeTile = "Translation Mode";
    const constexpr char* SwitchToRotationModeTile = "Rotation Mode";
    const constexpr char* SwitchToScaleModeTile = "Scale Mode";

    const constexpr char* SwitchToTranslationModeDesc = "Switch to Translation Mode";
    const constexpr char* SwitchToRotationModeDesc = "Switch to Rotation Mode";
    const constexpr char* SwitchToScaleModeDesc = "Switch to Scale Mode";

    static void SetViewportUiClusterActiveButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, AzToolsFramework::ViewportUi::ButtonId buttonId)
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterActiveButton,
            clusterId,
            buttonId);
    }

    static void SetViewportUiClusterDisableButton(
        AzToolsFramework::ViewportUi::ClusterId clusterId, AzToolsFramework::ViewportUi::ButtonId buttonId, bool isDisabled)
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::SetClusterDisableButton,
            clusterId,
            buttonId,
            isDisabled);
    }

    TransformMode::TransformMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
        EditorWhiteBoxTransformModeRequestBus::Handler::BusConnect(entityComponentIdPair);

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            [&](AzToolsFramework::ViewportUi::ViewportUiRequests* requests)
            {
                auto fetchIcon = [](const char* iconName)
                {
                    return AZStd::string::format(":/stylesheet/img/UI20/toolbar/%s.svg", iconName);
                };

                m_transformClusterId = requests->CreateCluster(AzToolsFramework::ViewportUi::Alignment::TopLeft);
                
                m_transformTranslateButtonId = requests->CreateClusterButton(m_transformClusterId, fetchIcon("Move"));
                m_transformRotateButtonId = requests->CreateClusterButton(m_transformClusterId, fetchIcon("Rotate"));
                m_transformScaleButtonId = requests->CreateClusterButton(m_transformClusterId, fetchIcon("Scale"));

                // set translation tooltips
                requests->SetClusterButtonTooltip(m_transformClusterId, m_transformTranslateButtonId, 
                    ManipulatorModeClusterTranslateTooltip);
                requests->SetClusterButtonTooltip(m_transformClusterId, m_transformRotateButtonId, 
                    ManipulatorModeClusterRotateTooltip);
                requests->SetClusterButtonTooltip(m_transformClusterId, m_transformScaleButtonId, 
                    ManipulatorModeClusterScaleTooltip);
            }
        );

        m_transformSelectionHandler = AZ::Event<AzToolsFramework::ViewportUi::ButtonId>::Handler(
            [this](AzToolsFramework::ViewportUi::ButtonId buttonId)
            {
                if (buttonId == m_transformTranslateButtonId)
                {
                    ChangeTransformType(TransformType::Translation);
                }
                else if (buttonId == m_transformRotateButtonId)
                {
                    ChangeTransformType(TransformType::Rotation);
                }
                else if (buttonId == m_transformScaleButtonId)
                {
                    ChangeTransformType(TransformType::Scale);
                }
            }
        );

        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RegisterClusterEventHandler,
            m_transformClusterId,
            m_transformSelectionHandler);

        RefreshManipulator();
    }

    TransformMode::~TransformMode()
    {
        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
            AzToolsFramework::ViewportUi::DefaultViewportId,
            &AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events::RemoveCluster,
            m_transformClusterId);

        DestroyManipulators();

        EditorWhiteBoxTransformModeRequestBus::Handler::BusDisconnect();
    }

    void TransformMode::RegisterActionUpdaters()
    {
    }

    void TransformMode::RegisterActions()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxTransformMode - could not get ActionManagerInterface on RegisterActions.");

        auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(hotKeyManagerInterface, "WhiteBoxTransformMode - could not get HotKeyManagerInterface on RegisterActions.");

        // Translation
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.whiteBoxComponentMode.transform.translation";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = SwitchToTranslationModeTile;
            actionProperties.m_description = SwitchToTranslationModeDesc;
            actionProperties.m_category = "White Box Component Mode - Transform";

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
                            EditorWhiteBoxTransformModeRequestBus::Event(
                                entityComponentIdPair,
                                &EditorWhiteBoxTransformModeRequests::ChangeTransformType,
                                TransformType::Translation);
                        }
                    );
                }
            );

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "1");
        }

        // Rotation
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.whiteBoxComponentMode.transform.rotation";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = SwitchToRotationModeTile;
            actionProperties.m_description = SwitchToRotationModeDesc;
            actionProperties.m_category = "White Box Component Mode - Transform";

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
                            EditorWhiteBoxTransformModeRequestBus::Event(
                                entityComponentIdPair,
                                &EditorWhiteBoxTransformModeRequests::ChangeTransformType,
                                TransformType::Rotation);
                        }
                    );
                }
            );

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "2");
        }

        // Scale
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.whiteBoxComponentMode.transform.scale";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = SwitchToScaleModeTile;
            actionProperties.m_description = SwitchToScaleModeDesc;
            actionProperties.m_category = "White Box Component Mode - Transform";

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
                            EditorWhiteBoxTransformModeRequestBus::Event(
                                entityComponentIdPair,
                                &EditorWhiteBoxTransformModeRequests::ChangeTransformType,
                                TransformType::Scale);
                        }
                    );
                }
            );

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "3");
        }

        // ------------------------------------------------------------------ //
        // Blender-style numeric input actions                                 //
        // ------------------------------------------------------------------ //
        // Helper: dispatch a lambda to every active TransformMode instance.
        auto dispatchToTransformModes = [](auto fn)
        {
            auto componentModeCollectionInterface = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
            AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");
            componentModeCollectionInterface->EnumerateActiveComponents(
                [&fn](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                {
                    EditorWhiteBoxTransformModeRequestBus::Event(entityComponentIdPair, fn);
                });
        };

        // G – begin move
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Move";
            p.m_description = "Start a Blender-style numeric move (G then type a value)";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericBeginMoveId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericBeginMove);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericBeginMoveId, "M");
        }

        // R – begin rotate
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Rotate";
            p.m_description = "Start a Blender-style numeric rotate (R then type a value)";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericBeginRotateId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericBeginRotate);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericBeginRotateId, "U");
        }

        // S – begin scale
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Scale";
            p.m_description = "Start a Blender-style numeric scale (S then type a value)";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericBeginScaleId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericBeginScale);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericBeginScaleId, "J");
        }

        // X / Y / Z – axis constraint
        {
            AzToolsFramework::ActionProperties px;
            px.m_name = "Numeric Input Axis X";
            px.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericAxisXId, px,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericSetAxisX);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericAxisXId, "X");

            AzToolsFramework::ActionProperties py;
            py.m_name = "Numeric Input Axis Y";
            py.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericAxisYId, py,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericSetAxisY);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericAxisYId, "Y");

            AzToolsFramework::ActionProperties pz;
            pz.m_name = "Numeric Input Axis Z";
            pz.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericAxisZId, pz,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericSetAxisZ);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericAxisZId, "Z");
        }

        // Enter – confirm
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Input Confirm";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericConfirmId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericConfirm);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericConfirmId, "Return");
        }

        // Escape – cancel
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Input Cancel";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericCancelId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericCancel);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericCancelId, "Escape");
        }

        // Backspace
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Input Backspace";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericBackspaceId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericBackspace);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericBackspaceId, "Backspace");
        }

        // Period (decimal point)
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Input Decimal";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericDecimalId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericDecimal);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericDecimalId, ".");
        }

        // Minus / negate
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Operator Minus";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericNegateId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericNegate);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericNegateId, "-");
        }
        // Plus
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Operator Plus";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericOpPlusId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericAppendOperatorPlus);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericOpPlusId, "+");
        }
        // Multiply
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Operator Multiply";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericOpMultId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericAppendOperatorMult);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericOpMultId, "*");
        }
        // Divide
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Numeric Operator Divide";
            p.m_category = "White Box Component Mode - Transform";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericOpDivId, p,
                [dispatchToTransformModes]
                {
                    dispatchToTransformModes(&EditorWhiteBoxTransformModeRequests::NumericAppendOperatorDiv);
                });
            hotKeyManagerInterface->SetActionHotKey(NumericOpDivId, "/");
        }

        // Digits 0-9
        // NOTE: EBus::Event requires a member function pointer, not a lambda.
        // For NumericAppendDigit (which takes a char arg) we inline the dispatch
        // and pass the arg as a trailing parameter to EBus::Event.
        const char* digitKeys[10] = {"0","1","2","3","4","5","6","7","8","9"};
        for (int d = 0; d <= 9; ++d)
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = AZStd::string::format("Numeric Input Digit %d", d).c_str();
            p.m_category = "White Box Component Mode - Transform";
            const char digit = static_cast<char>('0' + d);
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, NumericDigitIds[d], p,
                [digit]
                {
                    auto cmci = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
                    AZ_Assert(cmci, "Could not retrieve component mode collection.");
                    cmci->EnumerateActiveComponents(
                        [digit](const AZ::EntityComponentIdPair& id, const AZ::Uuid&)
                        {
                            EditorWhiteBoxTransformModeRequestBus::Event(
                                id, &EditorWhiteBoxTransformModeRequests::NumericAppendDigit, digit);
                        });
                });
            hotKeyManagerInterface->SetActionHotKey(NumericDigitIds[d], digitKeys[d]);
        }
    }

    void TransformMode::BindActionsToModes(const AZStd::string& modeIdentifier)
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxTransformMode - could not get ActionManagerInterface on BindActionsToModes.");

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.whiteBoxComponentMode.transform.translation");
        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.whiteBoxComponentMode.transform.rotation");
        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.whiteBoxComponentMode.transform.scale");

        // Numeric input actions
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericBeginMoveId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericBeginRotateId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericBeginScaleId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericAxisXId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericAxisYId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericAxisZId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericConfirmId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericCancelId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericBackspaceId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericDecimalId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericNegateId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericOpPlusId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericOpMultId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, NumericOpDivId);
        for (const auto& digitId : NumericDigitIds)
        {
            actionManagerInterface->AssignModeToAction(modeIdentifier, digitId);
        }

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.componentMode.end");
    }

    void TransformMode::BindActionsToMenus()
    {
        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(menuManagerInterface, "WhiteBoxTransformMode - could not get MenuManagerInterface on BindActionsToMenus.");

        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.whiteBoxComponentMode.transform.translation", 6000);
        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.whiteBoxComponentMode.transform.rotation", 6001);
        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.whiteBoxComponentMode.transform.scale", 6002);
    }

    void TransformMode::DestroyManipulators()
    {
        if (m_manipulator)
        {
            m_manipulator->Unregister();
            m_manipulator.reset();
        }
    }

    void TransformMode::ChangeTransformType(TransformType subModeType)
    {
        m_transformType = subModeType;
        RefreshManipulator();
    }

    void TransformMode::Refresh()
    {
        m_whiteBoxSelection.reset();
        DestroyManipulators();
    }

    AZStd::vector<AzToolsFramework::ActionOverride> TransformMode::PopulateActions(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        return {
            AzToolsFramework::ActionOverride()
                .SetUri(SwitchTranslationMode)
                .SetKeySequence(QKeySequence{Qt::Key_1})
                .SetTitle(SwitchToTranslationModeTile)
                .SetTip(SwitchToTranslationModeDesc)
                .SetEntityComponentIdPair(entityComponentIdPair)
                .SetCallback(
                    [clusterId = m_transformClusterId, buttonId = m_transformTranslateButtonId]()
                    {
                            AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                                AzToolsFramework::ViewportUi::DefaultViewportId,
                                [](AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events* event,
                                   AzToolsFramework::ViewportUi::ClusterId clusterId,
                                   AzToolsFramework::ViewportUi::ButtonId buttonId)
                                    {
                                        event->PressButton(clusterId, buttonId);
                                    },
                                clusterId,
                                buttonId);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(SwitchRotationMode)
                .SetKeySequence(QKeySequence{Qt::Key_2})
                .SetTitle(SwitchToRotationModeTile)
                .SetTip(SwitchToRotationModeDesc)
                .SetEntityComponentIdPair(entityComponentIdPair)
                .SetCallback(
                    [clusterId = m_transformClusterId, buttonId = m_transformRotateButtonId]()
                    {
                        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                            AzToolsFramework::ViewportUi::DefaultViewportId,
                            [](AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events* event,
                                   AzToolsFramework::ViewportUi::ClusterId clusterId,
                                   AzToolsFramework::ViewportUi::ButtonId buttonId)
                                    {
                                        event->PressButton(clusterId, buttonId);
                                    },
                            clusterId,
                            buttonId);
                    }),
            AzToolsFramework::ActionOverride()
                .SetUri(SwitchScaleMode)
                .SetKeySequence(QKeySequence{Qt::Key_3})
                .SetTitle(SwitchToScaleModeTile)
                .SetTip(SwitchToScaleModeDesc)
                .SetEntityComponentIdPair(entityComponentIdPair)
                .SetCallback(
                    [clusterId = m_transformClusterId, buttonId = m_transformScaleButtonId]()
                    {
                        AzToolsFramework::ViewportUi::ViewportUiRequestBus::Event(
                            AzToolsFramework::ViewportUi::DefaultViewportId,
                            [](AzToolsFramework::ViewportUi::ViewportUiRequestBus::Events* event,
                                   AzToolsFramework::ViewportUi::ClusterId clusterId,
                                   AzToolsFramework::ViewportUi::ButtonId buttonId)
                                    {
                                        event->PressButton(clusterId, buttonId);
                                    },
                            clusterId,
                            buttonId);
                    })
        };
    }

    void TransformMode::Display(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZ::Transform& worldFromLocal,
        [[maybe_unused]] const IntersectionAndRenderData& renderData,
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        debugDisplay.DepthTestOn();
        debugDisplay.PushMatrix(worldFromLocal);

        if (m_polygonIntersection.has_value())
        {
            auto& polygonIntersection = m_polygonIntersection.value();
            DrawFace(debugDisplay, whiteBox, polygonIntersection.GetHandle(), ed_whiteBoxPolygonHover);
            DrawOutline(debugDisplay, whiteBox, polygonIntersection.GetHandle(), ed_whiteBoxOutlineHover);
        }

        if (m_edgeIntersection.has_value())
        {
            auto& edgeIntersection = m_edgeIntersection.value();
            DrawEdge(debugDisplay, whiteBox, edgeIntersection.GetHandle(), ed_whiteBoxOutlineHover);
        }

        if (m_vertexIntersection.has_value())
        {
            auto& vertexIntersection = m_vertexIntersection.value();
            auto handles = AZStd::array<Api::VertexHandle, 1>({ vertexIntersection.GetHandle() });
            DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, handles, ed_whiteBoxVertexHover);
        }

        if (m_whiteBoxSelection)
        {
            if (auto polygonSelection = AZStd::get_if<PolygonIntersection>(&m_whiteBoxSelection->m_selection))
            {
                auto vertexHandles = Api::PolygonVertexHandles(*whiteBox, polygonSelection->GetHandle());
                DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, vertexHandles, ed_whiteBoxVertexSelection);
                if (m_polygonIntersection.value_or(PolygonIntersection{}).GetHandle() != polygonSelection->GetHandle())
                {
                    DrawFace(debugDisplay, whiteBox, polygonSelection->GetHandle(), ed_whiteBoxPolygonSelection);
                    DrawOutline(debugDisplay, whiteBox, polygonSelection->GetHandle(), ed_whiteBoxOutlineSelection);
                }
            }
            else if (auto edgeSelection = AZStd::get_if<EdgeIntersection>(&m_whiteBoxSelection->m_selection))
            {
                auto vertexHandles = Api::EdgeVertexHandles(*whiteBox, edgeSelection->GetHandle());
                DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, vertexHandles, ed_whiteBoxVertexSelection);
                if (m_edgeIntersection.value_or(EdgeIntersection{}).GetHandle() != edgeSelection->GetHandle())
                {
                    DrawEdge(debugDisplay, whiteBox, edgeSelection->GetHandle(), ed_whiteBoxOutlineSelection);
                }
            }
            else if (auto vertexSelection = AZStd::get_if<VertexIntersection>(&m_whiteBoxSelection->m_selection))
            {
                if (m_vertexIntersection.value_or(VertexIntersection{}).GetHandle() != vertexSelection->GetHandle())
                {
                    auto handles = AZStd::array<Api::VertexHandle, 1>({ vertexSelection->GetHandle() });
                    DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, handles, ed_whiteBoxVertexSelection);
                }
            }
        }
        debugDisplay.PopMatrix();
        debugDisplay.DepthTestOff();

        // Draw Blender-style numeric input overlay when active.
        if (m_numericInput.IsActive() && m_whiteBoxSelection)
        {
            const AZStd::string statusText = m_numericInput.GetStatusText();
            // Draw the status text above the selection midpoint in world space.
            const AZ::Vector3 worldPos = worldFromLocal.TransformPoint(m_whiteBoxSelection->m_localPosition)
                + AZ::Vector3::CreateAxisZ(0.3f);
            debugDisplay.SetColor(AZ::Colors::White);
            debugDisplay.DrawTextLabel(worldPos, 1.5f, statusText.c_str(), true, 0, 0);
        }
    }

    bool TransformMode::HandleMouseInteraction(const ModeMouseInteraction& mouse)
    {
        const auto& mouseInteraction = mouse.m_mouseInteraction;
        const AZStd::optional<EdgeIntersection>& edgeIntersection = mouse.m_edgeIntersection;
        const AZStd::optional<PolygonIntersection>& polygonIntersection = mouse.m_polygonIntersection;
        const AZStd::optional<VertexIntersection>& vertexIntersection = mouse.m_vertexIntersection;

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        bool mouseOverManipulator = false;
        if (m_manipulator)
        {
            m_manipulator->ProcessManipulators(
                [&mouseOverManipulator](auto manipulator)
                {
                    mouseOverManipulator = manipulator->MouseOver() || mouseOverManipulator;
                });
        }

        auto closestIntersection = mouseOverManipulator
            ? GeometryIntersection::None
            : FindClosestGeometryIntersection(edgeIntersection, polygonIntersection, vertexIntersection);
        m_polygonIntersection.reset();
        m_edgeIntersection.reset();
        m_vertexIntersection.reset();

        // update stored edge and vertex intersection
        switch (closestIntersection)
        {
        case GeometryIntersection::Polygon:
            m_polygonIntersection = polygonIntersection;
            break;
        case GeometryIntersection::Edge:
            m_edgeIntersection = edgeIntersection;
            break;
        case GeometryIntersection::Vertex:
            m_vertexIntersection = vertexIntersection;
            break;
        default:
            // do nothing
            break;
        }

        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            switch (closestIntersection)
            {
            case GeometryIntersection::Polygon:
                if (polygonIntersection.has_value())
                {
                    m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    m_whiteBoxSelection->m_selection = polygonIntersection.value();
                    RefreshManipulator();
                }
                break;
            case GeometryIntersection::Edge:
                if (edgeIntersection.has_value())
                {
                    m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    m_whiteBoxSelection->m_selection = edgeIntersection.value();
                    RefreshManipulator();
                }
                break;
            case GeometryIntersection::Vertex:
                if (vertexIntersection.has_value())
                {
                    m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    m_whiteBoxSelection->m_selection = vertexIntersection.value();
                    RefreshManipulator();
                }
                break;
            default: 
                m_whiteBoxSelection.reset();
                DestroyManipulators();
                break;
            }
        }

        return false;
    }

    void TransformMode::RefreshManipulator()
    {
        TransformType activeTransformType = m_transformType;
        if (m_whiteBoxSelection && AZStd::get_if<VertexIntersection>(&m_whiteBoxSelection->m_selection))
        {
            SetViewportUiClusterDisableButton(m_transformClusterId, m_transformRotateButtonId, true);
            SetViewportUiClusterDisableButton(m_transformClusterId, m_transformScaleButtonId, true);
            activeTransformType = TransformType::Translation;
        }
        else
        {
            SetViewportUiClusterDisableButton(m_transformClusterId, m_transformRotateButtonId, false);
            SetViewportUiClusterDisableButton(m_transformClusterId, m_transformScaleButtonId, false);
        }

        DestroyManipulators();
        switch (activeTransformType)
        {
        case TransformType::Translation:
            CreateTranslationManipulators();
            SetViewportUiClusterActiveButton(m_transformClusterId, m_transformTranslateButtonId);
            break;
        case TransformType::Rotation:
            CreateRotationManipulators();
            SetViewportUiClusterActiveButton(m_transformClusterId, m_transformRotateButtonId);
            break;
        case TransformType::Scale:
            CreateScaleManipulators();
            SetViewportUiClusterActiveButton(m_transformClusterId, m_transformScaleButtonId);
            break;
        default:
            break;
        }
    }

    void TransformMode::UpdateTransformHandles(WhiteBoxMesh* mesh)
    {
        if (auto polygonSelection = AZStd::get_if<PolygonIntersection>(&m_whiteBoxSelection->m_selection))
        {
            m_whiteBoxSelection->m_vertexHandles = Api::PolygonVertexHandles(*mesh, polygonSelection->GetHandle());
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition = Api::PolygonMidpoint(*mesh, polygonSelection->GetHandle());
        }
        else if (auto edgeSelection = AZStd::get_if<EdgeIntersection>(&m_whiteBoxSelection->m_selection))
        {
            auto edgeHandle = Api::EdgeVertexHandles(*mesh, edgeSelection->GetHandle());
            m_whiteBoxSelection->m_vertexHandles = Api::VertexHandles(edgeHandle.cbegin(), edgeHandle.cend());
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition = Api::EdgeMidpoint(*mesh, edgeSelection->GetHandle());
        }
        else if (auto vertexSelection = AZStd::get_if<VertexIntersection>(&m_whiteBoxSelection->m_selection))
        {
            m_whiteBoxSelection->m_vertexHandles = Api::VertexHandles({ vertexSelection->GetHandle() });
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition = Api::VertexPosition(*mesh, vertexSelection->GetHandle());
        }
        m_whiteBoxSelection->m_localRotation = AZ::Quaternion::CreateIdentity();
    }

    void TransformMode::CreateTranslationManipulators()
    {
        if (!m_whiteBoxSelection)
        {
            return;
        }

        AZ::Transform worldTransform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTransform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        AZStd::shared_ptr<AzToolsFramework::TranslationManipulators> translationManipulators =
            AZStd::make_shared<AzToolsFramework::TranslationManipulators>(
                AzToolsFramework::TranslationManipulators::Dimensions::Three, worldTransform, AZ::Vector3::CreateOne());

        translationManipulators->SetLineBoundWidth(AzToolsFramework::ManipulatorLineBoundWidth());
        translationManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);
        AzToolsFramework::ConfigureTranslationManipulatorAppearance3d(translationManipulators.get());

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        UpdateTransformHandles(whiteBox);
        translationManipulators->SetLocalPosition(m_whiteBoxSelection->m_localPosition);

        auto mouseMoveHandlerFn = [entityComponentIdPair = m_entityComponentIdPair,
                                   transformSelection = m_whiteBoxSelection,
                                   currentManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(translationManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
            {
                const AZ::Vector3 vertexPosition = transformSelection->m_vertexPositions[vertexIndex++] + action.LocalPositionOffset();
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }
            if (auto manipulator = currentManipulator.lock())
            {
                manipulator->SetLocalPosition(transformSelection->m_localPosition + action.LocalPositionOffset());
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);

            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        };

        auto mouseUpHandlerFn = [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
                                 transformSelection = m_whiteBoxSelection,
                                 currentManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(
                                     translationManipulators)](const auto& action)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            mouseMoveHandlerFn(action);

            transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);
            transformSelection->m_localPosition = transformSelection->m_localPosition + action.LocalPositionOffset();
            if (auto manipulator = currentManipulator.lock())
            {
                manipulator->SetLocalPosition(transformSelection->m_localPosition);
            }
            EditorWhiteBoxComponentRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        };

        translationManipulators->InstallLinearManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        translationManipulators->InstallPlanarManipulatorMouseMoveCallback(mouseMoveHandlerFn);
        translationManipulators->InstallSurfaceManipulatorMouseMoveCallback(mouseMoveHandlerFn);

        translationManipulators->InstallSurfaceManipulatorMouseUpCallback(mouseUpHandlerFn);
        translationManipulators->InstallPlanarManipulatorMouseUpCallback(mouseUpHandlerFn);
        translationManipulators->InstallLinearManipulatorMouseUpCallback(mouseUpHandlerFn);

        translationManipulators->Register(AzToolsFramework::GetMainManipulatorManagerId());
        m_manipulator = AZStd::move(translationManipulators);
    }

    void TransformMode::CreateRotationManipulators()
    {
        if (!m_whiteBoxSelection)
        {
            return;
        }
        
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        AZ::Transform worldTranform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTranform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZStd::shared_ptr<AzToolsFramework::RotationManipulators> rotationManipulators =
            AZStd::make_shared<AzToolsFramework::RotationManipulators>(worldTranform);
        rotationManipulators->SetCircleBoundWidth(AzToolsFramework::ManipulatorCicleBoundWidth());
        rotationManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);

        UpdateTransformHandles(whiteBox);
        rotationManipulators->SetLocalPosition(m_whiteBoxSelection->m_localPosition);
        rotationManipulators->SetLocalOrientation(AZ::Quaternion::CreateIdentity());

        rotationManipulators->SetLocalAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        rotationManipulators->ConfigureView(
            AzToolsFramework::RotationManipulatorRadius(),
            AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        auto mouseMoveHandlerFn = [entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection,
             currentManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                size_t vertexIndex = 0;
                for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
                {
                    const AZ::Vector3 vertexPosition =
                        (action.LocalOrientation() * transformSelection->m_localRotation.GetInverseFull())
                            .TransformVector(transformSelection->m_vertexPositions[vertexIndex++] - transformSelection->m_localPosition) +
                        transformSelection->m_localPosition;
                    Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
                }

                if (auto manipulator = currentManipulator.lock())
                {
                    manipulator->SetLocalOrientation(action.LocalOrientation());
                }

                Api::CalculateNormals(*whiteBox);
                Api::CalculatePlanarUVs(*whiteBox);
                EditorWhiteBoxComponentNotificationBus::Event(
                    entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            };

        rotationManipulators->InstallMouseMoveCallback(mouseMoveHandlerFn);
        rotationManipulators->InstallLeftMouseUpCallback(
            [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection,
             currentManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                WhiteBoxMesh* whiteBox = nullptr;
                EditorWhiteBoxComponentRequestBus::EventResult(
                    whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
                mouseMoveHandlerFn(action);

                transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);
                transformSelection->m_localRotation = action.LocalOrientation();
                if (auto manipulator = currentManipulator.lock())
                {
                    manipulator->SetLocalOrientation(transformSelection->m_localRotation);
                }
                EditorWhiteBoxComponentRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
            });

        rotationManipulators->Register(AzToolsFramework::GetMainManipulatorManagerId());
        m_manipulator = AZStd::move(rotationManipulators);
    }

    void TransformMode::CreateScaleManipulators()
    {
        if (!m_whiteBoxSelection)
        {
            return;
        }
        
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        AZ::Transform worldTranform = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(worldTranform, m_entityComponentIdPair.GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        AZStd::shared_ptr<AzToolsFramework::ScaleManipulators> scaleManipulators =
            AZStd::make_shared<AzToolsFramework::ScaleManipulators>(worldTranform);
        scaleManipulators->SetLineBoundWidth(AzToolsFramework::ManipulatorLineBoundWidth());
        scaleManipulators->AddEntityComponentIdPair(m_entityComponentIdPair);
        scaleManipulators->SetAxes(AZ::Vector3::CreateAxisX(), AZ::Vector3::CreateAxisY(), AZ::Vector3::CreateAxisZ());
        scaleManipulators->ConfigureView(
            AzToolsFramework::LinearManipulatorAxisLength(),
            AzFramework::ViewportColors::XAxisColor,
            AzFramework::ViewportColors::YAxisColor,
            AzFramework::ViewportColors::ZAxisColor);

        UpdateTransformHandles(whiteBox);
        scaleManipulators->SetLocalPosition(m_whiteBoxSelection->m_localPosition);

        enum class ScaleType
        {
            Uniform,
            NonUniform
        };

        auto mouseMoveHandlerFn =
            [entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection](const auto& action, ScaleType scaleType)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
            {
                const AZ::Vector3 vertexLocalPosition =
                    (transformSelection->m_vertexPositions[vertexIndex++] - transformSelection->m_localPosition);
                const AZ::Vector3 scale = [&action, &scaleType]
                {
                    switch (scaleType)
                    {
                    case ScaleType::Uniform:
                        return AZ::Vector3(action.LocalScaleOffset().GetZ());
                    case ScaleType::NonUniform:
                        return action.LocalScaleOffset();
                    default:
                        break;
                    }
                    return AZ::Vector3();
                }();
                const AZ::Vector3 manipulatorScale = AZ::Vector3::CreateOne() + (action.m_start.m_sign * scale);
                const AZ::Vector3 vertexPosition = (vertexLocalPosition * manipulatorScale) + transformSelection->m_localPosition;
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        };

        auto mouseUpHandlerFn =
            [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection](const auto& action, ScaleType scaleType)
        {
            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            mouseMoveHandlerFn(action, scaleType);
            transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);

            EditorWhiteBoxComponentRequestBus::Event(entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);
        };

        scaleManipulators->InstallAxisMouseMoveCallback([mouseMoveHandlerFn](const auto& action)
            {
                return mouseMoveHandlerFn(action, ScaleType::NonUniform);
            });
        scaleManipulators->InstallAxisLeftMouseUpCallback([mouseUpHandlerFn](const auto& action)
            {
                return mouseUpHandlerFn(action, ScaleType::NonUniform);
            });

        scaleManipulators->InstallUniformMouseMoveCallback([mouseMoveHandlerFn](const auto& action)
            {
                return mouseMoveHandlerFn(action, ScaleType::Uniform);
            });
        scaleManipulators->InstallUniformLeftMouseUpCallback([mouseUpHandlerFn](const auto& action)
            {
                return mouseUpHandlerFn(action, ScaleType::Uniform);
            });

        scaleManipulators->Register(AzToolsFramework::GetMainManipulatorManagerId());
        m_manipulator = AZStd::move(scaleManipulators);
    }

    void TransformMode::ApplyNumericTransform()
    {
        if (!m_numericInput.IsActive() || !m_whiteBoxSelection)
        {
            m_numericInput.Reset();
            return;
        }

        const float value = m_numericInput.GetValue();
        if (value == 0.0f && m_numericInput.IsEmpty())
        {
            m_numericInput.Reset();
            return;
        }

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        switch (m_numericInput.m_mode)
        {
        case NumericOpMode::Move:
        {
            // Determine translation delta.
            AZ::Vector3 delta;
            if (m_numericInput.m_axis != NumericAxisConstraint::Free)
            {
                delta = m_numericInput.GetAxisVector() * value;
            }
            else
            {
                // No axis locked: treat value as offset along the face/edge normal,
                // or just apply uniformly (user can re-run with an axis if needed).
                delta = AZ::Vector3(value, value, value);
            }

            size_t idx = 0;
            for (const Api::VertexHandle& vh : m_whiteBoxSelection->m_vertexHandles)
            {
                Api::SetVertexPosition(*whiteBox, vh, m_whiteBoxSelection->m_vertexPositions[idx++] + delta);
            }

            // Update cached positions so subsequent operations stack correctly.
            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localPosition   = m_whiteBoxSelection->m_localPosition + delta;
            if (m_manipulator)
            {
                if (auto* tm = dynamic_cast<AzToolsFramework::TranslationManipulators*>(m_manipulator.get()))
                {
                    tm->SetLocalPosition(m_whiteBoxSelection->m_localPosition);
                }
            }
            break;
        }
        case NumericOpMode::Rotate:
        {
            // Rotation axis: explicit constraint or Z if free (Blender default).
            const AZ::Vector3 axis = (m_numericInput.m_axis != NumericAxisConstraint::Free)
                ? m_numericInput.GetAxisVector()
                : AZ::Vector3::CreateAxisZ();

            const AZ::Quaternion rotation = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(value));

            size_t idx = 0;
            for (const Api::VertexHandle& vh : m_whiteBoxSelection->m_vertexHandles)
            {
                const AZ::Vector3 localPos =
                    m_whiteBoxSelection->m_vertexPositions[idx++] - m_whiteBoxSelection->m_localPosition;
                Api::SetVertexPosition(*whiteBox, vh, rotation.TransformVector(localPos) + m_whiteBoxSelection->m_localPosition);
            }

            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, m_whiteBoxSelection->m_vertexHandles);
            m_whiteBoxSelection->m_localRotation   = rotation * m_whiteBoxSelection->m_localRotation;
            if (m_manipulator)
            {
                if (auto* rm = dynamic_cast<AzToolsFramework::RotationManipulators*>(m_manipulator.get()))
                {
                    rm->SetLocalOrientation(m_whiteBoxSelection->m_localRotation);
                }
            }
            break;
        }
        case NumericOpMode::Scale:
        {
            // Per-axis or uniform scale factor.
            AZ::Vector3 scaleFactor;
            if (m_numericInput.m_axis != NumericAxisConstraint::Free)
            {
                // Only scale along the locked axis; leave the others at 1.
                scaleFactor = AZ::Vector3::CreateOne();
                if (m_numericInput.m_axis == NumericAxisConstraint::X) scaleFactor.SetX(value);
                else if (m_numericInput.m_axis == NumericAxisConstraint::Y) scaleFactor.SetY(value);
                else                                                         scaleFactor.SetZ(value);
            }
            else
            {
                scaleFactor = AZ::Vector3(value, value, value);
            }

            size_t idx = 0;
            for (const Api::VertexHandle& vh : m_whiteBoxSelection->m_vertexHandles)
            {
                const AZ::Vector3 localPos =
                    m_whiteBoxSelection->m_vertexPositions[idx++] - m_whiteBoxSelection->m_localPosition;
                Api::SetVertexPosition(*whiteBox, vh, (localPos * scaleFactor) + m_whiteBoxSelection->m_localPosition);
            }

            m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, m_whiteBoxSelection->m_vertexHandles);
            break;
        }
        default:
            break;
        }

        Api::CalculateNormals(*whiteBox);
        Api::CalculatePlanarUVs(*whiteBox);
        EditorWhiteBoxComponentNotificationBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        EditorWhiteBoxComponentRequestBus::Event(m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);

        m_numericInput.Reset();
    }

} // namespace WhiteBox
