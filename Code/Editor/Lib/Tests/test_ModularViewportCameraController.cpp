/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzToolsFramework/Input/QtEventToAzInputManager.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <EditorViewportWidget.h>
#include <Mocks/MockWindowRequests.h>

namespace UnitTest
{
    const QSize WidgetSize = QSize(1920, 1080);

    using AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    class ModularViewportCameraControllerFixture : public AllocatorsTestFixture
    {
    public:
        static const AzFramework::ViewportId TestViewportId = AzFramework::ViewportId(0);

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(WidgetSize);

            m_controllerList = AZStd::make_shared<AzFramework::ViewportControllerList>();
            m_controllerList->RegisterViewportContext(TestViewportId);

            m_inputChannelMapper = AZStd::make_unique<AzToolsFramework::QtEventToAzInputMapper>(m_rootWidget.get(), TestViewportId);
        }

        void TearDown()
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

    class TestModularCameraViewportContextImpl : public AtomToolsFramework::ModularCameraViewportContext
    {
    public:
        AZ::Transform GetCameraTransform() const override
        {
            return m_cameraTransform;
        }

        void SetCameraTransform(const AZ::Transform& transform) override
        {
            m_cameraTransform = transform;
        }

        void ConnectViewMatrixChangedHandler(AZ::RPI::ViewportContext::MatrixChangedEvent::Handler&) override
        {
            // noop
        }

    private:
        AZ::Transform m_cameraTransform = AZ::Transform::CreateIdentity();
    };

    TEST_F(ModularViewportCameraControllerFixture, Mouse_movement_does_not_accumulate_excessive_drift_in_modular_viewport_camera)
    {
        AzFramework::NativeWindowHandle nativeWindowHandle = nullptr;

        // Given
        // listen for events signaled from QtEventToAzInputMapper and forward to the controller list
        QObject::connect(
            m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
            [this, nativeWindowHandle](const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
            {
                m_controllerList->HandleInputChannelEvent(
                    AzFramework::ViewportControllerInputEvent{ TestViewportId, nativeWindowHandle, *inputChannel });
            });

        using ::testing::NiceMock;
        using ::testing::Return;

        NiceMock<MockWindowRequests> mockWindowRequests;
        mockWindowRequests.Connect(nativeWindowHandle);

        // note: WindowRequests is used internally by ModularViewportCameraController, this ensures it returns the viewport size we want
        ON_CALL(mockWindowRequests, GetClientAreaSize())
            .WillByDefault(Return(AzFramework::WindowSize(WidgetSize.width(), WidgetSize.height())));

        // create editor modular camera
        auto controller = CreateModularViewportCameraController(TestViewportId);

        // set some overrides for the test
        AtomToolsFramework::ModularCameraViewportContext* cameraViewportContextView = nullptr;
        controller->SetCameraViewportContextBuilderCallback(
            [&cameraViewportContextView](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext)
            {
                cameraViewportContext = AZStd::make_unique<TestModularCameraViewportContextImpl>();
                cameraViewportContextView = cameraViewportContext.get();
            });

        controller->SetCameraPropsBuilderCallback(
            [](AzFramework::CameraProps& cameraProps)
            {
                cameraProps.m_rotateSmoothingEnabledFn = []
                {
                    return false;
                };

                cameraProps.m_translateSmoothingEnabledFn = []
                {
                    return false;
                };
            });

        m_controllerList->Add(controller);

        // move to the center of the screen
        auto start = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        MouseMove(m_rootWidget.get(), start, QPoint(0, 0));
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(0.01666f), AZ::ScriptTimePoint() });

        // When
        // move mouse diagonally to top right, then to bottom left and back repeatedly
        auto current = start;
        auto halfDelta = QPoint(200, -200);
        for (int diagonals = 0; diagonals < 80; ++diagonals)
        {
            for (int i = 0; i < 50; ++i)
            {
                MousePressAndMove(m_rootWidget.get(), current, halfDelta / 50, Qt::MouseButton::RightButton);
                m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(0.01666f), AZ::ScriptTimePoint() });
                current += halfDelta / 50;
            }

            if (diagonals % 2 == 0)
            {
                halfDelta.setX(halfDelta.x() * -1);
                halfDelta.setY(halfDelta.y() * -1);
            }
        }

        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::RightButton, Qt::KeyboardModifier::NoModifier, current);
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(0.01666f), AZ::ScriptTimePoint() });

        // Then
        // ensure the camera rotation is the identity (no significant dift has occurred as we moved the mouse)
        const AZ::Transform cameraRotation = cameraViewportContextView->GetCameraTransform();
        EXPECT_THAT(cameraRotation.GetRotation(), IsClose(AZ::Quaternion::CreateIdentity()));

        mockWindowRequests.Disconnect();
    }
} // namespace UnitTest
