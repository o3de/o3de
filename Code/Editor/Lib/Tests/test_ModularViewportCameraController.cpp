/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Viewport/ModularViewportCameraController.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzFramework/Viewport/ViewportControllerList.h>
#include <AzToolsFramework/Input/QtEventToAzInputMapper.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <EditorViewportWidget.h>
#include <Mocks/MockWindowRequests.h>

namespace UnitTest
{
    const QSize WidgetSize = QSize(1920, 1080);

    using AzToolsFramework::ViewportInteraction::MouseInteractionEvent;

    class ViewportMouseCursorRequestImpl : public AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler
    {
    public:
        void Connect(const AzFramework::ViewportId viewportId, AzToolsFramework::QtEventToAzInputMapper* inputChannelMapper)
        {
            AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler::BusConnect(viewportId);
            m_inputChannelMapper = inputChannelMapper;
        }

        void Disconnect()
        {
            AzToolsFramework::ViewportInteraction::ViewportMouseCursorRequestBus::Handler::BusDisconnect();
        }

        // ViewportMouseCursorRequestBus overrides ...
        void BeginCursorCapture() override;
        void EndCursorCapture() override;
        bool IsMouseOver() const override;
        void SetOverrideCursor(AzToolsFramework::ViewportInteraction::CursorStyleOverride cursorStyleOverride) override;
        void ClearOverrideCursor() override;

    private:
        AzToolsFramework::QtEventToAzInputMapper* m_inputChannelMapper = nullptr;
    };

    void ViewportMouseCursorRequestImpl::BeginCursorCapture()
    {
        m_inputChannelMapper->SetCursorCaptureEnabled(true);
    }

    void ViewportMouseCursorRequestImpl::EndCursorCapture()
    {
        m_inputChannelMapper->SetCursorCaptureEnabled(false);
    }

    bool ViewportMouseCursorRequestImpl::IsMouseOver() const
    {
        return true;
    }

    void ViewportMouseCursorRequestImpl::SetOverrideCursor(
        [[maybe_unused]] AzToolsFramework::ViewportInteraction::CursorStyleOverride cursorStyleOverride)
    {
        // noop
    }

    void ViewportMouseCursorRequestImpl::ClearOverrideCursor()
    {
        // noop
    }

    class ModularViewportCameraControllerFixture : public AllocatorsTestFixture
    {
    public:
        static inline constexpr AzFramework::ViewportId TestViewportId = 1234;

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(WidgetSize);
            m_rootWidget->move(0, 0); // explicitly set the widget to be in the upper left corner

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

            AllocatorsTestFixture::TearDown();
        }

