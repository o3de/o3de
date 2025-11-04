/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Settings/SettingsRegistryImpl.h>

#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzToolsFramework/Input/QtEventToAzInputMapper.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <Editor/ViewportManipulatorController.h>
#include <Mocks/MockWindowRequests.h>

namespace UnitTest
{
    using AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    class EditorInteractionViewportSelectionFake : public AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Handler
    {
    public:
        void Connect();
        void Disconnect();

        // EditorInteractionSystemViewportSelectionRequestBus overrides ...
        const AzToolsFramework::EditorVisibleEntityDataCacheInterface* GetEntityDataCache() const override;
        void SetHandler(const AzToolsFramework::ViewportSelectionRequestsBuilderFn& interactionRequestsBuilder) override;
        void SetDefaultHandler() override;
        bool InternalHandleMouseViewportInteraction(const MouseInteractionEvent& mouseInteraction) override;
        bool InternalHandleMouseManipulatorInteraction(const MouseInteractionEvent& mouseInteraction) override;

        AZStd::function<bool(const MouseInteractionEvent& mouseInteraction)> m_internalHandleMouseViewportInteraction;
        AZStd::function<bool(const MouseInteractionEvent& mouseInteraction)> m_internalHandleMouseManipulatorInteraction;
    };

    void EditorInteractionViewportSelectionFake::Connect()
    {
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Handler::BusConnect(AzToolsFramework::GetEntityContextId());
    }

    void EditorInteractionViewportSelectionFake::Disconnect()
    {
        AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Handler::BusDisconnect();
    }

    const AzToolsFramework::EditorVisibleEntityDataCacheInterface* EditorInteractionViewportSelectionFake::GetEntityDataCache() const
    {
        return nullptr;
    }

    void EditorInteractionViewportSelectionFake::SetHandler(
        [[maybe_unused]] const AzToolsFramework::ViewportSelectionRequestsBuilderFn& interactionRequestsBuilder)
    {
        // noop
    }

    void EditorInteractionViewportSelectionFake::SetDefaultHandler()
    {
        // noop
    }

    bool EditorInteractionViewportSelectionFake::InternalHandleMouseViewportInteraction(const MouseInteractionEvent& mouseInteraction)
    {
        if (m_internalHandleMouseViewportInteraction)
        {
            return m_internalHandleMouseViewportInteraction(mouseInteraction);
        }

        return false;
    }

    bool EditorInteractionViewportSelectionFake::InternalHandleMouseManipulatorInteraction(const MouseInteractionEvent& mouseInteraction)
    {
        if (m_internalHandleMouseManipulatorInteraction)
        {
            return m_internalHandleMouseManipulatorInteraction(mouseInteraction);
        }

        return false;
    }

    class ViewportManipulatorControllerFixture : public LeakDetectionFixture
    {
    public:
        static inline constexpr AzFramework::ViewportId TestViewportId = 1234;
        static inline const QSize WidgetSize = QSize(1920, 1080);

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(WidgetSize);
            QApplication::setActiveWindow(m_rootWidget.get());

            m_controllerList = AZStd::make_shared<AzFramework::ViewportControllerList>();
            m_controllerList->RegisterViewportContext(TestViewportId);

            m_inputChannelMapper = AZStd::make_unique<AzToolsFramework::QtEventToAzInputMapper>(m_rootWidget.get(), TestViewportId);

            m_settingsRegistry = AZStd::make_unique<AZ::SettingsRegistryImpl>();
            AZ::SettingsRegistry::Register(m_settingsRegistry.get());
        }

        void TearDown() override
        {
            AZ::SettingsRegistry::Unregister(m_settingsRegistry.get());
            m_settingsRegistry.reset();

            m_inputChannelMapper.reset();

            m_controllerList->UnregisterViewportContext(TestViewportId);
            m_controllerList.reset();
            m_rootWidget.reset();

            QApplication::setActiveWindow(nullptr);

            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<QWidget> m_rootWidget;
        AzFramework::ViewportControllerListPtr m_controllerList;
        AZStd::unique_ptr<AzToolsFramework::QtEventToAzInputMapper> m_inputChannelMapper;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
    };

