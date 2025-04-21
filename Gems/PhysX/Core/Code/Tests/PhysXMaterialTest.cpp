/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AZTestShared/Math/MathTestHelpers.h>
#include <AzCore/Asset/AssetManager.h>

#include <AzFramework/Physics/Material/PhysicsMaterialSlots.h>
#include <AzFramework/Physics/Material/PhysicsMaterialManager.h>

#include <PhysX/Material/PhysXMaterial.h>
#include <PhysX/Material/PhysXMaterialConfiguration.h>

#include <Tests/PhysXGenericTestFixture.h>

namespace UnitTest
{
    static constexpr float Tolerance = 1e-4f;

    using PhysXMaterialFixture = PhysX::GenericPhysicsInterfaceTest;

    TEST_F(PhysXMaterialFixture, Material_FindOrCreateMaterial)
    {
        AZStd::shared_ptr<PhysX::Material> materialNull = PhysX::Material::FindOrCreateMaterial(AZ::Data::Asset<Physics::MaterialAsset>());

        EXPECT_TRUE(materialNull.get() == nullptr);

        PhysX::MaterialConfiguration materialConfiguration;
        const auto materialAsset = materialConfiguration.CreateMaterialAsset();

        AZStd::shared_ptr<PhysX::Material> material1 = PhysX::Material::FindOrCreateMaterial(materialAsset);

        EXPECT_TRUE(material1.get() != nullptr);

        AZStd::shared_ptr<PhysX::Material> material2 = PhysX::Material::FindOrCreateMaterial(materialAsset);

        EXPECT_TRUE(material2.get() != nullptr);
        EXPECT_EQ(material1.get(), material2.get());
        EXPECT_EQ(material1->GetId(), material2->GetId());
    }

    TEST_F(PhysXMaterialFixture, Material_FindOrCreateMaterials)
    {
        const auto defaultMaterial = AZ::Interface<Physics::MaterialManager>::Get()->GetDefaultMaterial();

        Physics::MaterialSlots defaultMaterialSlots;

        AZStd::vector<AZStd::shared_ptr<PhysX::Material>> materials = PhysX::Material::FindOrCreateMaterials(defaultMaterialSlots);

        EXPECT_EQ(materials.size(), 1);
        EXPECT_EQ(materials[0]->GetId(), defaultMaterial->GetId());

        Physics::MaterialSlots materialSlotsWithNoAssets;
        materialSlotsWithNoAssets.SetSlots({"Slot1", "Slot2", "Slot3"});

        AZStd::vector<AZStd::shared_ptr<PhysX::Material>> materials2 = PhysX::Material::FindOrCreateMaterials(materialSlotsWithNoAssets);

        EXPECT_EQ(materials2.size(), 3);
        for (const auto& material : materials2)
        {
            EXPECT_EQ(material->GetId(), defaultMaterial->GetId());
        }

        PhysX::MaterialConfiguration materialConfiguration;

        const auto materialAsset1 = materialConfiguration.CreateMaterialAsset();
        const auto materialAsset2 = materialConfiguration.CreateMaterialAsset();

        Physics::MaterialSlots materialSlotsWithAssets;
        materialSlotsWithAssets.SetSlots({ "Slot1", "Slot2" });
        materialSlotsWithAssets.SetMaterialAsset(0, materialAsset1);
        materialSlotsWithAssets.SetMaterialAsset(1, materialAsset2);

        AZStd::vector<AZStd::shared_ptr<PhysX::Material>> materials3 = PhysX::Material::FindOrCreateMaterials(materialSlotsWithAssets);

        EXPECT_EQ(materials3.size(), 2);
        EXPECT_EQ(materials3[0]->GetMaterialAsset(), materialAsset1);
        EXPECT_EQ(materials3[1]->GetMaterialAsset(), materialAsset2);
    }

