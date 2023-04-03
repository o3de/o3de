/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Input/QtEventToAzInputMapper.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>

#include <AzFramework/Input/Events/InputTextEventListener.h>


namespace UnitTest
{
    static bool IsMouseButton(const AzFramework::InputChannelId& inputChannelId)
    {
        const auto& buttons = AzFramework::InputDeviceMouse::Button::All;
        const auto& it = AZStd::find(buttons.cbegin(), buttons.cend(), inputChannelId);
        return it != buttons.cend();
    }

    class QtEventToAzInputMapperFixture
        : public LeakDetectionFixture
        , public AzFramework::InputChannelNotificationBus::Handler
        , public AzFramework::InputTextNotificationBus::Handler
    {
    public:
        static inline constexpr QSize WidgetSize = QSize(1920, 1080);
        static inline constexpr int TestDeviceIdSeed = 4321;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(WidgetSize);
            m_rootWidget->move(0, 0);

            m_inputChannelMapper = AZStd::make_unique<AzToolsFramework::QtEventToAzInputMapper>(m_rootWidget.get(), TestDeviceIdSeed);

            // listen for events signaled from QtEventToAzInputMapper and forward to the controller list
            QObject::connect(m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
                [this]([[maybe_unused]] const AzFramework::InputChannel* inputChannel, QEvent* event)
                {
                    if(event == nullptr) 
                    {
                        return;
                    }
                    const QEvent::Type eventType = event->type();

                    if (eventType == QEvent::Type::MouseButtonPress ||
                        eventType == QEvent::Type::MouseButtonRelease ||
                        eventType == QEvent::Type::MouseButtonDblClick)
                    {
                        m_signalEvents.push_back(QtEventInfo(static_cast<QMouseEvent*>(event)));
                        event->accept();
                    }
                    else if (eventType == QEvent::Type::Wheel)
                    {
                        m_signalEvents.push_back(QtEventInfo(static_cast<QWheelEvent*>(event)));
                        event->accept();
                    }
                    else if (eventType == QEvent::Type::KeyPress ||
                            eventType == QEvent::Type::KeyRelease ||
                            eventType == QEvent::Type::ShortcutOverride)
                    {
                        m_signalEvents.push_back(QtEventInfo(static_cast<QKeyEvent*>(event)));
                        event->accept();
                    }
                });
        }

        void TearDown() override
        {
            m_inputChannelMapper.reset();

            m_rootWidget.reset();

            LeakDetectionFixture::TearDown();
        }

        void OnInputChannelEvent(const AzFramework::InputChannel& inputChannel, bool& hasBeenConsumed) override
        {
            AZ_Assert(hasBeenConsumed == false, "Unexpected input event consumed elsewhere during QtEventToAzInputMapper tests");

            const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
            const AzFramework::InputDeviceId& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

            if (AzFramework::InputDeviceMouse::IsMouseDevice(inputDeviceId))
            {
                if (IsMouseButton(inputChannelId))
                {
                    m_azChannelEvents.push_back(AzEventInfo(inputChannel));
                    hasBeenConsumed = m_captureAzEvents;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::Movement::Z)
                {
                    m_azChannelEvents.push_back(AzEventInfo(inputChannel));
                    hasBeenConsumed = m_captureAzEvents;
                }
                else if (inputChannelId == AzFramework::InputDeviceMouse::SystemCursorPosition) 
                {
                    m_azCursorPositions.push_back(*inputChannel.GetCustomData<AzFramework::InputChannel::PositionData2D>());
                    hasBeenConsumed = m_captureAzEvents;
                }
            }
            else if (AzFramework::InputDeviceKeyboard::IsKeyboardDevice(inputDeviceId))
            {
                m_azChannelEvents.push_back(AzEventInfo(inputChannel));
                hasBeenConsumed = m_captureAzEvents;
            }
        }

        void OnInputTextEvent(const AZStd::string& textUtf8, bool& hasBeenConsumed) override
        {
            AZ_Assert(hasBeenConsumed == false, "Unexpected text event consumed elsewhere during QtEventToAzInputMapper tests");

            m_azTextEvents.push_back(textUtf8);
            hasBeenConsumed = m_captureTextEvents;
        }