    TEST_F(ViewportManipulatorControllerFixture, AnEventIsNotPropagatedToTheViewportWhenAManipulatorHandlesItFirst)
    {
        // forward input events to our controller list
        QObject::connect(
            m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
            [this](const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
            {
                m_controllerList->HandleInputChannelEvent(
                    AzFramework::ViewportControllerInputEvent{ TestViewportId, nullptr, *inputChannel });
            });

        EditorInteractionViewportSelectionFake editorInteractionViewportFake;
        editorInteractionViewportFake.m_internalHandleMouseManipulatorInteraction = [](const MouseInteractionEvent&)
        {
            // report the event was handled (manipulator was interacted with)
            return true;
        };

        bool viewportInteractionCalled = false;
        editorInteractionViewportFake.m_internalHandleMouseViewportInteraction = [&viewportInteractionCalled](const MouseInteractionEvent&)
        {
            // we should not call this as the manipulator will have consumed this event
            viewportInteractionCalled = true;
            return true;
        };

        editorInteractionViewportFake.Connect();

        m_controllerList->Add(AZStd::make_shared<SandboxEditor::ViewportManipulatorController>());

        // simulate a press and move
        MousePressAndMove(m_rootWidget.get(), QPoint(10, 10), QPoint(10, 10), Qt::MouseButton::LeftButton);
        MouseMove(m_rootWidget.get(), QPoint(20, 20), QPoint(10, 10), Qt::MouseButton::LeftButton);
        MouseMove(m_rootWidget.get(), QPoint(30, 30), QPoint(0, 0), Qt::MouseButton::LeftButton);
        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(30, 30));

        // ensure the viewport did not receive the event when it was intercepted first by the manipulator
        EXPECT_FALSE(viewportInteractionCalled);

        editorInteractionViewportFake.Disconnect();
    }