    TEST_F(PhysXMaterialFixture, Material_CreateMaterialWithRandomId)
    {
        AZStd::shared_ptr<PhysX::Material> materialNull = PhysX::Material::CreateMaterialWithRandomId(AZ::Data::Asset<Physics::MaterialAsset>());

        EXPECT_TRUE(materialNull.get() == nullptr);

        PhysX::MaterialConfiguration materialConfiguration;
        const auto materialAsset = materialConfiguration.CreateMaterialAsset();

        AZStd::shared_ptr<PhysX::Material> material1 = PhysX::Material::CreateMaterialWithRandomId(materialAsset);

        EXPECT_TRUE(material1.get() != nullptr);

        AZStd::shared_ptr<PhysX::Material> material2 = PhysX::Material::CreateMaterialWithRandomId(materialAsset);

        EXPECT_TRUE(material2.get() != nullptr);
        EXPECT_NE(material1.get(), material2.get());
        EXPECT_NE(material1->GetId(), material2->GetId());
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_DynamicFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_dynamicFriction = 68.6f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetDynamicFriction(), 68.6f, Tolerance);

        material->SetDynamicFriction(31.2f);
        EXPECT_NEAR(material->GetDynamicFriction(), 31.2f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_Clamps_DynamicFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_dynamicFriction = -7.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetDynamicFriction(), 0.0f, Tolerance);

        material->SetDynamicFriction(-61.0f);
        EXPECT_NEAR(material->GetDynamicFriction(), 0.0f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_StaticFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_staticFriction = 68.6f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetStaticFriction(), 68.6f, Tolerance);

        material->SetStaticFriction(31.2f);
        EXPECT_NEAR(material->GetStaticFriction(), 31.2f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_Clamps_StaticFriction)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_staticFriction = -7.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetStaticFriction(), 0.0f, Tolerance);

        material->SetStaticFriction(-61.0f);
        EXPECT_NEAR(material->GetStaticFriction(), 0.0f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_Restitution)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitution = 0.43f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetRestitution(), 0.43f, Tolerance);

        material->SetRestitution(0.78f);
        EXPECT_NEAR(material->GetRestitution(), 0.78f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_Clamps_Restitution)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitution = -13.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetRestitution(), 0.0f, Tolerance);

        material->SetRestitution(0.0f);
        EXPECT_NEAR(material->GetRestitution(), 0.0f, Tolerance);

        material->SetRestitution(1.0f);
        EXPECT_NEAR(material->GetRestitution(), 1.0f, Tolerance);

        material->SetRestitution(61.0f);
        EXPECT_NEAR(material->GetRestitution(), 1.0f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_Density)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_density = 245.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetDensity(), 245.0f, Tolerance);

        material->SetDensity(43.1f);
        EXPECT_NEAR(material->GetDensity(), 43.1f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_Clamps_Density)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_density = -13.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetDensity(), PhysX::MaterialConstants::MinDensityLimit, Tolerance);

        material->SetDensity(0.0f);
        EXPECT_NEAR(material->GetDensity(), PhysX::MaterialConstants::MinDensityLimit, Tolerance);

        material->SetDensity(PhysX::MaterialConstants::MinDensityLimit);
        EXPECT_NEAR(material->GetDensity(), PhysX::MaterialConstants::MinDensityLimit, Tolerance);

        material->SetDensity(PhysX::MaterialConstants::MaxDensityLimit);
        EXPECT_NEAR(material->GetDensity(), PhysX::MaterialConstants::MaxDensityLimit, Tolerance);

        material->SetDensity(200000.0f);
        EXPECT_NEAR(material->GetDensity(), PhysX::MaterialConstants::MaxDensityLimit, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_FrictionCombineMode)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_frictionCombine = PhysX::CombineMode::Maximum;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_EQ(material->GetFrictionCombineMode(), PhysX::CombineMode::Maximum);

        material->SetFrictionCombineMode(PhysX::CombineMode::Minimum);
        EXPECT_EQ(material->GetFrictionCombineMode(), PhysX::CombineMode::Minimum);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_RestitutionCombineMode)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitutionCombine = PhysX::CombineMode::Maximum;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_EQ(material->GetRestitutionCombineMode(), PhysX::CombineMode::Maximum);

        material->SetRestitutionCombineMode(PhysX::CombineMode::Minimum);
        EXPECT_EQ(material->GetRestitutionCombineMode(), PhysX::CombineMode::Minimum);
    }

