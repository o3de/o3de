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
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_dynamicFriction = 68.6f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDynamicFriction(), 68.6f, Tolerance);

        material.SetDynamicFriction(31.2f);
        EXPECT_NEAR(material.GetDynamicFriction(), 31.2f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_DynamicFriction)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_dynamicFriction = -7.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDynamicFriction(), 0.0f, Tolerance);

        material.SetDynamicFriction(-61.0f);
        EXPECT_NEAR(material.GetDynamicFriction(), 0.0f, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_StaticFriction)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_staticFriction = 68.6f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetStaticFriction(), 68.6f, Tolerance);

        material.SetStaticFriction(31.2f);
        EXPECT_NEAR(material.GetStaticFriction(), 31.2f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_StaticFriction)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_staticFriction = -7.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetStaticFriction(), 0.0f, Tolerance);

        material.SetStaticFriction(-61.0f);
        EXPECT_NEAR(material.GetStaticFriction(), 0.0f, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_Restitution)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_restitution = 0.43f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetRestitution(), 0.43f, Tolerance);

        material.SetRestitution(0.78f);
        EXPECT_NEAR(material.GetRestitution(), 0.78f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_Restitution)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
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
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_density = 245.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDensity(), 245.0f, Tolerance);

        material.SetDensity(43.1f);
        EXPECT_NEAR(material.GetDensity(), 43.1f, Tolerance);
    }

    TEST(PhysXMaterial, Material_Clamps_Density)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_density = -13.0f;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_NEAR(material.GetDensity(), Physics::MaterialConfiguration2::MinDensityLimit, Tolerance);

        material.SetDensity(0.0f);
        EXPECT_NEAR(material.GetDensity(), Physics::MaterialConfiguration2::MinDensityLimit, Tolerance);

        material.SetDensity(Physics::MaterialConfiguration2::MinDensityLimit);
        EXPECT_NEAR(material.GetDensity(), Physics::MaterialConfiguration2::MinDensityLimit, Tolerance);

        material.SetDensity(Physics::MaterialConfiguration2::MaxDensityLimit);
        EXPECT_NEAR(material.GetDensity(), Physics::MaterialConfiguration2::MaxDensityLimit, Tolerance);

        material.SetDensity(200000.0f);
        EXPECT_NEAR(material.GetDensity(), Physics::MaterialConfiguration2::MaxDensityLimit, Tolerance);
    }

    TEST(PhysXMaterial, Material_GetSet_FrictionCombineMode)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_frictionCombine = Physics::CombineMode::Maximum;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_EQ(material.GetFrictionCombineMode(), Physics::CombineMode::Maximum);

        material.SetFrictionCombineMode(Physics::CombineMode::Minimum);
        EXPECT_EQ(material.GetFrictionCombineMode(), Physics::CombineMode::Minimum);
    }

    TEST(PhysXMaterial, Material_GetSet_RestitutionCombineMode)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_restitutionCombine = Physics::CombineMode::Maximum;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_EQ(material.GetRestitutionCombineMode(), Physics::CombineMode::Maximum);

        material.SetRestitutionCombineMode(Physics::CombineMode::Minimum);
        EXPECT_EQ(material.GetRestitutionCombineMode(), Physics::CombineMode::Minimum);
    }

    TEST(PhysXMaterial, Material_GetSet_DebugColor)
    {
        Physics::MaterialConfiguration2 materialConfiguration;
        materialConfiguration.m_debugColor = AZ::Colors::Lavender;

        PhysX::Material2 material(materialConfiguration);

        EXPECT_THAT(material.GetDebugColor(), IsClose(AZ::Colors::Lavender));

        material.SetDebugColor(AZ::Colors::Aquamarine);
        EXPECT_THAT(material.GetDebugColor(), IsClose(AZ::Colors::Aquamarine));
    }

    TEST(PhysXMaterial, Material_ReturnsValid_NativePointer)
    {
        PhysX::Material2 material(Physics::MaterialConfiguration2{});

        EXPECT_TRUE(material.GetNativePointer() != nullptr);
    }
} // namespace UnitTest
