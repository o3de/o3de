/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "AzToolsFrameworkTestHelpers.h"

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Entity/EntityContext.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLayerComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>

#include <QAction>

using namespace AzToolsFramework;

namespace UnitTest
{
    void MousePressAndMove(
        QWidget* widget, const QPoint& initialPositionWidget, const QPoint& mouseDelta, const Qt::MouseButton mouseButton)
    {
        QPoint position = widget->mapToGlobal(initialPositionWidget);
        QTest::mousePress(widget, mouseButton, Qt::NoModifier, position);

        MouseMove(widget, initialPositionWidget, mouseDelta, mouseButton);
    }

    // Note: There are a series of bugs in Qt that appear to be preventing mouseMove events
    // firing when sent through the QTest framework. This is a work around for our version
    // of Qt. In future this can hopefully be simplified. See ^1 for workaround.
    // More info: Issues with mouse move in Qt
    // - https://bugreports.qt.io/browse/QTBUG-5232
    // - https://bugreports.qt.io/browse/QTBUG-69414
    // - https://lists.qt-project.org/pipermail/development/2019-July/036873.html
    void MouseMove(QWidget* widget, const QPoint& initialPositionWidget, const QPoint& mouseDelta, const Qt::MouseButton mouseButton)
    {
        QPoint nextPosition = widget->mapToGlobal(initialPositionWidget + mouseDelta);

        // ^1 To ensure a mouse move event is fired we must call the test mouse move function
        // and also send a mouse move event that matches. Each on their own do not appear to
        // work - please see the links above for more context.
        QTest::mouseMove(widget, nextPosition);
        QMouseEvent mouseMoveEvent(
            QEvent::MouseMove, QPointF(nextPosition), QPointF(nextPosition), Qt::NoButton, mouseButton, Qt::NoModifier);
        QApplication::sendEvent(widget, &mouseMoveEvent);
    }

    bool TestWidget::eventFilter(QObject* watched, QEvent* event)
    {
        AZ_UNUSED(watched);
        switch (event->type())
        {
        case QEvent::ShortcutOverride:
        {
            const auto& cachedActions = actions();

            // perform cast after type check (now safe to do so)
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            // do our best to build a key sequence
            // note: this will not handle more complex multi-key shortcuts at this time
            const auto keySequence = QKeySequence(keyEvent->modifiers() + keyEvent->key());
            // lookup the action that corresponds to this key event
            const auto actionIt = AZStd::find_if(
                cachedActions.begin(), cachedActions.end(), [keySequence](QAction* action)
            {
                return action->shortcut() == keySequence;
            });

            // if we get a match, generate the shortcut for this key combination
            if (actionIt != cachedActions.end())
            {
                // trigger shortcut
                QShortcutEvent shortcutEvent(keySequence, 0);
                QApplication::sendEvent(*actionIt, &shortcutEvent);
                keyEvent->accept();
                return true;
            }

            return false;
        }
        default:
            return false;
        }
    }

    bool FocusInteractionWidget::event(QEvent* event)
    {
        using AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus;

        auto eventType = event->type();

        switch (eventType)
        {
        case QEvent::MouseButtonPress:
            EditorInteractionSystemViewportSelectionRequestBus::Event(
                AzToolsFramework::GetEntityContextId(), &EditorInteractionSystemViewportSelectionRequestBus::Events::SetDefaultHandler);
            return true;
        case QEvent::FocusIn:
        case QEvent::FocusOut:
            {
                bool handled = false;
                AzToolsFramework::ViewportInteraction::MouseInteraction mouseInteraction;
                EditorInteractionSystemViewportSelectionRequestBus::EventResult(
                    handled, AzToolsFramework::GetEntityContextId(),
                    &EditorInteractionSystemViewportSelectionRequestBus::Events::InternalHandleMouseViewportInteraction,
                    AzToolsFramework::ViewportInteraction::MouseInteractionEvent(
                        mouseInteraction, AzToolsFramework::ViewportInteraction::MouseEvent::Down, /*captured=*/false));
                return handled;
            }
        }

        return QWidget::event(event);
    }

    void TestEditorActions::Connect()
    {
        using AzToolsFramework::GetEntityContextId;
        using AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus;

        AzToolsFramework::EditorActionRequestBus::Handler::BusConnect();
        ViewportEditorModeNotificationsBus::Handler::BusConnect(GetEntityContextId());
        m_defaultWidget.setFocus();
    }

    void TestEditorActions::Disconnect()
    {
        using AzToolsFramework::ComponentModeFramework::EditorComponentModeNotificationBus;

        ViewportEditorModeNotificationsBus::Handler::BusDisconnect();
        AzToolsFramework::EditorActionRequestBus::Handler::BusDisconnect();
    }

