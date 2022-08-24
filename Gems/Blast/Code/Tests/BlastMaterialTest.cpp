/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <Material/BlastMaterial.h>

namespace UnitTest
{
    static constexpr float Tolerance = 1e-4f;

    TEST(BlastMaterial, Material_ReturnsCorrect_Health)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_health = 68.6f;

        Blast::Material material(materialConfiguration);

        EXPECT_NEAR(material.GetHealth(), 68.6f, Tolerance);
    }

    TEST(BlastMaterial, Material_ReturnsCorrect_ForceDivider)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_forceDivider = 0.6f;

        Blast::Material material(materialConfiguration);

        EXPECT_NEAR(material.GetForceDivider(), 0.6f, Tolerance);
    }

    TEST(BlastMaterial, Material_ReturnsCorrect_DamageThresholds)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_minDamageThreshold = 0.2f;
        materialConfiguration.m_maxDamageThreshold = 0.8f;

        Blast::Material material(materialConfiguration);

        EXPECT_NEAR(material.GetMinDamageThreshold(), 0.2f, Tolerance);
        EXPECT_NEAR(material.GetMaxDamageThreshold(), 0.8f, Tolerance);
    }

    TEST(BlastMaterial, Material_ReturnsCorrect_StressFactors)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_stressLinearFactor = 0.6f;
        materialConfiguration.m_stressAngularFactor = 0.7f;

        Blast::Material material(materialConfiguration);

        EXPECT_NEAR(material.GetStressLinearFactor(), 0.6f, Tolerance);
        EXPECT_NEAR(material.GetStressAngularFactor(), 0.7f, Tolerance);
    }

    TEST(BlastMaterial, Material_ReturnsCorrect_NormalizedDamage_WithForceDividerZero)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_forceDivider = 0.0f;
        materialConfiguration.m_minDamageThreshold = 0.2f;
        materialConfiguration.m_maxDamageThreshold = 0.8f;

        Blast::Material material(materialConfiguration);

        // When forceDivider is 0 the damage value that will be normalized is always
        // 1.0f (ignoring input parameter), which will be clamped by min/max thresholds.
        EXPECT_NEAR(material.GetNormalizedDamage(0.0f), 0.8f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.2f), 0.8f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.5f), 0.8f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.8f), 0.8f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(-0.3f), 0.8f, Tolerance);
    }

    using BlastMaterialForceDividerFixture = ::testing::TestWithParam<float>;

    TEST_P(BlastMaterialForceDividerFixture, Material_ReturnsCorrect_NormalizedDamage)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_forceDivider = GetParam(); // Force divider shouldn't affect the result of damage normalization (except when it's 0)
        materialConfiguration.m_minDamageThreshold = 0.2f;
        materialConfiguration.m_maxDamageThreshold = 0.8f;

        Blast::Material material(materialConfiguration);

        EXPECT_NEAR(material.GetNormalizedDamage(0.0f), 0.0f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.2f), 0.0f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.21f), 0.21f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.3f), 0.3f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.5f), 0.5f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.79f), 0.79f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.8f), 0.8f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(0.9f), 0.8f, Tolerance);
        EXPECT_NEAR(material.GetNormalizedDamage(-0.3f), 0.0f, Tolerance);
    }

    INSTANTIATE_TEST_CASE_P(
        All,
        BlastMaterialForceDividerFixture,
        ::testing::Values(
            1.0f, 0.2f, 0.6f, 0.8f, 1.3f
        ));

    TEST(BlastMaterial, Material_ReturnsCorrect_StressSolverSettings)
    {
        Blast::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_health = 68.6f;
        materialConfiguration.m_forceDivider = 0.6f;
        materialConfiguration.m_minDamageThreshold = 0.2f;
        materialConfiguration.m_maxDamageThreshold = 0.8f;
        materialConfiguration.m_stressLinearFactor = 0.65f;
        materialConfiguration.m_stressAngularFactor = 0.7f;

        Blast::Material material(materialConfiguration);

        const uint32_t iterationCount = 2;
        const Nv::Blast::ExtStressSolverSettings stressSolverSettings = material.GetStressSolverSettings(iterationCount);

        EXPECT_NEAR(stressSolverSettings.hardness, 0.6f, Tolerance);
        EXPECT_NEAR(stressSolverSettings.stressLinearFactor, 0.65f, Tolerance);
        EXPECT_NEAR(stressSolverSettings.stressAngularFactor, 0.7f, Tolerance);
        EXPECT_EQ(stressSolverSettings.graphReductionLevel, 0);
        EXPECT_EQ(stressSolverSettings.bondIterationsPerFrame, 2);
    }

    TEST(BlastMaterial, Material_ReturnsValid_NativePointer)
    {
        Blast::Material material(Blast::MaterialConfiguration{});

        EXPECT_TRUE(material.GetNativePointer() != nullptr);
    }
} // namespace UnitTest
