/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/Math/MathUtils.h>
#include "Tests/GradientSignalTestMocks.h"

#include <Source/Components/ConstantGradientComponent.h>
#include <Source/Components/GradientSurfaceDataComponent.h>

namespace UnitTest
{
    struct GradientSignalSurfaceTestsFixture
        : public GradientSignalTest
    {
        void SetSurfacePoint(SurfaceData::SurfacePoint& point, AZ::EntityId id, AZ::Vector3 position, AZ::Vector3 normal, AZStd::vector<AZStd::pair<AZStd::string, float>> tags)
        {
            point.m_entityId = id;
            point.m_position = position;
            point.m_normal = normal;
            for (auto& tag : tags)
            {
                point.m_masks[SurfaceData::SurfaceTag(tag.first)] = tag.second;
            }
        }

        bool SurfacePointsAreEqual(const SurfaceData::SurfacePoint& lhs, const SurfaceData::SurfacePoint& rhs)
        {
            return (lhs.m_entityId == rhs.m_entityId)
            && (lhs.m_position == rhs.m_position)
            && (lhs.m_normal == rhs.m_normal)
            && (lhs.m_masks == rhs.m_masks);
        }

        void TestGradientSurfaceDataComponent(float gradientValue, float thresholdMin, float thresholdMax, AZStd::vector<AZStd::string> tags, bool usesShape,
                                              const SurfaceData::SurfacePoint& input, const SurfaceData::SurfacePoint& expectedOutput)
        {
            // This lets our component register with surfaceData successfully.
            MockSurfaceDataSystem mockSurfaceDataSystem;

            // Create a mock shape entity in case we want to use it.
            // The mock shape is a cube that goes from -0.5 to 0.5 in space.
            auto mockShapeEntity = CreateEntity();
            CreateComponent<MockShapeComponent>(mockShapeEntity.get());
            MockShapeComponentHandler mockShapeHandler(mockShapeEntity->GetId());
            ActivateEntity(mockShapeEntity.get());

            // For ease of testing, use a constant gradient as our input gradient.
            GradientSignal::ConstantGradientConfig constantGradientConfig;
            constantGradientConfig.m_value = gradientValue;

            // Create the test configuration for the GradientSignalSurfaceData component
            GradientSignal::GradientSurfaceDataConfig config;
            config.m_thresholdMin = thresholdMin;
            config.m_thresholdMax = thresholdMax;
            for (auto& tag : tags)
            {
                config.AddTag(tag);
            }

            // Either point to our shape entity or set it to an invalid ID if we don't want to use a shape constraint for this test.
            if (usesShape)
            {
                config.m_shapeConstraintEntityId = mockShapeEntity->GetId();
            }
            else
            {
                config.m_shapeConstraintEntityId = AZ::EntityId();
            }

            // Create the test entity with the GradientSurfaceData component and the required gradient dependency
            auto entity = CreateEntity();
            CreateComponent<GradientSignal::ConstantGradientComponent>(entity.get(), constantGradientConfig);
            CreateComponent<GradientSignal::GradientSurfaceDataComponent>(entity.get(), config);
            ActivateEntity(entity.get());

            // Get our registered modifier handle (and verify that it's valid)
            auto modifierHandle = mockSurfaceDataSystem.GetSurfaceModifierHandle(entity->GetId());
            EXPECT_TRUE(modifierHandle != SurfaceData::InvalidSurfaceDataRegistryHandle);

            // Call ModifySurfacePoints and verify the results
            SurfaceData::SurfacePointList pointList;
            pointList.emplace_back(input);
            SurfaceData::SurfaceDataModifierRequestBus::Event(modifierHandle, &SurfaceData::SurfaceDataModifierRequestBus::Events::ModifySurfacePoints, pointList);
            EXPECT_TRUE(SurfacePointsAreEqual(pointList[0],expectedOutput));
        }


    };

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_PointInThreshold)
    {
        // Verify that for a gradient value within the threshold, the output point contains the
        // correct tag and gradient value.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;

        // Set arbitrary input data
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input, but with an added tag / value
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal, { AZStd::make_pair<AZStd::string, float>(tag, gradientValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_PointOutsideThreshold)
    {
        // Verify that for a gradient value outside the threshold, the output point contains no tags / values.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Choose a value outside the threshold range
        float gradientValue = 0.05f;

        // Set arbitrary input data
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input - no extra tags / values should be added.
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal, {});

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_PointInThresholdMultipleTags)
    {
        // Verify that if the component has multiple tags, all of them get put on the output with the same gradient value.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag1 = "test_mask1";
        const char* tag2 = "test_mask2";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;

        // Set arbitrary input data
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), {});
        // Output should match the input, but with two added tags
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal,
                        { AZStd::make_pair<AZStd::string, float>(tag1, gradientValue), AZStd::make_pair<AZStd::string, float>(tag2, gradientValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag1, tag2 },     // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_PreservesInputTags)
    {
        // Verify that the output contains input tags that are NOT on the modification list and adds any
        // new tags that weren't in the input

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* preservedTag = "preserved_tag";
        const char* modifierTag = "modifier_tag";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;

        // Set arbitrary input data
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), { AZStd::make_pair<AZStd::string, float>(preservedTag, 1.0f) });
        // Output should match the input, but with two added tags
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(preservedTag, 1.0f), AZStd::make_pair<AZStd::string, float>(modifierTag, gradientValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { modifierTag },    // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_KeepsHigherValueFromInput)
    {
        // Verify that if the input has a higher value on the tag than the modifier, it keeps the higher value.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;
        // Select an input value that's higher than the gradient value
        float inputValue = 0.75f;

        // Set arbitrary input data
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), { AZStd::make_pair<AZStd::string, float>(tag, inputValue) });
        // Output should match the input - the higher input value on the tag is preserved
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(tag, inputValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_KeepsHigherValueFromModifier)
    {
        // Verify that if the input has a lower value on the tag than the modifier, it keeps the higher value.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;
        // Select an input value that's lower than the gradient value
        float inputValue = 0.25f;

        // Set arbitrary input data
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(1.0f), AZ::Vector3(0.0f), { AZStd::make_pair<AZStd::string, float>(tag, inputValue) });
        // Output should match the input, except that the value on the tag gets the higher modifier value
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(tag, gradientValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_UnboundedRangeWithoutShape)
    {
        // Verify that if no shape has been added, the component modifies points in unbounded space

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;

        // Set arbitrary input data, but with a point that's extremely far away in space
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(-100000000.0f), AZ::Vector3(0.0f), {});
        // Output should match the input but with the tag added, even though the point was far away.
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(tag, gradientValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            false,              // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_ModifyPointInShapeConstraint)
    {
        // Verify that if a shape constraint is added, points within the shape are still modified.
        // Our default mock shape is a cube that exists from -0.5 to 0.5 in space.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;

        // Set arbitrary input data, but with a point that's within the mock shape cube (0.25 vs -0.5 to 0.5)
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(0.25f), AZ::Vector3(0.0f), {});
        // Output should match the input but with the tag added, since the point is within the shape constraint.
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal,
            { AZStd::make_pair<AZStd::string, float>(tag, gradientValue) });

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            true,               // uses surface bounds?
            input,
            expectedOutput);
    }

    TEST_F(GradientSignalSurfaceTestsFixture, GradientSignalSurfaceComponent_DoNotModifyPointOutsideShapeConstraint)
    {
        // Verify that if a shape constraint is added, points outside the shape are not modified.
        // Our default mock shape is a cube that exists from -0.5 to 0.5 in space.

        SurfaceData::SurfacePoint input;
        SurfaceData::SurfacePoint expectedOutput;
        const char* tag = "test_mask";

        // Select a gradient value within the threshold range below
        float gradientValue = 0.5f;

        // Set arbitrary input data, but with a point that's outside the mock shape cube (10.0 vs -0.5 to 0.5)
        SetSurfacePoint(input, AZ::EntityId(0x12345678), AZ::Vector3(10.0f), AZ::Vector3(0.0f), {});
        // Output should match the input with no tag added, since the point is outside the shape constraint
        SetSurfacePoint(expectedOutput, input.m_entityId, input.m_position, input.m_normal, {});

        TestGradientSurfaceDataComponent(
            gradientValue,      // constant gradient value
            0.1f,               // min threshold
            1.0f,               // max threshold
            { tag },            // supported tags
            true,               // uses surface bounds?
            input,
            expectedOutput);
    }
}