    void TestEditorActions::OnEditorModeActivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            m_componentModeWidget.setFocus();
        }
    }

    void TestEditorActions::OnEditorModeDeactivated(
        [[maybe_unused]] const AzToolsFramework::ViewportEditorModesInterface& editorModeState, AzToolsFramework::ViewportEditorMode mode)
    {
        if (mode == ViewportEditorMode::Component)
        {
            m_defaultWidget.setFocus();
        }
    }

    void TestEditorActions::AddActionViaBus(int id, QAction* action)
    {
        AZ_Assert(action, "Attempting to add a null action");

        if (action)
        {
            action->setData(id);
            action->setShortcutContext(Qt::ApplicationShortcut);
            m_defaultWidget.addAction(action);
        }
    }

    void TestEditorActions::AddActionViaBusCrc(AZ::Crc32 id, QAction* action) 
    {
        AZ_Assert(action, "Attempting to add a null action");

        if (action)
        {
            action->setData(aznumeric_cast<AZ::u32>(id));
            action->setShortcutContext(Qt::ApplicationShortcut);
            m_defaultWidget.addAction(action);
        }
    }

    void TestEditorActions::RemoveActionViaBus(QAction* action)
    {
        AZ_Assert(action, "Attempting to remove a null action");

        if (action)
        {
            m_defaultWidget.removeAction(action);
        }
    }

    static void SetDefaultActionsEnabled(
        const QList<QAction*>& actions, const bool enabled)
    {
        AZStd::for_each(
            actions.begin(), actions.end(),
            [enabled](QAction* action)
            {
                if (!action->property("Reserved").toBool())
                {
                    action->setEnabled(enabled);
                }
            });
    }

    void TestEditorActions::EnableDefaultActions()
    {
        SetDefaultActionsEnabled(m_defaultWidget.actions(), true);
    }

    void TestEditorActions::DisableDefaultActions()
    {
        SetDefaultActionsEnabled(m_defaultWidget.actions(), false);
    }

    EditorEntityComponentChangeDetector::EditorEntityComponentChangeDetector(const AZ::EntityId entityId)
    {
        PropertyEditorEntityChangeNotificationBus::Handler::BusConnect(entityId);
        EditorTransformChangeNotificationBus::Handler::BusConnect();
        ToolsApplicationNotificationBus::Handler::BusConnect();
    }

    EditorEntityComponentChangeDetector::~EditorEntityComponentChangeDetector()
    {
        ToolsApplicationNotificationBus::Handler::BusDisconnect();
        EditorTransformChangeNotificationBus::Handler::BusDisconnect();
        PropertyEditorEntityChangeNotificationBus::Handler::BusDisconnect();
    }

    void EditorEntityComponentChangeDetector::OnEntityComponentPropertyChanged(const AZ::ComponentId componentId)
    {
        m_componentIds.push_back(componentId);
    }

    void EditorEntityComponentChangeDetector::OnEntityTransformChanged(
        const AzToolsFramework::EntityIdList& entityIds)
    {
        m_entityIds = entityIds;

        for (const AZ::EntityId& entityId : entityIds)
        {
            if (const auto* entity = GetEntityById(entityId))
            {
                if (AZ::Component* transformComponent = entity->FindComponent<Components::TransformComponent>())
                {
                    OnEntityComponentPropertyChanged(transformComponent->GetId());
                }
            }
        }
    }

    void EditorEntityComponentChangeDetector::InvalidatePropertyDisplay(
        AzToolsFramework::PropertyModificationRefreshLevel /*level*/)
    {
        m_propertyDisplayInvalidated = true;
    }

    ToolsApplicationMessageHandler::ToolsApplicationMessageHandler()
    {
        m_gridMateMessageHandler = AZStd::make_unique<ErrorHandler>("GridMate");
        m_enginePathMessageHandler = AZStd::make_unique<ErrorHandler>("Engine Path");
        m_skippingDriveMessageHandler = AZStd::make_unique<ErrorHandler>("Skipping drive");
        m_storageDriveMessageHandler = AZStd::make_unique<ErrorHandler>("Storage drive");
        m_jsonComponentErrorHandler = AZStd::make_unique<ErrorHandler>("JsonSystem");
    }

    void MockPerforceCommand::ExecuteCommand()
    {
        m_rawOutput.Clear();
        m_applicationFound = true;

        if (m_commandArgs == "set")
        {
            m_rawOutput.outputResult = 
                R"(P4PORT=ssl:unittest.amazon.com:1666 (set))" "\r\n"
                "\r\n";
        }
        else if (m_commandArgs == "info")
        {
            m_rawOutput.outputResult =
                R"(... userName unittest)" "\r\n"
                R"(... clientName unittest)" "\r\n"
                R"(... clientRoot c:\depot)" "\r\n"
                R"(... clientLock none)" "\r\n"
                R"(... clientCwd c:\depot\dev\Engine\Fonts)" "\r\n"
                R"(... clientHost unittest)" "\r\n"
                R"(... clientCase insensitive)" "\r\n"
                R"(... peerAddress 127.0.0.1:64565)" "\r\n"
                R"(... clientAddress 127.0.0.1)" "\r\n"
                R"(... serverName unittest)" "\r\n"
                R"(... lbr.replication readonly)" "\r\n"
                R"(... monitor enabled)" "\r\n"
                R"(... security enabled)" "\r\n"
                R"(... externalAuth enabled)" "\r\n"
                R"(... serverAddress unittest.amazon.com:1666)" "\r\n"
                R"(... serverRoot /data/repos/p4root/root)" "\r\n"
                R"(... serverDate 2020/01/01 10:00:00 -0500 PST)" "\r\n"
                R"(... tzoffset -28800)" "\r\n"
                R"(... serverUptime 1234:12:34)" "\r\n"
                R"(... serverVersion P4D/LINUX33X86_64/2020.4/1234567 (2020/06/06))" "\r\n"
                R"(... serverEncryption encrypted)" "\r\n"
                R"(... serverCertExpires Dec 24 04:10:00 3030 GMT)" "\r\n"
                R"(... ServerID unittest)" "\r\n"
                R"(... serverServices edge-server)" "\r\n"
                R"(... authServer ssl:127.0.0.1:1666)" "\r\n"
                R"(... changeServer ssl:127.0.0.1:1666)" "\r\n"
                R"(... serverLicense ssl:127.0.0.1:1666)" "\r\n"
                R"(... caseHandling insensitive)" "\r\n"
                R"(... replica ssl:127.0.0.1:1666)" "\r\n"
                "\r\n";
        }
        else if (m_commandArgs.starts_with("changes"))
        {
            m_rawOutput.outputResult =
                R"(... change 12345)" "\r\n"
                R"(... time 1234565432)" "\r\n"
                R"(... user unittest)" "\r\n"
                R"(... client unittest_workspace)" "\r\n"
                R"(... status pending)" "\r\n"
                R"(... changeType public)" "\r\n"
                R"(... desc *Open 3D Engine Auto)" "\r\n"
                "\r\n";
        }
        else if (m_commandArgs.starts_with("fstat"))
        {
            m_rawOutput.outputResult = m_fstatResponse;
            m_rawOutput.errorResult = m_fstatErrorResponse;

            if (!m_persistFstatResponse)
            {
                m_fstatResponse = m_fstatErrorResponse = "";
            }
        }
        else if (m_commandArgs.starts_with("add"))
        {
            if (m_addCallback)
            {
                m_addCallback(m_commandArgs);
            }
        }
        else if (m_commandArgs.starts_with("edit"))
        {
            if (m_editCallback)
            {
                m_editCallback(m_commandArgs);
            }
        }
        else if (m_commandArgs.starts_with("move"))
        {
            if (m_moveCallback)
            {
                m_moveCallback(m_commandArgs);
            }
        }
        else if (m_commandArgs.starts_with("delete"))
        {
            if (m_deleteCallback)
            {
                m_deleteCallback(m_commandArgs);
            }
        }
    }

    void MockPerforceCommand::ExecuteRawCommand()
    {
        ExecuteCommand();
    }

    void WaitForSourceControl(AZStd::binary_semaphore& waitSignal)
    {
        constexpr int WaitTimeMS = 2;
        constexpr int TimeLimitMS = 5000;

        int retryLimit = TimeLimitMS / WaitTimeMS;

        do
        {
            AZ::TickBus::ExecuteQueuedEvents();
            --retryLimit;
        }
        while (!waitSignal.try_acquire_for(AZStd::chrono::milliseconds(WaitTimeMS)) && retryLimit >= 0);

        ASSERT_GE(retryLimit, 0);
    }

    SourceControlTest::SourceControlTest()
    {
        BusConnect();
    }

    SourceControlTest::~SourceControlTest()
    {
        BusDisconnect();
    }

    void SourceControlTest::ConnectivityStateChanged(const AzToolsFramework::SourceControlState state)
    {
        m_connected = (state == AzToolsFramework::SourceControlState::Active);
        m_connectSignal.release();
    }

    void SourceControlTest::EnableSourceControl()
    {
        AzToolsFramework::SourceControlConnectionRequestBus::Broadcast(
            &AzToolsFramework::SourceControlConnectionRequestBus::Events::EnableSourceControl, true);

        WaitForSourceControl(m_connectSignal);
        ASSERT_TRUE(m_connected);
    }

    AZ::EntityId CreateDefaultEditorEntity(const char* name, AZ::Entity** outEntity /*= nullptr*/)
    {
        AZ::EntityId entityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            entityId, &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity, name);

        if (!entityId.IsValid())
        {
            AZ_Error("CreateDefaultEditorEntity", false, "Failed to create editor entity '%s'", name);
            return AZ::EntityId();
        }

        AZ::Entity* entity = GetEntityById(entityId);

        if (!entity)
        {
            AZ_Error("CreateDefaultEditorEntity", false, "Invalid entity obtained from Id %s", entityId.ToString().c_str());
            return AZ::EntityId();
        }

        entity->Deactivate();

        // add required components for the Editor entity
        entity->CreateComponent<Components::TransformComponent>();
        entity->CreateComponent<Components::EditorLockComponent>();
        entity->CreateComponent<Components::EditorVisibilityComponent>();

        // This is necessary to prevent a warning in the undo system.
        AzToolsFramework::ToolsApplicationRequests::Bus::Broadcast(
            &AzToolsFramework::ToolsApplicationRequests::Bus::Events::AddDirtyEntity,
            entity->GetId());

        entity->Activate();

        if (outEntity)
        {
            *outEntity = entity;
        }

        return entity->GetId();
    }

    AZ::EntityId CreateEditorLayerEntity(const char* name, AZ::Entity** outEntity /*= nullptr*/)
    {
        AZ::Entity* entity = nullptr;
        CreateDefaultEditorEntity(name, &entity);

        auto layer = aznew Layers::EditorLayerComponent();
        AZStd::vector<AZ::Component*> newComponents{ layer };

        EntityCompositionRequests::AddExistingComponentsOutcome componentAddResult;
        EntityCompositionRequestBus::BroadcastResult(
            componentAddResult, &EntityCompositionRequests::AddExistingComponentsToEntityById,
            entity->GetId(), newComponents);

        if (outEntity)
        {
            *outEntity = entity;
        }

        return entity->GetId();
    }

    AZ::Data::AssetId SaveAsSlice(
        AZStd::vector<AZ::Entity*> entities, AzToolsFramework::ToolsApplication* toolsApplication,
        SliceAssets& inoutSliceAssets)
    {
        AZ::Entity* sliceEntity = aznew AZ::Entity();
        AZ::SliceComponent* sliceComponent = aznew AZ::SliceComponent();
        sliceComponent->SetSerializeContext(toolsApplication->GetSerializeContext());
        for (const auto& entity : entities)
        {
            sliceComponent->AddEntity(entity);
        }

        // don't activate `sliceEntity`, whose purpose is to be attached by `sliceComponent`
        sliceEntity->AddComponent(sliceComponent);

        AZ::Data::AssetId assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom(), 0);
        AZ::Data::Asset<AZ::SliceAsset> sliceAssetHolder =
            AZ::Data::AssetManager::Instance().CreateAsset<AZ::SliceAsset>(assetId, AZ::Data::AssetLoadBehavior::Default);
        sliceAssetHolder.GetAs<AZ::SliceAsset>()->SetData(sliceEntity, sliceComponent);

        // hold on to sliceAssetHolder so it's not ref-counted away
        inoutSliceAssets.emplace(assetId, sliceAssetHolder);

        return assetId;
    }

    AZ::SliceComponent::EntityList InstantiateSlice(
        AZ::Data::AssetId sliceAssetId, const SliceAssets& sliceAssets)
    {
        auto foundItr = sliceAssets.find(sliceAssetId);
        AZ_Assert(foundItr != sliceAssets.end(), "sliceAssetId not in sliceAssets");

        AZ::SliceComponent* rootSlice;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);
        AZ::SliceComponent::SliceInstanceAddress sliceInstAddress = rootSlice->AddSlice(foundItr->second);
        rootSlice->Instantiate();

        const AZ::SliceComponent::InstantiatedContainer* instanceContainer =
            sliceInstAddress.GetInstance()->GetInstantiated();

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::HandleEntitiesAdded,
            instanceContainer->m_entities);

        return instanceContainer->m_entities;
    }

    void DestroySlices(SliceAssets& sliceAssets)
    {
        AZ::SliceComponent* rootSlice = nullptr;
        AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(
            rootSlice, &AzToolsFramework::SliceEditorEntityOwnershipServiceRequestBus::Events::GetEditorRootSlice);

        for (const auto& sliceAssetPair : sliceAssets)
        {
            rootSlice->RemoveSlice(sliceAssetPair.second);
        }

        sliceAssets.clear();
    }
} // namespace UnitTest
