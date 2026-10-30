/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxTransformMode.h"
#include "EditorWhiteBoxComponent.h"
#include "Tools/WhiteBoxToolStatus.h"
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include "EditorWhiteBoxComponentModeCommon.h"
#include "EditorWhiteBoxComponentModeTypes.h"
#include "Util/WhiteBoxEditorDrawUtil.h"
#include "Util/WhiteBoxEditorUtil.h"
#include "Util/WhiteBoxSnapUtil.h"

#include <AzCore/Math/IntersectSegment.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/Viewport/ViewportColors.h>
#include <AzFramework/Viewport/CameraState.h>
#include <AzFramework/Viewport/ViewportScreen.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/Viewport/ViewportMessages.h>
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

    // A drag step refreshes the render mesh now and debounces the collider cook and game bake until the drag settles.
    static void PublishDragStep(const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        auto* component = FindWhiteBoxComponent(entityComponentIdPair);
        if (component == nullptr || component->AssetInUse())
        {
            // A shared asset has to tell its other users, which only the full notification does.
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
            return;
        }
        component->RebuildWhiteBoxDeferred();
        EditorWhiteBoxComponentModeRequestBus::Event(
            entityComponentIdPair, &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
    }

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
        for (auto& entry : m_toolStatus) { delete entry.second.data(); }
        m_toolStatus.clear();
        // A drag in flight has edited the live mesh but not serialized it. Leaving now would strand
        // that edit: the component's bytes still hold the pre-drag mesh.
        ClearLatchedDrag();

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

        // G - begin move
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

        // R - begin rotate
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

        // S - begin scale
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

        // X / Y / Z - axis constraint
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

        // Enter - confirm
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

        // Escape - cancel
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
        if (m_loopCutActive || m_knifeActive || m_insertVertexActive) { Refresh(); }
        m_transformType = subModeType;
        RefreshManipulator();
    }

    void TransformMode::Refresh()
    {
        HideToolStatus();
        ClearLatchedDrag();
        m_knifeActive = false;
        m_knifeMesh.reset();
        m_knifeHoverMesh.reset();
        m_knifeSourceBytes.clear();
        m_knifeAnchor.reset();
        m_knifeHover.reset();
        m_knifeLines.clear();
        m_knifeHoverLines.clear();
        m_knifeError.clear();
        m_loopCutActive = false;
        m_loopCutSliding = false;
        m_loopCutSlide = 0.0f;
        m_loopCutSeed = Api::EdgeHandle{};
        m_loopCutLines.clear();
        m_loopCutError.clear();
        m_insertVertexActive = false;
        m_insertVertexEdge = Api::EdgeHandle{};
        m_insertedVertices.clear();
        m_insertVertexError.clear();
        // Refresh is also called after replacing the active layer's working mesh.
        // Hover handles belong to that old mesh just as selection handles do.
        m_whiteBoxSelection.reset();
        DestroyManipulators();
        m_polygonIntersection.reset();
        m_edgeIntersection.reset();
        m_vertexIntersection.reset();
        m_numericInput.Reset();
        SnapUtil::ClearActiveSnapTarget();
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

    void TransformMode::HideToolStatus()
    {
        for (auto& entry : m_toolStatus)
        {
            if (entry.second) { entry.second->HideStatus(); }
        }
    }

    void TransformMode::UpdateToolStatus(const int viewportId)
    {
        // A middle dot from its code point (MSVC without /utf-8 mangles non-ASCII literals).
        const QString dot = QStringLiteral(" ") + QChar(0x00B7) + QStringLiteral(" ");
        QString instructions;
        QString error;
        if (m_knifeActive)
        {
            instructions = QObject::tr("Click add%1Enter apply%1Esc cancel%1Alt orbit").arg(dot);
            error = QString::fromUtf8(m_knifeError.c_str());
        }
        else if (m_loopCutActive)
        {
            instructions = m_loopCutSliding
                ? QObject::tr("Move position%1Click cut%1Esc cancel").arg(dot)
                : QObject::tr("Scroll count%1Click confirm%1Esc cancel").arg(dot);
            const QString count = m_loopCutCount == 1 ? QObject::tr("1 cut") : QObject::tr("%1 cuts").arg(m_loopCutCount);
            instructions.prepend(count + dot);
            error = QString::fromUtf8(m_loopCutError.c_str());
        }
        else if (m_insertVertexActive)
        {
            instructions = QObject::tr("Click edge add%1Ctrl midpoint%1Shift tenths%1Esc finish").arg(dot);
            if (!m_insertedVertices.empty())
            {
                instructions.prepend(QObject::tr("%n added", nullptr, static_cast<int>(m_insertedVertices.size())) + dot);
            }
            error = QString::fromUtf8(m_insertVertexError.c_str());
        }
        else if (m_modelingLatch != TransformModelingLatch::None)
        {
            const bool extrude = m_modelingLatch == TransformModelingLatch::Extrude;
            instructions = (extrude
                ? QObject::tr("Drag polygon/edge%1Ctrl-click select%1Esc cancel")
                : QObject::tr("Drag polygon right%1Ctrl-click select%1Esc cancel")).arg(dot);
            error = QString::fromUtf8(m_latchError.c_str());
        }
        else
        {
            HideToolStatus();
            return;
        }

        auto& status = m_toolStatus[viewportId];
        if (!status)
        {
            QWidget* viewport = nullptr;
            namespace Viewport = AzToolsFramework::ViewportInteraction;
            Viewport::MainEditorViewportInteractionRequestBus::EventResult(
                viewport, viewportId, &Viewport::MainEditorViewportInteractionRequests::GetWidgetForViewportContextMenu);
            if (!viewport) { return; }
            status = new WhiteBoxToolStatus(viewport);
        }
        status->SetStatus(instructions, error);
    }

    void TransformMode::Display(
        [[maybe_unused]] const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZ::Transform& worldFromLocal,
        [[maybe_unused]] const IntersectionAndRenderData& renderData,
        const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        UpdateToolStatus(viewportInfo.m_viewportId);
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // Editing overlays remain visible over the shaded surface, including back faces.
        debugDisplay.DepthTestOff();
        debugDisplay.DepthWriteOff();
        debugDisplay.SetDrawInFrontMode(true);
        debugDisplay.CullOff();
        debugDisplay.PushMatrix(worldFromLocal);

        if (m_knifeActive)
        {
            debugDisplay.SetLineWidth(3.0f);
            debugDisplay.DrawLines(m_knifeLines, AZ::Color(1.0f, 0.8f, 0.1f, 1.0f));
            debugDisplay.DrawLines(m_knifeHoverLines, AZ::Color(0.2f, 1.0f, 0.6f, 1.0f));
            debugDisplay.SetLineWidth(1.0f);
            debugDisplay.PopMatrix();
            debugDisplay.SetColor(AZ::Color(1.0f, 0.8f, 0.1f, 1.0f));
            if (m_knifeAnchor)
            {
                debugDisplay.DrawTextLabel(worldFromLocal.TransformPoint(m_knifeAnchor->m_position), 1.6f, "+", true, 0, 0);
            }
            debugDisplay.SetColor(AZ::Color(0.2f, 1.0f, 0.6f, 1.0f));
            if (m_knifeHover)
            {
                debugDisplay.DrawTextLabel(worldFromLocal.TransformPoint(m_knifeHover->m_position), 1.6f, "+", true, 0, 0);
            }
            return;
        }

        if (m_loopCutActive)
        {
            debugDisplay.SetLineWidth(3.0f);
            debugDisplay.DrawLines(m_loopCutLines, AZ::Color(1.0f, 0.8f, 0.1f, 1.0f));
            debugDisplay.SetLineWidth(1.0f);
            debugDisplay.PopMatrix();
            return;
        }

        if (m_insertVertexActive)
        {
            // Undo can shrink the mesh under the tool, so only still-existing vertices are drawn.
            Api::VertexHandles inserted;
            for (const auto vertex : m_insertedVertices)
            {
                if (whiteBox && static_cast<AZ::u64>(vertex.Index()) < Api::MeshVertexCount(*whiteBox))
                {
                    inserted.push_back(vertex);
                }
            }
            if (whiteBox && !inserted.empty())
            {
                DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, inserted, ed_whiteBoxVertexSelection);
            }
            // Drawn from stored points rather than the edge handle, which an undo can invalidate between frames.
            if (m_insertVertexEdge.IsValid())
            {
                debugDisplay.SetLineWidth(3.0f);
                debugDisplay.SetColor(AZ::Color(0.2f, 1.0f, 0.6f, 1.0f));
                debugDisplay.DrawLine(m_insertVertexLine[0], m_insertVertexLine[1]);
                debugDisplay.SetLineWidth(1.0f);
            }
            debugDisplay.PopMatrix();
            if (m_insertVertexEdge.IsValid())
            {
                debugDisplay.SetColor(AZ::Color(0.2f, 1.0f, 0.6f, 1.0f));
                debugDisplay.DrawTextLabel(worldFromLocal.TransformPoint(m_insertVertexPoint), 1.6f, "+", true, 0, 0);
            }
            return;
        }

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
            DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, handles, ed_whiteBoxVertexHover, true);
        }

        if (m_whiteBoxSelection)
        {
            if (AZStd::holds_alternative<PolygonIntersection>(m_whiteBoxSelection->m_selection))
            {
                DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo,
                    m_whiteBoxSelection->m_vertexHandles, ed_whiteBoxVertexSelection);
                for (const Api::PolygonHandle& polygon : m_whiteBoxSelection->m_polygons)
                {
                    DrawFace(debugDisplay, whiteBox, polygon, ed_whiteBoxPolygonSelection);
                    DrawOutline(debugDisplay, whiteBox, polygon, ed_whiteBoxOutlineSelection);
                }
            }
            else if (AZStd::holds_alternative<EdgeIntersection>(m_whiteBoxSelection->m_selection))
            {
                DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo,
                    m_whiteBoxSelection->m_vertexHandles, ed_whiteBoxVertexSelection);
                for (const Api::EdgeHandle edge : m_whiteBoxSelection->m_edges)
                {
                    if (m_edgeIntersection.value_or(EdgeIntersection{}).GetHandle() != edge)
                    {
                        DrawEdge(debugDisplay, whiteBox, edge, ed_whiteBoxOutlineSelection);
                    }
                }
            }
            else if (AZStd::holds_alternative<VertexIntersection>(m_whiteBoxSelection->m_selection))
            {
                for (const Api::VertexHandle vertex : m_whiteBoxSelection->m_vertices)
                {
                    if (m_vertexIntersection.value_or(VertexIntersection{}).GetHandle() != vertex)
                    {
                        auto handles = AZStd::array<Api::VertexHandle, 1>({vertex});
                        DrawPoints(debugDisplay, whiteBox, worldFromLocal, viewportInfo, handles, ed_whiteBoxVertexSelection);
                    }
                }
            }
        }
        if (m_latchApplied && whiteBox)
        {
            // The mesh itself carries the extrusion, so all this adds is which polygons came out of
            // it - an outline only, because a filled overlay hides the faces being judged by eye.
            for (const auto& polygon : m_latchResultPolygons)
            {
                DrawOutline(debugDisplay, whiteBox, polygon, ed_whiteBoxOutlineSelection);
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

        if (m_boxSelectActive)
        {
            // DrawLine2d takes y-down normalised coords, which is the mouse's own axis: scale, do not flip.
            const auto camera = AzToolsFramework::GetCameraState(viewportInfo.m_viewportId);
            const float width = AZStd::max(aznumeric_cast<float>(camera.m_viewportSize.m_width), 1.0f);
            const float height = AZStd::max(aznumeric_cast<float>(camera.m_viewportSize.m_height), 1.0f);
            const auto normalised = [width, height](const AzFramework::ScreenPoint& point)
            {
                return AZ::Vector2(
                    aznumeric_cast<float>(point.m_x) / width, aznumeric_cast<float>(point.m_y) / height);
            };
            const auto from = normalised(m_boxSelectAnchor);
            const auto to = normalised(m_boxSelectCursor);
            // The two corners the drag does not touch, named for the axis they borrow from the anchor.
            const AZ::Vector2 acrossX(from.GetX(), to.GetY());
            const AZ::Vector2 acrossY(to.GetX(), from.GetY());
            debugDisplay.SetColor(ed_whiteBoxOutlineSelection);
            debugDisplay.SetLineWidth(2.0f);
            debugDisplay.DrawLine2d(from, acrossX, 1.0f);
            debugDisplay.DrawLine2d(acrossX, to, 1.0f);
            debugDisplay.DrawLine2d(to, acrossY, 1.0f);
            debugDisplay.DrawLine2d(acrossY, from, 1.0f);
            debugDisplay.SetLineWidth(1.0f);
        }
    }

    void TransformMode::ApplyBoxSelection(const ModeMouseInteraction& mouse)
    {
        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (whiteBox == nullptr)
        {
            return;
        }

        const auto& interaction = mouse.m_mouseInteraction.m_mouseInteraction;
        const auto camera = AzToolsFramework::GetCameraState(interaction.m_interactionId.m_viewportId);
        const int left = AZStd::min(m_boxSelectAnchor.m_x, m_boxSelectCursor.m_x);
        const int right = AZStd::max(m_boxSelectAnchor.m_x, m_boxSelectCursor.m_x);
        const int top = AZStd::min(m_boxSelectAnchor.m_y, m_boxSelectCursor.m_y);
        const int bottom = AZStd::max(m_boxSelectAnchor.m_y, m_boxSelectCursor.m_y);
        // A box this small is a click that wandered a pixel or two, not a marquee.
        if (right - left < 2 && bottom - top < 2)
        {
            return;
        }
        const auto inside = [&](const AZ::Vector3& local)
        {
            const auto screen = AzFramework::WorldToScreen(mouse.m_worldFromLocal.TransformPoint(local), camera);
            return screen.m_x >= left && screen.m_x <= right && screen.m_y >= top && screen.m_y <= bottom;
        };

        Api::PolygonHandles polygons;
        Api::EdgeHandles edges;
        Api::VertexHandles vertices;
        switch (mouse.m_selectionFilter)
        {
        case SelectionFilter::Vertices:
            for (const auto vertex : Api::MeshVertexHandles(*whiteBox))
            {
                if (inside(Api::VertexPosition(*whiteBox, vertex)))
                {
                    vertices.push_back(vertex);
                }
            }
            break;
        case SelectionFilter::Edges:
            // Both ends have to be in, so a box never takes an edge that only passes through it.
            for (const auto edge : Api::MeshPolygonEdgeHandles(*whiteBox))
            {
                const auto endpoints = Api::EdgeVertexHandles(*whiteBox, edge);
                if (inside(Api::VertexPosition(*whiteBox, endpoints[0])) &&
                    inside(Api::VertexPosition(*whiteBox, endpoints[1])))
                {
                    edges.push_back(edge);
                }
            }
            break;
        default:
            // Polygons, and what an unrestricted box takes: one selection cannot mix the three types.
            for (const auto& polygon : Api::MeshPolygonHandles(*whiteBox))
            {
                if (inside(Api::PolygonMidpoint(*whiteBox, polygon)))
                {
                    polygons.push_back(polygon);
                }
            }
            break;
        }

        if (vertices.empty() && edges.empty() && polygons.empty())
        {
            if (!interaction.m_keyboardModifiers.Ctrl())
            {
                m_whiteBoxSelection.reset();
                DestroyManipulators();
            }
            return;
        }

        // Ctrl adds to what is already selected, the same as Ctrl-clicking does.
        const bool extend = interaction.m_keyboardModifiers.Ctrl();
        const auto restart = [this, extend](const IntersectionSelection& selection)
        {
            if (!extend || !m_whiteBoxSelection || m_whiteBoxSelection->m_selection.index() != selection.index())
            {
                m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
            }
            m_whiteBoxSelection->m_selection = selection;
        };
        const auto merge = [](auto& handles, const auto& found)
        {
            for (const auto& handle : found)
            {
                if (AZStd::find(handles.begin(), handles.end(), handle) == handles.end())
                {
                    handles.push_back(handle);
                }
            }
        };
        if (!vertices.empty())
        {
            restart(VertexIntersection{});
            merge(m_whiteBoxSelection->m_vertices, vertices);
        }
        else if (!edges.empty())
        {
            restart(EdgeIntersection{});
            merge(m_whiteBoxSelection->m_edges, edges);
        }
        else
        {
            restart(PolygonIntersection{});
            merge(m_whiteBoxSelection->m_polygons, polygons);
        }

        m_numericInput.Reset();
        RefreshManipulator();
    }

    bool TransformMode::AddHitToSelection(const ModeMouseInteraction& mouse, const GeometryIntersection hit)
    {
        // Starts a fresh selection when the element type changes, matching what a click does.
        const auto begin = [this](const auto& intersection)
        {
            const IntersectionSelection selection = intersection;
            if (!m_whiteBoxSelection || m_whiteBoxSelection->m_selection.index() != selection.index())
            {
                m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
            }
            m_whiteBoxSelection->m_selection = selection;
        };
        const auto append = [](auto& handles, const auto& handle)
        {
            if (AZStd::find(handles.begin(), handles.end(), handle) != handles.end())
            {
                return false;
            }
            handles.push_back(handle);
            return true;
        };

        bool added = false;
        switch (hit)
        {
        case GeometryIntersection::Polygon:
            if (mouse.m_polygonIntersection.has_value())
            {
                begin(mouse.m_polygonIntersection.value());
                added = append(m_whiteBoxSelection->m_polygons, mouse.m_polygonIntersection->GetHandle());
            }
            break;
        case GeometryIntersection::Edge:
            if (mouse.m_edgeIntersection.has_value())
            {
                begin(mouse.m_edgeIntersection.value());
                added = append(m_whiteBoxSelection->m_edges, mouse.m_edgeIntersection->GetHandle());
            }
            break;
        case GeometryIntersection::Vertex:
            if (mouse.m_vertexIntersection.has_value())
            {
                begin(mouse.m_vertexIntersection.value());
                added = append(m_whiteBoxSelection->m_vertices, mouse.m_vertexIntersection->GetHandle());
            }
            break;
        default:
            break;
        }
        if (added)
        {
            m_numericInput.Reset();
            RefreshManipulator();
        }
        return added;
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

        if (m_knifeActive) { return HandleKnife(mouse); }
        if (m_latchSource) { return HandleLatchedDrag(mouse); }
        if (m_loopCutActive && whiteBox)
        {
            return HandleLoopCut(mouse, *whiteBox);
        }
        if (m_insertVertexActive && whiteBox)
        {
            return HandleInsertVertex(mouse, *whiteBox);
        }

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

        if (whiteBox && m_modelingLatch != TransformModelingLatch::None &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down &&
            mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl() &&
            !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Alt() &&
            BeginLatchedDrag(mouse, *whiteBox, closestIntersection))
        {
            return true;
        }

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

        // Box select owns the drag: down anchors, move stretches, up takes whatever fell inside.
        if (mouse.m_selectionTool == SelectionTool::Box && mouseInteraction.m_mouseInteraction.m_mouseButtons.Left())
        {
            const auto cursor = mouseInteraction.m_mouseInteraction.m_mousePick.m_screenCoordinates;
            const auto event = mouseInteraction.m_mouseEvent;
            if (event == AzToolsFramework::ViewportInteraction::MouseEvent::Down && !mouseOverManipulator)
            {
                // Armed only - a click that never travels falls through and selects as it always did.
                m_boxSelectPending = true;
                m_boxSelectAnchor = cursor;
                m_boxSelectCursor = cursor;
            }
            else if (event == AzToolsFramework::ViewportInteraction::MouseEvent::Move && m_boxSelectPending)
            {
                m_boxSelectCursor = cursor;
                constexpr int travelled = 2;
                m_boxSelectActive = m_boxSelectActive ||
                    AZStd::abs(cursor.m_x - m_boxSelectAnchor.m_x) > travelled ||
                    AZStd::abs(cursor.m_y - m_boxSelectAnchor.m_y) > travelled;
                if (m_boxSelectActive)
                {
                    return true;
                }
            }
            else if (event == AzToolsFramework::ViewportInteraction::MouseEvent::Up)
            {
                const bool dragged = m_boxSelectActive;
                m_boxSelectPending = false;
                m_boxSelectActive = false;
                if (dragged)
                {
                    m_boxSelectCursor = cursor;
                    ApplyBoxSelection(mouse);
                    return true;
                }
            }
        }

        // Sticky paints with the button held, only ever adding, so crossing back does not undo.
        if (mouse.m_selectionTool == SelectionTool::Sticky && !mouseOverManipulator &&
            mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            (mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down ||
             mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Move))
        {
            AddHitToSelection(mouse, closestIntersection);
            return closestIntersection != GeometryIntersection::None;
        }

        if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
            mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Down)
        {
            // Each element type has its own selection. A plain click or a different
            // element type replaces it; Ctrl-click toggles elements of the same type.
            const auto selectElement = [this, &mouseInteraction](const auto& intersection, auto member)
            {
                const bool extend = mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
                const IntersectionSelection selection = intersection;
                if (!extend || !m_whiteBoxSelection || m_whiteBoxSelection->m_selection.index() != selection.index())
                {
                    m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
                }
                m_whiteBoxSelection->m_selection = selection;
                auto& handles = (*m_whiteBoxSelection).*member;
                const auto selected = AZStd::find(handles.begin(), handles.end(), intersection.GetHandle());
                if (selected == handles.end())
                {
                    handles.push_back(intersection.GetHandle());
                }
                else
                {
                    handles.erase(selected);
                }
                m_numericInput.Reset();
                if (handles.empty())
                {
                    m_whiteBoxSelection.reset();
                }
                RefreshManipulator();
            };
            switch (closestIntersection)
            {
            case GeometryIntersection::Polygon:
                if (polygonIntersection.has_value())
                {
                    const bool extend = mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl();
                    if (!extend || !m_whiteBoxSelection ||
                        !AZStd::holds_alternative<PolygonIntersection>(m_whiteBoxSelection->m_selection))
                    {
                        m_whiteBoxSelection = AZStd::make_shared<TransformMode::VertexTransformSelection>();
                    }
                    m_whiteBoxSelection->m_selection = polygonIntersection.value();
                    auto& polygons = m_whiteBoxSelection->m_polygons;
                    const auto selected = AZStd::find(polygons.begin(), polygons.end(), polygonIntersection->GetHandle());
                    if (selected == polygons.end())
                    {
                        polygons.push_back(polygonIntersection->GetHandle());
                    }
                    else
                    {
                        polygons.erase(selected);
                    }
                    m_numericInput.Reset();
                    if (polygons.empty())
                    {
                        m_whiteBoxSelection.reset();
                        DestroyManipulators();
                    }
                    else
                    {
                        RefreshManipulator();
                    }
                }
                break;
            case GeometryIntersection::Edge:
                if (edgeIntersection.has_value())
                {
                    selectElement(edgeIntersection.value(), &VertexTransformSelection::m_edges);
                }
                break;
            case GeometryIntersection::Vertex:
                if (vertexIntersection.has_value())
                {
                    selectElement(vertexIntersection.value(), &VertexTransformSelection::m_vertices);
                }
                break;
            default:
                if (!mouseOverManipulator && !mouseInteraction.m_mouseInteraction.m_keyboardModifiers.Ctrl())
                {
                    m_whiteBoxSelection.reset();
                    m_numericInput.Reset();
                    DestroyManipulators();
                }
                break;
            }
        }

        return false;
    }

    Api::FaceHandles TransformMode::GetHoveredFaces() const
    {
        return m_polygonIntersection.has_value() ? m_polygonIntersection->GetHandle().m_faceHandles : Api::FaceHandles{};
    }

    Api::PolygonHandles TransformMode::GetSelectedPolygons() const
    {
        return m_whiteBoxSelection ? m_whiteBoxSelection->m_polygons : Api::PolygonHandles{};
    }

    Api::EdgeHandles TransformMode::GetSelectedEdges() const
    {
        return m_whiteBoxSelection ? m_whiteBoxSelection->m_edges : Api::EdgeHandles{};
    }

    Api::VertexHandles TransformMode::GetSelectedVertices() const
    {
        return m_whiteBoxSelection ? m_whiteBoxSelection->m_vertices : Api::VertexHandles{};
    }

    bool TransformMode::ExpandEdgeSelection(const bool ring)
    {
        if (m_loopCutActive || !m_whiteBoxSelection || m_whiteBoxSelection->m_edges.empty() ||
            (m_manipulator && m_manipulator->PerformingAction()))
        {
            return false;
        }
        WhiteBoxMesh* mesh = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            mesh, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (!mesh) { return false; }
        auto edges = ring ? Api::FindEdgeRing(*mesh, m_whiteBoxSelection->m_edges)
                          : Api::FindEdgeLoop(*mesh, m_whiteBoxSelection->m_edges);
        if (edges.empty()) { return false; }
        Refresh();
        m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
        EdgeIntersection selection{};
        selection.m_closestEdgeWithHandle.m_handle = edges.front();
        m_whiteBoxSelection->m_selection = selection;
        m_whiteBoxSelection->m_edges = AZStd::move(edges);
        RefreshManipulator();
        return true;
    }

    void TransformMode::SetSelectedPolygons(const Api::PolygonHandles& polygons)
    {
        Refresh();
        if (polygons.empty()) { return; }
        m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
        PolygonIntersection selection{};
        selection.m_closestPolygonWithHandle.m_handle = polygons.front();
        m_whiteBoxSelection->m_selection = selection;
        m_whiteBoxSelection->m_polygons = polygons;
        RefreshManipulator();
    }

    void TransformMode::ClearSelection()
    {
        Refresh();
    }

    void TransformMode::RefreshManipulator()
    {
        // A latch provides direct surface dragging; leave ordinary manipulators available
        // for element types that this operation cannot edit.
        if (m_modelingLatch != TransformModelingLatch::None && m_whiteBoxSelection &&
            (!m_whiteBoxSelection->m_polygons.empty() ||
             (m_modelingLatch == TransformModelingLatch::Extrude && !m_whiteBoxSelection->m_edges.empty())))
        {
            DestroyManipulators();
            WhiteBoxMesh* mesh = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(mesh, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
            if (mesh) { UpdateTransformHandles(mesh); }
            return;
        }
        TransformType activeTransformType = m_transformType;
        if (m_whiteBoxSelection && AZStd::holds_alternative<VertexIntersection>(m_whiteBoxSelection->m_selection) &&
            m_whiteBoxSelection->m_vertices.size() == 1)
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
        auto& handles = m_whiteBoxSelection->m_vertexHandles;
        handles.clear();
        const auto addVertex = [&handles](const Api::VertexHandle vertex)
        {
            if (AZStd::find(handles.begin(), handles.end(), vertex) == handles.end())
            {
                handles.push_back(vertex);
            }
        };
        if (AZStd::holds_alternative<PolygonIntersection>(m_whiteBoxSelection->m_selection))
        {
            for (const Api::PolygonHandle& polygon : m_whiteBoxSelection->m_polygons)
            {
                for (const Api::VertexHandle vertex : Api::PolygonVertexHandles(*mesh, polygon))
                {
                    addVertex(vertex);
                }
            }
        }
        else if (AZStd::holds_alternative<EdgeIntersection>(m_whiteBoxSelection->m_selection))
        {
            for (const Api::EdgeHandle edge : m_whiteBoxSelection->m_edges)
            {
                for (const Api::VertexHandle vertex : Api::EdgeVertexHandles(*mesh, edge))
                {
                    addVertex(vertex);
                }
            }
        }
        else if (AZStd::holds_alternative<VertexIntersection>(m_whiteBoxSelection->m_selection))
        {
            handles = m_whiteBoxSelection->m_vertices;
        }
        m_whiteBoxSelection->m_vertexPositions = Api::VertexPositions(*mesh, handles);
        AZ::Vector3 center = AZ::Vector3::CreateZero();
        for (const AZ::Vector3& position : m_whiteBoxSelection->m_vertexPositions)
        {
            center += position;
        }
        m_whiteBoxSelection->m_localPosition = handles.empty()
            ? AZ::Vector3::CreateZero() : center / static_cast<float>(handles.size());
        m_whiteBoxSelection->m_localRotation = AZ::Quaternion::CreateIdentity();
    }

    bool TransformMode::HandleEscape()
    {
        if (m_insertVertexActive)
        {
            FinishInsertVertex();
            return true;
        }
        if (m_loopCutActive || m_knifeActive)
        {
            Refresh();
            return true;
        }
        if (CancelActiveDrag())
        {
            return true;
        }

        if (m_numericInput.IsActive())
        {
            m_numericInput.Reset();
            return true;
        }

        return false;
    }

    void TransformMode::BeginKnife()
    {
        if (m_knifeActive) { Refresh(); return; }
        CancelActiveDrag();
        Refresh();
        m_modelingLatch = TransformModelingLatch::None;
        auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        if (!component || !component->GetWhiteBoxMesh()) { return; }
        m_knifeMesh = Api::CloneMesh(*component->GetWhiteBoxMesh());
        if (!Api::WriteMesh(*component->GetWhiteBoxMesh(), m_knifeSourceBytes))
        {
            Refresh();
            return;
        }
        m_knifeLayerId = component->GetActiveLayerId();
        m_knifeActive = true;
    }

    bool TransformMode::HandleKnife(const ModeMouseInteraction& mouse)
    {
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        const auto& event = mouse.m_mouseInteraction;
        const auto& interaction = event.m_mouseInteraction;
        const auto& buttons = interaction.m_mouseButtons;
        auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        if (!component || component->GetActiveLayerId() != m_knifeLayerId || !m_knifeMesh)
        {
            Refresh();
            return true;
        }
        if (interaction.m_keyboardModifiers.Alt() || buttons.Middle())
        {
            m_knifeHover.reset();
            m_knifeHoverMesh.reset();
            m_knifeHoverLines.clear();
            return false;
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Right())
        {
            Refresh();
            return true;
        }
        if (event.m_mouseEvent != Viewport::MouseEvent::Move &&
            !(event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Left()))
        {
            return buttons.Left();
        }

        m_knifeHover.reset();
        m_knifeHoverMesh.reset();
        m_knifeHoverLines.clear();
        m_knifeError.clear();
        const auto localFromWorld = mouse.m_worldFromLocal.GetInverse();
        const auto origin = localFromWorld.TransformPoint(interaction.m_mousePick.m_rayOrigin);
        const auto direction = localFromWorld.TransformVector(interaction.m_mousePick.m_rayDirection).GetNormalizedSafe();
        const auto end = origin + direction * 100000.0f;
        AZ::Intersect::SegmentTriangleHitTester tester(origin, end);
        float closest = AZ::Constants::FloatMax;
        // Use the scratch mesh: a later segment can snap to a cut that has not been committed yet.
        for (const auto face : Api::MeshFaceHandles(*m_knifeMesh))
        {
            const auto points = Api::FaceVertexPositions(*m_knifeMesh, face);
            float t = 0.0f;
            AZ::Vector3 normal;
            if (tester.IntersectSegmentTriangle(points[0], points[1], points[2], normal, t) && t < closest)
            {
                closest = t;
                m_knifeHover = Api::KnifePoint{face, origin + (end - origin) * t};
            }
        }
        if (!m_knifeHover) { return true; }

        const auto camera = AzToolsFramework::GetCameraState(interaction.m_interactionId.m_viewportId);
        const auto cursor = interaction.m_mousePick.m_screenCoordinates;
        const auto distanceToCursor = [&](const AZ::Vector3& point)
        {
            const auto screen = AzFramework::WorldToScreen(mouse.m_worldFromLocal.TransformPoint(point), camera);
            const float x = static_cast<float>(screen.m_x - cursor.m_x);
            const float y = static_cast<float>(screen.m_y - cursor.m_y);
            return x * x + y * y;
        };
        const auto polygon = Api::FacePolygonHandle(*m_knifeMesh, m_knifeHover->m_face);
        const auto edges = Api::PolygonBorderEdgeHandlesFlattened(*m_knifeMesh, polygon);
        const auto faceForEdge = [&](const Api::EdgeHandle edge)
        {
            for (const auto face : Api::EdgeFaceHandles(*m_knifeMesh, edge))
            {
                if (AZStd::find(polygon.m_faceHandles.begin(), polygon.m_faceHandles.end(), face) != polygon.m_faceHandles.end())
                {
                    return face;
                }
            }
            return m_knifeHover->m_face;
        };
        // Prefer a vertex within ten screen pixels, then an edge within eight pixels.
        // Project the mouse ray onto the edge in 3D, so perspective does not distort snapping.
        float snapDistance = 100.0f;
        bool vertexSnapped = false;
        for (const auto edge : edges)
        {
            for (const auto vertex : Api::EdgeVertexHandles(*m_knifeMesh, edge))
            {
                const auto point = Api::VertexPosition(*m_knifeMesh, vertex);
                const float distance = distanceToCursor(point);
                if (distance < snapDistance)
                {
                    snapDistance = distance;
                    vertexSnapped = true;
                    *m_knifeHover = Api::KnifePoint{faceForEdge(edge), point};
                }
            }
        }
        if (!vertexSnapped)
        {
            snapDistance = 64.0f;
            for (const auto edge : edges)
            {
                const auto points = Api::EdgeVertexPositions(*m_knifeMesh, edge);
                const auto axis = points[1] - points[0];
                const float dot = axis.Dot(direction);
                const float denominator = axis.GetLengthSq() - dot * dot;
                if (denominator <= axis.GetLengthSq() * 1e-6f) { continue; }
                const auto offset = origin - points[0];
                const float t = AZStd::clamp(
                    (axis.Dot(offset) - dot * direction.Dot(offset)) / denominator, 0.0f, 1.0f);
                const auto point = points[0].Lerp(points[1], t);
                const float distance = distanceToCursor(point);
                if (distance < snapDistance)
                {
                    snapDistance = distance;
                    *m_knifeHover = Api::KnifePoint{faceForEdge(edge), point};
                }
            }
        }

        Api::KnifePoint next = *m_knifeHover;
        if (m_knifeAnchor)
        {
            m_knifeHoverMesh = Api::CloneMesh(*m_knifeMesh);
            if (!Api::KnifeCut(*m_knifeHoverMesh, *m_knifeAnchor, next, direction, m_knifeHoverLines, m_knifeError))
            {
                m_knifeHoverMesh.reset();
            }
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Left())
        {
            if (!m_knifeAnchor) { m_knifeAnchor = m_knifeHover; }
            else if (m_knifeHoverMesh)
            {
                m_knifeMesh = AZStd::move(m_knifeHoverMesh);
                m_knifeLines.insert(m_knifeLines.end(), m_knifeHoverLines.begin(), m_knifeHoverLines.end());
                m_knifeHoverLines.clear();
                m_knifeAnchor = next;
            }
        }
        return true;
    }

    void TransformMode::ConfirmKnife()
    {
        auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        if (!component || component->GetActiveLayerId() != m_knifeLayerId || !m_knifeMesh)
        {
            Refresh();
            return;
        }
        if (m_knifeLines.empty())
        {
            m_knifeError = "Click at least two surface points before applying the cut.";
            return;
        }
        auto* mesh = component->GetWhiteBoxMesh();
        if (!mesh) { Refresh(); return; }
        Api::WhiteBoxMeshStream current;
        if (!Api::WriteMesh(*mesh, current))
        {
            m_knifeError = "Could not verify the source mesh. The cut has not been applied.";
            return;
        }
        if (current != m_knifeSourceBytes)
        {
            Refresh(); // another operation replaced the mesh; never overwrite that edit
            return;
        }
        AzToolsFramework::ScopedUndoBatch undo("White Box Knife");
        Api::AssignMesh(*mesh, *m_knifeMesh);
        Refresh();
        component->BakeParametricLayer(component->GetActiveLayerIndex());
        component->SerializeWhiteBox();
        EditorWhiteBoxComponentNotificationBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
        undo.MarkEntityDirty(m_entityComponentIdPair.GetEntityId());
    }

    void TransformMode::BeginLoopCut()
    {
        CancelActiveDrag();
        Refresh();
        m_loopCutCount = 1;
        m_loopCutActive = true;
    }

    bool TransformMode::HandleLoopCut(const ModeMouseInteraction& mouse, WhiteBoxMesh& mesh)
    {
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        const auto& event = mouse.m_mouseInteraction;
        const auto& buttons = event.m_mouseInteraction.m_mouseButtons;
        if (event.m_mouseInteraction.m_keyboardModifiers.Alt() || buttons.Middle()) { return false; }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Right())
        {
            Refresh();
            return true;
        }
        // Once locked, wheel input must not alter either count or slide position.
        if (event.m_mouseEvent == Viewport::MouseEvent::Wheel && m_loopCutSliding) { return true; }
        bool changed = false;
        if (event.m_mouseEvent == Viewport::MouseEvent::Wheel)
        {
            const float delta = Viewport::MouseWheelDelta(event);
            const int count = AZStd::clamp(m_loopCutCount + (delta > 0.0f ? 1 : delta < 0.0f ? -1 : 0), 1, 64);
            changed = count != m_loopCutCount;
            m_loopCutCount = count;
        }
        Api::EdgeHandle seed = m_loopCutSeed;
        if (m_loopCutSliding)
        {
            const auto cursor = event.m_mouseInteraction.m_mousePick.m_screenCoordinates;
            const AZ::Vector2 point(static_cast<float>(cursor.m_x), static_cast<float>(cursor.m_y));
            const float slide = AZStd::clamp((point - m_loopCutMouseAnchor).Dot(m_loopCutScreenAxis), -1.0f, 1.0f);
            changed = AZStd::abs(slide - m_loopCutSlide) > 1e-5f;
            m_loopCutSlide = slide;
        }
        else
        {
            seed = Api::EdgeHandle{};
            if (mouse.m_edgeIntersection &&
                (!mouse.m_polygonIntersection ||
                 mouse.m_edgeIntersection->m_intersection.m_closestDistance <=
                     mouse.m_polygonIntersection->m_intersection.m_closestDistance + 0.01f))
            {
                seed = mouse.m_edgeIntersection->GetHandle();
            }
            else if (mouse.m_polygonIntersection)
            {
                // Choose the face boundary nearest the hit point, so moving across a
                // quad changes the direction of the preview without selecting it.
                const auto& hit = *mouse.m_polygonIntersection;
                const auto borders = Api::PolygonBorderHalfedgeHandles(mesh, hit.GetHandle());
                float closest = AZ::Constants::FloatMax;
                for (const auto& border : borders)
                {
                    for (const auto halfedge : border)
                    {
                        const auto edge = Api::HalfedgeEdgeHandle(mesh, halfedge);
                        const auto points = Api::EdgeVertexPositions(mesh, edge);
                        const auto direction = points[1] - points[0];
                        const float lengthSq = direction.GetLengthSq();
                        if (lengthSq <= 1e-10f) { continue; }
                        const float t = AZStd::clamp(
                            (hit.m_intersection.m_localIntersectionPoint - points[0]).Dot(direction) / lengthSq, 0.0f, 1.0f);
                        const float distance = (hit.m_intersection.m_localIntersectionPoint - points[0].Lerp(points[1], t)).GetLengthSq();
                        if (distance < closest) { closest = distance; seed = edge; }
                    }
                }
            }
        }
        if (seed != m_loopCutSeed || changed)
        {
            m_loopCutSeed = seed;
            m_loopCutLines.clear();
            m_loopCutError.clear();
            if (seed.IsValid())
            {
                Api::PreviewEdgeLoops(mesh, seed, m_loopCutCount, m_loopCutLines, m_loopCutError, m_loopCutSlide);
            }
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Left() && !m_loopCutLines.empty())
        {
            if (!m_loopCutSliding)
            {
                m_loopCutSliding = true;
                const auto& interaction = event.m_mouseInteraction;
                const auto cursor = interaction.m_mousePick.m_screenCoordinates;
                m_loopCutMouseAnchor = AZ::Vector2(static_cast<float>(cursor.m_x), static_cast<float>(cursor.m_y));
                const auto endpoints = Api::EdgeVertexPositions(mesh, seed);
                const auto camera = AzToolsFramework::GetCameraState(interaction.m_interactionId.m_viewportId);
                const auto start = AzFramework::WorldToScreen(mouse.m_worldFromLocal.TransformPoint(endpoints[0]), camera);
                const auto end = AzFramework::WorldToScreen(mouse.m_worldFromLocal.TransformPoint(endpoints[1]), camera);
                const AZ::Vector2 direction(static_cast<float>(end.m_x - start.m_x), static_cast<float>(end.m_y - start.m_y));
                // Follow the seed edge's projected direction. For an edge viewed
                // end-on, use a horizontal gesture rather than divide by zero.
                m_loopCutScreenAxis = direction.GetLengthSq() >= 16.0f
                    ? direction * (static_cast<float>(m_loopCutCount + 1) / (0.98f * direction.GetLengthSq()))
                    : AZ::Vector2(1.0f / 200.0f, 0.0f);
                return true;
            }
            AZ::Entity* entity = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(
                entity, &AZ::ComponentApplicationRequests::FindEntity, m_entityComponentIdPair.GetEntityId());
            auto* component = entity
                ? azrtti_cast<EditorWhiteBoxComponent*>(entity->FindComponent(m_entityComponentIdPair.GetComponentId())) : nullptr;
            if (!component) { return true; }
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Loop Cut");
            if (!Api::InsertEdgeLoops(mesh, seed, m_loopCutCount, m_loopCutError, m_loopCutSlide)) { return true; }
            Refresh();
            component->BakeParametricLayer(component->GetActiveLayerIndex());
            component->SerializeWhiteBox();
            EditorWhiteBoxComponentNotificationBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            undoBatch.MarkEntityDirty(m_entityComponentIdPair.GetEntityId());
        }
        // Retain viewport orbit/pan while consuming cut and wheel input.
        return event.m_mouseEvent == Viewport::MouseEvent::Wheel || buttons.Left() ||
            event.m_mouseEvent == Viewport::MouseEvent::Move;
    }

    void TransformMode::BeginInsertVertex()
    {
        // The button toggles, so a second press finishes the way Escape does.
        if (m_insertVertexActive)
        {
            FinishInsertVertex();
            return;
        }
        CancelActiveDrag();
        Refresh();
        m_modelingLatch = TransformModelingLatch::None;
        m_insertVertexActive = true;
    }

    void TransformMode::FinishInsertVertex()
    {
        Api::VertexHandles inserted = AZStd::move(m_insertedVertices);
        Refresh();
        WhiteBoxMesh* mesh = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            mesh, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (mesh == nullptr)
        {
            return;
        }
        // An undo inside the tool can take some of them away again.
        const AZ::u64 vertexCount = Api::MeshVertexCount(*mesh);
        inserted.erase(
            AZStd::remove_if(
                inserted.begin(), inserted.end(),
                [vertexCount, mesh](const Api::VertexHandle vertex)
                {
                    return static_cast<AZ::u64>(vertex.Index()) >= vertexCount || Api::VertexIsHidden(*mesh, vertex);
                }),
            inserted.end());
        if (!inserted.empty())
        {
            SetSelectedVertices(inserted);
        }
    }

    bool TransformMode::HandleInsertVertex(const ModeMouseInteraction& mouse, WhiteBoxMesh& mesh)
    {
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        const auto& event = mouse.m_mouseInteraction;
        const auto& interaction = event.m_mouseInteraction;
        const auto& buttons = interaction.m_mouseButtons;
        if (interaction.m_keyboardModifiers.Alt() || buttons.Middle() || event.m_mouseEvent == Viewport::MouseEvent::Wheel)
        {
            return false; // orbit, pan and zoom keep working
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Right())
        {
            FinishInsertVertex();
            return true;
        }

        // The hovered edge, or the hovered polygon's border edge nearest the hit, picked the way loop cut picks its seed.
        Api::EdgeHandle edge;
        if (mouse.m_edgeIntersection &&
            (!mouse.m_polygonIntersection ||
             mouse.m_edgeIntersection->m_intersection.m_closestDistance <=
                 mouse.m_polygonIntersection->m_intersection.m_closestDistance + 0.01f))
        {
            edge = mouse.m_edgeIntersection->GetHandle();
        }
        else if (mouse.m_polygonIntersection)
        {
            const auto& hit = *mouse.m_polygonIntersection;
            float closest = AZ::Constants::FloatMax;
            for (const auto candidate : Api::PolygonBorderEdgeHandlesFlattened(mesh, hit.GetHandle()))
            {
                const auto points = Api::EdgeVertexPositions(mesh, candidate);
                const auto direction = points[1] - points[0];
                const float lengthSq = direction.GetLengthSq();
                if (lengthSq <= 1e-10f)
                {
                    continue;
                }
                const float t = AZStd::clamp(
                    (hit.m_intersection.m_localIntersectionPoint - points[0]).Dot(direction) / lengthSq, 0.0f, 1.0f);
                const float distance = (hit.m_intersection.m_localIntersectionPoint - points[0].Lerp(points[1], t)).GetLengthSq();
                if (distance < closest)
                {
                    closest = distance;
                    edge = candidate;
                }
            }
        }

        m_insertVertexEdge = Api::EdgeHandle{};
        if (edge.IsValid())
        {
            // The point on the edge nearest the cursor ray, in the mesh's own space.
            const AZ::Transform localFromWorld = mouse.m_worldFromLocal.GetInverse();
            const AZ::Vector3 origin = localFromWorld.TransformPoint(interaction.m_mousePick.m_rayOrigin);
            const AZ::Vector3 ray = localFromWorld.TransformVector(interaction.m_mousePick.m_rayDirection).GetNormalizedSafe();
            const auto points = Api::EdgeVertexPositions(mesh, edge);
            const AZ::Vector3 along = points[1] - points[0];
            const AZ::Vector3 gap = points[0] - origin;
            const float a = along.Dot(along);
            const float b = along.Dot(ray);
            const float denominator = a - b * b;
            float fraction = denominator > 1e-10f ? (b * ray.Dot(gap) - along.Dot(gap)) / denominator : 0.5f;
            fraction = AZStd::clamp(fraction, 0.0f, 1.0f);
            if (interaction.m_keyboardModifiers.Ctrl())
            {
                fraction = 0.5f;
            }
            else if (interaction.m_keyboardModifiers.Shift())
            {
                fraction = static_cast<float>(static_cast<int>(fraction * 10.0f + 0.5f)) / 10.0f; // fraction is never negative here
            }
            m_insertVertexEdge = edge;
            m_insertVertexFraction = fraction;
            m_insertVertexLine = { points[0], points[1] };
            m_insertVertexPoint = points[0].Lerp(points[1], fraction);
        }

        if (event.m_mouseEvent == Viewport::MouseEvent::Down && buttons.Left())
        {
            if (!m_insertVertexEdge.IsValid())
            {
                m_insertVertexError = "Hover a polygon edge to place the vertex.";
                return true;
            }
            auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
            if (!component)
            {
                return true;
            }
            AzToolsFramework::ScopedUndoBatch undoBatch("White Box Insert Vertex");
            const auto vertex = Api::InsertVertexOnEdge(mesh, m_insertVertexEdge, m_insertVertexFraction, m_insertVertexError);
            if (!vertex.IsValid())
            {
                return true;
            }
            m_insertedVertices.push_back(vertex);
            // The split edge is gone; the next move picks whichever half is under the cursor.
            m_insertVertexEdge = Api::EdgeHandle{};
            component->BakeParametricLayer(component->GetActiveLayerIndex());
            component->SerializeWhiteBox();
            EditorWhiteBoxComponentNotificationBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            undoBatch.MarkEntityDirty(m_entityComponentIdPair.GetEntityId());
            return true;
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Move && m_insertVertexEdge.IsValid())
        {
            m_insertVertexError.clear();
        }
        return buttons.Left() || event.m_mouseEvent == Viewport::MouseEvent::Move;
    }

    void TransformMode::PublishLatchMesh()
    {
        // Deferred rebuild: the collider cook and bake wait for OnTick instead of every mouse move.
        if (auto* component = FindWhiteBoxComponent(m_entityComponentIdPair))
        {
            component->RebuildWhiteBoxDeferred();
        }
        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair,
            &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
    }

    void TransformMode::RestoreLatchSource()
    {
        WhiteBoxMesh* mesh = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            mesh, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (mesh != nullptr && m_latchSource)
        {
            Api::AssignMesh(*mesh, *m_latchSource);
            PublishLatchMesh();
        }
    }

    bool TransformMode::ApplyLatch(const float amount, const AZ::Vector3& edgeOffset, AZStd::string& error)
    {
        WhiteBoxMesh* mesh = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            mesh, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        if (mesh == nullptr || !m_latchSource)
        {
            return false;
        }

        // Back to the mesh the drag started from every time: the stored handles index into it, and
        // running the operation on the previous step's result would extrude the extrusion.
        Api::AssignMesh(*mesh, *m_latchSource);
        bool applied = false;
        if (!m_latchPolygons.empty())
        {
            applied = Api::ExtrudeInsetRegions(
                *mesh, m_latchPolygons, amount, m_modelingLatch == TransformModelingLatch::Inset,
                m_latchResultPolygons, error);
        }
        else
        {
            Api::PolygonHandles sideFaces;
            applied = Api::ExtrudeEdgeSelection(
                *mesh, m_latchEdges, edgeOffset, m_latchResultEdges, sideFaces, error);
        }
        // Both operations are transactional, so a refusal leaves the mesh exactly at the source -
        // publish either way, or the viewport keeps showing the step before this one.
        PublishLatchMesh();
        return applied;
    }

    void TransformMode::ClearLatchedDrag()
    {
        // Anything still applied is an abandoned drag. The commit path releases the source first, so
        // this never undoes what it just wrote.
        if (m_latchApplied)
        {
            RestoreLatchSource();
        }
        m_latchSource.reset();
        m_latchPolygons.clear();
        m_latchEdges.clear();
        m_latchResultPolygons.clear();
        m_latchResultEdges.clear();
        m_latchAmount = 0.0f;
        m_latchEdgeOffset = AZ::Vector3::CreateZero();
        m_latchApplied = false;
        m_latchError.clear();
    }

    void TransformMode::SetModelingLatch(const TransformModelingLatch latch)
    {
        CancelActiveDrag();
        m_numericInput.Reset();
        HideToolStatus();
        m_modelingLatch = latch;
        if (m_loopCutActive || m_knifeActive || m_insertVertexActive) { Refresh(); }
        RefreshManipulator();
    }

    void TransformMode::SetSelectedEdges(const Api::EdgeHandles& edges)
    {
        Refresh();
        if (edges.empty()) { return; }
        m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
        EdgeIntersection selection{};
        selection.m_closestEdgeWithHandle.m_handle = edges.front();
        m_whiteBoxSelection->m_selection = selection;
        m_whiteBoxSelection->m_edges = edges;
        RefreshManipulator();
    }

    void TransformMode::SetSelectedVertices(const Api::VertexHandles& vertices)
    {
        Refresh();
        if (vertices.empty()) { return; }
        m_whiteBoxSelection = AZStd::make_shared<VertexTransformSelection>();
        VertexIntersection selection{};
        selection.m_closestVertexWithHandle.m_handle = vertices.front();
        m_whiteBoxSelection->m_selection = selection;
        m_whiteBoxSelection->m_vertices = vertices;
        RefreshManipulator();
    }

    bool TransformMode::BeginLatchedDrag(
        const ModeMouseInteraction& mouse, WhiteBoxMesh& mesh, const GeometryIntersection hit)
    {
        auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        if (!component) { return false; }
        AZ::Vector3 localHit;
        if (hit == GeometryIntersection::Polygon && mouse.m_polygonIntersection)
        {
            const auto polygon = mouse.m_polygonIntersection->GetHandle();
            if (!m_whiteBoxSelection ||
                AZStd::find(m_whiteBoxSelection->m_polygons.begin(), m_whiteBoxSelection->m_polygons.end(), polygon)
                    == m_whiteBoxSelection->m_polygons.end())
            {
                SetSelectedPolygons({polygon});
            }
            localHit = mouse.m_polygonIntersection->m_intersection.m_localIntersectionPoint;
        }
        else if (hit == GeometryIntersection::Edge && mouse.m_edgeIntersection &&
            m_modelingLatch == TransformModelingLatch::Extrude)
        {
            const auto edge = mouse.m_edgeIntersection->GetHandle();
            if (!m_whiteBoxSelection ||
                AZStd::find(m_whiteBoxSelection->m_edges.begin(), m_whiteBoxSelection->m_edges.end(), edge)
                    == m_whiteBoxSelection->m_edges.end())
            {
                SetSelectedEdges({edge});
            }
            localHit = mouse.m_edgeIntersection->m_intersection.m_localIntersectionPoint;
        }
        else { return false; }

        ClearLatchedDrag();
        m_numericInput.Reset();
        m_latchSource = Api::CloneMesh(mesh);
        if (!m_latchSource) { ClearLatchedDrag(); return false; }
        m_latchLayerId = component->GetActiveLayerId();
        m_latchPolygons = m_whiteBoxSelection->m_polygons;
        m_latchEdges = m_whiteBoxSelection->m_edges;
        m_latchWorldFromLocal = mouse.m_worldFromLocal;
        m_latchAnchor = m_latchWorldFromLocal.TransformPoint(localHit);
        const auto& interaction = mouse.m_mouseInteraction.m_mouseInteraction;
        const auto& pick = interaction.m_mousePick;
        m_latchStartScreen = AZ::Vector2(float(pick.m_screenCoordinates.m_x), float(pick.m_screenCoordinates.m_y));
        m_latchPlaneNormal = pick.m_rayDirection.GetNormalized();
        const auto camera = AzToolsFramework::GetCameraState(interaction.m_interactionId.m_viewportId);
        m_latchFallbackScale = AzToolsFramework::CalculateScreenToWorldMultiplier(m_latchAnchor, camera) * 0.01f /
            AZStd::max(AZStd::abs(m_latchWorldFromLocal.GetUniformScale()), 1e-6f);
        AZ::Vector3 normal = AZ::Vector3::CreateZero();
        AZStd::optional<AZ::Vector3> minimum;
        AZ::Vector3 maximum = AZ::Vector3::CreateZero();
        const auto grow = [&minimum, &maximum](const AZ::Vector3& position)
        {
            if (!minimum)
            {
                minimum = position;
                maximum = position;
                return;
            }
            minimum = minimum->GetMin(position);
            maximum = maximum.GetMax(position);
        };
        for (const auto& polygon : m_latchPolygons)
        {
            for (const auto face : polygon.m_faceHandles)
            {
                const auto p = Api::FaceVertexPositions(mesh, face);
                normal += (p[1] - p[0]).Cross(p[2] - p[0]);
                for (const auto& position : p)
                {
                    grow(position);
                }
            }
        }
        for (const auto edge : m_latchEdges)
        {
            for (const auto vertex : Api::EdgeVertexHandles(mesh, edge))
            {
                grow(Api::VertexPosition(mesh, vertex));
            }
        }
        normal = normal.GetNormalizedSafe();
        m_latchExtent = minimum ? AZStd::max((maximum - minimum.value()).GetLength(), 1e-6f) : 1.0f;
        const auto screenStart = AzFramework::WorldToScreen(m_latchAnchor, camera);
        const auto screenEnd = AzFramework::WorldToScreen(
            m_latchAnchor + m_latchWorldFromLocal.TransformVector(normal), camera);
        m_latchScreenNormal = AZ::Vector2(float(screenEnd.m_x - screenStart.m_x), float(screenEnd.m_y - screenStart.m_y));
        return true;
    }

    bool TransformMode::HandleLatchedDrag(const ModeMouseInteraction& mouse)
    {
        namespace Viewport = AzToolsFramework::ViewportInteraction;
        auto* component = FindWhiteBoxComponent(m_entityComponentIdPair);
        if (!component || component->GetActiveLayerId() != m_latchLayerId)
        {
            ClearLatchedDrag();
            return true;
        }
        const auto& event = mouse.m_mouseInteraction;
        const auto& interaction = event.m_mouseInteraction;
        if (interaction.m_keyboardModifiers.Alt())
        {
            ClearLatchedDrag();
            return false;
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Down && interaction.m_mouseButtons.Right())
        {
            ClearLatchedDrag();
            return true;
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Move)
        {
            const auto& pick = interaction.m_mousePick;
            const AZ::Vector2 delta = AZ::Vector2(float(pick.m_screenCoordinates.m_x), float(pick.m_screenCoordinates.m_y))
                - m_latchStartScreen;
            if (delta.GetLengthSq() < 9.0f) { return true; } // A click selects without adding topology.

            float amount = 0.0f;
            AZ::Vector3 offset = AZ::Vector3::CreateZero();
            if (!m_latchPolygons.empty())
            {
                const bool inset = m_modelingLatch == TransformModelingLatch::Inset;
                amount = inset ? AZStd::clamp(delta.GetX() / 200.0f, 0.0f, 0.99f)
                    : (m_latchScreenNormal.GetLengthSq() > 4.0f
                        ? delta.Dot(m_latchScreenNormal) / m_latchScreenNormal.GetLengthSq()
                        : -delta.GetY() * m_latchFallbackScale);
            }
            else
            {
                const float denominator = pick.m_rayDirection.Dot(m_latchPlaneNormal);
                if (AZStd::abs(denominator) < 1e-6f) { return true; }
                const float t = (m_latchAnchor - pick.m_rayOrigin).Dot(m_latchPlaneNormal) / denominator;
                if (t <= 0.0f) { return true; }
                offset = m_latchWorldFromLocal.GetInverse().TransformVector(
                    pick.m_rayOrigin + pick.m_rayDirection * t - m_latchAnchor);
            }

            // Back at the start means no extrusion, not a sliver.
            const bool moved = m_latchPolygons.empty()
                ? !offset.IsZero(m_latchExtent * 1e-3f)
                : AZStd::abs(amount) > (m_modelingLatch == TransformModelingLatch::Inset ? 1e-3f : m_latchExtent * 1e-3f);
            if (!moved)
            {
                if (m_latchApplied)
                {
                    RestoreLatchSource();
                    m_latchApplied = false;
                }
                m_latchError.clear();
                return true;
            }

            AZStd::string error;
            if (ApplyLatch(amount, offset, error))
            {
                m_latchAmount = amount;
                m_latchEdgeOffset = offset;
                m_latchApplied = true;
                m_latchError.clear();
            }
            else
            {
                // Hold the last amount that worked. Without this the mesh flashes back to flat for as
                // long as the cursor sits past whatever the operation refuses.
                m_latchError = error;
                if (m_latchApplied)
                {
                    AZStd::string ignored;
                    ApplyLatch(m_latchAmount, m_latchEdgeOffset, ignored);
                }
            }
            return true;
        }
        if (event.m_mouseEvent == Viewport::MouseEvent::Up && interaction.m_mouseButtons.Left())
        {
            if (!m_latchApplied)
            {
                ClearLatchedDrag();
                return true;
            }

            // The mesh already holds the result - this is only where it becomes permanent. Releasing
            // the source first is what stops the Refresh below putting the pre-drag mesh back.
            const auto polygons = m_latchResultPolygons;
            const auto edges = m_latchResultEdges;
            m_latchSource.reset();
            m_latchApplied = false;

            AzToolsFramework::ScopedUndoBatch undo(
                m_modelingLatch == TransformModelingLatch::Inset ? "White Box Drag Inset" : "White Box Drag Extrude");
            Refresh();
            component->BakeParametricLayer(component->GetActiveLayerIndex());
            component->SerializeWhiteBox();
            EditorWhiteBoxComponentNotificationBus::Event(
                m_entityComponentIdPair, &EditorWhiteBoxComponentNotifications::OnWhiteBoxMeshModified);
            if (!polygons.empty()) { SetSelectedPolygons(polygons); }
            else { SetSelectedEdges(edges); }
            undo.MarkEntityDirty(m_entityComponentIdPair.GetEntityId());
            return true;
        }
        return true;
    }

    bool TransformMode::CancelActiveDrag()
    {
        if (m_latchSource)
        {
            ClearLatchedDrag();
            return true;
        }
        if (!m_whiteBoxSelection || !m_manipulator || !m_manipulator->PerformingAction() ||
            m_whiteBoxSelection->m_dragCancelled)
        {
            return false;
        }

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        if (whiteBox == nullptr)
        {
            return false;
        }

        // Transform mode never changes topology, so restoring the start positions is a full revert.
        size_t vertexIndex = 0;
        for (const Api::VertexHandle& vertexHandle : m_whiteBoxSelection->m_vertexHandles)
        {
            Api::SetVertexPosition(*whiteBox, vertexHandle, m_whiteBoxSelection->m_vertexPositions[vertexIndex++]);
        }

        Api::CalculateNormals(*whiteBox);
        Api::CalculatePlanarUVs(*whiteBox);

        // The manipulators hold their own interaction state until the mouse is released, so rather
        // than trying to abort them we latch a flag and ignore movement for the rest of the drag.
        m_whiteBoxSelection->m_dragCancelled = true;
        m_whiteBoxSelection->m_snapOffset = AZ::Vector3::CreateZero();
        m_whiteBoxSelection->m_snapAnchorResolved = false;
        m_whiteBoxSelection->m_snapAnchorIndex.reset();
        SnapUtil::ClearActiveSnapTarget();

        m_manipulator->SetLocalPosition(m_whiteBoxSelection->m_localPosition);

        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair,
            &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);

        EditorWhiteBoxComponentNotificationBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);

        return true;
    }

    void TransformMode::CreateTranslationManipulators()
    {
        if (!m_whiteBoxSelection)
        {
            return;
        }

        // The same space as every other White Box manipulator (layer transform included).
        const AZ::Transform worldTransform = EditorSpaceFromLocal(m_entityComponentIdPair);
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
            // the drag was abandoned with Escape - ignore movement until the button is released
            if (transformSelection->m_dragCancelled)
            {
                return;
            }

            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            // vertex snapping: resolve the anchor on the first move of this drag (there is no
            // mouse-down callback on TranslationManipulators), then hold it.
            if (!transformSelection->m_snapAnchorResolved)
            {
                transformSelection->m_snapAnchorIndex = SnapUtil::SnappingActive()
                    ? SnapUtil::FindAnchorIndex(
                          entityComponentIdPair.GetEntityId(), SnapUtil::ActiveViewportId(),
                          transformSelection->m_vertexPositions)
                    : AZStd::nullopt;
                transformSelection->m_snapAnchorResolved = true;
            }

            // Pull the whole selection rigidly so the anchor vertex lands on the target under the
            // cursor. Recomputed from the drag-start positions each frame, so it is not cumulative.
            AZStd::optional<AZ::Vector3> snapTarget;
            AZ::Vector3 snapOffset = AZ::Vector3::CreateZero();

            if (transformSelection->m_snapAnchorIndex.has_value() &&
                transformSelection->m_snapAnchorIndex.value() < transformSelection->m_vertexPositions.size())
            {
                snapTarget = SnapUtil::FindSnapTargetWorld(
                    entityComponentIdPair.GetEntityId(), SnapUtil::ActiveViewportId(),
                    SnapUtil::ExcludeIndicesFromHandles(transformSelection->m_vertexHandles));

                if (snapTarget.has_value())
                {
                    const AZ::Vector3 anchorUnsnapped =
                        transformSelection->m_vertexPositions[transformSelection->m_snapAnchorIndex.value()] +
                        action.LocalPositionOffset();

                    snapOffset =
                        SnapUtil::MeshLocalFromWorld(entityComponentIdPair.GetEntityId(), snapTarget.value()) -
                        anchorUnsnapped;
                }
            }

            SnapUtil::SetActiveSnapTarget(snapTarget);
            transformSelection->m_snapOffset = snapOffset;

            size_t vertexIndex = 0;
            for (const Api::VertexHandle& vertexHandle : transformSelection->m_vertexHandles)
            {
                const AZ::Vector3 vertexPosition =
                    transformSelection->m_vertexPositions[vertexIndex++] + action.LocalPositionOffset() + snapOffset;
                Api::SetVertexPosition(*whiteBox, vertexHandle, vertexPosition);
            }
            if (auto manipulator = currentManipulator.lock())
            {
                manipulator->SetLocalPosition(
                    transformSelection->m_localPosition + action.LocalPositionOffset() + snapOffset);
            }

            Api::CalculateNormals(*whiteBox);
            Api::CalculatePlanarUVs(*whiteBox);

            PublishDragStep(entityComponentIdPair);
        };

        auto mouseUpHandlerFn = [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
                                 transformSelection = m_whiteBoxSelection,
                                 currentManipulator = AZStd::weak_ptr<AzToolsFramework::TranslationManipulators>(
                                     translationManipulators)](const auto& action)
        {
            // reverted by Escape - nothing to commit, just clear the latch for the next drag
            if (transformSelection->m_dragCancelled)
            {
                transformSelection->m_dragCancelled = false;
                SnapUtil::ClearActiveSnapTarget();
                return;
            }

            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            mouseMoveHandlerFn(action);

            transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);
            // m_snapOffset is whatever the move above just applied - fold it in or the gizmo pivot
            // ends up displaced from the geometry it just moved.
            transformSelection->m_localPosition =
                transformSelection->m_localPosition + action.LocalPositionOffset() + transformSelection->m_snapOffset;
            if (auto manipulator = currentManipulator.lock())
            {
                manipulator->SetLocalPosition(transformSelection->m_localPosition);
            }

            transformSelection->m_snapAnchorResolved = false;
            transformSelection->m_snapAnchorIndex.reset();
            transformSelection->m_snapOffset = AZ::Vector3::CreateZero();
            SnapUtil::ClearActiveSnapTarget();

            // The drag only refreshed what is on screen; release pays for the collider and game bake once.
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
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

        // The selection pivot is in layer-local space, just as it is for Move.
        // Include the active layer transform when displaying and interacting with the gizmo.
        const AZ::Transform worldTransform = EditorSpaceFromLocal(m_entityComponentIdPair);

        AZStd::shared_ptr<AzToolsFramework::RotationManipulators> rotationManipulators =
            AZStd::make_shared<AzToolsFramework::RotationManipulators>(worldTransform);
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
                // the drag was abandoned with Escape - ignore movement until the button is released
                if (transformSelection->m_dragCancelled)
                {
                    return;
                }

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
                PublishDragStep(entityComponentIdPair);
            };

        rotationManipulators->InstallMouseMoveCallback(mouseMoveHandlerFn);
        rotationManipulators->InstallLeftMouseUpCallback(
            [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection,
             currentManipulator = AZStd::weak_ptr<AzToolsFramework::RotationManipulators>(rotationManipulators)](
                const AzToolsFramework::AngularManipulator::Action& action)
            {
                // reverted by Escape - nothing to commit, just clear the latch for the next drag
                if (transformSelection->m_dragCancelled)
                {
                    transformSelection->m_dragCancelled = false;
                    SnapUtil::ClearActiveSnapTarget();
                    return;
                }

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
                // The drag only refreshed what is on screen; release pays for the collider and game bake once.
                EditorWhiteBoxComponentNotificationBus::Event(
                    entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
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

        // The selection pivot is in layer-local space, just as it is for Move.
        // Include the active layer transform when displaying and interacting with the gizmo.
        const AZ::Transform worldTransform = EditorSpaceFromLocal(m_entityComponentIdPair);

        AZStd::shared_ptr<AzToolsFramework::ScaleManipulators> scaleManipulators =
            AZStd::make_shared<AzToolsFramework::ScaleManipulators>(worldTransform);
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
            // the drag was abandoned with Escape - ignore movement until the button is released
            if (transformSelection->m_dragCancelled)
            {
                return;
            }

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
            PublishDragStep(entityComponentIdPair);
        };

        auto mouseUpHandlerFn =
            [mouseMoveHandlerFn, entityComponentIdPair = m_entityComponentIdPair,
             transformSelection = m_whiteBoxSelection](const auto& action, ScaleType scaleType)
        {
            // reverted by Escape - nothing to commit, just clear the latch for the next drag
            if (transformSelection->m_dragCancelled)
            {
                transformSelection->m_dragCancelled = false;
                SnapUtil::ClearActiveSnapTarget();
                return;
            }

            WhiteBoxMesh* whiteBox = nullptr;
            EditorWhiteBoxComponentRequestBus::EventResult(
                whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

            mouseMoveHandlerFn(action, scaleType);
            transformSelection->m_vertexPositions = Api::VertexPositions(*whiteBox, transformSelection->m_vertexHandles);

            // The drag only refreshed what is on screen; release pays for the collider and game bake once.
            EditorWhiteBoxComponentNotificationBus::Event(
                entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
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
