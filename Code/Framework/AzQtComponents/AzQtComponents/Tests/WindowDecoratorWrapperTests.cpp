/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzQtComponents/Components/WindowDecorationWrapper.h>

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/make_shared.h>

#include <QApplication>

namespace AzQtComponents
{
    class WindowDecorationWrapperTests
        : public ::UnitTest::LeakDetectionFixture
    {
    public:
        WindowDecorationWrapperTests()
        {
            // Used to initiaize QApplication
            char programName[] = "test";
            char* argv[] = { &programName[0], nullptr };
            int argc = 1;

            // Must be created for widgets to work
            m_application = AZStd::make_unique<QApplication>(argc, argv);
            m_wrapper = AZStd::make_unique<WindowDecorationWrapper>(WindowDecorationWrapper::Option::OptionDisabled);
            m_guest = AZStd::make_unique<QWidget>();
        }

        ~WindowDecorationWrapperTests()
        {
            m_guest.reset();
            m_wrapper.reset();
            m_application.reset();
        }

        AZStd::unique_ptr<QApplication> m_application;
        AZStd::unique_ptr<WindowDecorationWrapper> m_wrapper;
        AZStd::unique_ptr<QWidget> m_guest;
    };

    TEST_F(WindowDecorationWrapperTests, WindowDecorationWrapper_ShrinkWindow_KeepsGuestMinimumSize)
    {
        QSize minimumSize(100, 100);
        m_guest->setMinimumSize(minimumSize);
        m_wrapper->setGuest(m_guest.get());

        m_wrapper->resize(50, 50);

        QSize guestSize = m_guest->size();
        EXPECT_TRUE(guestSize.width() >= minimumSize.width());
        EXPECT_TRUE(guestSize.height() >= minimumSize.height());
    }

} // namespace AzQtComponents
