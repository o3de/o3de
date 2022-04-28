/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>

#include <Material/PhysXMaterial.h>

namespace UnitTest
{
    static constexpr float Tolerance = 1e-4f;

    TEST(PhysXMaterial, Material_GetSet_DynamicFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_dynamicFriction = 68.6f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDynamicFriction(), 68.6f, Tolerance);

        material.SetDynamicFriction(31.2f);
        EXPECT_NEAR(material.GetDynamicFriction(), 31.2f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_DynamicFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_dynamicFriction = -7.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDynamicFriction(), 0.0f, Tolerance);

        material.SetDynamicFriction(-61.0f);
        EXPECT_NEAR(material.GetDynamicFriction(), 0.0f, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_StaticFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_staticFriction = 68.6f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetStaticFriction(), 68.6f, Tolerance);

        material.SetStaticFriction(31.2f);
        EXPECT_NEAR(material.GetStaticFriction(), 31.2f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_StaticFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_staticFriction = -7.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetStaticFriction(), 0.0f, Tolerance);

        material.SetStaticFriction(-61.0f);
        EXPECT_NEAR(material.GetStaticFriction(), 0.0f, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_Restitution)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitution = 0.43f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetRestitution(), 0.43f, Tolerance);

        material.SetRestitution(0.78f);
        EXPECT_NEAR(material.GetRestitution(), 0.78f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_Restitution)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitution = -13.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetRestitution(), 0.0f, Tolerance);

        material.SetRestitution(0.0f);
        EXPECT_NEAR(material.GetRestitution(), 0.0f, Tolerance);

        material.SetRestitution(1.0f);
        EXPECT_NEAR(material.GetRestitution(), 1.0f, Tolerance);

        material.SetRestitution(61.0f);
        EXPECT_NEAR(material.GetRestitution(), 1.0f, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_Density)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_density = 245.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDensity(), 245.0f, Tolerance);

        material.SetDensity(43.1f);
        EXPECT_NEAR(material.GetDensity(), 43.1f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_Density)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_density = -13.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDensity(), PhysX::MaterialConfiguration::MinDensityLimit, Tolerance);

        material.SetDensity(0.0f);
        EXPECT_NEAR(material.GetDensity(), PhysX::MaterialConfiguration::MinDensityLimit, Tolerance);

        material.SetDensity(PhysX::MaterialConfiguration::MinDensityLimit);
        EXPECT_NEAR(material.GetDensity(), PhysX::MaterialConfiguration::MinDensityLimit, Tolerance);

        material.SetDensity(PhysX::MaterialConfiguration::MaxDensityLimit);
        EXPECT_NEAR(material.GetDensity(), PhysX::MaterialConfiguration::MaxDensityLimit, Tolerance);

        material.SetDensity(200000.0f);
        EXPECT_NEAR(material.GetDensity(), PhysX::MaterialConfiguration::MaxDensityLimit, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_FrictionCombineMode)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_frictionCombine = PhysX::CombineMode::Maximum;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_EQ(material.GetFrictionCombineMode(), PhysX::CombineMode::Maximum);

        material.SetFrictionCombineMode(PhysX::CombineMode::Minimum);
        EXPECT_EQ(material.GetFrictionCombineMode(), PhysX::CombineMode::Minimum);
    }

    TEST(PhysXMaterial, Material_GetSet_RestitutionCombineMode)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitutionCombine = PhysX::CombineMode::Maximum;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_EQ(material.GetRestitutionCombineMode(), PhysX::CombineMode::Maximum);

        material.SetRestitutionCombineMode(PhysX::CombineMode::Minimum);
        EXPECT_EQ(material.GetRestitutionCombineMode(), PhysX::CombineMode::Minimum);
    }

    TEST(PhysXMaterial, Material_GetSet_DebugColor)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_debugColor = AZ::Colors::Lavender;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_THAT(material.GetDebugColor(), IsClose(AZ::Colors::Lavender));

        material.SetDebugColor(AZ::Colors::Aquamarine);
        EXPECT_THAT(material.GetDebugColor(), IsClose(AZ::Colors::Aquamarine));
    }

    TEST(PhysXMaterial, Material_ReturnsValid_NativePointer)
    {
        PhysX::Material2 material(PhysX::MaterialConfiguration{});

        EXPECT_TRUE(material.GetNativePointer() != nullptr);
    }
} // namespace UnitTest
