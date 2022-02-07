/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <Atom/Utils/TestUtils/AssetSystemStub.h>
#include <AtomToolsFramework/Util/MaterialPropertyUtil.h>

namespace UnitTest
{
    class AtomToolsFrameworkTestEnvironment : public AZ::Test::ITestEnvironment
    {
    protected:
        void SetupEnvironment() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
        }

        void TeardownEnvironment() override
        {
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }
    };

    class AtomToolsFrameworkTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_assetSystemStub.Activate();

            RegisterSourceAsset("objects/upgrades/materials/supercondor.material");
            RegisterSourceAsset("materials/condor.material");
            RegisterSourceAsset("materials/talisman.material");
            RegisterSourceAsset("materials/city.material");
            RegisterSourceAsset("materials/totem.material");
            RegisterSourceAsset("textures/orange.png");
            RegisterSourceAsset("textures/red.png");
            RegisterSourceAsset("textures/gold.png");
            RegisterSourceAsset("textures/fuzz.png");
        }

        void TearDown() override
        {
            m_assetSystemStub.Deactivate();
        }

        void RegisterSourceAsset(const AZStd::string& path)
        {
            const AZ::IO::BasicPath assetRootPath = AZ::IO::PathView(m_assetRoot).LexicallyNormal();
            const AZ::IO::BasicPath normalizedPath = AZ::IO::BasicPath(assetRootPath).Append(path).LexicallyNormal();

            AZ::Data::AssetInfo assetInfo = {};
            assetInfo.m_assetId = AZ::Uuid::CreateRandom();
            assetInfo.m_relativePath = normalizedPath.LexicallyRelative(assetRootPath).StringAsPosix();
            m_assetSystemStub.RegisterSourceInfo(normalizedPath.StringAsPosix().c_str(), assetInfo, assetRootPath.StringAsPosix().c_str());
        }

        static constexpr const char* m_assetRoot = "d:/project/assets/";
        AssetSystemStub m_assetSystemStub;
    };

    TEST_F(AtomToolsFrameworkTest, GetExteralReferencePath_Succeeds)
    {
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("", "", 2), "");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/condor.material", "", 2), "");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/talisman.material", "", 2), "");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/talisman.material", "d:/project/assets/textures/gold.png", 2), "../textures/gold.png");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/materials/talisman.material", "d:/project/assets/textures/gold.png", 0), "textures/gold.png");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", 3), "../../../materials/condor.material");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", 2), "materials/condor.material");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", 1), "materials/condor.material");
        ASSERT_EQ(AtomToolsFramework::GetExteralReferencePath("d:/project/assets/objects/upgrades/materials/supercondor.material", "d:/project/assets/materials/condor.material", 0), "materials/condor.material");
    }

    AZ_UNIT_TEST_HOOK(new AtomToolsFrameworkTestEnvironment);
} // namespace UnitTest
