/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Input/QtEventToAzInputMapper.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>


namespace UnitTest
{
    const QSize WidgetSize = QSize(1920, 1080);

    bool IsMouseButton(const AzFramework::InputChannelId& inputChannelId)
    {
        const auto& buttons = AzFramework::InputDeviceMouse::Button::All;
        const auto& it = AZStd::find(buttons.cbegin(), buttons.cend(), inputChannelId);
        return it != buttons.cend();
    }

    class QtEventToAzInputMapperFixture
        : public AllocatorsTestFixture
        , public AzFramework::InputChannelNotificationBus::Handler
    {
    public:
        static inline constexpr int TestDeviceIdSeed = 4321;

        void SetUp() override
        {
            AllocatorsTestFixture::SetUp();

            m_rootWidget = AZStd::make_unique<QWidget>();
            m_rootWidget->setFixedSize(WidgetSize);
            m_rootWidget->move(0, 0);

            m_inputChannelMapper = AZStd::make_unique<AzToolsFramework::QtEventToAzInputMapper>(m_rootWidget.get(), TestDeviceIdSeed);

            // listen for events signaled from QtEventToAzInputMapper and forward to the controller list
            QObject::connect(m_inputChannelMapper.get(), &AzToolsFramework::QtEventToAzInputMapper::InputChannelUpdated, m_rootWidget.get(),
                [this]([[maybe_unused]] const AzFramework::InputChannel* inputChannel, [[maybe_unused]] QEvent* event)
                {
                    const QEvent::Type eventType = event->type();

                    if (eventType == QEvent::Type::MouseButtonPress ||
                        eventType == QEvent::Type::MouseButtonRelease ||
                        eventType == QEvent::Type::MouseButtonDblClick)
                    {
                        m_signalEvents.push_back(QtEventInfo(static_cast<QMouseEvent*>(event)));
                        event->accept();
                    }
                });
        }

        void TearDown() override
        {
            m_inputChannelMapper.reset();

            m_rootWidget.reset();

            AllocatorsTestFixture::TearDown();
        }

        void OnInputChannelEvent(const AzFramework::InputChannel& inputChannel, bool& hasBeenConsumed) override
        {
            ASSERT_FALSE(hasBeenConsumed);

            const AzFramework::InputChannelId& inputChannelId = inputChannel.GetInputChannelId();
            const AzFramework::InputDeviceId& inputDeviceId = inputChannel.GetInputDevice().GetInputDeviceId();

            if (AzFramework::InputDeviceMouse::IsMouseDevice(inputDeviceId))
            {
                if (IsMouseButton(inputChannelId))
                {
                    m_azChannelEvents.push_back(AzEventInfo(inputChannel));
                    hasBeenConsumed = m_captureAzEvents;
                }
            }
        }

        struct QtEventInfo
        {
            explicit QtEventInfo(QMouseEvent* mouseEvent)
                : m_eventType(mouseEvent->type())
                , m_button(mouseEvent->button())
            {
            }

            explicit QtEventInfo(QKeyEvent* keyEvent)
                : m_eventType(keyEvent->type())
                , m_key(keyEvent->key())
            {
            }

            QEvent::Type m_eventType{ QEvent::None };
            Qt::MouseButton m_button{ Qt::NoButton };
            int m_key{ 0 };
        };

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

        bool m_captureAzEvents{ false };
    };

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

    TEST_P(MouseButtonParamQtEventToAzInputMapperFixture, MouseClick_NoAzHandlers_ReceivedTwoSignalAndZeroAzChannelEvents)
    {
        const MouseButtonIdsParam mouseButtonIds = GetParam();

        auto mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        QTest::mouseClick(m_rootWidget.get(), mouseButtonIds.m_qt, Qt::NoModifier, mouseEventPos);

        ASSERT_EQ(m_signalEvents.size(), 2);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::MouseButtonPress);
        EXPECT_EQ(m_signalEvents[0].m_button, mouseButtonIds.m_qt);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::MouseButtonRelease);
        EXPECT_EQ(m_signalEvents[1].m_button, mouseButtonIds.m_qt);

        EXPECT_EQ(m_azChannelEvents.size(), 0);
    }

    TEST_P(MouseButtonParamQtEventToAzInputMapperFixture, MouseClick_AzHandlerNotCaptured_ReceivedTwoSignalAndTwoAzChannelEvents)
    {
        const MouseButtonIdsParam mouseButtonIds = GetParam();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = false;

        auto mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        QTest::mouseClick(m_rootWidget.get(), mouseButtonIds.m_qt, Qt::NoModifier, mouseEventPos);

        ASSERT_EQ(m_signalEvents.size(), 2);

        EXPECT_EQ(m_signalEvents[0].m_eventType, QEvent::Type::MouseButtonPress);
        EXPECT_EQ(m_signalEvents[0].m_button, mouseButtonIds.m_qt);

        EXPECT_EQ(m_signalEvents[1].m_eventType, QEvent::Type::MouseButtonRelease);
        EXPECT_EQ(m_signalEvents[1].m_button, mouseButtonIds.m_qt);

        ASSERT_EQ(m_azChannelEvents.size(), 2);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_TRUE(m_azChannelEvents[0].m_isActive);

        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_FALSE(m_azChannelEvents[1].m_isActive);

        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    TEST_P(MouseButtonParamQtEventToAzInputMapperFixture, MouseClick_AzHandlerCaptured_ReceivedZeroSignalAndTwoAzChannelEvents)
    {
        const MouseButtonIdsParam mouseButtonIds = GetParam();

        AzFramework::InputChannelNotificationBus::Handler::BusConnect();
        m_captureAzEvents = true;

        auto mouseEventPos = QPoint(WidgetSize.width() / 2, WidgetSize.height() / 2);
        QTest::mouseClick(m_rootWidget.get(), mouseButtonIds.m_qt, Qt::NoModifier, mouseEventPos);

        EXPECT_EQ(m_signalEvents.size(), 0);

        ASSERT_EQ(m_azChannelEvents.size(), 2);

        EXPECT_STREQ(m_azChannelEvents[0].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_TRUE(m_azChannelEvents[0].m_isActive);

        EXPECT_STREQ(m_azChannelEvents[1].m_inputChannelId.GetName(), mouseButtonIds.m_az.GetName());
        EXPECT_FALSE(m_azChannelEvents[1].m_isActive);

        AzFramework::InputChannelNotificationBus::Handler::BusDisconnect();
    }

    INSTANTIATE_TEST_CASE_P(All, MouseButtonParamQtEventToAzInputMapperFixture,
        testing::Values(
            MouseButtonIdsParam{ Qt::MouseButton::LeftButton, AzFramework::InputDeviceMouse::Button::Left },
            MouseButtonIdsParam{ Qt::MouseButton::RightButton, AzFramework::InputDeviceMouse::Button::Right },
            MouseButtonIdsParam{ Qt::MouseButton::MiddleButton, AzFramework::InputDeviceMouse::Button::Middle }
    ));
} // namespace UnitTest
