/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Console/Console.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Math/MatrixUtils.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>
#include <random>

using namespace AzFramework;

namespace UnitTest
{
    class OctreeTests
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        { 
            m_console = aznew AZ::Console();
            AZ::Interface<AZ::IConsole>::Register(m_console);
            m_console->LinkDeferredFunctors(AZ::ConsoleFunctorBase::GetDeferredHead());

            m_console->GetCvarValue("bg_octreeNodeMaxEntries", m_savedMaxEntries);
            m_console->GetCvarValue("bg_octreeNodeMinEntries", m_savedMinEntries);
            m_console->GetCvarValue("bg_octreeMaxWorldExtents", m_savedBounds);

            // To ease unit testing, configure the octreeSystemComponent to only allow one entry per node
            m_console->PerformCommand("bg_octreeNodeMaxEntries 1");
            m_console->PerformCommand("bg_octreeNodeMinEntries 1");
            m_console->PerformCommand("bg_octreeMaxWorldExtents 1"); // Create a -1,-1,-1 to 1,1,1 world volume

            if (!AZ::NameDictionary::IsReady())
            {
                AZ::NameDictionary::Create();
            }
            m_octreeSystemComponent = new OctreeSystemComponent;
            IVisibilityScene* visScene = m_octreeSystemComponent->CreateVisibilityScene(AZ::Name("OctreeUnitTestScene"));
            m_octreeScene = azdynamic_cast<OctreeScene*>(visScene);
        }

        void TearDown() override
        {
            //Restore octreeSystemComponent cvars for any future tests or benchmarks that might get executed
            AZStd::string commandString;
            commandString.format("bg_octreeNodeMaxEntries %u", m_savedMaxEntries);
            m_console->PerformCommand(commandString.c_str());
            commandString.format("bg_octreeNodeMinEntries %u", m_savedMinEntries);
            m_console->PerformCommand(commandString.c_str());
            commandString.format("bg_octreeMaxWorldExtents %f", m_savedBounds);
            m_console->PerformCommand(commandString.c_str());

            m_octreeSystemComponent->DestroyVisibilityScene(m_octreeScene);
            delete m_octreeSystemComponent;
            m_octreeSystemComponent = nullptr;

            AZ::NameDictionary::Destroy();

            AZ::Interface<AZ::IConsole>::Unregister(m_console);
            delete m_console;
            m_console = nullptr;
        }

