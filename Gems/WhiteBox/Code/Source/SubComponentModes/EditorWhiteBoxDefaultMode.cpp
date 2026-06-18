/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "EditorWhiteBoxComponentModeCommon.h"
#include <EditorWhiteBoxComponentModeBus.h>
#include "EditorWhiteBoxDefaultMode.h"

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/MathUtils.h>
#include "Util/WhiteBoxMathUtil.h"
#include "Viewport/WhiteBoxEdgeScaleModifier.h"
#include "Viewport/WhiteBoxEdgeTranslationModifier.h"
#include "Viewport/WhiteBoxPolygonScaleModifier.h"
#include "Viewport/WhiteBoxPolygonTranslationModifier.h"
#include "Viewport/WhiteBoxVertexTranslationModifier.h"
#include "Viewport/WhiteBoxViewportConstants.h"

#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>
#include <AzToolsFramework/ActionManager/HotKey/HotKeyManagerInterface.h>
#include <AzToolsFramework/API/ComponentModeCollectionInterface.h>
#include <AzToolsFramework/ComponentMode/EditorComponentModeBus.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorContextIdentifiers.h>
#include <AzToolsFramework/Editor/ActionManagerIdentifiers/EditorMenuIdentifiers.h>
#include <AzToolsFramework/ViewportSelection/EditorSelectionUtil.h>
#include <QKeySequence>
#include <QLayout>
#include <WhiteBox/EditorWhiteBoxComponentBus.h>

