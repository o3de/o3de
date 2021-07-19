/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/State/HSM.h>
#include <AzCore/std/string/string.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <AzCore/std/allocator_static.h>
#include <AzCore/std/allocator_ref.h>

using namespace AZ;

namespace
{
    class HSMTest
        : public ::testing::Test
    {
        int myFoo;
        HSM m_hsm;
        // we are not allowed to use dynamic memory in this lib (although we can for test, we use it to check if the actual lib leaks)
        AZStd::basic_string<char, AZStd::char_traits<char>, AZStd::static_buffer_allocator<256, 4> > result;

        enum HsmTestEvents
        {
            A_SIG, B_SIG, C_SIG, D_SIG, E_SIG, F_SIG, G_SIG, H_SIG
        };
        enum HsmTestStates
        {
            TOP, S1, S11, S2, S21, S211
        };

        bool topHndlr(HSM& sm, const HSM::Event& e)
        {
            switch (e.id)
            {
            case HSM::EnterEventId:
                result += "top-ENTRY;";
                return true;
            case HSM::ExitEventId:
                result += "top-EXIT;";
                return true;
            case E_SIG:
                result += "top-E;";
                sm.Transition(S211);
                return true;
            }
            return false;
        }

        bool s1Hndlr(HSM& sm, const HSM::Event& e)
        {
            switch (e.id)
            {
            case HSM::EnterEventId:
                result += "s1-ENTRY;";
                return true;
            case HSM::ExitEventId:
                result += "s1-EXIT;";
                return true;
            case A_SIG:
                result += "s1-A;";
                sm.Transition(S1);
                return true;
            case B_SIG:
                result += "s1-B;";
                sm.Transition(S11);
                return true;
            case C_SIG:
                result += "s1-C;";
                sm.Transition(S2);
                return true;
            case D_SIG:
                result += "s1-D;";
                sm.Transition(TOP);
                return true;
            case F_SIG:
                result += "s1-F;";
                sm.Transition(S211);
                return true;
            }
            return false;
        }

        bool s11Hndlr(HSM& sm, const HSM::Event& e)
        {
            switch (e.id)
            {
            case HSM::EnterEventId:
                result += "s11-ENTRY;";
                return true;
            case HSM::ExitEventId:
                result += "s11-EXIT;";
                return true;
            case G_SIG:
                result += "s11-G;";
                sm.Transition(S211);
                return true;
            case H_SIG:
                if (myFoo)
                {
                    result += "s11-H;";
                    myFoo = 0;
                    return true;
                }
                break;
            }
            return false;
        }

        bool s2Hndlr(HSM& sm, const HSM::Event& e)
        {
            switch (e.id)
            {
            case HSM::EnterEventId:
                result += "s2-ENTRY;";
                return true;
            case HSM::ExitEventId:
                result += "s2-EXIT;";
                return true;
            case C_SIG:
                result += "s2-C;";
                sm.Transition(S1);
                return true;
            case F_SIG:
                result += "s2-F;";
                sm.Transition(S11);
                return true;
            }
            return false;
        }

        bool s21Hndlr(HSM& sm, const HSM::Event& e)
        {
            switch (e.id)
            {
            case HSM::EnterEventId:
                result += "s21-ENTRY;";
                return true;
            case HSM::ExitEventId:
                result += "s21-EXIT;";
                return true;
            case B_SIG:
                result += "s21-B;";
                sm.Transition(S211);
                return true;
            case H_SIG:
                if (!myFoo)
                {
                    result += "s21-H;";
                    myFoo = 1;
                    sm.Transition(S21);
                    return true;
                }
                break;
            }
            return false;
        }

        bool s211Hndlr(HSM& sm, const HSM::Event& e)
        {
            switch (e.id)
            {
            case HSM::EnterEventId:
                result += "s211-ENTRY;";
                return true;
            case HSM::ExitEventId:
                result += "s211-EXIT;";
                return true;
            case D_SIG:
                result += "s211-D;";
                sm.Transition(S21);
                return true;
            case G_SIG:
                result += "s211-G;";
                sm.Transition(TOP);
                return true;
            }
            return false;
        }

    public:
        void run()
        {
            myFoo = 0;
            m_hsm.SetStateHandler(TOP, "TOP", HSM::StateHandler(this, &HSMTest::topHndlr), HSM::InvalidStateId, S1);
            m_hsm.SetStateHandler(S1, "S1", HSM::StateHandler(this, &HSMTest::s1Hndlr), TOP, S11);
            m_hsm.SetStateHandler(AZ_HSM_STATE_NAME(S11), HSM::StateHandler(this, &HSMTest::s11Hndlr), S1);
            m_hsm.SetStateHandler(AZ_HSM_STATE_NAME(S2), HSM::StateHandler(this, &HSMTest::s2Hndlr), TOP, S21);
            m_hsm.SetStateHandler(AZ_HSM_STATE_NAME(S21), HSM::StateHandler(this, &HSMTest::s21Hndlr), S2, S211);
            m_hsm.SetStateHandler(AZ_HSM_STATE_NAME(S211), HSM::StateHandler(this, &HSMTest::s211Hndlr), S21);

            result.clear();
            bool processed;
            m_hsm.Start();
            AZ_TEST_ASSERT(result == "top-ENTRY;s1-ENTRY;s11-ENTRY;");

            result.clear();
            processed = m_hsm.Dispatch(A_SIG);
            AZ_TEST_ASSERT(processed == true);
            AZ_TEST_ASSERT(result == "s1-A;s11-EXIT;s1-EXIT;s1-ENTRY;s11-ENTRY;");

            result.clear();
            processed = m_hsm.Dispatch(E_SIG);
            AZ_TEST_ASSERT(processed == true);
            AZ_TEST_ASSERT(result == "top-E;s11-EXIT;s1-EXIT;s2-ENTRY;s21-ENTRY;s211-ENTRY;");

            result.clear();
            processed = m_hsm.Dispatch(E_SIG);
            AZ_TEST_ASSERT(processed == true);
            AZ_TEST_ASSERT(result == "top-E;s211-EXIT;s21-EXIT;s2-EXIT;s2-ENTRY;s21-ENTRY;s211-ENTRY;");

            result.clear();
            processed = m_hsm.Dispatch(A_SIG);
            AZ_TEST_ASSERT(processed == false);
            AZ_TEST_ASSERT(result == "");

            result.clear();
            processed = m_hsm.Dispatch(H_SIG);
            AZ_TEST_ASSERT(processed == true);
            AZ_TEST_ASSERT(result == "s21-H;s211-EXIT;s21-EXIT;s21-ENTRY;s211-ENTRY;");

            result.clear();
            processed = m_hsm.Dispatch(H_SIG);
            AZ_TEST_ASSERT(processed == false);
            AZ_TEST_ASSERT(result == "");
        }
    };

    TEST_F(HSMTest, Test)
    {
        run();
    }
}