        void PrepareCollaborators()
        {
            AzFramework::NativeWindowHandle nativeWindowHandle = nullptr;

            // listen for events signaled from QtEventToAzInputMapper and forward to the controller list
            QObject::connect(
                m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
                [this, nativeWindowHandle](const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
                {
                    m_controllerList->HandleInputChannelEvent(
                        AzFramework::ViewportControllerInputEvent{ TestViewportId, nativeWindowHandle, *inputChannel });
                });

            m_mockWindowRequests.Connect(nativeWindowHandle);

            using ::testing::Return;
            // note: WindowRequests is used internally by ModularViewportCameraController, this ensures it returns the viewport size we want
            ON_CALL(m_mockWindowRequests, GetClientAreaSize())
                .WillByDefault(Return(AzFramework::WindowSize(WidgetSize.width(), WidgetSize.height())));

            // respond to begin/end cursor capture events
            m_viewportMouseCursorRequests.Connect(TestViewportId, m_inputChannelMapper.get());

            // create editor modular camera
            m_editorModularViewportCameraComposer = AZStd::make_unique<SandboxEditor::EditorModularViewportCameraComposer>(TestViewportId);
            auto controller = m_editorModularViewportCameraComposer->CreateModularViewportCameraController();

            // set some overrides for the test
            controller->SetCameraViewportContextBuilderCallback(
                [this](AZStd::unique_ptr<AtomToolsFramework::ModularCameraViewportContext>& cameraViewportContext)
                {
                    cameraViewportContext = AZStd::make_unique<AtomToolsFramework::PlaceholderModularCameraViewportContextImpl>();
                    m_cameraViewportContextView = cameraViewportContext.get();
                });

            // disable smoothing in the test
            controller->SetCameraPropsBuilderCallback(
                [](AzFramework::CameraProps& cameraProps)
                {
                    // note: rotateSmoothness is also used for roll (not related to camera input directly)
                    cameraProps.m_rotateSmoothnessFn = []
                    {
                        return 5.0f;
                    };

                    cameraProps.m_translateSmoothnessFn = []
                    {
                        return 5.0f;
                    };

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
        }

        void HaltCollaborators()
        {
            m_editorModularViewportCameraComposer.reset();
            m_mockWindowRequests.Disconnect();
            m_viewportMouseCursorRequests.Disconnect();
            m_cameraViewportContextView = nullptr;
        }

        void RepeatDiagonalMouseMovements(const AZStd::function<float()>& deltaTimeFn)
        {
            // move to the center of the screen
            const auto start = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
            MouseMove(m_rootWidget.get(), start, QPoint(0, 0));
            m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTimeFn()), AZ::ScriptTimePoint() });

            // move mouse diagonally to top right, then to bottom left and back repeatedly
            auto current = start;
            auto halfDelta = QPoint(200, -200);
            const int iterationsPerDiagonal = 50;
            for (int diagonals = 0; diagonals < 80; ++diagonals)
            {
                for (int i = 0; i < iterationsPerDiagonal; ++i)
                {
                    MousePressAndMove(m_rootWidget.get(), current, halfDelta / iterationsPerDiagonal, Qt::MouseButton::RightButton);
                    m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTimeFn()), AZ::ScriptTimePoint() });
                    current += halfDelta / iterationsPerDiagonal;
                }

                if (diagonals % 2 == 0)
                {
                    halfDelta.setX(halfDelta.x() * -1);
                    halfDelta.setY(halfDelta.y() * -1);
                }
            }

            QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::RightButton, Qt::KeyboardModifier::NoModifier, current);
            m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTimeFn()), AZ::ScriptTimePoint() });
        }

        AZStd::unique_ptr<QWidget> m_rootWidget;
        AzFramework::ViewportControllerListPtr m_controllerList;
        AZStd::unique_ptr<AzToolsFramework::QtEventToAzInputMapper> m_inputChannelMapper;
        ::testing::NiceMock<MockWindowRequests> m_mockWindowRequests;
        ViewportMouseCursorRequestImpl m_viewportMouseCursorRequests;
        AtomToolsFramework::ModularCameraViewportContext* m_cameraViewportContextView = nullptr;
        AZStd::unique_ptr<AZ::SettingsRegistryInterface> m_settingsRegistry;
        AZStd::unique_ptr<SandboxEditor::EditorModularViewportCameraComposer> m_editorModularViewportCameraComposer;
    };

    TEST_F(ModularViewportCameraControllerFixture, MouseMovementDoesNotAccumulateExcessiveDriftInModularViewportCameraWithVaryingDeltaTime)
    {
        SandboxEditor::SetCameraCaptureCursorForLook(false);

        // Given
        PrepareCollaborators();

        // When
        RepeatDiagonalMouseMovements(
            [t = 0.0f]() mutable
            {
                // vary between 30 and 50 fps (40 +/- 10)
                const float fps = 40.0f + (10.0f * AZStd::sin(t));
                t += AZ::DegToRad(5.0f);
                return 1.0f / fps;
            });

        // Then
        // ensure the camera rotation is the identity (no significant drift has occurred as we moved the mouse)
        const AZ::Transform cameraRotation = m_cameraViewportContextView->GetCameraTransform();
        EXPECT_THAT(cameraRotation.GetRotation(), IsClose(AZ::Quaternion::CreateIdentity()));

        // Clean-up
        HaltCollaborators();
    }

    class ModularViewportCameraControllerDeltaTimeParamFixture
        : public ModularViewportCameraControllerFixture
        , public ::testing::WithParamInterface<float> // delta time
    {
    };

    TEST_P(
        ModularViewportCameraControllerDeltaTimeParamFixture,
        MouseMovementDoesNotAccumulateExcessiveDriftInModularViewportCameraWithFixedDeltaTime)
    {
        SandboxEditor::SetCameraCaptureCursorForLook(false);

        // Given
        PrepareCollaborators();

        // When
        RepeatDiagonalMouseMovements(
            [this]
            {
                return GetParam();
            });

        // Then
        // ensure the camera rotation is the identity (no significant drift has occurred as we moved the mouse)
        const AZ::Transform cameraRotation = m_cameraViewportContextView->GetCameraTransform();
        EXPECT_THAT(cameraRotation.GetRotation(), IsClose(AZ::Quaternion::CreateIdentity()));

        // Clean-up
        HaltCollaborators();
    }

    INSTANTIATE_TEST_CASE_P(
        All, ModularViewportCameraControllerDeltaTimeParamFixture, testing::Values(1.0f / 60.0f, 1.0f / 50.0f, 1.0f / 30.0f));

    TEST_F(ModularViewportCameraControllerFixture, MouseMovementOrientatesCameraWhenCursorIsCaptured)
    {
        // Given
        PrepareCollaborators();
        // ensure cursor is captured
        SandboxEditor::SetCameraCaptureCursorForLook(true);

        const float deltaTime = 1.0f / 60.0f;

        // When
        // move to the center of the screen
        auto start = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        MouseMove(m_rootWidget.get(), start, QPoint(0, 0));
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        const auto mouseDelta = QPoint(5, 0);

        // initial movement to begin the camera behavior
        MousePressAndMove(m_rootWidget.get(), start, mouseDelta, Qt::MouseButton::RightButton);
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // move the cursor right
        for (int i = 0; i < 50; ++i)
        {
            MousePressAndMove(m_rootWidget.get(), start + mouseDelta, mouseDelta, Qt::MouseButton::RightButton);
            m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });
        }

        // move the cursor left (do an extra iteration moving left to account for the initial dead-zone)
        for (int i = 0; i < 51; ++i)
        {
            MousePressAndMove(m_rootWidget.get(), start + mouseDelta, -mouseDelta, Qt::MouseButton::RightButton);
            m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });
        }

        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::RightButton, Qt::KeyboardModifier::NoModifier, start + mouseDelta);
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // Then
        // retrieve the amount of yaw rotation
        const AZ::Quaternion cameraRotation = m_cameraViewportContextView->GetCameraTransform().GetRotation();
        const auto eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromQuaternion(cameraRotation));

        // camera should be back at the center (no yaw)
        using ::testing::FloatNear;
        EXPECT_THAT(eulerAngles.GetZ(), FloatNear(0.0f, 0.001f));

        // Clean-up
        HaltCollaborators();
    }

    TEST_F(ModularViewportCameraControllerFixture, CameraDoesNotContinueToRotateGivenNoInputWhenCaptured)
    {
        // Given
        PrepareCollaborators();
        SandboxEditor::SetCameraCaptureCursorForLook(true);

        const float deltaTime = 1.0f / 60.0f;

        // When
        // move to the center of the screen
        auto start = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        MouseMove(m_rootWidget.get(), start, QPoint(0, 0));
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // will move a small amount initially
        const auto mouseDelta = QPoint(5, 0);
        MousePressAndMove(m_rootWidget.get(), start, mouseDelta, Qt::MouseButton::RightButton);

        // ensure further updates to not continue to rotate
        for (int i = 0; i < 50; ++i)
        {
            m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });
        }

        // Then
        // ensure the camera rotation is no longer the identity
        const AZ::Quaternion cameraRotation = m_cameraViewportContextView->GetCameraTransform().GetRotation();
        const auto eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromQuaternion(cameraRotation));

        // initial amount of rotation after first mouse move
        using ::testing::FloatNear;
        EXPECT_THAT(eulerAngles.GetZ(), FloatNear(-0.025f, 0.001f));

        // Clean-up
        HaltCollaborators();
    }

    // test to verify deltas and cursor positions are handled correctly when the widget is moved
    TEST_F(ModularViewportCameraControllerFixture, CameraDoesNotStutterAfterWidgetIsMoved)
    {
        // Given
        PrepareCollaborators();
        SandboxEditor::SetCameraCaptureCursorForLook(true);

        const float deltaTime = 1.0f / 60.0f;

        // When
        // move cursor to the center of the screen
        auto start = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        MouseMove(m_rootWidget.get(), start, QPoint(0, 0));
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // move camera right
        const auto mouseDelta = QPoint(200, 0);
        MousePressAndMove(m_rootWidget.get(), start, mouseDelta, Qt::MouseButton::RightButton);
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        QTest::mouseRelease(m_rootWidget.get(), Qt::MouseButton::RightButton, Qt::NoModifier, start + mouseDelta);
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // update the position of the widget
        const auto offset = QPoint(500, 500);
        m_rootWidget->move(offset);

        // move cursor back to widget center
        MouseMove(m_rootWidget.get(), start, QPoint(0, 0));
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // move camera left
        MousePressAndMove(m_rootWidget.get(), start, -mouseDelta, Qt::MouseButton::RightButton);
        m_controllerList->UpdateViewport({ TestViewportId, AzFramework::FloatSeconds(deltaTime), AZ::ScriptTimePoint() });

        // Then
        // ensure the camera rotation has returned to the identity
        const AZ::Quaternion cameraRotation = m_cameraViewportContextView->GetCameraTransform().GetRotation();
        const auto eulerAngles = AzFramework::EulerAngles(AZ::Matrix3x3::CreateFromQuaternion(cameraRotation));

        using ::testing::FloatNear;
        EXPECT_THAT(eulerAngles.GetX(), FloatNear(0.0f, 0.001f));
        EXPECT_THAT(eulerAngles.GetZ(), FloatNear(0.0f, 0.001f));

        // Clean-up
        HaltCollaborators();
    }
} // namespace UnitTest