#if (PX_PHYSICS_VERSION_MAJOR >= 5)
    TEST_F(PhysXMaterialFixture, Material_GetSet_CompliantContactMode_Enabled)
    {
        PhysX::MaterialConfiguration materialConfiguration;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_FALSE(material->IsCompliantContactModeEnabled());

        material->EnableCompliantContactMode(true);
        EXPECT_TRUE(material->IsCompliantContactModeEnabled());

        material->EnableCompliantContactMode(false);
        EXPECT_FALSE(material->IsCompliantContactModeEnabled());
    }

    TEST_F(PhysXMaterialFixture, Material_CompliantContactModeEnabled_RestitutionRemainsUnchanged)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_restitution = 1.0f;
        materialConfiguration.m_compliantContactMode.m_damping = 2.0f;
        materialConfiguration.m_compliantContactMode.m_stiffness = 3.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_FALSE(material->IsCompliantContactModeEnabled());
        EXPECT_NEAR(material->GetRestitution(), 1.0f, Tolerance);
        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 2.0f, Tolerance);
        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 3.0f, Tolerance);

        material->EnableCompliantContactMode(true);

        EXPECT_TRUE(material->IsCompliantContactModeEnabled());
        EXPECT_NEAR(material->GetRestitution(), 1.0f, Tolerance);
        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 2.0f, Tolerance);
        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 3.0f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_CompliantContactMode_Damping)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_compliantContactMode.m_damping = 245.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 245.0f, Tolerance);

        // Damping property is set correctly with compliant contact mode enabled
        material->EnableCompliantContactMode(true);
        material->SetCompliantContactModeDamping(64.2f);
        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 64.2f, Tolerance);

        // Damping property is set correctly with compliant contact mode disabled
        material->EnableCompliantContactMode(false);
        material->SetCompliantContactModeDamping(43.1f);
        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 43.1f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_Clamps_CompliantContactMode_Damping)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_compliantContactMode.m_damping = -13.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 0.0f, Tolerance);

        material->SetCompliantContactModeDamping(0.0f);
        EXPECT_NEAR(material->GetCompliantContactModeDamping(), 0.0f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_GetSet_CompliantContactMode_Stiffness)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_compliantContactMode.m_stiffness = 245.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 245.0f, Tolerance);

        // Stiffness property is set correctly with compliant contact mode enabled
        material->EnableCompliantContactMode(true);
        material->SetCompliantContactModeStiffness(64.2f);
        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 64.2f, Tolerance);

        // Stiffness property is set correctly with compliant contact mode disabled
        material->EnableCompliantContactMode(false);
        material->SetCompliantContactModeStiffness(43.1f);
        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 43.1f, Tolerance);
    }

    TEST_F(PhysXMaterialFixture, Material_Clamps_CompliantContactMode_Stiffness)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_compliantContactMode.m_stiffness = -13.0f;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 0.0f, Tolerance);

        material->SetCompliantContactModeStiffness(0.0f);
        EXPECT_NEAR(material->GetCompliantContactModeStiffness(), 0.0f, Tolerance);
    }
#endif // (PX_PHYSICS_VERSION_MAJOR >= 5)

    TEST_F(PhysXMaterialFixture, Material_GetSet_DebugColor)
    {
        PhysX::MaterialConfiguration materialConfiguration;
        materialConfiguration.m_debugColor = AZ::Colors::Lavender;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_THAT(material->GetDebugColor(), IsClose(AZ::Colors::Lavender));

        material->SetDebugColor(AZ::Colors::Aquamarine);
        EXPECT_THAT(material->GetDebugColor(), IsClose(AZ::Colors::Aquamarine));
    }

    TEST_F(PhysXMaterialFixture, Material_ReturnsValid_PxMaterial)
    {
        PhysX::MaterialConfiguration materialConfiguration;

        AZStd::shared_ptr<PhysX::Material> material = PhysX::Material::FindOrCreateMaterial(materialConfiguration.CreateMaterialAsset());

        EXPECT_TRUE(material->GetPxMaterial() != nullptr);
    }
} // namespace UnitTest
