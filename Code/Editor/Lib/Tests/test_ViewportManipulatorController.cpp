/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Viewport/CameraInput.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzToolsFramework/Input/QtEventToAzInputManager.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ViewportSelection/EditorInteractionSystemViewportSelectionRequestBus.h>
#include <Editor/ViewportManipulatorController.h>

namespace UnitTest
{
    using AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    class EditorInteractionViewportSelectionFake : public AzToolsFramework::EditorInteractionSystemViewportSelectionRequestBus::Handler
    {
    public:
        void Connect();
        void Disconnect();

        // EditorInteractionSystemViewportSelectionRequestBus overrides ...
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

    class ViewportManipulatorControllerFixture : public AllocatorsTestFixture
    {
    public:
        static const AzFramework::ViewportId TestViewportId;

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(QSize(100, 100));

            m_controllerList = AZStd::make_shared<AzFramework::ViewportControllerList>();
            m_controllerList->RegisterViewportContext(TestViewportId);

            m_inputChannelMapper = AZStd::make_unique<AzToolsFramework::QtEventToAzInputMapper>(m_rootWidget.get(), TestViewportId);
        }

        void TearDown() override
        {
            m_inputChannelMapper.reset();

            m_controllerList->UnregisterViewportContext(TestViewportId);
            m_controllerList.reset();
            m_rootWidget.reset();

            AllocatorsTestFixture::TearDown();
        }

        AZStd::unique_ptr<QWidget> m_rootWidget;
        AzFramework::ViewportControllerListPtr m_controllerList;
        AZStd::unique_ptr<AzToolsFramework::QtEventToAzInputMapper> m_inputChannelMapper;
    };

    const AzFramework::ViewportId ViewportManipulatorControllerFixture::TestViewportId = AzFramework::ViewportId(0);

    TEST_F(ViewportManipulatorControllerFixture, An_event_is_not_propagated_to_the_viewport_when_a_manipulator_handles_it_first)
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
} // namespace UnitTest