        // simple structure for caching minimal QtEvent data necessary for testing
        struct QtEventInfo
        {
            explicit QtEventInfo(QMouseEvent* mouseEvent)
                : m_eventType(mouseEvent->type())
                , m_button(mouseEvent->button())
            {
            }

            explicit QtEventInfo(QWheelEvent* mouseWheelEvent)
                : m_eventType(mouseWheelEvent->type())
                , m_scrollPhase(mouseWheelEvent->phase())
            {
            }

            explicit QtEventInfo(QKeyEvent* keyEvent)
                : m_eventType(keyEvent->type())
                , m_key(keyEvent->key())
            {
            }

            QEvent::Type m_eventType{ QEvent::None };
            Qt::MouseButton m_button{ Qt::NoButton };
            Qt::ScrollPhase m_scrollPhase{ Qt::NoScrollPhase };
            int m_key{ 0 };
        };

        // simple structure for caching minimal AzInput event data necessary for testing
        struct AzEventInfo
        {
            AzEventInfo() = delete;
            explicit AzEventInfo(const AzFramework::InputChannel& inputChannel)
                : m_inputChannelId(inputChannel.GetInputChannelId())
                , m_isActive(inputChannel.IsActive())
            {
            }

            AzFramework::InputChannelId m_inputChannelId;
            bool m_isActive;
        };


        AZStd::unique_ptr<QWidget> m_rootWidget;

        AZStd::unique_ptr<AzToolsFramework::QtEventToAzInputMapper> m_inputChannelMapper;

        AZStd::vector<QtEventInfo> m_signalEvents;
        AZStd::vector<AzEventInfo> m_azChannelEvents;
        AZStd::vector<AZStd::string> m_azTextEvents;
        AZStd::vector<AzFramework::InputChannel::PositionData2D> m_azCursorPositions;

