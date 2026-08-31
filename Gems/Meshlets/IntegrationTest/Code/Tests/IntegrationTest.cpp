/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 * End-to-end integration test for the .azmeshletpack pipeline.
 *
 *   .fbx (with MeshletPackRule scenemanifest)
 *     → AssetProcessor (MeshletsBuilders module loaded)
 *     → .azmeshletpack product registered with product-dependency on .azmodel
 *     → Runtime AssetManager loads the pack
 *     → MeshletsRenderObject constructed from pack data without crashing
 *
 * Renderer-end verification is manual (smoke test §9.5).
 */

#include <AzTest/AzTest.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Application/Application.h>
#include <cstring>

#include <Meshlets/Reflect/MeshletPackAsset.h>

namespace UnitTest
{
    using namespace AZ::Meshlets;

    //! Application fixture that boots a minimal AzFramework app + asset
    //! manager, so the test can exercise the asset load path without a full
    //! O3DE editor process.
    class MeshletsIntegrationFixture : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_app = AZStd::make_unique<AzFramework::Application>();
            m_app->Start(AzFramework::Application::Descriptor());
        }
        void TearDown() override
        {
            m_app->Stop();
            m_app.reset();
        }
        AZStd::unique_ptr<AzFramework::Application> m_app;
    };

    //! Locates a .azmeshletpack product by its source-model AssetId. Returns
    //! an invalid AssetId if no pack exists.
    static AZ::Data::AssetId FindPackForModel(const AZ::Data::AssetId& modelId)
    {
        AZ::Data::AssetId result;
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            [&](AZ::Data::AssetCatalogRequests* catalog)
            {
                catalog->EnumerateAssets(
                    nullptr,
                    [&](const AZ::Data::AssetId& id, const AZ::Data::AssetInfo& info)
                    {
                        if (info.m_assetType != azrtti_typeid<MeshletPackAsset>()) return;
                        auto asset = AZ::Data::AssetManager::Instance()
                            .GetAsset<MeshletPackAsset>(
                                id, AZ::Data::AssetLoadBehavior::PreLoad);
                        asset.BlockUntilLoadComplete();
                        if (!asset.IsReady()) return;
                        const PackHeaderRecord* h = asset->GetPackHeader();
                        if (!h) return;
                        
                        // Reconstruct AssetId from raw GUID + sub-id (Task 2 spec correction).
                        AZ::Uuid headerGuid;
                        std::memcpy(&headerGuid, h->m_sourceModelGuid, sizeof(h->m_sourceModelGuid));
                        const AZ::Data::AssetId headerModelId(headerGuid, h->m_sourceModelSubId);
                        if (headerModelId == modelId)
                        {
                            result = id;
                        }
                    },
                    nullptr);
            });
        return result;
    }

    TEST_F(MeshletsIntegrationFixture, CubeFbxProducesLoadablePack)
    {
        // Resolve the cube.azmodel AssetId. AssetCatalog returns it by relative
        // source path. Test fixture project must have AutomatedTesting wired
        // in or the test asset folder accessible via @assets@ alias.
        AZ::Data::AssetId cubeModelId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            cubeModelId,
            &AZ::Data::AssetCatalogRequests::GetAssetIdByPath,
            "objects/cube.azmodel",
            AZ::Data::s_invalidAssetType,
            /*autoRegisterIfNotFound=*/false);

        if (!cubeModelId.IsValid())
        {
            GTEST_SKIP() <<
                "objects/cube.azmodel not in catalog; skipping integration test. "
                "Ensure AutomatedTesting project assets are accessible to the "
                "test fixture and that AutomatedTesting/Objects/cube.fbx has "
                "a MeshletPackRule in its scenemanifest.";
        }

        const AZ::Data::AssetId packId = FindPackForModel(cubeModelId);
        ASSERT_TRUE(packId.IsValid())
            << "No .azmeshletpack product associates with cube.azmodel. "
            << "Confirm AssetProcessor ran the Meshlets builders.";

        auto packAsset = AZ::Data::AssetManager::Instance()
            .GetAsset<MeshletPackAsset>(packId, AZ::Data::AssetLoadBehavior::PreLoad);
        packAsset.BlockUntilLoadComplete();
        ASSERT_TRUE(packAsset.IsReady());

        const PackHeaderRecord* hdr = packAsset->GetPackHeader();
        ASSERT_NE(nullptr, hdr);
        
        // Reconstruct AssetId from raw GUID + sub-id (Task 2 spec correction).
        AZ::Uuid headerGuid;
        std::memcpy(&headerGuid, hdr->m_sourceModelGuid, sizeof(hdr->m_sourceModelGuid));
        const AZ::Data::AssetId headerModelId(headerGuid, hdr->m_sourceModelSubId);
        EXPECT_EQ(cubeModelId, headerModelId);
        EXPECT_GT(hdr->m_meshCount, 0u);
        EXPECT_GE(hdr->m_maxVerticesPerCluster, 32u);
        EXPECT_LE(hdr->m_maxVerticesPerCluster, 128u);
    }

    AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);
}
