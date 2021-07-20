/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/Json/JsonSerializationResult.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <Tests/Serialization/Json/BaseJsonSerializerFixture.h>

namespace JsonSerializationTests
{
    class JsonSerializationResultTest
        : public BaseJsonSerializerFixture
    {
    public:
        ~JsonSerializationResultTest() override = default;
    };

    TEST_F(JsonSerializationResultTest, ResultCode_SizeOf_32bits)
    {
        using namespace AZ::JsonSerializationResult;
        static_assert(sizeof(ResultCode) == 4, "ResultCode is expected to be the size of a unsigned 32-bit integer.");
    }

    TEST_F(JsonSerializationResultTest, Combine_CombineFullyDifferentResult_HigherReplacesLower)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::Convert, Outcomes::Success);
        ResultCode second(Tasks::WriteValue, Outcomes::Catastrophic);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetTask(), second.GetTask());
        EXPECT_EQ(combined.GetProcessing(), second.GetProcessing());
        EXPECT_EQ(combined.GetOutcome(), second.GetOutcome());
    }

    TEST_F(JsonSerializationResultTest, Combine_OnlyTaskDiffers_HighestTask)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::Convert, Outcomes::Skipped);
        ResultCode second(Tasks::ReadField, Outcomes::Skipped);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetTask(), second.GetTask());
    }

    TEST_F(JsonSerializationResultTest, Combine_SuccessAndSkipped_PartialSkip)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::Success);
        ResultCode second(Tasks::ReadField, Outcomes::Skipped);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetOutcome(), Outcomes::PartialSkip);
    }

    TEST_F(JsonSerializationResultTest, Combine_SkippedAndSuccess_PartialSkip)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::Skipped);
        ResultCode second(Tasks::ReadField, Outcomes::Success);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetOutcome(), Outcomes::PartialSkip);
    }

    TEST_F(JsonSerializationResultTest, Combine_OnlyOutComeDiffers_HighestOutcome)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::Skipped);
        ResultCode second(Tasks::ReadField, Outcomes::Catastrophic);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetOutcome(), second.GetOutcome());
    }

    TEST_F(JsonSerializationResultTest, Combine_CompletedAndAltered_PartialAltered)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::Success);
        ResultCode second(Tasks::ReadField, Outcomes::Unsupported);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetProcessing(), Processing::PartialAlter);
    }

    TEST_F(JsonSerializationResultTest, Combine_AlteredAndCompleted_PartialAltered)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::Unsupported);
        ResultCode second(Tasks::ReadField, Outcomes::Success);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetProcessing(), Processing::PartialAlter);
    }

    TEST_F(JsonSerializationResultTest, Combine_CompletedAndDefault_PartialDefaults)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::Success);
        ResultCode second(Tasks::ReadField, Outcomes::DefaultsUsed);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetOutcome(), Outcomes::PartialDefaults);
    }

    TEST_F(JsonSerializationResultTest, Combine_DefaultAndCompleted_PartialDefaults)
    {
        using namespace AZ::JsonSerializationResult;
        ResultCode first(Tasks::ReadField, Outcomes::DefaultsUsed);
        ResultCode second(Tasks::ReadField, Outcomes::Success);
        ResultCode combined = ResultCode::Combine(first, second);

        EXPECT_EQ(combined.GetOutcome(), Outcomes::PartialDefaults);
    }
} // namespace JsonSerializationTests