        bool m_captureAzEvents{ false };
        bool m_captureTextEvents{ false };
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Qt event forwarding through the internal signal handler test
    TEST_F(QtEventToAzInputMapperFixture, MouseWheel_NoAzHandlers_ReceivedThreeSignalAndZeroAzChannelEvents)
    {
        // setup
        const QPoint mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        const QPoint scrollDelta = QPoint(10, 10);

        MouseScroll(m_rootWidget.get(), mouseEventPos, scrollDelta);

        // qt validation
        ASSERT_EQ(m_signalEvents.size(), 3);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::Wheel);
        EXPECT_EQ(m_signalEvents[0].m_scrollPhase, Qt::ScrollBegin);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::Wheel);
        EXPECT_EQ(m_signalEvents[1].m_scrollPhase, Qt::ScrollUpdate);

        EXPECT_EQ(m_signalEvents[2].m_eventType, QEvent::Type::Wheel);
        EXPECT_EQ(m_signalEvents[2].m_scrollPhase, Qt::ScrollEnd);

        // az validation
        EXPECT_EQ(m_azChannelEvents.size(), 0);
    }

    // Qt event to AzInput event conversion test
    TEST_F(QtEventToAzInputMapperFixture, MouseWheel_AzHandlerNotCaptured_ReceivedThreeSignalAndThreeAzChannelEvents)
    {
        // setup
        const AzFramework::InputChannelId mouseWheelId = AzFramework::InputDeviceMouse::Movement::Z;
        const char* mouseWheelChannelName = mouseWheelId.GetName();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = false;

        const QPoint mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        const QPoint scrollDelta = QPoint(10, 10);

        MouseScroll(m_rootWidget.get(), mouseEventPos, scrollDelta);

        // qt validation
        ASSERT_EQ(m_signalEvents.size(), 3);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::Wheel);
        EXPECT_EQ(m_signalEvents[0].m_scrollPhase, Qt::ScrollBegin);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::Wheel);
        EXPECT_EQ(m_signalEvents[1].m_scrollPhase, Qt::ScrollUpdate);

        EXPECT_EQ(m_signalEvents[2].m_eventType, QEvent::Type::Wheel);
        EXPECT_EQ(m_signalEvents[2].m_scrollPhase, Qt::ScrollEnd);

        // az validation
        ASSERT_EQ(m_azChannelEvents.size(), 3);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), mouseWheelChannelName);
        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), mouseWheelChannelName);
        EXPECT_STREQ(m_azChannelEvents[2].m_inputChannelId.GetName(), mouseWheelChannelName);

        // cleanup
        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    // AzInput event handler consumption test
    TEST_F(QtEventToAzInputMapperFixture, MouseWheel_AzHandlerCaptured_ReceivedZeroSignalAndThreeAzChannelEvents)
    {
        // setup
        const AzFramework::InputChannelId mouseWheelId = AzFramework::InputDeviceMouse::Movement::Z;
        const char* mouseWheelChannelName = mouseWheelId.GetName();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = true;

        const QPoint mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        const QPoint scrollDelta = QPoint(10, 10);

        MouseScroll(m_rootWidget.get(), mouseEventPos, scrollDelta);

        // qt validation
        EXPECT_EQ(m_signalEvents.size(), 0);

        // az validation
        ASSERT_EQ(m_azChannelEvents.size(), 3);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), mouseWheelChannelName);
        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), mouseWheelChannelName);
        EXPECT_STREQ(m_azChannelEvents[2].m_inputChannelId.GetName(), mouseWheelChannelName);

        // cleanup
        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct MouseButtonIdsParam
    {
        Qt::MouseButton m_qt;
        AzFramework::InputChannelId m_az;
    };

    class MouseButtonParamQtEventToAzInputMapperFixture
        : public QtEventToAzInputMapperFixture
        , public ::testing::WithParamInterface<MouseButtonIdsParam>
    {
    };

    // Qt event forwarding through the internal signal handler test
    TEST_P(MouseButtonParamQtEventToAzInputMapperFixture, MouseClick_NoAzHandlers_ReceivedTwoSignalAndZeroAzChannelEvents)
    {
        // setup
        const MouseButtonIdsParam mouseButtonIds = GetParam();

        const QPoint mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        QTest::mouseClick(m_rootWidget.get(), mouseButtonIds.m_qt, Qt::NoModifier, mouseEventPos);

        // qt validation
        ASSERT_EQ(m_signalEvents.size(), 2);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::MouseButtonPress);
        EXPECT_EQ(m_signalEvents[0].m_button, mouseButtonIds.m_qt);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::MouseButtonRelease);
        EXPECT_EQ(m_signalEvents[1].m_button, mouseButtonIds.m_qt);

        // az validation
        EXPECT_EQ(m_azChannelEvents.size(), 0);
    }

    // Qt event to AzInput event conversion test
    TEST_P(MouseButtonParamQtEventToAzInputMapperFixture, MouseClick_AzHandlerNotCaptured_ReceivedTwoSignalAndTwoAzChannelEvents)
    {
        // setup
        const MouseButtonIdsParam mouseButtonIds = GetParam();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = false;

        const QPoint mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        QTest::mouseClick(m_rootWidget.get(), mouseButtonIds.m_qt, Qt::NoModifier, mouseEventPos);

        // qt validation
        ASSERT_EQ(m_signalEvents.size(), 2);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::MouseButtonPress);
        EXPECT_EQ(m_signalEvents[0].m_button, mouseButtonIds.m_qt);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::MouseButtonRelease);
        EXPECT_EQ(m_signalEvents[1].m_button, mouseButtonIds.m_qt);

        // az validation
        ASSERT_EQ(m_azChannelEvents.size(), 2);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_TRUE(m_azChannelEvents[0].m_isActive);

        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_FALSE(m_azChannelEvents[1].m_isActive);

        // cleanup
        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    // AzInput event handler consumption test
    TEST_P(MouseButtonParamQtEventToAzInputMapperFixture, MouseClick_AzHandlerCaptured_ReceivedZeroSignalAndTwoAzChannelEvents)
    {
        // setup
        const MouseButtonIdsParam mouseButtonIds = GetParam();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = true;

        const QPoint mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        QTest::mouseClick(m_rootWidget.get(), mouseButtonIds.m_qt, Qt::NoModifier, mouseEventPos);

        // qt validation
        EXPECT_EQ(m_signalEvents.size(), 0);

        // az validation
        ASSERT_EQ(m_azChannelEvents.size(), 2);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_TRUE(m_azChannelEvents[0].m_isActive);

        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_FALSE(m_azChannelEvents[1].m_isActive);

        // cleanup
        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    INSTANTIATE_TEST_CASE_P(All, MouseButtonParamQtEventToAzInputMapperFixture,
        testing::Values(
            MouseButtonIdsParam{ Qt::MouseButton::LeftButton, AzFramework::InputDeviceMouse::Button::Left },
            MouseButtonIdsParam{ Qt::MouseButton::RightButton, AzFramework::InputDeviceMouse::Button::Right },
            MouseButtonIdsParam{ Qt::MouseButton::MiddleButton, AzFramework::InputDeviceMouse::Button::Middle }
        ),
        [](const ::testing::TestParamInfo<MouseButtonIdsParam>& info)
        {
            return info.param.m_az.GetName();
        }
    );

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    struct KeyEventIdsParam
    {
        Qt::Key m_qt;
        AzFramework::InputChannelId m_az;
    };

    class PrintableKeyEventParamQtEventToAzInputMapperFixture
        : public QtEventToAzInputMapperFixture
        , public ::testing::WithParamInterface<KeyEventIdsParam>
    {
    };

    // Qt event forwarding through the internal signal handler test
    TEST_P(PrintableKeyEventParamQtEventToAzInputMapperFixture, KeyClick_NoAzHandlers_ReceivedTwoSignalAndZeroAzEvents)
    {
        // setup
        const KeyEventIdsParam keyEventIds = GetParam();
        const Qt::KeyboardModifiers modifiers = Qt::NoModifier;

        QTest::keyClick(m_rootWidget.get(), keyEventIds.m_qt, modifiers);

        // qt validation
        ASSERT_EQ(m_signalEvents.size(), 2);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::KeyPress);
        EXPECT_EQ(m_signalEvents[0].m_key, keyEventIds.m_qt);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::KeyRelease);
        EXPECT_EQ(m_signalEvents[1].m_key, keyEventIds.m_qt);

        // az validation
        EXPECT_EQ(m_azChannelEvents.size(), 0);
        EXPECT_EQ(m_azTextEvents.size(), 0);
    }

    // Qt event to AzInput event conversion test
    TEST_P(PrintableKeyEventParamQtEventToAzInputMapperFixture, KeyClick_AzHandlersNotCaptured_ReceivedTwoSignalAndThreeAzEvents)
    {
        // setup
        const KeyEventIdsParam keyEventIds = GetParam();
        const Qt::KeyboardModifiers modifiers = Qt::NoModifier;

        AZStd::string keyAsText = QtKeyToAzString(keyEventIds.m_qt, modifiers);

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = false;

        AzFramework::InputTextNotificationBus::Handler::BusConnect();
        m_captureAzEvents = false;

        QTest::keyClick(m_rootWidget.get(), keyEventIds.m_qt, modifiers);

        // qt validation
        ASSERT_EQ(m_signalEvents.size(), 2);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::KeyPress);
        EXPECT_EQ(m_signalEvents[0].m_key, keyEventIds.m_qt);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::KeyRelease);
        EXPECT_EQ(m_signalEvents[1].m_key, keyEventIds.m_qt);

        // az validation
        ASSERT_EQ(m_azTextEvents.size(), 1);

        EXPECT_STREQ(m_azTextEvents[0].c_str(), keyAsText.c_str());

        ASSERT_EQ(m_azChannelEvents.size(), 2);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), keyEventIds.m_az.GetName());
        EXPECT_TRUE(m_azChannelEvents[0].m_isActive);

        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), keyEventIds.m_az.GetName());
        EXPECT_FALSE(m_azChannelEvents[1].m_isActive);

        // cleanup
        AzFramework::InputTextNotificationBus::Handler::BusDisconnect();
        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    INSTANTIATE_TEST_CASE_P(All, PrintableKeyEventParamQtEventToAzInputMapperFixture,
        testing::Values(
            KeyEventIdsParam{ Qt::Key_0, AzFramework::InputDeviceKeyboard::Key::Alphanumeric0 },
            KeyEventIdsParam{ Qt::Key_1, AzFramework::InputDeviceKeyboard::Key::Alphanumeric1 },
            KeyEventIdsParam{ Qt::Key_2, AzFramework::InputDeviceKeyboard::Key::Alphanumeric2 },
            KeyEventIdsParam{ Qt::Key_3, AzFramework::InputDeviceKeyboard::Key::Alphanumeric3 },
            KeyEventIdsParam{ Qt::Key_4, AzFramework::InputDeviceKeyboard::Key::Alphanumeric4 },
            KeyEventIdsParam{ Qt::Key_5, AzFramework::InputDeviceKeyboard::Key::Alphanumeric5 },
            KeyEventIdsParam{ Qt::Key_6, AzFramework::InputDeviceKeyboard::Key::Alphanumeric6 },
            KeyEventIdsParam{ Qt::Key_7, AzFramework::InputDeviceKeyboard::Key::Alphanumeric7 },
            KeyEventIdsParam{ Qt::Key_8, AzFramework::InputDeviceKeyboard::Key::Alphanumeric8 },
            KeyEventIdsParam{ Qt::Key_9, AzFramework::InputDeviceKeyboard::Key::Alphanumeric9 },

            KeyEventIdsParam{ Qt::Key_A, AzFramework::InputDeviceKeyboard::Key::AlphanumericA },
            KeyEventIdsParam{ Qt::Key_B, AzFramework::InputDeviceKeyboard::Key::AlphanumericB },
            KeyEventIdsParam{ Qt::Key_C, AzFramework::InputDeviceKeyboard::Key::AlphanumericC },
            KeyEventIdsParam{ Qt::Key_D, AzFramework::InputDeviceKeyboard::Key::AlphanumericD },
            KeyEventIdsParam{ Qt::Key_E, AzFramework::InputDeviceKeyboard::Key::AlphanumericE },
            KeyEventIdsParam{ Qt::Key_F, AzFramework::InputDeviceKeyboard::Key::AlphanumericF },
            KeyEventIdsParam{ Qt::Key_G, AzFramework::InputDeviceKeyboard::Key::AlphanumericG },
            KeyEventIdsParam{ Qt::Key_H, AzFramework::InputDeviceKeyboard::Key::AlphanumericH },
            KeyEventIdsParam{ Qt::Key_I, AzFramework::InputDeviceKeyboard::Key::AlphanumericI },
            KeyEventIdsParam{ Qt::Key_J, AzFramework::InputDeviceKeyboard::Key::AlphanumericJ },
            KeyEventIdsParam{ Qt::Key_K, AzFramework::InputDeviceKeyboard::Key::AlphanumericK },
            KeyEventIdsParam{ Qt::Key_L, AzFramework::InputDeviceKeyboard::Key::AlphanumericL },
            KeyEventIdsParam{ Qt::Key_M, AzFramework::InputDeviceKeyboard::Key::AlphanumericM },
            KeyEventIdsParam{ Qt::Key_N, AzFramework::InputDeviceKeyboard::Key::AlphanumericN },
            KeyEventIdsParam{ Qt::Key_O, AzFramework::InputDeviceKeyboard::Key::AlphanumericO },
            KeyEventIdsParam{ Qt::Key_P, AzFramework::InputDeviceKeyboard::Key::AlphanumericP },
            KeyEventIdsParam{ Qt::Key_Q, AzFramework::InputDeviceKeyboard::Key::AlphanumericQ },
            KeyEventIdsParam{ Qt::Key_R, AzFramework::InputDeviceKeyboard::Key::AlphanumericR },
            KeyEventIdsParam{ Qt::Key_S, AzFramework::InputDeviceKeyboard::Key::AlphanumericS },
            KeyEventIdsParam{ Qt::Key_T, AzFramework::InputDeviceKeyboard::Key::AlphanumericT },
            KeyEventIdsParam{ Qt::Key_U, AzFramework::InputDeviceKeyboard::Key::AlphanumericU },
            KeyEventIdsParam{ Qt::Key_V, AzFramework::InputDeviceKeyboard::Key::AlphanumericV },
            KeyEventIdsParam{ Qt::Key_W, AzFramework::InputDeviceKeyboard::Key::AlphanumericW },
            KeyEventIdsParam{ Qt::Key_X, AzFramework::InputDeviceKeyboard::Key::AlphanumericX },
            KeyEventIdsParam{ Qt::Key_Y, AzFramework::InputDeviceKeyboard::Key::AlphanumericY },
            KeyEventIdsParam{ Qt::Key_Z, AzFramework::InputDeviceKeyboard::Key::AlphanumericZ },

            // these may need to be special cased due to the printable text conversion
            //KeyEventIdsParam{ Qt::Key_Space, AzFramework::InputDeviceKeyboard::Key::EditSpace },
            //KeyEventIdsParam{ Qt::Key_Tab, AzFramework::InputDeviceKeyboard::Key::EditTab },

            KeyEventIdsParam{ Qt::Key_Apostrophe, AzFramework::InputDeviceKeyboard::Key::PunctuationApostrophe },
            KeyEventIdsParam{ Qt::Key_Backslash, AzFramework::InputDeviceKeyboard::Key::PunctuationBackslash },
            KeyEventIdsParam{ Qt::Key_BracketLeft, AzFramework::InputDeviceKeyboard::Key::PunctuationBracketL },
            KeyEventIdsParam{ Qt::Key_BracketRight, AzFramework::InputDeviceKeyboard::Key::PunctuationBracketR },
            KeyEventIdsParam{ Qt::Key_Comma, AzFramework::InputDeviceKeyboard::Key::PunctuationComma },
            KeyEventIdsParam{ Qt::Key_Equal, AzFramework::InputDeviceKeyboard::Key::PunctuationEquals },
            KeyEventIdsParam{ Qt::Key_hyphen, AzFramework::InputDeviceKeyboard::Key::PunctuationHyphen },
            KeyEventIdsParam{ Qt::Key_Period, AzFramework::InputDeviceKeyboard::Key::PunctuationPeriod },
            KeyEventIdsParam{ Qt::Key_Semicolon, AzFramework::InputDeviceKeyboard::Key::PunctuationSemicolon },
            KeyEventIdsParam{ Qt::Key_Slash, AzFramework::InputDeviceKeyboard::Key::PunctuationSlash },
            KeyEventIdsParam{ Qt::Key_QuoteLeft, AzFramework::InputDeviceKeyboard::Key::PunctuationTilde }
        ),
        [](const ::testing::TestParamInfo<KeyEventIdsParam>& info)
        {
            return info.param.m_az.GetName();
        }
    );

    struct MouseMoveParam
    {
        AzToolsFramework::CursorInputMode mode;
        int iterations;
        QPoint startPos;
        QPoint deltaPos;
        QPoint expectedPos;
        const char* name;
    };

    class MoveMoveWrapParamQtEventToAzInputMapperFixture
        : public QtEventToAzInputMapperFixture
        , public ::testing::WithParamInterface<MouseMoveParam>
    {
    };

    TEST_P(MoveMoveWrapParamQtEventToAzInputMapperFixture, DISABLED_MouseMove_NoAzHandlers_VerifyMouseMovementViewport)
    {

        // setup
        const MouseMoveParam mouseMoveParam = GetParam();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = true;

        m_rootWidget->move(100, 100);
        QScreen* screen = m_rootWidget->screen();
        MouseMove(m_rootWidget.get(), mouseMoveParam.startPos, QPoint(0, 0));

        // given
        m_inputChannelMapper->SetCursorMode(mouseMoveParam.mode);
        m_azCursorPositions.clear();
        for (float i = 0; i < mouseMoveParam.iterations; i++)
        {
            MouseMove(m_rootWidget.get(), m_rootWidget->mapFromGlobal(QCursor::pos(screen)), (mouseMoveParam.deltaPos / mouseMoveParam.iterations));            
        }

        AZ::Vector2 accumulatedPosition(0.0f,0.0f);
        for(const auto& pos: m_azCursorPositions) {
            accumulatedPosition += (pos.m_normalizedPositionDelta * AZ::Vector2(aznumeric_cast<float>(WidgetSize.width()), aznumeric_cast<float>(WidgetSize.height())));
        }

        // validate
        const QPoint endPosition = m_rootWidget->mapFromGlobal(QCursor::pos(screen));
        EXPECT_NEAR(endPosition.x(), mouseMoveParam.expectedPos.x(), 1.0f);
        EXPECT_NEAR(endPosition.y(), mouseMoveParam.expectedPos.y(), 1.0f);

        EXPECT_NEAR(accumulatedPosition.GetX(), mouseMoveParam.deltaPos.x(), 1.0f);
        EXPECT_NEAR(accumulatedPosition.GetY(), mouseMoveParam.deltaPos.y(), 1.0f);

        // cleanup
        m_rootWidget->move(0, 0);
        m_inputChannelMapper->SetCursorMode(AzToolsFramework::CursorInputMode::CursorModeNone);
        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    INSTANTIATE_TEST_CASE_P(All, MoveMoveWrapParamQtEventToAzInputMapperFixture,
        testing::Values(
            // verify CursorModeWrappedX wrapping
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedX, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() - 20, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                QPoint(40, 0),
                QPoint(20, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                "CursorModeWrappedX_Test_Right"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedX, 
                40,
                QPoint(20, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                QPoint(-40, 0),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() - 20, QtEventToAzInputMapperFixture::WidgetSize.height()/2),
                 "CursorModeWrappedX_Test_Left"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedX, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, 20),
                QPoint(0, -40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, -20),
                 "CursorModeWrappedX_Test_Top"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedX, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() - 20),
                QPoint(0, 40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() + 20),
                 "CursorModeWrappedX_Test_Bottom"
                },

            // verify CursorModeWrappedY wrapping
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedY, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() - 20, QtEventToAzInputMapperFixture::WidgetSize.height()/2),
                QPoint(40, 0),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() + 20, QtEventToAzInputMapperFixture::WidgetSize.height()/2),
                "CursorModeWrappedY_Test_Right"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedY, 
                40,
                QPoint(20, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                QPoint(-40, 0),
                QPoint(-20, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                 "CursorModeWrappedY_Test_Left"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedY, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, 20),
                QPoint(0, -40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() - 20),
                 "CursorModeWrappedY_Test_Top"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrappedY, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() - 20),
                QPoint(0, 40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, 20),
                 "CursorModeWrappedY_Test_Bottom"
                },

            // verify CursorModeWrapped wrapping
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrapped, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() - 20, QtEventToAzInputMapperFixture::WidgetSize.height()/2),
                QPoint(40, 0),
                QPoint(20, QtEventToAzInputMapperFixture::WidgetSize.height()/2),
                "CursorModeWrapped_Test_Right"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrapped, 
                40,
                QPoint(20, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                QPoint(-40, 0),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() - 20, QtEventToAzInputMapperFixture::WidgetSize.height()/2),
                 "CursorModeWrapped_Test_Left"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrapped, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, 20),
                QPoint(0, -40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() - 20),
                 "CursorModeWrapped_Test_Top"
                },
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeWrapped, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() - 20),
                QPoint(0, 40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, 20),
                 "CursorModeWrapped_Test_Bottom"
                },
            // verify CursorModeCaptured 
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeCaptured, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                QPoint(0, 40),
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                 "CursorModeCaptured"
                },
            // verify CursorModeNone 
            MouseMoveParam {AzToolsFramework::CursorInputMode::CursorModeNone, 
                40,
                QPoint(QtEventToAzInputMapperFixture::WidgetSize.width() / 2, QtEventToAzInputMapperFixture::WidgetSize.height() / 2),
                QPoint(40, 0),
                QPoint((QtEventToAzInputMapperFixture::WidgetSize.width() / 2) + 40, (QtEventToAzInputMapperFixture::WidgetSize.height() / 2)),
                 "CursorModeNone"
                }
        ),
        [](const ::testing::TestParamInfo<MouseMoveParam>& info)
        {
            return info.param.name;
        }
    );

} // namespace UnitTest
