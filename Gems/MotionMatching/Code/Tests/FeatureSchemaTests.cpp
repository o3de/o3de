/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Fixture.h>
#include <FeaturePosition.h>
#include <FeatureSchema.h>
#include <FeatureSchemaDefault.h>
#include <FeatureTrajectory.h>
#include <FeatureVelocity.h>

namespace EMotionFX::MotionMatching
{
    class FeatureSchemaFixture
        : public Fixture
    {
    public:
        void SetUp() override
        {
            Fixture::SetUp();
            m_featureSchema = AZStd::make_unique<FeatureSchema>();
            DefaultFeatureSchema(*m_featureSchema.get(), {});
        }

        void TearDown() override
        {
            Fixture::TearDown();
            m_featureSchema.reset();
        }

        AZStd::unique_ptr<FeatureSchema> m_featureSchema;
    };

    TEST_F(FeatureSchemaFixture, AddFeature)
    {
        m_featureSchema->AddFeature(aznew FeaturePosition());
        m_featureSchema->AddFeature(aznew FeatureVelocity());
        m_featureSchema->AddFeature(aznew FeatureTrajectory());
        EXPECT_EQ(m_featureSchema->GetNumFeatures(), 9);
    }

    TEST_F(FeatureSchemaFixture, Clear)
    {
        m_featureSchema->Clear();
        EXPECT_EQ(m_featureSchema->GetNumFeatures(), 0);
    }

    TEST_F(FeatureSchemaFixture, GetNumFeatures)
    {
        EXPECT_EQ(m_featureSchema->GetNumFeatures(), 6);
    }

    TEST_F(FeatureSchemaFixture, GetFeature)
    {
        EXPECT_EQ(m_featureSchema->GetFeature(1)->RTTI_GetType(), azrtti_typeid<FeaturePosition>());
        EXPECT_STREQ(m_featureSchema->GetFeature(3)->GetName().c_str(), "Left Foot Velocity");
    }

    TEST_F(FeatureSchemaFixture, GetFeatures)
    {
        int counter = 0;
        for (const Feature* feature : m_featureSchema->GetFeatures())
        {
            AZ_UNUSED(feature);
            counter++;
        }
        EXPECT_EQ(counter, 6);
    }

    TEST_F(FeatureSchemaFixture, FindFeatureById)
    {
        const Feature* feature = m_featureSchema->GetFeature(1);
        const AZ::TypeId id = feature->GetId();
        const Feature* result = m_featureSchema->FindFeatureById(id);
        EXPECT_EQ(result, feature);
    }
} // EMotionFX::MotionMatching