namespace WhiteBox
{
    AZ_CVAR(
        float, cl_whiteBoxVertexIndicatorLength, 0.1f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The length of each vertex indicator axis");
    AZ_CVAR(
        float, cl_whiteBoxVertexIndicatorWidth, 5.0f, nullptr, AZ::ConsoleFunctorFlags::Null,
        "The width/thickness of each vertex indicator axis");
    AZ_CVAR(
        AZ::Color, cl_whiteBoxVertexIndicatorColor, AZ::Color::CreateFromRgba(0, 0, 0, 102), nullptr,
        AZ::ConsoleFunctorFlags::Null, "The color of the vertex indicator");

    static const AZ::Crc32 HideEdge = AZ_CRC_CE("org.o3de.action.whitebox.hide_edge");
    static const AZ::Crc32 HideVertex = AZ_CRC_CE("org.o3de.action.whitebox.hide_vertex");

    // Numeric input action identifiers (Default mode)
    constexpr AZStd::string_view DefaultNumericBeginMoveId    = "o3de.action.whiteBoxDefault.numeric.beginMove";
    constexpr AZStd::string_view DefaultNumericBeginScaleId   = "o3de.action.whiteBoxDefault.numeric.beginScale";
    constexpr AZStd::string_view DefaultNumericBeginRotateId  = "o3de.action.whiteBoxDefault.numeric.beginRotate";
    constexpr AZStd::string_view DefaultNumericBeginExtrudeId = "o3de.action.whiteBoxDefault.numeric.beginExtrude";
    constexpr AZStd::string_view DefaultNumericBeginInsetId   = "o3de.action.whiteBoxDefault.numeric.beginInset";
    constexpr AZStd::string_view DefaultNumericAxisXId      = "o3de.action.whiteBoxDefault.numeric.axisX";
    constexpr AZStd::string_view DefaultNumericAxisYId      = "o3de.action.whiteBoxDefault.numeric.axisY";
    constexpr AZStd::string_view DefaultNumericAxisZId      = "o3de.action.whiteBoxDefault.numeric.axisZ";
    constexpr AZStd::string_view DefaultNumericConfirmId    = "o3de.action.whiteBoxDefault.numeric.confirm";
    constexpr AZStd::string_view DefaultNumericCancelId     = "o3de.action.whiteBoxDefault.numeric.cancel";
    constexpr AZStd::string_view DefaultNumericBackspaceId  = "o3de.action.whiteBoxDefault.numeric.backspace";
    constexpr AZStd::string_view DefaultNumericDecimalId    = "o3de.action.whiteBoxDefault.numeric.decimal";
    constexpr AZStd::string_view DefaultNumericNegateId     = "o3de.action.whiteBoxDefault.numeric.negate";
    constexpr AZStd::string_view DefaultNumericDigitIds[10] = {
        "o3de.action.whiteBoxDefault.numeric.digit0",
        "o3de.action.whiteBoxDefault.numeric.digit1",
        "o3de.action.whiteBoxDefault.numeric.digit2",
        "o3de.action.whiteBoxDefault.numeric.digit3",
        "o3de.action.whiteBoxDefault.numeric.digit4",
        "o3de.action.whiteBoxDefault.numeric.digit5",
        "o3de.action.whiteBoxDefault.numeric.digit6",
        "o3de.action.whiteBoxDefault.numeric.digit7",
        "o3de.action.whiteBoxDefault.numeric.digit8",
        "o3de.action.whiteBoxDefault.numeric.digit9",
    };

    static const char* const HideEdgeTitle = "Hide Edge";
    static const char* const HideEdgeDesc = "Hide the selected edge to merge the two connected polygons";
    static const char* const HideVertexTitle = "Hide Vertex";
    static const char* const HideVertexDesc = "Hide the selected vertex to merge the two connected edges";

    static const char* const HideEdgeUndoRedoDesc = "Hide an edge to merge two connected polygons together";
    static const char* const HideVertexUndoRedoDesc = "Hide a vertex to merge two connected edges together";

    static constexpr AZStd::string_view WhiteBoxDefaultSelectionChangeUpdaterIdentifier = "o3de.updater.onWhiteBoxDefaultComponentModeSelectionChanged";

    const QKeySequence HideKey = QKeySequence{Qt::Key_H};

    // handle translation and scale modifiers for either polygon or edge - if a translation
    // modifier is set (either polygon or edge), update the intersection point so the next time
    // mouse down happens the delta offsets of the manipulator are calculated from the correct
    // position and also handle clicking off of a selected modifier to clear the selected state
    // note: the Intersection type must match the geometry for the translation and scale modifier
    template<typename TranslationModifier, typename Intersection, typename DestroyOtherModifierFn>
    static void HandleMouseInteractionForModifier(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        DefaultMode::SelectedTranslationModifier& selectedTranslationModifier,
        DestroyOtherModifierFn&& destroyOtherModifierFn, const AZStd::optional<Intersection>& geometryIntersection)
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<TranslationModifier>>(&selectedTranslationModifier))
        {
            // handle clicking off of selected geometry
            if (mouseInteraction.m_mouseInteraction.m_mouseButtons.Left() &&
                mouseInteraction.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::Up &&
                !geometryIntersection.has_value())
            {
                selectedTranslationModifier = AZStd::monostate{};
                destroyOtherModifierFn();

                AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                    &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

                // Update actions defined with the Action Manager, if enabled.
                auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
                if (actionManagerInterface)
                {
                    actionManagerInterface->TriggerActionUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier);
                }
            }
        }
    }

    // in this generic context TranslationModifier and OtherTranslationModifier correspond
    // to Polygon/Edge or Edge/Polygon depending on the context (which was intersected)
    template<typename Intersection, typename TranslationModifier, typename DestroyOtherModifierFn>
    static void HandleCreatingDestroyingModifiersOnIntersectionChange(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        DefaultMode::SelectedTranslationModifier& selectedTranslationModifier,
        AZStd::unique_ptr<TranslationModifier>& translationModifier, DestroyOtherModifierFn&& destroyOtherModifierFn,
        const AZStd::optional<Intersection>& geometryIntersection,
        const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        // if we have a valid mouse ray intersection with the geometry (e.g. edge or polygon)
        if (geometryIntersection.has_value())
        {
            // does the geometry the mouse is hovering over match the currently selected geometry
            const bool matchesSelected = [&geometryIntersection, &selectedTranslationModifier]()
            {
                if (auto modifier = AZStd::get_if<AZStd::unique_ptr<TranslationModifier>>(&selectedTranslationModifier))
                {
                    return (*modifier)->GetHandle() == geometryIntersection->GetHandle();
                }
                return false;
            }();

            // check if there's currently no modifier or if we need to make a different modifier as
            // the geometry is different
            const bool shouldCreateTranslationModifier =
                !translationModifier || (translationModifier->GetHandle() != geometryIntersection->GetHandle());

            if (shouldCreateTranslationModifier && !matchesSelected)
            {
                // created a modifier for the intersected geometry
                translationModifier = AZStd::make_unique<TranslationModifier>(
                    entityComponentIdPair, geometryIntersection->GetHandle(),
                    geometryIntersection->m_intersection.m_localIntersectionPoint);

                translationModifier->ForwardMouseOverEvent(mouseInteraction.m_mouseInteraction);

                destroyOtherModifierFn();
            }
        }
    }

    AZ_CLASS_ALLOCATOR_IMPL(DefaultMode, AZ::SystemAllocator)

    DefaultMode::DefaultMode(const AZ::EntityComponentIdPair& entityComponentIdPair)
        : m_entityComponentIdPair(entityComponentIdPair)
    {
        EditorWhiteBoxDefaultModeRequestBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
        EditorWhiteBoxEdgeModifierNotificationBus::Handler::BusConnect(entityComponentIdPair);
    }

    DefaultMode::~DefaultMode()
    {
        EditorWhiteBoxEdgeModifierNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxPolygonModifierNotificationBus::Handler::BusDisconnect();
        EditorWhiteBoxDefaultModeRequestBus::Handler::BusDisconnect();
    }

    void DefaultMode::RegisterActionUpdaters()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxDefaultMode - could not get ActionManagerInterface on RegisterActionUpdaters.");

        actionManagerInterface->RegisterActionUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier);
    }

    void DefaultMode::RegisterActions()
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxDefaultMode - could not get ActionManagerInterface on RegisterActions.");

        auto hotKeyManagerInterface = AZ::Interface<AzToolsFramework::HotKeyManagerInterface>::Get();
        AZ_Assert(hotKeyManagerInterface, "WhiteBoxDefaultMode - could not get HotKeyManagerInterface on RegisterActions.");

        // Hide Edge
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.whiteBoxComponentMode.Default.hideEdge";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = HideEdgeTitle;
            actionProperties.m_description = HideEdgeDesc;
            actionProperties.m_category = "White Box Component Mode - Default";

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
                            EditorWhiteBoxDefaultModeRequestBus::Event(
                                entityComponentIdPair, &EditorWhiteBoxDefaultModeRequests::HideSelectedEdge);
                        }
                    );
                }
            );

            actionManagerInterface->InstallEnabledStateCallback(
                actionIdentifier,
                []()
                {
                    // edge selection test - ensure an edge is selected before enabling this shortcut
                    auto componentModeCollectionInterface = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
                    AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");

                    bool isEdgeSelected = false;

                    componentModeCollectionInterface->EnumerateActiveComponents(
                        [&isEdgeSelected](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                        {
                            WhiteBox::Api::EdgeHandles handles;

                            EditorWhiteBoxDefaultModeRequestBus::EventResult(
                                handles, entityComponentIdPair, &EditorWhiteBoxDefaultModeRequests::SelectedEdgeHandles);

                            if (!handles.empty())
                            {
                                isEdgeSelected = true;
                            }
                        }
                    );

                    return isEdgeSelected;
                }
            );
            actionManagerInterface->AddActionToUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier, actionIdentifier);

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "H");
        }

        // Hide Vertex
        {
            constexpr AZStd::string_view actionIdentifier = "o3de.action.whiteBoxComponentMode.Default.hideVertex";
            AzToolsFramework::ActionProperties actionProperties;
            actionProperties.m_name = HideVertexTitle;
            actionProperties.m_description = HideVertexDesc;
            actionProperties.m_category = "White Box Component Mode - Default";

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
                            EditorWhiteBoxDefaultModeRequestBus::Event(
                                entityComponentIdPair, &EditorWhiteBoxDefaultModeRequests::HideSelectedVertex);
                        }
                    );
                }
            );

            actionManagerInterface->InstallEnabledStateCallback(
                actionIdentifier,
                []()
                {
                    // vertex selection test - ensure a vertex is selected before enabling this shortcut
                    auto componentModeCollectionInterface = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
                    AZ_Assert(componentModeCollectionInterface, "Could not retrieve component mode collection.");

                    bool isVertexSelected = false;

                    componentModeCollectionInterface->EnumerateActiveComponents(
                        [&isVertexSelected](const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Uuid&)
                        {
                            WhiteBox::Api::EdgeHandles edgeHandles;
                            EditorWhiteBoxDefaultModeRequestBus::EventResult(
                                edgeHandles, entityComponentIdPair, &EditorWhiteBoxDefaultModeRequests::SelectedEdgeHandles);

                            WhiteBox::Api::VertexHandles vertexHandles;
                            EditorWhiteBoxDefaultModeRequestBus::EventResult(
                                vertexHandles, entityComponentIdPair, &EditorWhiteBoxDefaultModeRequests::SelectedVertexHandles);

                            if (edgeHandles.empty() && !vertexHandles.empty())
                            {
                                isVertexSelected = true;
                            }
                        }
                    );

                    return isVertexSelected;
                }
            );
            actionManagerInterface->AddActionToUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier, actionIdentifier);

            hotKeyManagerInterface->SetActionHotKey(actionIdentifier, "H");
        }

        // ------------------------------------------------------------------ //
        // Blender-style numeric move (M key) for Default mode                //
        // Each action dispatches to all active DefaultMode instances via bus. //
        // ------------------------------------------------------------------ //

        // Helper macro to avoid repeating the enumerate+EBus pattern.
        // (We use a local lambda instead of a macro to keep it type-safe.)
        auto dispatchDefault = [](auto memberFn)
        {
            auto cmci = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
            AZ_Assert(cmci, "Could not retrieve component mode collection.");
            cmci->EnumerateActiveComponents(
                [memberFn](const AZ::EntityComponentIdPair& id, const AZ::Uuid&)
                {
                    EditorWhiteBoxDefaultModeRequestBus::Event(id, memberFn);
                });
        };

        // M – begin numeric move
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Move";
            p.m_description = "Start a numeric move on the hovered polygon / edge / vertex (M, then type a value)";
            p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericBeginMoveId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveBegin); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericBeginMoveId, "M");
        }
        // J – begin numeric scale
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Scale";
            p.m_description = "Scale the hovered polygon or edge numerically (J, then type a factor)";
            p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericBeginScaleId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericScaleBegin); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericBeginScaleId, "J");
        }
        // U – begin numeric rotate
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Rotate";
            p.m_description = "Rotate the hovered polygon or edge numerically (U, then type degrees)";
            p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericBeginRotateId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericRotateBegin); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericBeginRotateId, "U");
        }
        // O – begin numeric extrude (polygon along normal, or edge along displacement)
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Extrude";
            p.m_description = "Extrude the hovered polygon (along normal) or edge numerically (O, then type distance)";
            p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericBeginExtrudeId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericExtrudeBegin); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericBeginExtrudeId, "O");
        }
        // F – begin numeric inset (face inset on polygon, ratio 0=collapse → 1=no inset)
        {
            AzToolsFramework::ActionProperties p;
            p.m_name = "Begin Numeric Face Inset";
            p.m_description = "Inset the hovered polygon numerically (F, then type 0-1 ratio; e.g. 0.8 = 20% inset)";
            p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(
                EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericBeginInsetId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericInsetBegin); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericBeginInsetId, "F");
        }
        // X / Y / Z – axis constraint
        {
            AzToolsFramework::ActionProperties px; px.m_name = "Numeric Move Axis X"; px.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericAxisXId, px,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveSetAxisX); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericAxisXId, "X");

            AzToolsFramework::ActionProperties py; py.m_name = "Numeric Move Axis Y"; py.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericAxisYId, py,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveSetAxisY); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericAxisYId, "Y");

            AzToolsFramework::ActionProperties pz; pz.m_name = "Numeric Move Axis Z"; pz.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericAxisZId, pz,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveSetAxisZ); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericAxisZId, "Z");
        }
        // Enter – confirm
        {
            AzToolsFramework::ActionProperties p; p.m_name = "Numeric Move Confirm"; p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericConfirmId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveConfirm); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericConfirmId, "Return");
        }
        // Escape – cancel
        {
            AzToolsFramework::ActionProperties p; p.m_name = "Numeric Move Cancel"; p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericCancelId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveCancel); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericCancelId, "Escape");
        }
        // Backspace
        {
            AzToolsFramework::ActionProperties p; p.m_name = "Numeric Move Backspace"; p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericBackspaceId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveBackspace); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericBackspaceId, "Backspace");
        }
        // Period
        {
            AzToolsFramework::ActionProperties p; p.m_name = "Numeric Move Decimal"; p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericDecimalId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveDecimal); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericDecimalId, ".");
        }
        // Minus ( '-' operator / leading minus )
        {
            AzToolsFramework::ActionProperties p; p.m_name = "Numeric Operator Minus"; p.m_category = "White Box Component Mode - Default";
            actionManagerInterface->RegisterAction(EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericNegateId, p,
                [dispatchDefault] { dispatchDefault(&EditorWhiteBoxDefaultModeRequests::NumericMoveNegate); });
            hotKeyManagerInterface->SetActionHotKey(DefaultNumericNegateId, "-");
        }
        // Note: +, *, / operators are handled directly in HandleMouseInteraction (not via action system)
        // because shifted-key symbols are unreliable as QKeySequence strings across platforms.
        // Digits 0-9
        {
            const char* dkeys[10] = {"0","1","2","3","4","5","6","7","8","9"};
            for (int d = 0; d <= 9; ++d)
            {
                AzToolsFramework::ActionProperties p;
                p.m_name = AZStd::string::format("Numeric Move Digit %d", d).c_str();
                p.m_category = "White Box Component Mode - Default";
                const char digit = static_cast<char>('0' + d);
                actionManagerInterface->RegisterAction(
                    EditorIdentifiers::MainWindowActionContextIdentifier, DefaultNumericDigitIds[d], p,
                    [digit]
                    {
                        auto cmci = AZ::Interface<AzToolsFramework::ComponentModeCollectionInterface>::Get();
                        AZ_Assert(cmci, "Could not retrieve component mode collection.");
                        cmci->EnumerateActiveComponents(
                            [digit](const AZ::EntityComponentIdPair& id, const AZ::Uuid&)
                            {
                                EditorWhiteBoxDefaultModeRequestBus::Event(
                                    id, &EditorWhiteBoxDefaultModeRequests::NumericMoveAppendDigit, digit);
                            });
                    });
                hotKeyManagerInterface->SetActionHotKey(DefaultNumericDigitIds[d], dkeys[d]);
            }
        }
    }

    void DefaultMode::BindActionsToModes(const AZStd::string& modeIdentifier)
    {
        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "WhiteBoxDefaultMode - could not get ActionManagerInterface on BindActionsToModes.");

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.whiteBoxComponentMode.Default.hideEdge");
        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.whiteBoxComponentMode.Default.hideVertex");

        // Numeric input actions
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericBeginMoveId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericBeginScaleId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericBeginRotateId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericBeginExtrudeId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericBeginInsetId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericAxisXId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericAxisYId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericAxisZId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericConfirmId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericCancelId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericBackspaceId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericDecimalId);
        actionManagerInterface->AssignModeToAction(modeIdentifier, DefaultNumericNegateId);
        // +, *, / are handled in HandleMouseInteraction, not via the action system
        for (const auto& did : DefaultNumericDigitIds)
        {
            actionManagerInterface->AssignModeToAction(modeIdentifier, did);
        }

        actionManagerInterface->AssignModeToAction(modeIdentifier, "o3de.action.componentMode.end");
    }

    void DefaultMode::BindActionsToMenus()
    {
        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(menuManagerInterface, "WhiteBoxDefaultMode - could not get MenuManagerInterface on BindActionsToMenus.");

        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.whiteBoxComponentMode.Default.hideEdge", 6000);
        menuManagerInterface->AddActionToMenu(EditorIdentifiers::EditMenuIdentifier, "o3de.action.whiteBoxComponentMode.Default.hideVertex", 6001);
    }

    void DefaultMode::Refresh()
    {
        // destroy all active modifiers
        m_polygonScaleModifier.reset();
        m_edgeScaleModifier.reset();
        m_polygonTranslationModifier.reset();
        m_edgeTranslationModifier.reset();
        m_vertexTranslationModifier.reset();
        m_selectedModifier = AZStd::monostate{};
    }

    AZStd::vector<AzToolsFramework::ActionOverride> DefaultMode::PopulateActions(
        const AZ::EntityComponentIdPair& entityComponentIdPair)
    {
        // edge selection test - ensure an edge is selected before allowing this shortcut
        if ([[maybe_unused]] auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            return AZStd::vector<AzToolsFramework::ActionOverride>{
                AzToolsFramework::ActionOverride()
                    .SetUri(HideEdge)
                    .SetKeySequence(HideKey)
                    .SetTitle(HideEdgeTitle)
                    .SetTip(HideEdgeDesc)
                    .SetEntityComponentIdPair(entityComponentIdPair)
                    .SetCallback(
                        [this]()
                        {
                            HideSelectedEdge();
                        }
                    )
                };
        }
        // vertex selection test - ensure a vertex is selected before allowing this shortcut
        else if ([[maybe_unused]] auto modifier2 = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier))
        {
            return AZStd::vector<AzToolsFramework::ActionOverride>{
                AzToolsFramework::ActionOverride()
                    .SetUri(HideVertex)
                    .SetKeySequence(HideKey)
                    .SetTitle(HideVertexTitle)
                    .SetTip(HideVertexDesc)
                    .SetEntityComponentIdPair(entityComponentIdPair)
                    .SetCallback(
                        [this]()
                        {
                            HideSelectedVertex();
                        }
                    )
                };
        }
        else
        {
            return {};
        }
    }

    template<typename ModifierUniquePtr>
    static void TryDestroyModifier(ModifierUniquePtr& modifier)
    {
        // has the mouse moved off of the modifier
        if (modifier && !modifier->MouseOver())
        {
            modifier.reset();
        }
    }

    static void DrawVertices(
        AzFramework::DebugDisplayRequests& debugDisplay, const AZ::Transform& worldFromLocal,
        const AzFramework::CameraState& cameraState, const IntersectionAndRenderData& renderData)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        const float vertexIndicatorLength = cl_whiteBoxVertexIndicatorLength;
        const float vertexIndicatorWidth = cl_whiteBoxVertexIndicatorWidth;
        const AZ::Color vertexIndicatorColor = cl_whiteBoxVertexIndicatorColor;

        debugDisplay.SetLineWidth(vertexIndicatorWidth);
        debugDisplay.SetColor(vertexIndicatorColor);

        const auto drawVertIndicator = [&debugDisplay, &worldFromLocal, &cameraState, vertexIndicatorLength](
                                           const AZ::Vector3& start, const AZ::Vector3& axis, const float length)
        {
            const auto scale =
                AzToolsFramework::CalculateScreenToWorldMultiplier(worldFromLocal.TransformPoint(start), cameraState);
            debugDisplay.DrawLine(start, start + axis * AZ::GetMin<float>(length, scale * vertexIndicatorLength));
        };

        for (const auto& edgeBound : renderData.m_whiteBoxEdgeRenderData.m_bounds.m_user)
        {
            const auto& start = edgeBound.m_bound.m_start;
            const auto& end = edgeBound.m_bound.m_end;
            const auto edge = end - start;
            const auto length = edge.GetLength();

            if (length > 0.0f)
            {
                const auto axis = edge / length;
                drawVertIndicator(start, axis, length);
                drawVertIndicator(end, -axis, length);
            }
        }

        debugDisplay.SetLineWidth(1.0f);
    }

    void DefaultMode::Display(
        const AZ::EntityComponentIdPair& entityComponentIdPair, const AZ::Transform& worldFromLocal,
        const IntersectionAndRenderData& renderData, [[maybe_unused]] const AzFramework::ViewportInfo& viewportInfo,
        AzFramework::DebugDisplayRequests& debugDisplay)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        TryDestroyModifier(m_polygonTranslationModifier);
        TryDestroyModifier(m_edgeTranslationModifier);
        TryDestroyModifier(m_vertexTranslationModifier);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        debugDisplay.PushMatrix(worldFromLocal);

        DrawEdges(
            debugDisplay, ed_whiteBoxEdgeDefault, renderData.m_whiteBoxIntersectionData.m_edgeBounds,
            FindInteractiveEdgeHandles(*whiteBox));

        DrawVertices(
            debugDisplay, worldFromLocal, AzToolsFramework::GetCameraState(viewportInfo.m_viewportId), renderData);

        debugDisplay.PopMatrix();

        // Draw Blender-style numeric input overlay when active.
        // DepthTestOff prevents the text from being hidden behind mesh surfaces (same pattern as TransformMode).
        debugDisplay.DepthTestOff();
        if (m_numericInput.IsActive())
        {
            const AZ::Vector3 worldAnchor = worldFromLocal.TransformPoint(m_numericAnchorLocal)
                + AZ::Vector3::CreateAxisZ(0.25f);
            debugDisplay.SetColor(AZ::Colors::White);
            debugDisplay.DrawTextLabel(worldAnchor, 1.5f, m_numericInput.GetStatusText().c_str(), true, 0, 0);
        }
    }

    Api::EdgeHandles DefaultMode::FindInteractiveEdgeHandles(const WhiteBoxMesh& whiteBox)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        // get all edge handles for hovered polygon
        const Api::EdgeHandles polygonHoveredEdgeHandles = m_polygonTranslationModifier
            ? Api::PolygonBorderEdgeHandlesFlattened(whiteBox, m_polygonTranslationModifier->GetPolygonHandle())
            : Api::EdgeHandles{};

        // find edge handles being used by active modifiers
        const Api::EdgeHandles selectedEdgeHandles = [&whiteBox, this]()
        {
            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
            {
                return Api::PolygonBorderEdgeHandlesFlattened(whiteBox, (*modifier)->GetPolygonHandle());
            }

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
            {
                return Api::EdgeHandles{(*modifier)->GetEdgeHandle()};
            }

            return Api::EdgeHandles{};
        }();

        // combine all potentially interactive edge handles
        Api::EdgeHandles interactiveEdgeHandles{polygonHoveredEdgeHandles};
        interactiveEdgeHandles.insert(
            interactiveEdgeHandles.end(), selectedEdgeHandles.begin(), selectedEdgeHandles.end());

        if (m_edgeTranslationModifier)
        {
            // get edge handle for hovered edge (and associated group)
            interactiveEdgeHandles.insert(
                interactiveEdgeHandles.end(), m_edgeTranslationModifier->EdgeHandlesBegin(),
                m_edgeTranslationModifier->EdgeHandlesEnd());
        }

        return interactiveEdgeHandles;
    }

    // if an edge or polygon scale modifier are selected, their scale manipulators (situated
    // at the same position as a vertex) should take priority, so do not attempt to create
    // a vertex selection modifier for those vertices that currently have a scale modifier
    static bool IgnoreVertexHandle(
        const WhiteBoxMesh& whiteBox, const PolygonScaleModifier* polygonScaleModifier,
        const EdgeScaleModifier* edgeScaleModifier, const Api::VertexHandle vertexHandle)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        if (Api::VertexIsHidden(whiteBox, vertexHandle))
        {
            return true;
        }

        Api::VertexHandles vertexHandlesToIgnore;

        if (polygonScaleModifier)
        {
            const auto polygonVertexHandles =
                Api::PolygonVertexHandles(whiteBox, polygonScaleModifier->GetPolygonHandle());

            vertexHandlesToIgnore.insert(
                vertexHandlesToIgnore.end(), polygonVertexHandles.cbegin(), polygonVertexHandles.cend());
        }

        if (edgeScaleModifier)
        {
            const auto edgeVertexHandles = Api::EdgeVertexHandles(whiteBox, edgeScaleModifier->GetEdgeHandle());

            vertexHandlesToIgnore.insert(
                vertexHandlesToIgnore.end(), edgeVertexHandles.cbegin(), edgeVertexHandles.cend());
        }

        return AZStd::find(vertexHandlesToIgnore.cbegin(), vertexHandlesToIgnore.cend(), vertexHandle) !=
            vertexHandlesToIgnore.cend();
    }

    // only return a valid optional if the vertex intersection is valid and it is not hidden
    static AZStd::optional<VertexIntersection> FilterHiddenVertexIntersection(
        const AZStd::optional<VertexIntersection>& vertexIntersection, const WhiteBoxMesh& whiteBox)
    {
        if (!vertexIntersection.has_value() || Api::VertexIsHidden(whiteBox, vertexIntersection->GetHandle()))
        {
            return AZStd::nullopt;
        }

        return vertexIntersection;
    }

    bool DefaultMode::HandleMouseInteraction(
        const AzToolsFramework::ViewportInteraction::MouseInteractionEvent& mouseInteraction,
        const AZ::EntityComponentIdPair& entityComponentIdPair,
        const AZStd::optional<EdgeIntersection>& edgeIntersection,
        const AZStd::optional<PolygonIntersection>& polygonIntersection,
        const AZStd::optional<VertexIntersection>& vertexIntersection)
    {
        AZ_PROFILE_FUNCTION(AzToolsFramework);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        // polygon
        HandleMouseInteractionForModifier<PolygonTranslationModifier, PolygonIntersection>(
            mouseInteraction, m_selectedModifier,
            [this]
            {
                m_polygonScaleModifier.reset();
            },
            polygonIntersection);

        // edge
        HandleMouseInteractionForModifier<EdgeTranslationModifier, EdgeIntersection>(
            mouseInteraction, m_selectedModifier,
            [this]
            {
                m_edgeScaleModifier.reset();
            },
            edgeIntersection);

        // do not allow intersections with hidden vertices in the default mode
        const auto allowedVertexIntersection = FilterHiddenVertexIntersection(vertexIntersection, *whiteBox);

        // vertex
        HandleMouseInteractionForModifier<VertexTranslationModifier, VertexIntersection>(
            mouseInteraction, m_selectedModifier, [] { /*noop*/ }, allowedVertexIntersection);

        switch (FindClosestGeometryIntersection(edgeIntersection, polygonIntersection, allowedVertexIntersection))
        {
        case GeometryIntersection::Edge:
            {
                HandleCreatingDestroyingModifiersOnIntersectionChange<EdgeIntersection, EdgeTranslationModifier>(
                    mouseInteraction, m_selectedModifier, m_edgeTranslationModifier,
                    [this]
                    {
                        m_polygonTranslationModifier.reset();
                        m_vertexTranslationModifier.reset();
                    },
                    edgeIntersection, entityComponentIdPair);
            }
            break;
        case GeometryIntersection::Polygon:
            {
                HandleCreatingDestroyingModifiersOnIntersectionChange<PolygonIntersection, PolygonTranslationModifier>(
                    mouseInteraction, m_selectedModifier, m_polygonTranslationModifier,
                    [this]
                    {
                        m_edgeTranslationModifier.reset();
                        m_vertexTranslationModifier.reset();
                    },
                    polygonIntersection, entityComponentIdPair);
            }
            break;
        case GeometryIntersection::Vertex:
            {
                if (!IgnoreVertexHandle(
                        *whiteBox, m_polygonScaleModifier.get(), m_edgeScaleModifier.get(),
                        allowedVertexIntersection->GetHandle()))
                {
                    HandleCreatingDestroyingModifiersOnIntersectionChange<
                        VertexIntersection, VertexTranslationModifier>(
                        mouseInteraction, m_selectedModifier, m_vertexTranslationModifier,
                        [this]
                        {
                            m_edgeTranslationModifier.reset();
                            m_polygonTranslationModifier.reset();
                        },
                        allowedVertexIntersection, entityComponentIdPair);
                }
            }
            break;
        case GeometryIntersection::None:
            // do nothing
            break;
        default:
            // do nothing
            break;
        }

        return false;
    }

    void DefaultMode::CreatePolygonScaleModifier(const Api::PolygonHandle& polygonHandle)
    {
        m_polygonScaleModifier = AZStd::make_unique<PolygonScaleModifier>(polygonHandle, m_entityComponentIdPair);
    }

    void DefaultMode::CreateEdgeScaleModifier(const Api::EdgeHandle edgeHandle)
    {
        m_edgeScaleModifier = AZStd::make_unique<EdgeScaleModifier>(edgeHandle, m_entityComponentIdPair);
    }

    void DefaultMode::AssignSelectedPolygonTranslationModifier()
    {
        if (m_polygonTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_polygonTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            // Update actions defined with the Action Manager, if enabled.
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            if (actionManagerInterface)
            {
                actionManagerInterface->TriggerActionUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier);
            }

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColors(ed_whiteBoxPolygonSelection, ed_whiteBoxOutlineSelection);
                (*modifier)->CreateView();
            }

            m_edgeScaleModifier.reset();
            m_vertexTranslationModifier.reset();

            m_polygonTranslationModifier = nullptr;
        }
    }

    void DefaultMode::AssignSelectedEdgeTranslationModifier()
    {
        if (m_edgeTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_edgeTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            // Update actions defined with the Action Manager, if enabled.
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            if (actionManagerInterface)
            {
                actionManagerInterface->TriggerActionUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier);
            }

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColors(ed_whiteBoxOutlineSelection, ed_whiteBoxOutlineSelection);
                (*modifier)->SetWidths(cl_whiteBoxSelectedEdgeVisualWidth, cl_whiteBoxSelectedEdgeVisualWidth);
                (*modifier)->CreateView();
            }

            m_polygonScaleModifier.reset();
            m_vertexTranslationModifier.reset();

            m_edgeTranslationModifier = nullptr;
        }
    }

    void DefaultMode::AssignSelectedVertexSelectionModifier()
    {
        if (m_vertexTranslationModifier)
        {
            m_selectedModifier = AZStd::move(m_vertexTranslationModifier);

            AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequestBus::Broadcast(
                &AzToolsFramework::ComponentModeFramework::ComponentModeSystemRequests::RefreshActions);

            // Update actions defined with the Action Manager, if enabled.
            auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
            if (actionManagerInterface)
            {
                actionManagerInterface->TriggerActionUpdater(WhiteBoxDefaultSelectionChangeUpdaterIdentifier);
            }

            if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier))
            {
                (*modifier)->SetColor(ed_whiteBoxVertexSelection);
                (*modifier)->CreateView();
            }

            m_polygonScaleModifier.reset();
            m_edgeScaleModifier.reset();

            m_vertexTranslationModifier = nullptr;
        }
    }

    void DefaultMode::RefreshPolygonScaleModifier()
    {
        if (m_polygonScaleModifier)
        {
            m_polygonScaleModifier->Refresh();
        }
    }

    void DefaultMode::RefreshEdgeScaleModifier()
    {
        if (m_edgeScaleModifier)
        {
            m_edgeScaleModifier->Refresh();
        }
    }

    void DefaultMode::RefreshPolygonTranslationModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_polygonTranslationModifier && !m_polygonTranslationModifier->PerformingAction())
        {
            m_polygonTranslationModifier->Refresh();
        }
    }

    void DefaultMode::RefreshEdgeTranslationModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_edgeTranslationModifier && !m_edgeTranslationModifier->PerformingAction())
        {
            m_edgeTranslationModifier->Refresh();
        }
    }

    void DefaultMode::RefreshVertexSelectionModifier()
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier))
        {
            if (!(*modifier)->PerformingAction())
            {
                (*modifier)->Refresh();
            }
        }

        if (m_vertexTranslationModifier && !m_vertexTranslationModifier->PerformingAction())
        {
            m_vertexTranslationModifier->Refresh();
        }
    }

    template<typename Modifier>
    auto ModifierHandles(const DefaultMode::SelectedTranslationModifier& selectedModifier)
    {
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<Modifier>>(&selectedModifier))
        {
            return AZStd::vector<typename Modifier::HandleType>{(*modifier)->GetHandle()};
        }

        return AZStd::vector<typename Modifier::HandleType>{};
    }

    Api::VertexHandles DefaultMode::SelectedVertexHandles() const
    {
        return ModifierHandles<VertexTranslationModifier>(m_selectedModifier);
    }

    Api::EdgeHandles DefaultMode::SelectedEdgeHandles() const
    {
        return ModifierHandles<EdgeTranslationModifier>(m_selectedModifier);
    }

    Api::PolygonHandles DefaultMode::SelectedPolygonHandles() const
    {
        return ModifierHandles<PolygonTranslationModifier>(m_selectedModifier);
    }

    Api::VertexHandle DefaultMode::HoveredVertexHandle() const
    {
        return m_vertexTranslationModifier ? m_vertexTranslationModifier->GetVertexHandle() : Api::VertexHandle();
    }

    Api::EdgeHandle DefaultMode::HoveredEdgeHandle() const
    {
        return m_edgeTranslationModifier ? m_edgeTranslationModifier->GetEdgeHandle() : Api::EdgeHandle();
    }

    Api::PolygonHandle DefaultMode::HoveredPolygonHandle() const
    {
        return m_polygonTranslationModifier ? m_polygonTranslationModifier->GetPolygonHandle() : Api::PolygonHandle();
    }

    void DefaultMode::HideSelectedEdge()
    {
        auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        if (modifier)
        {
            Api::HideEdge(*whiteBox, (*modifier)->GetEdgeHandle());
            (*modifier)->SetEdgeHandle(Api::EdgeHandle{});

            RecordWhiteBoxAction(*whiteBox, m_entityComponentIdPair, HideEdgeUndoRedoDesc);
        }
    }

    void DefaultMode::HideSelectedVertex()
    {
        auto modifier = AZStd::get_if<AZStd::unique_ptr<VertexTranslationModifier>>(&m_selectedModifier);

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        if (modifier)
        {
            Api::HideVertex(*whiteBox, (*modifier)->GetVertexHandle());
            (*modifier)->SetVertexHandle(Api::VertexHandle{});

            RecordWhiteBoxAction(*whiteBox, m_entityComponentIdPair, HideVertexUndoRedoDesc);
        }
    }

    void DefaultMode::OnPolygonModifierUpdatedPolygonHandle(
        const Api::PolygonHandle& previousPolygonHandle, const Api::PolygonHandle& nextPolygonHandle)
    {
        // an operation has caused the currently selected polygon handle to update (e.g. an append/extrusion)
        // if the previous polygon handle matches the selected polygon translation modifier, we know it caused
        // the extrusion and should be updated
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<PolygonTranslationModifier>>(&m_selectedModifier))
        {
            if ((*modifier)->GetPolygonHandle() == previousPolygonHandle)
            {
                (*modifier)->SetPolygonHandle(nextPolygonHandle);
                m_polygonScaleModifier->SetPolygonHandle(nextPolygonHandle);
            }
        }
    }

    void DefaultMode::OnEdgeModifierUpdatedEdgeHandle(
        const Api::EdgeHandle previousEdgeHandle, const Api::EdgeHandle nextEdgeHandle)
    {
        // an operation has caused the currently selected edge handle to update (e.g. an append/extrusion)
        // if the previous edge handle matches the selected edge translation modifier, we know it caused
        // the extrusion and should be updated
        if (auto modifier = AZStd::get_if<AZStd::unique_ptr<EdgeTranslationModifier>>(&m_selectedModifier))
        {
            if ((*modifier)->GetEdgeHandle() == previousEdgeHandle)
            {
                m_edgeScaleModifier->SetEdgeHandle(nextEdgeHandle);
            }
        }
    }
    // Helper: capture local-space anchor from the current hover state.
    // Called once at Begin time; the position is then held for the lifetime of the input session.
    static AZ::Vector3 CaptureAnchor(
        WhiteBoxMesh* wb,
        const Api::PolygonHandle& ph,
        Api::VertexHandle vh,
        Api::EdgeHandle eh)
    {
        if (wb && !ph.m_faceHandles.empty())
        {
            return Api::PolygonMidpoint(*wb, ph);
        }
        if (wb && vh.IsValid())
        {
            return Api::VertexPosition(*wb, vh);
        }
        if (wb && eh.IsValid())
        {
            return Api::EdgeMidpoint(*wb, eh);
        }
        return AZ::Vector3::CreateZero();
    }

    void DefaultMode::NumericMoveBegin()
    {
        // Prefer hovered geometry; fall back to the currently selected geometry.
        Api::PolygonHandle ph = HoveredPolygonHandle();
        Api::VertexHandle  vh = HoveredVertexHandle();
        Api::EdgeHandle    eh = HoveredEdgeHandle();

        if (ph.m_faceHandles.empty() && !vh.IsValid() && !eh.IsValid())
        {
            // Nothing hovered — check whether something is selected.
            ph = SelectedPolygonHandles().empty() ? Api::PolygonHandle{} : SelectedPolygonHandles().front();
            if (ph.m_faceHandles.empty())
            {
                vh = SelectedVertexHandles().empty() ? Api::VertexHandle{} : SelectedVertexHandles().front();
            }
            if (ph.m_faceHandles.empty() && !vh.IsValid())
            {
                const auto ehs = SelectedEdgeHandles();
                eh = ehs.empty() ? Api::EdgeHandle{} : ehs.front();
            }
        }

        if (ph.m_faceHandles.empty() && !vh.IsValid() && !eh.IsValid())
        {
            return;
        }
        WhiteBoxMesh* wb = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            wb, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        m_numericAnchorLocal = CaptureAnchor(wb, ph, vh, eh);
        m_numericInput.Begin(NumericOpMode::Move);
    }

    void DefaultMode::NumericScaleBegin()
    {
        Api::PolygonHandle ph = HoveredPolygonHandle();
        Api::EdgeHandle    eh = HoveredEdgeHandle();
        if (ph.m_faceHandles.empty() && !eh.IsValid())
        {
            ph = SelectedPolygonHandles().empty() ? Api::PolygonHandle{} : SelectedPolygonHandles().front();
            if (ph.m_faceHandles.empty())
            {
                const auto ehs = SelectedEdgeHandles();
                eh = ehs.empty() ? Api::EdgeHandle{} : ehs.front();
            }
        }
        if (ph.m_faceHandles.empty() && !eh.IsValid())
        {
            return;
        }
        WhiteBoxMesh* wb = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            wb, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        m_numericAnchorLocal = CaptureAnchor(wb, ph, Api::VertexHandle{}, eh);
        m_numericInput.Begin(NumericOpMode::Scale);
    }

    void DefaultMode::NumericRotateBegin()
    {
        Api::PolygonHandle ph = HoveredPolygonHandle();
        Api::EdgeHandle    eh = HoveredEdgeHandle();
        if (ph.m_faceHandles.empty() && !eh.IsValid())
        {
            ph = SelectedPolygonHandles().empty() ? Api::PolygonHandle{} : SelectedPolygonHandles().front();
            if (ph.m_faceHandles.empty())
            {
                const auto ehs = SelectedEdgeHandles();
                eh = ehs.empty() ? Api::EdgeHandle{} : ehs.front();
            }
        }
        if (ph.m_faceHandles.empty() && !eh.IsValid())
        {
            return;
        }
        WhiteBoxMesh* wb = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            wb, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        m_numericAnchorLocal = CaptureAnchor(wb, ph, Api::VertexHandle{}, eh);
        m_numericInput.Begin(NumericOpMode::Rotate);
    }

    void DefaultMode::NumericExtrudeBegin()
    {
        Api::PolygonHandle ph = HoveredPolygonHandle();
        Api::EdgeHandle    eh = HoveredEdgeHandle();
        if (ph.m_faceHandles.empty() && !eh.IsValid())
        {
            ph = SelectedPolygonHandles().empty() ? Api::PolygonHandle{} : SelectedPolygonHandles().front();
            if (ph.m_faceHandles.empty())
            {
                const auto ehs = SelectedEdgeHandles();
                eh = ehs.empty() ? Api::EdgeHandle{} : ehs.front();
            }
        }
        if (ph.m_faceHandles.empty() && !eh.IsValid())
        {
            return;
        }
        WhiteBoxMesh* wb = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            wb, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        m_numericAnchorLocal = CaptureAnchor(wb, ph, Api::VertexHandle{}, eh);
        m_numericInput.Begin(NumericOpMode::Extrude);
    }

    void DefaultMode::NumericInsetBegin()
    {
        Api::PolygonHandle ph = HoveredPolygonHandle();
        if (ph.m_faceHandles.empty())
        {
            ph = SelectedPolygonHandles().empty() ? Api::PolygonHandle{} : SelectedPolygonHandles().front();
        }
        if (ph.m_faceHandles.empty())
        {
            return;
        }
        WhiteBoxMesh* wb = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            wb, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);
        m_numericAnchorLocal = wb ? Api::PolygonMidpoint(*wb, ph) : AZ::Vector3::CreateZero();
        m_numericInput.Begin(NumericOpMode::Inset);
    }

    void DefaultMode::ApplyNumeric()
    {
        if (!m_numericInput.IsActive())
        {
            m_numericInput.Reset();
            return;
        }

        // Require at least one digit typed before confirming.
        if (m_numericInput.IsEmpty())
        {
            m_numericInput.Reset();
            return;
        }

        // Resolve current geometry: prefer hover, fall back to selection.
        auto resolvePolygon = [this]() -> Api::PolygonHandle
        {
            Api::PolygonHandle ph = HoveredPolygonHandle();
            if (ph.m_faceHandles.empty())
            {
                const auto sel = SelectedPolygonHandles();
                if (!sel.empty()) { ph = sel.front(); }
            }
            return ph;
        };
        auto resolveVertex = [this]() -> Api::VertexHandle
        {
            Api::VertexHandle vh = HoveredVertexHandle();
            if (!vh.IsValid())
            {
                const auto sel = SelectedVertexHandles();
                if (!sel.empty()) { vh = sel.front(); }
            }
            return vh;
        };
        auto resolveEdge = [this]() -> Api::EdgeHandle
        {
            Api::EdgeHandle eh = HoveredEdgeHandle();
            if (!eh.IsValid())
            {
                const auto sel = SelectedEdgeHandles();
                if (!sel.empty()) { eh = sel.front(); }
            }
            return eh;
        };

        WhiteBoxMesh* whiteBox = nullptr;
        EditorWhiteBoxComponentRequestBus::EventResult(
            whiteBox, m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::GetWhiteBoxMesh);

        const float value = m_numericInput.GetValue();

        switch (m_numericInput.m_mode)
        {
        // ------------------------------------------------------------------ //
        // Move                                                                //
        // ------------------------------------------------------------------ //
        case NumericOpMode::Move:
        {
            const Api::PolygonHandle polygonHandle = resolvePolygon();
            if (!polygonHandle.m_faceHandles.empty())
            {
                const AZ::Vector3 direction = (m_numericInput.m_axis != NumericAxisConstraint::Free)
                    ? m_numericInput.GetAxisVector()
                    : Api::PolygonNormal(*whiteBox, polygonHandle);

                const AZ::Vector3 delta = direction * value;
                for (const Api::VertexHandle& vh : Api::PolygonVertexHandles(*whiteBox, polygonHandle))
                {
                    Api::SetVertexPosition(*whiteBox, vh, Api::VertexPosition(*whiteBox, vh) + delta);
                }
                RefreshPolygonTranslationModifier();
                RefreshPolygonScaleModifier();
                RefreshEdgeTranslationModifier();
                RefreshEdgeScaleModifier();
            }
            else if (const Api::VertexHandle vh = resolveVertex(); vh.IsValid())
            {
                if (m_numericInput.m_axis == NumericAxisConstraint::Free)
                {
                    m_numericInput.Reset(); return;
                }
                Api::SetVertexPosition(*whiteBox, vh, Api::VertexPosition(*whiteBox, vh) + m_numericInput.GetAxisVector() * value);
                RefreshPolygonTranslationModifier();
                RefreshPolygonScaleModifier();
                RefreshEdgeTranslationModifier();
                RefreshEdgeScaleModifier();
                RefreshVertexSelectionModifier();
            }
            else if (const Api::EdgeHandle eh = resolveEdge(); eh.IsValid())
            {
                if (m_numericInput.m_axis == NumericAxisConstraint::Free)
                {
                    m_numericInput.Reset(); return;
                }
                const AZ::Vector3 delta = m_numericInput.GetAxisVector() * value;
                for (const Api::VertexHandle& vh : Api::EdgeVertexHandles(*whiteBox, eh))
                {
                    Api::SetVertexPosition(*whiteBox, vh, Api::VertexPosition(*whiteBox, vh) + delta);
                }
                RefreshEdgeTranslationModifier();
                RefreshEdgeScaleModifier();
                RefreshPolygonTranslationModifier();
            }
            else
            {
                m_numericInput.Reset(); return;
            }
            break;
        }
        // ------------------------------------------------------------------ //
        // Scale                                                               //
        // ------------------------------------------------------------------ //
        case NumericOpMode::Scale:
        {
            // factor = typed value (e.g., 2 = double, 0.5 = half)
            const Api::PolygonHandle polygonHandle = resolvePolygon();
            if (!polygonHandle.m_faceHandles.empty())
            {
                const AZ::Vector3 center = Api::PolygonMidpoint(*whiteBox, polygonHandle);
                for (const Api::VertexHandle& vh : Api::PolygonVertexHandles(*whiteBox, polygonHandle))
                {
                    const AZ::Vector3 local = Api::VertexPosition(*whiteBox, vh) - center;
                    AZ::Vector3 scaled = local;
                    switch (m_numericInput.m_axis)
                    {
                    case NumericAxisConstraint::X: scaled.SetX(local.GetX() * value); break;
                    case NumericAxisConstraint::Y: scaled.SetY(local.GetY() * value); break;
                    case NumericAxisConstraint::Z: scaled.SetZ(local.GetZ() * value); break;
                    default: scaled = local * value; break; // Free = uniform
                    }
                    Api::SetVertexPosition(*whiteBox, vh, center + scaled);
                }
                RefreshPolygonScaleModifier();
                RefreshPolygonTranslationModifier();
                RefreshEdgeTranslationModifier();
                RefreshEdgeScaleModifier();
            }
            else if (const Api::EdgeHandle eh = resolveEdge(); eh.IsValid())
            {
                const AZ::Vector3 center = Api::EdgeMidpoint(*whiteBox, eh);
                for (const Api::VertexHandle& vh : Api::EdgeVertexHandles(*whiteBox, eh))
                {
                    const AZ::Vector3 local = Api::VertexPosition(*whiteBox, vh) - center;
                    AZ::Vector3 scaled = local;
                    switch (m_numericInput.m_axis)
                    {
                    case NumericAxisConstraint::X: scaled.SetX(local.GetX() * value); break;
                    case NumericAxisConstraint::Y: scaled.SetY(local.GetY() * value); break;
                    case NumericAxisConstraint::Z: scaled.SetZ(local.GetZ() * value); break;
                    default: scaled = local * value; break;
                    }
                    Api::SetVertexPosition(*whiteBox, vh, center + scaled);
                }
                RefreshEdgeScaleModifier();
                RefreshEdgeTranslationModifier();
                RefreshPolygonTranslationModifier();
            }
            else
            {
                m_numericInput.Reset(); return;
            }
            break;
        }
        // ------------------------------------------------------------------ //
        // Rotate                                                              //
        // ------------------------------------------------------------------ //
        case NumericOpMode::Rotate:
        {
            const Api::PolygonHandle polygonHandle = resolvePolygon();
            if (!polygonHandle.m_faceHandles.empty())
            {
                // Default axis = face normal; X/Y/Z overrides to world axis.
                const AZ::Vector3 axis = (m_numericInput.m_axis != NumericAxisConstraint::Free)
                    ? m_numericInput.GetAxisVector()
                    : Api::PolygonNormal(*whiteBox, polygonHandle);
                const AZ::Vector3    center = Api::PolygonMidpoint(*whiteBox, polygonHandle);
                const AZ::Quaternion rot    = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(value));

                for (const Api::VertexHandle& vh : Api::PolygonVertexHandles(*whiteBox, polygonHandle))
                {
                    const AZ::Vector3 local = Api::VertexPosition(*whiteBox, vh) - center;
                    Api::SetVertexPosition(*whiteBox, vh, rot.TransformVector(local) + center);
                }
                RefreshPolygonTranslationModifier();
                RefreshPolygonScaleModifier();
                RefreshEdgeTranslationModifier();
                RefreshEdgeScaleModifier();
            }
            else if (const Api::EdgeHandle eh = resolveEdge(); eh.IsValid())
            {
                // Edge has no natural rotation axis — require explicit X/Y/Z.
                if (m_numericInput.m_axis == NumericAxisConstraint::Free)
                {
                    m_numericInput.Reset(); return;
                }
                const AZ::Vector3    axis   = m_numericInput.GetAxisVector();
                const AZ::Vector3    center = Api::EdgeMidpoint(*whiteBox, eh);
                const AZ::Quaternion rot    = AZ::Quaternion::CreateFromAxisAngle(axis, AZ::DegToRad(value));

                for (const Api::VertexHandle& vh : Api::EdgeVertexHandles(*whiteBox, eh))
                {
                    const AZ::Vector3 local = Api::VertexPosition(*whiteBox, vh) - center;
                    Api::SetVertexPosition(*whiteBox, vh, rot.TransformVector(local) + center);
                }
                RefreshEdgeTranslationModifier();
                RefreshEdgeScaleModifier();
                RefreshPolygonTranslationModifier();
            }
            else
            {
                m_numericInput.Reset(); return;
            }
            break;
        }
        // ------------------------------------------------------------------ //
        // Extrude                                                            //
        // ------------------------------------------------------------------ //
        case NumericOpMode::Extrude:
        {
            const Api::PolygonHandle polygonHandle = resolvePolygon();
            if (!polygonHandle.m_faceHandles.empty())
            {
                // value = distance along face normal (positive = outward, negative = inward)
                if (AZ::IsClose(value, 0.0f, 1e-5f)) { m_numericInput.Reset(); return; }
                Api::TranslatePolygonAppend(*whiteBox, polygonHandle, value);

                // The polygon handle changed; destroy stale modifiers so they rebuild on next hover.
                m_polygonTranslationModifier.reset();
                m_polygonScaleModifier.reset();
                m_edgeTranslationModifier.reset();
                m_edgeScaleModifier.reset();
                m_selectedModifier = AZStd::monostate{};
            }
            else if (const Api::EdgeHandle eh = resolveEdge(); eh.IsValid())
            {
                // Direction: average of adjacent face normals (outward); X/Y/Z overrides.
                AZ::Vector3 extrudeDir;
                if (m_numericInput.m_axis != NumericAxisConstraint::Free)
                {
                    extrudeDir = m_numericInput.GetAxisVector();
                }
                else
                {
                    // Compute outward direction from the average of adjacent polygon normals.
                    const auto faceHandles = Api::EdgeFaceHandles(*whiteBox, eh);
                    extrudeDir = AZ::Vector3::CreateZero();
                    for (const auto& fh : faceHandles)
                    {
                        extrudeDir += Api::PolygonNormal(*whiteBox, Api::FacePolygonHandle(*whiteBox, fh));
                    }
                    const float lenSq = extrudeDir.GetLengthSq();
                    extrudeDir = (lenSq > 1e-6f) ? extrudeDir.GetNormalized() : AZ::Vector3::CreateAxisZ();
                }

                if (AZ::IsClose(value, 0.0f, 1e-5f)) { m_numericInput.Reset(); return; }
                const AZ::Vector3 displacement = extrudeDir * value;
                Api::TranslateEdgeAppend(*whiteBox, eh, displacement);

                // Edge handle changed; destroy stale modifiers.
                m_edgeTranslationModifier.reset();
                m_edgeScaleModifier.reset();
                m_polygonTranslationModifier.reset();
                m_polygonScaleModifier.reset();
                m_selectedModifier = AZStd::monostate{};
            }
            else
            {
                m_numericInput.Reset(); return;
            }
            break;
        }
        // ------------------------------------------------------------------ //
        // Face Inset                                                          //
        // ------------------------------------------------------------------ //
        case NumericOpMode::Inset:
        {
            // value is an inset ratio: 1.0 = original size (no inset), 0.0 = fully collapsed.
            // Shorthand: F *0.8 Enter = 80% of original = 20% inset.
            const Api::PolygonHandle polygonHandle = resolvePolygon();
            if (polygonHandle.m_faceHandles.empty()) { m_numericInput.Reset(); return; }

            // Step 1: create inset topology (inner polygon starts at same size as outer).
            const Api::PolygonHandle innerPh = Api::ScalePolygonAppendRelative(*whiteBox, polygonHandle, 0.0f);

            // Step 2: scale inner polygon vertices toward center by factor (replicates PolygonScaleModifier).
            const AZ::Vector3    midpoint     = Api::PolygonMidpoint(*whiteBox, innerPh);
            const AZ::Transform  polygonSpace = Api::PolygonSpace(*whiteBox, innerPh, midpoint);
            const Api::VertexHandles vhs      = Api::PolygonVertexHandles(*whiteBox, innerPh);
            const auto initialPositions       = Api::VertexPositions(*whiteBox, vhs);

            const float clampedFactor = AZ::GetClamp(value, 0.0f, 2.0f); // allow slight outset via >1
            for (size_t i = 0; i < vhs.size(); ++i)
            {
                Api::SetVertexPosition(
                    *whiteBox, vhs[i],
                    ScalePosition(clampedFactor, initialPositions[i], polygonSpace));
            }

            // Topology changed; destroy stale modifiers.
            m_polygonTranslationModifier.reset();
            m_polygonScaleModifier.reset();
            m_edgeTranslationModifier.reset();
            m_edgeScaleModifier.reset();
            m_selectedModifier = AZStd::monostate{};
            break;
        }
        default:
            m_numericInput.Reset();
            return;
        }

        Api::CalculateNormals(*whiteBox);
        Api::CalculatePlanarUVs(*whiteBox);
        EditorWhiteBoxComponentModeRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentModeRequestBus::Events::MarkWhiteBoxIntersectionDataDirty);
        EditorWhiteBoxComponentNotificationBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentNotificationBus::Events::OnWhiteBoxMeshModified);
        EditorWhiteBoxComponentRequestBus::Event(
            m_entityComponentIdPair, &EditorWhiteBoxComponentRequests::SerializeWhiteBox);

        m_numericInput.Reset();
    }

} // namespace WhiteBox