    TEST_F(ViewportManipulatorControllerFixture, ChangingFocusDoesNotClearInput)
    {
        bool endedEvent = false;
        // detect input events and ensure that the Alt key press does not end before the end of the test
        QObject::connect(
            m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
            [&endedEvent](const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
            {
                if (inputChannel->GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::ModifierAltL &&
                    inputChannel->IsStateEnded())
                {
                    endedEvent = true;
                }
            });

        // given
        auto* secondaryWidget = new QWidget(m_rootWidget.get());

        m_rootWidget->show();
        secondaryWidget->show();

        m_rootWidget->setFocus();

        // simulate a key press when root widget has focus
        QTest::keyPress(m_rootWidget.get(), Qt::Key_Alt, Qt::KeyboardModifier::AltModifier);

        // when
        // change focus to secondary widget
        secondaryWidget->setFocus();

        // then
        // the alt key was not released (cleared)
        EXPECT_FALSE(endedEvent);
    }

    // note: Application State Change includes events such as switching to another application or minimizing
    // the current application
    TEST_F(ViewportManipulatorControllerFixture, ApplicationStateChangeDoesClearInput)
    {
        bool endedEvent = false;
        // detect input events and ensure that the Alt key press does not end before the end of the test
        QObject::connect(
            m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
            [&endedEvent](const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
            {
                if (inputChannel->GetInputChannelId() == AzFramework::InputDeviceKeyboard::Key::AlphanumericW &&
                    inputChannel->IsStateEnded())
                {
                    endedEvent = true;
                }
            });

        // given
        auto* secondaryWidget = new QWidget(m_rootWidget.get());

        m_rootWidget->show();
        secondaryWidget->show();

        m_rootWidget->setFocus();

        // simulate a key press when root widget has focus
        QTest::keyPress(m_rootWidget.get(), Qt::Key_W);

        // when
        // simulate changing the window state
        QApplicationStateChangeEvent applicationStateChangeEvent(Qt::ApplicationState::ApplicationInactive);
        QCoreApplication::sendEvent(m_rootWidget.get(), &applicationStateChangeEvent);

        // then
        // the key was released (cleared)
        EXPECT_TRUE(endedEvent);
    }

    TEST_F(ViewportManipulatorControllerFixture, DoubleClickIsNotRegisteredIfMouseDeltaHasMovedMoreThanDeadzoneInClickInterval)
    {
        AzFramework::NativeWindowHandle nativeWindowHandle = nullptr;

        // forward input events to our controller list
        QObject::connect(
            m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
            [this, nativeWindowHandle](const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
            {
                m_controllerList->HandleInputChannelEvent(
                    AzFramework::ViewportControllerInputEvent{ TestViewportId, nativeWindowHandle, *inputChannel });
            });

        ::testing::NiceMock<MockWindowRequests> mockWindowRequests;
        mockWindowRequests.Connect(nativeWindowHandle);

        using ::testing::Return;
        // note: WindowRequests is used internally by ViewportManipulatorController
        ON_CALL(mockWindowRequests, GetClientAreaSize())
            .WillByDefault(Return(AzFramework::WindowSize(WidgetSize.width(), WidgetSize.height())));
        ON_CALL(mockWindowRequests, GetRenderResolution())
            .WillByDefault(Return(AzFramework::WindowSize(WidgetSize.width(), WidgetSize.height())));

        EditorInteractionViewportSelectionFake editorInteractionViewportFake;
        editorInteractionViewportFake.m_internalHandleMouseManipulatorInteraction = [](const MouseInteractionEvent&)
        {
            // report the event was not handled (manipulator was not interacted with)
            return false;
        };

        bool doubleClickDetected = false;
        editorInteractionViewportFake.m_internalHandleMouseViewportInteraction =
            [&doubleClickDetected](const MouseInteractionEvent& mouseInteractionEvent)
        {
            // ensure no double click event is detected with the given inputs below
            if (mouseInteractionEvent.m_mouseEvent == AzToolsFramework::ViewportInteraction::MouseEvent::DoubleClick)
            {
                doubleClickDetected = true;
            }

            return true;
        };

        editorInteractionViewportFake.Connect();

        m_controllerList->Add(AZStd::make_shared<SandboxEditor::ViewportManipulatorController>());

        // simulate a click, move, click
        MouseMove(m_rootWidget.get(), QPoint(0, 0), QPoint(10, 10));
        MousePressAndMove(m_rootWidget.get(), QPoint(10, 10), QPoint(0, 0), Qt::MouseButton::LeftButton);
        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(10, 10));
        MouseMove(m_rootWidget.get(), QPoint(10, 10), QPoint(20, 20));
        MousePressAndMove(m_rootWidget.get(), QPoint(20, 20), QPoint(0, 0), Qt::MouseButton::LeftButton);
        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(20, 20));

        // ensure no double click was detected
        EXPECT_FALSE(doubleClickDetected);

        // simulate double click (sanity check it still is detected correctly with no movement)
        MouseMove(m_rootWidget.get(), QPoint(0, 0), QPoint(10, 10));
        MousePressAndMove(m_rootWidget.get(), QPoint(10, 10), QPoint(0, 0), Qt::MouseButton::LeftButton);
        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(10, 10));
        MousePressAndMove(m_rootWidget.get(), QPoint(10, 10), QPoint(0, 0), Qt::MouseButton::LeftButton);
        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::LeftButton, Qt::KeyboardModifier::NoModifier, QPoint(10, 10));

        // ensure a double click was detected
        EXPECT_TRUE(doubleClickDetected);

        mockWindowRequests.Disconnect();
        editorInteractionViewportFake.Disconnect();
    }
} // namespace UnitTest