        OctreeSystemComponent* m_octreeSystemComponent = nullptr;
        OctreeScene* m_octreeScene = nullptr;
        uint32_t m_savedMaxEntries = 0;
        uint32_t m_savedMinEntries = 0;
        float m_savedBounds = 0.0f;
        AZ::Console* m_console;
    };

    void ValidateEntryCountEqualsExpectedCount(const IVisibilityScene* visScene, uint32_t expectedEntryCount)
    {
        // InsertOrUpdateEntry assumes that updating an existing entry won't change the count
        // so it doesn't modify the counter used by GetEntryCount.
        // If an entry is removed from the octree as an unintended side effect of updating an existing entry,
        // GetEntryCount can't be relied upon to report the actual entry count.
        // So manually count the entries when using the entry count for validation.
        size_t manualEntryCount = 0;
        visScene->EnumerateNoCull([&manualEntryCount](const AzFramework::IVisibilityScene::NodeData& nodeData) { manualEntryCount += nodeData.m_entries.size(); });

        EXPECT_EQ(manualEntryCount, expectedEntryCount);
        EXPECT_EQ(visScene->GetEntryCount(), expectedEntryCount);
    }

    TEST_F(OctreeTests, InsertDeleteSingleEntry)
    {
        AzFramework::VisibilityEntry visEntry;
        visEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());

        m_octreeScene->InsertOrUpdateEntry(visEntry);
        EXPECT_TRUE(visEntry.m_internalNode != nullptr);
        EXPECT_TRUE(visEntry.m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);

        m_octreeScene->RemoveEntry(visEntry);
        EXPECT_TRUE(visEntry.m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);

        EXPECT_TRUE(true); //TEST
    }

    TEST_F(OctreeTests, InsertDeleteSplitMerge)
    {
        AzFramework::VisibilityEntry visEntry[3];
        visEntry[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        visEntry[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.1f), AZ::Vector3( 0.4f));
        visEntry[2].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.6f), AZ::Vector3( 0.9f));

        m_octreeScene->InsertOrUpdateEntry(visEntry[0]);
        EXPECT_TRUE(visEntry[0].m_internalNode != nullptr);
        EXPECT_TRUE(visEntry[0].m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);

        m_octreeScene->InsertOrUpdateEntry(visEntry[1]); // This should force a split of the root node
        EXPECT_TRUE(visEntry[1].m_internalNode != nullptr);
        EXPECT_TRUE(visEntry[1].m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + m_octreeScene->GetChildNodeCount());

        m_octreeScene->InsertOrUpdateEntry(visEntry[2]); // This should force a split of the roots +/+/+ child node
        EXPECT_TRUE(visEntry[2].m_internalNode != nullptr);
        EXPECT_TRUE(visEntry[2].m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + (2 * m_octreeScene->GetChildNodeCount()));

        m_octreeScene->RemoveEntry(visEntry[2]);
        EXPECT_TRUE(visEntry[2].m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + m_octreeScene->GetChildNodeCount());

        m_octreeScene->RemoveEntry(visEntry[1]);
        EXPECT_TRUE(visEntry[1].m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);

        m_octreeScene->RemoveEntry(visEntry[0]);
        EXPECT_TRUE(visEntry[0].m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
    }

    TEST_F(OctreeTests, UpdateSingleEntry)
    {
        AzFramework::VisibilityEntry visEntry;
        visEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());

        m_octreeScene->InsertOrUpdateEntry(visEntry);
        EXPECT_TRUE(visEntry.m_internalNode != nullptr);
        EXPECT_TRUE(visEntry.m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);

        visEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));
        m_octreeScene->InsertOrUpdateEntry(visEntry);
        EXPECT_TRUE(visEntry.m_internalNode != nullptr);
        EXPECT_TRUE(visEntry.m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);

        m_octreeScene->RemoveEntry(visEntry);
        EXPECT_TRUE(visEntry.m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);
    }

    TEST_F(OctreeTests, UpdateSplitMerge)
    {
        AzFramework::VisibilityEntry visEntry[3];
        visEntry[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        visEntry[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.1f), AZ::Vector3( 0.4f));
        visEntry[2].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.6f), AZ::Vector3( 0.9f));

        m_octreeScene->InsertOrUpdateEntry(visEntry[0]);
        EXPECT_TRUE(visEntry[0].m_internalNode != nullptr);
        EXPECT_TRUE(visEntry[0].m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);

        m_octreeScene->InsertOrUpdateEntry(visEntry[1]); // This should force a split of the root node
        EXPECT_TRUE(visEntry[1].m_internalNode != nullptr);
        EXPECT_TRUE(visEntry[1].m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + m_octreeScene->GetChildNodeCount());

        m_octreeScene->InsertOrUpdateEntry(visEntry[2]); // This should force a split of the roots +/+/+ child node
        EXPECT_TRUE(visEntry[2].m_internalNode != nullptr);
        EXPECT_TRUE(visEntry[2].m_internalNodeIndex == 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + (2 * m_octreeScene->GetChildNodeCount()));

        visEntry[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        visEntry[2].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.1f), AZ::Vector3( 0.4f));
        visEntry[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.6f), AZ::Vector3( 0.9f));
        m_octreeScene->InsertOrUpdateEntry(visEntry[0]);
        m_octreeScene->InsertOrUpdateEntry(visEntry[1]);
        m_octreeScene->InsertOrUpdateEntry(visEntry[2]);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + (2 * m_octreeScene->GetChildNodeCount()));

        m_octreeScene->RemoveEntry(visEntry[2]);
        EXPECT_TRUE(visEntry[2].m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1 + m_octreeScene->GetChildNodeCount());

        m_octreeScene->RemoveEntry(visEntry[1]);
        EXPECT_TRUE(visEntry[1].m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);

        m_octreeScene->RemoveEntry(visEntry[0]);
        EXPECT_TRUE(visEntry[0].m_internalNode == nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        EXPECT_TRUE(m_octreeScene->GetNodeCount() == 1);
    }

    void AppendEntries(AZStd::vector<VisibilityEntry*>& gatheredEntries, const AzFramework::IVisibilityScene::NodeData& nodeData)
    {
        gatheredEntries.insert(gatheredEntries.end(), nodeData.m_entries.begin(), nodeData.m_entries.end());
    }

    template <typename BoundType>
    void EnumerateSingleEntryHelper(IVisibilityScene* visScene, const BoundType& bounds)
    {
        AzFramework::VisibilityEntry visEntry;
        visEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3::CreateZero(), AZ::Vector3::CreateOne());

        AZStd::vector<VisibilityEntry*> gatheredEntries;
        visScene->Enumerate(bounds, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.empty());

        visScene->InsertOrUpdateEntry(visEntry);
        visScene->Enumerate(bounds, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 1);
        EXPECT_TRUE(gatheredEntries[0] == &visEntry);

        visEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));
        visScene->InsertOrUpdateEntry(visEntry);
        gatheredEntries.clear();
        visScene->Enumerate(bounds, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 1);
        EXPECT_TRUE(gatheredEntries[0] == &visEntry);

        visScene->RemoveEntry(visEntry);
        gatheredEntries.clear();
        visScene->Enumerate(bounds, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.empty());
    }

    TEST_F(OctreeTests, EnumerateSphereSingleEntry)
    {
        AZ::Sphere bounds = AZ::Sphere::CreateUnitSphere();
        EnumerateSingleEntryHelper(m_octreeScene, bounds);
    }

    TEST_F(OctreeTests, EnumerateAabbSingleEntry)
    {
        AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(1.0f));
        EnumerateSingleEntryHelper(m_octreeScene, bounds);
    }

    TEST_F(OctreeTests, EnumerateFrustumSingleEntry)
    {
        AZ::Vector3 frustumOrigin = AZ::Vector3(0.0f, -2.0f, 0.0f);
        AZ::Quaternion frustumDirection = AZ::Quaternion::CreateIdentity();
        AZ::Transform frustumTransform = AZ::Transform::CreateFromQuaternionAndTranslation(frustumDirection, frustumOrigin);
        AZ::Frustum bounds = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 1.0f, 3.0f));
        EnumerateSingleEntryHelper(m_octreeScene, bounds);
    }

    // bound1 should cover the entire spatial hash
    // bound2 should not cross into the positive Y-axis
    // bound3 should only intersect the region inside 0.6, 0.6, 0.6 to 0.9, 0.9, 0.9
    template <typename BoundType>
    void EnumerateMultipleEntriesHelper(IVisibilityScene* visScene, const BoundType& bound1, const BoundType& bound2, const BoundType& bound3)
    {
        AZStd::vector<VisibilityEntry*> gatheredEntries;

        AzFramework::VisibilityEntry visEntry[3];
        visEntry[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        visEntry[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.1f), AZ::Vector3( 0.4f));
        visEntry[2].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.6f), AZ::Vector3( 0.9f));

        visScene->InsertOrUpdateEntry(visEntry[0]);
        visScene->InsertOrUpdateEntry(visEntry[1]);
        visScene->InsertOrUpdateEntry(visEntry[2]);

        gatheredEntries.clear();
        visScene->Enumerate(bound1, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 3);

        gatheredEntries.clear();
        visScene->Enumerate(bound2, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 1);
        EXPECT_TRUE(gatheredEntries[0] == &(visEntry[0]));

        gatheredEntries.clear();
        visScene->Enumerate(bound3, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 1);
        EXPECT_TRUE(gatheredEntries[0] == &(visEntry[2]));

        visEntry[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        visEntry[2].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.1f), AZ::Vector3( 0.4f));
        visEntry[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.6f), AZ::Vector3( 0.9f));
        visScene->InsertOrUpdateEntry(visEntry[0]);
        visScene->InsertOrUpdateEntry(visEntry[1]);
        visScene->InsertOrUpdateEntry(visEntry[2]);

        gatheredEntries.clear();
        visScene->Enumerate(bound1, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 3);

        gatheredEntries.clear();
        visScene->Enumerate(bound2, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 1);
        EXPECT_TRUE(gatheredEntries[0] == &(visEntry[1]));

        gatheredEntries.clear();
        visScene->Enumerate(bound3, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.size() == 1);
        EXPECT_TRUE(gatheredEntries[0] == &(visEntry[0]));

        visScene->RemoveEntry(visEntry[0]);
        visScene->RemoveEntry(visEntry[1]);
        visScene->RemoveEntry(visEntry[2]);
        gatheredEntries.clear();
        visScene->Enumerate(bound1, [&gatheredEntries](const AzFramework::IVisibilityScene::NodeData& nodeData) { AppendEntries(gatheredEntries, nodeData); });
        EXPECT_TRUE(gatheredEntries.empty());
    }

    TEST_F(OctreeTests, EnumerateSphereMultipleEntries)
    {
        AZ::Sphere bound1 = AZ::Sphere::CreateUnitSphere();
        AZ::Sphere bound2 = AZ::Sphere(AZ::Vector3(-0.5f), 0.5f);
        AZ::Sphere bound3 = AZ::Sphere(AZ::Vector3(0.75f), 0.2f);
        EnumerateMultipleEntriesHelper(m_octreeScene, bound1, bound2, bound3);
    }

    TEST_F(OctreeTests, EnumerateAabbMultipleEntries)
    {
        AZ::Aabb bound1 = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3( 1.0f));
        AZ::Aabb bound2 = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(-0.5f));
        AZ::Aabb bound3 = AZ::Aabb::CreateFromMinMax(AZ::Vector3( 0.6f), AZ::Vector3( 0.9f));
        EnumerateMultipleEntriesHelper(m_octreeScene, bound1, bound2, bound3);
    }

    TEST_F(OctreeTests, EnumerateFrustumMultipleEntries)
    {
        AZ::Vector3 frustumOrigin = AZ::Vector3(0.0f, -2.0f, 0.0f);
        AZ::Quaternion frustumDirection = AZ::Quaternion::CreateIdentity();
        AZ::Transform frustumTransform = AZ::Transform::CreateFromQuaternionAndTranslation(frustumDirection, frustumOrigin);
        AZ::Frustum bound1 = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 1.0f, 3.0f));
        AZ::Frustum bound2 = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 1.0f, 2.0f));
        AZ::Frustum bound3 = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 2.6f, 2.9f));
        EnumerateMultipleEntriesHelper(m_octreeScene, bound1, bound2, bound3);
    }

    TEST_F(OctreeTests, InsertOrUpdateEntry_OverFillRootNodeWithLargeEntries_EntriesAreNotLost)
    {
        // Validate that the octree works if you exceed the max entry count with large entries,
        // which will overfill the root node since they can't be distributed to child nodes

        // Get the max extents and entries-per-node for the octree
        AZ::IConsole* console = AZ::Interface<AZ::IConsole>::Get();
        EXPECT_TRUE(console);

        float maxExtents = 0.0f;
        AZ::GetValueResult getCvarResult = console->GetCvarValue("bg_octreeMaxWorldExtents", maxExtents);
        EXPECT_EQ(getCvarResult, AZ::GetValueResult::Success);

        uint32_t maxEntriesPerNode = 0;
        getCvarResult = console->GetCvarValue("bg_octreeNodeMaxEntries", maxEntriesPerNode);
        EXPECT_EQ(getCvarResult, AZ::GetValueResult::Success);

        // Create root entries that would exceed the size of the root node
        AZ::Aabb exceedMaxExtents = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-maxExtents - 1.0f), AZ::Vector3(maxExtents + 1.0f));
        uint32_t exceedMaxEntriesPerNode = maxEntriesPerNode + 1;

        AzFramework::VisibilityEntry visEntry;
        visEntry.m_boundingVolume = exceedMaxExtents;
        AZStd::vector<AzFramework::VisibilityEntry> visEntries(exceedMaxEntriesPerNode, visEntry);

        // Insert them all into the scene
        for (AzFramework::VisibilityEntry& entry : visEntries)
        {
            m_octreeScene->InsertOrUpdateEntry(entry);
        }

        // Expect all the entries to be in the scene
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, static_cast<uint32_t>(visEntries.size()));

        // Update them, without making any actual changes
        for (AzFramework::VisibilityEntry& entry : visEntries)
        {
            m_octreeScene->InsertOrUpdateEntry(entry);
        }

        // Expect all the entries to be in the scene
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, static_cast<uint32_t>(visEntries.size()));
    }

    TEST_F(OctreeTests, ExcludeFrustumTest)
    {
        // This test is made to be similar to EnumerateMultipleEntriesHelper, however needs to be
        // separate due to a different function signature. 

        AZStd::vector<VisibilityEntry*> gatheredEntries;
        auto gatherEntries = [&](const AzFramework::IVisibilityScene::NodeData& nodeData)
        {
            AppendEntries(gatheredEntries, nodeData);
        };

        AzFramework::VisibilityEntry visEntry[3];
        visEntry[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        visEntry[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.1f), AZ::Vector3(0.4f));
        visEntry[2].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));

        m_octreeScene->InsertOrUpdateEntry(visEntry[0]);
        m_octreeScene->InsertOrUpdateEntry(visEntry[1]);
        m_octreeScene->InsertOrUpdateEntry(visEntry[2]);

        {
            // Covers entire -1 to 1 region of the octree
            AZ::Vector3 frustumOrigin = AZ::Vector3(0.0f, -2.0f, 0.0f);
            AZ::Quaternion frustumDirection = AZ::Quaternion::CreateIdentity();
            AZ::Transform frustumTransform = AZ::Transform::CreateFromQuaternionAndTranslation(frustumDirection, frustumOrigin);
            AZ::Frustum include = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(2.0f), 1.0f, 5.0f));
            AZ::Frustum exclude = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 3.0f, 5.0f));

            m_octreeScene->Enumerate(include, exclude, gatherEntries);
            EXPECT_EQ(gatheredEntries.size(), 3);
            gatheredEntries.clear();
        }

        {
            // Covers only on -y side
            AZ::Vector3 frustumOrigin = AZ::Vector3(0.0f, -2.0f, 0.0f);
            AZ::Quaternion frustumDirection = AZ::Quaternion::CreateIdentity();
            AZ::Transform frustumTransform = AZ::Transform::CreateFromQuaternionAndTranslation(frustumDirection, frustumOrigin);
            AZ::Frustum include = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(2.0f), 1.0f, 5.0f));
            AZ::Frustum exclude = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 2.0f, 5.0f));

            m_octreeScene->Enumerate(include, exclude, gatherEntries);
            EXPECT_EQ(gatheredEntries.size(), 1);
            gatheredEntries.clear();
        }

        {
            // Entire world inside exclusion frustum
            AZ::Vector3 frustumOrigin = AZ::Vector3(0.0f, -2.0f, 0.0f);
            AZ::Quaternion frustumDirection = AZ::Quaternion::CreateIdentity();
            AZ::Transform frustumTransform = AZ::Transform::CreateFromQuaternionAndTranslation(frustumDirection, frustumOrigin);
            AZ::Frustum include = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(2.0f), 1.0f, 5.0f));
            AZ::Frustum exclude = AZ::Frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(1.0f), 0.5f, 4.0f));

            m_octreeScene->Enumerate(include, exclude, gatherEntries);
            EXPECT_EQ(gatheredEntries.size(), 0);
            gatheredEntries.clear();
        }

    }
}
