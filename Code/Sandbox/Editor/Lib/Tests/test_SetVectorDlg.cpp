/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#include "EditorDefs.h"
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <SetVectorDlg.h>

using namespace AZ;
using namespace ::testing;

namespace UnitTest
{
    class TestSetVectorDlg
        : public ::testing::Test
    {
    public:
        
    };

    const float SetVectorDlgNearTolerance = 0.0001f;

    TEST_F(TestSetVectorDlg, GetVectorFromString_ThreeParams_Success)
    {
        QString testStr{ "1,2,3" };
        Vec3 result{ 0, 0, 0 };

        result = CSetVectorDlg::GetVectorFromString(testStr);

        EXPECT_NEAR(result[0], 1.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[1], 2.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[2], 3.0f, SetVectorDlgNearTolerance);
    }

    TEST_F(TestSetVectorDlg, GetVectorFromString_FourParams_ThreeParsed)
    {
        QString testStr{ "1,2,3,4" };
        Vec3 result{ 0, 0, 0 };

        result = CSetVectorDlg::GetVectorFromString(testStr);

        EXPECT_NEAR(result[0], 1.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[1], 2.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[2], 3.0f, SetVectorDlgNearTolerance);
    }

    TEST_F(TestSetVectorDlg, GetVectorFromString_TwoParams_ThirdZero)
    {
        QString testStr{ "1,2" };
        Vec3 result{ 0, 0, 0 };

        result = CSetVectorDlg::GetVectorFromString(testStr);

        EXPECT_NEAR(result[0], 1.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[1], 2.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[2], 0.0f, SetVectorDlgNearTolerance);
    }

    TEST_F(TestSetVectorDlg, GetVectorFromString_NoParams_AllZero)
    {
        QString testStr;
        Vec3 result{ 0, 0, 0 };

        result = CSetVectorDlg::GetVectorFromString(testStr);

        EXPECT_NEAR(result[0], 0.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[1], 0.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[2], 0.0f, SetVectorDlgNearTolerance);
    }

    TEST_F(TestSetVectorDlg, GetVectorFromString_BadStrings_AllZero)
    {
        QString testStr{ "some,illegal,strings" };
        Vec3 resultExpected{ 0, 1, 0 };

        auto result = CSetVectorDlg::GetVectorFromString(testStr);

        EXPECT_NEAR(result[0], 0.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[1], 0.0f, SetVectorDlgNearTolerance);
        EXPECT_NEAR(result[2], 0.0f, SetVectorDlgNearTolerance);
    }
} // namespace UnitTest
