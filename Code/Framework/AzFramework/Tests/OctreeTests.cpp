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
#include <AzCore/Math/ShapeIntersection.h>
#include <AzCore/std/algorithm.h>
#include <AzCore/std/containers/array.h>
#include <AzFramework/Visibility/OctreeSystemComponent.h>
#include <limits>
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
            m_console->GetCvarValue("bg_octreeUseQuadtree", m_savedUseQuadtree);
            m_console->GetCvarValue("bg_octreeMaxWorldExtents", m_savedBounds);

            // To ease unit testing, configure the octreeSystemComponent to only allow one entry per node
            m_console->PerformCommand("bg_octreeNodeMaxEntries 1");
            m_console->PerformCommand("bg_octreeNodeMinEntries 1");
            m_console->PerformCommand("bg_octreeMaxWorldExtents 1"); // Create a -1,-1,-1 to 1,1,1 world volume
            SetUseQuadtree(false);

            if (!AZ::NameDictionary::IsReady())
            {
                AZ::NameDictionary::Create();
            }
            m_octreeSystemComponent = new OctreeSystemComponent;
            RecreateOctreeScene();
        }

        void TearDown() override
        {
            //Restore octreeSystemComponent cvars for any future tests or benchmarks that might get executed
            AZStd::string commandString;
            commandString = AZStd::string::format("bg_octreeNodeMaxEntries %u", m_savedMaxEntries);
            m_console->PerformCommand(commandString.c_str());
            commandString = AZStd::string::format("bg_octreeNodeMinEntries %u", m_savedMinEntries);
            m_console->PerformCommand(commandString.c_str());
            SetUseQuadtree(m_savedUseQuadtree);
            commandString = AZStd::string::format("bg_octreeMaxWorldExtents %f", m_savedBounds);
            m_console->PerformCommand(commandString.c_str());

            m_octreeSystemComponent->DestroyVisibilityScene(m_octreeScene);
            delete m_octreeSystemComponent;
            m_octreeSystemComponent = nullptr;

            AZ::NameDictionary::Destroy();

            AZ::Interface<AZ::IConsole>::Unregister(m_console);
            delete m_console;
            m_console = nullptr;
        }

        void RecreateOctreeScene()
        {
            if (m_octreeScene)
            {
                m_octreeSystemComponent->DestroyVisibilityScene(m_octreeScene);
            }
            IVisibilityScene* visScene = m_octreeSystemComponent->CreateVisibilityScene(AZ::Name("OctreeUnitTestScene"));
            m_octreeScene = azdynamic_cast<OctreeScene*>(visScene);
        }

        void SetUseQuadtree(bool useQuadtree)
        {
            AZStd::string commandString = "bg_octreeUseQuadtree false";
            if (useQuadtree)
            {
                commandString = "bg_octreeUseQuadtree true";
            }
            const AZ::PerformCommandResult commandResult = m_console->PerformCommand(
                commandString.c_str(),
                AZ::ConsoleSilentMode::Silent,
                AZ::ConsoleInvokedFrom::AzConsole,
                AZ::ConsoleFunctorFlags::Null,
                AZ::ConsoleFunctorFlags::Null);
            EXPECT_TRUE(commandResult.IsSuccess()) << commandResult.GetError().c_str();

            bool currentValue = false;
            EXPECT_EQ(m_console->GetCvarValue("bg_octreeUseQuadtree", currentValue), AZ::GetValueResult::Success);
            EXPECT_EQ(currentValue, useQuadtree);
        }

        OctreeSystemComponent* m_octreeSystemComponent = nullptr;
        OctreeScene* m_octreeScene = nullptr;
        uint32_t m_savedMaxEntries = 0;
        uint32_t m_savedMinEntries = 0;
        bool m_savedUseQuadtree = false;
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
        visScene->EnumerateNoCull(
            [&manualEntryCount](const AzFramework::IVisibilityScene::NodeData& nodeData)
            {
                manualEntryCount += nodeData.m_entries.size();
                EXPECT_TRUE(nodeData.m_bounds.IsFinite());
                EXPECT_TRUE(nodeData.m_bounds.GetExtents().IsFinite());
                EXPECT_TRUE(nodeData.m_bounds.GetCenter().IsFinite());
                for (const VisibilityEntry* entry : nodeData.m_entries)
                {
                    EXPECT_TRUE(AZ::ShapeIntersection::Contains(nodeData.m_bounds, entry->m_boundingVolume));
                    ASSERT_NE(entry->m_internalNode, nullptr);
                    const auto* node = static_cast<const OctreeNode*>(entry->m_internalNode);
                    ASSERT_LT(entry->m_internalNodeIndex, node->GetEntries().size());
                    EXPECT_EQ(node->GetEntries()[entry->m_internalNodeIndex], entry);
                }
            });

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

    TEST_F(OctreeTests, EnumerateWithoutFilterIncludesEmptyNodes)
    {
        const IVisibilityScene* visibilityScene = m_octreeScene;
        AZ::u32 visitedNodes = 0;
        visibilityScene->Enumerate(
            [&visitedNodes](const IVisibilityScene::NodeData&)
            {
                ++visitedNodes;
            });
        EXPECT_EQ(1u, visitedNodes);

        VisibilityEntry entries[2];
        entries[0].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        entries[1].m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
        for (VisibilityEntry& entry : entries)
        {
            m_octreeScene->InsertOrUpdateEntry(entry);
        }

        visitedNodes = 0;
        AZ::u32 emptyNodes = 0;
        visibilityScene->Enumerate(
            [&visitedNodes, &emptyNodes](const IVisibilityScene::NodeData& nodeData)
            {
                ++visitedNodes;
                emptyNodes += nodeData.m_entries.empty();
            });
        EXPECT_EQ(m_octreeScene->GetNodeCount(), visitedNodes);
        EXPECT_GT(emptyNodes, 0u);

        AZ::u32 occupiedNodes = 0;
        m_octreeScene->EnumerateNoCull(
            [&occupiedNodes](const IVisibilityScene::NodeData&)
            {
                ++occupiedNodes;
            });
        EXPECT_EQ(2u, occupiedNodes);

        for (VisibilityEntry& entry : entries)
        {
            m_octreeScene->RemoveEntry(entry);
        }
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
    AZStd::vector<VisibilityEntry*> GatherEntries(IVisibilityScene* visScene, const BoundType& bounds)
    {
        AZStd::vector<VisibilityEntry*> gatheredEntries;
        visScene->Enumerate(
            bounds,
            [&gatheredEntries](const IVisibilityScene::NodeData& nodeData)
            {
                AppendEntries(gatheredEntries, nodeData);
            });
        return gatheredEntries;
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

    TEST_F(OctreeTests, EnumerateFrustumFindsEntryBeyondInitialRootBounds)
    {
        constexpr float FarPosition = 35423.84375f;
        VisibilityEntry farEntry;
        farEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(FarPosition, 0.0f, 0.0f), AZ::Vector3(FarPosition + 0.25f, 0.25f, 0.25f));
        m_octreeScene->InsertOrUpdateEntry(farEntry);

        const AZ::Transform frustumTransform = AZ::Transform::CreateFromQuaternionAndTranslation(AZ::Quaternion::CreateIdentity(), AZ::Vector3(FarPosition, -2.0f, 0.0f));
        const AZ::Frustum frustum(AZ::ViewFrustumAttributes(frustumTransform, 1.0f, 2.0f * atanf(0.5f), 1.0f, 3.0f));

        const AZStd::vector<VisibilityEntry*> gatheredEntries = GatherEntries(m_octreeScene, frustum);

        ASSERT_EQ(gatheredEntries.size(), 1);
        EXPECT_EQ(gatheredEntries[0], &farEntry);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
    }

    TEST_F(OctreeTests, GrowToContainPreservesExistingEntries)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry movingEntry;
        movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.1f), AZ::Vector3(0.4f));
        VisibilityEntry nearEntry;
        nearEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));

        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        m_octreeScene->InsertOrUpdateEntry(nearEntry);
        movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f), AZ::Vector3(3.2f));
        m_octreeScene->InsertOrUpdateEntry(movingEntry);

        const AZStd::vector<VisibilityEntry*> localEntries = GatherEntries(m_octreeScene, AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(-0.5f)));
        ASSERT_EQ(localEntries.size(), 1);
        EXPECT_EQ(localEntries[0], &localEntry);

        const AZStd::vector<VisibilityEntry*> farEntries = GatherEntries(m_octreeScene, movingEntry.m_boundingVolume);
        ASSERT_EQ(farEntries.size(), 1);
        EXPECT_EQ(farEntries[0], &movingEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);
    }

    TEST_F(OctreeTests, GrowToContainSupportsAlternatingDirectionsAndRemoval)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry positiveEntry;
        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.1f), AZ::Vector3(3.2f));
        VisibilityEntry negativeEntry;
        negativeEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-8.0f), AZ::Vector3(-7.8f));

        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(positiveEntry);
        m_octreeScene->InsertOrUpdateEntry(negativeEntry);

        const AZStd::vector<VisibilityEntry*> localEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);
        const AZStd::vector<VisibilityEntry*> positiveEntries = GatherEntries(m_octreeScene, positiveEntry.m_boundingVolume);
        const AZStd::vector<VisibilityEntry*> negativeEntries = GatherEntries(m_octreeScene, negativeEntry.m_boundingVolume);
        ASSERT_EQ(localEntries.size(), 1);
        ASSERT_EQ(positiveEntries.size(), 1);
        ASSERT_EQ(negativeEntries.size(), 1);
        EXPECT_EQ(localEntries[0], &localEntry);
        EXPECT_EQ(positiveEntries[0], &positiveEntry);
        EXPECT_EQ(negativeEntries[0], &negativeEntry);

        m_octreeScene->RemoveEntry(localEntry);
        m_octreeScene->RemoveEntry(positiveEntry);
        m_octreeScene->RemoveEntry(negativeEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
    }

    TEST_F(OctreeTests, UpdateAfterGrowthDoesNotUseNodeReleasedByMerge)
    {
        VisibilityEntry spanningEntry;
        spanningEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.5f), AZ::Vector3(0.5f));
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
        VisibilityEntry farEntry;
        farEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(2.0f), AZ::Vector3(2.2f));

        m_octreeScene->InsertOrUpdateEntry(spanningEntry);
        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(farEntry);
        m_octreeScene->RemoveEntry(farEntry);
        m_octreeScene->InsertOrUpdateEntry(spanningEntry);

        const AZStd::vector<VisibilityEntry*> spanningEntries = GatherEntries(m_octreeScene, spanningEntry.m_boundingVolume);
        const AZStd::vector<VisibilityEntry*> localEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(spanningEntries.begin(), spanningEntries.end(), &spanningEntry), spanningEntries.end());
        EXPECT_NE(AZStd::find(localEntries.begin(), localEntries.end(), &localEntry), localEntries.end());
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);

        m_octreeScene->RemoveEntry(spanningEntry);
        m_octreeScene->RemoveEntry(localEntry);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
    }

    TEST_F(OctreeTests, QuadtreeGrowthPreservesXyPartitionAcrossZ)
    {
        SetUseQuadtree(true);
        RecreateOctreeScene();

        VisibilityEntry negativeEntry;
        negativeEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f, -0.9f, -0.9f), AZ::Vector3(-0.6f, -0.6f, -0.6f));
        VisibilityEntry positiveEntry;
        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f, 0.6f, 0.6f), AZ::Vector3(0.9f, 0.9f, 0.9f));

        m_octreeScene->InsertOrUpdateEntry(positiveEntry);
        m_octreeScene->InsertOrUpdateEntry(negativeEntry);

        AZStd::vector<VisibilityEntry*> negativeEntries = GatherEntries(m_octreeScene, negativeEntry.m_boundingVolume);
        ASSERT_EQ(negativeEntries.size(), 1);
        EXPECT_EQ(negativeEntries[0], &negativeEntry);

        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f, 3.0f, 3.0f), AZ::Vector3(3.2f, 3.2f, 3.2f));
        m_octreeScene->InsertOrUpdateEntry(positiveEntry);

        negativeEntries = GatherEntries(m_octreeScene, negativeEntry.m_boundingVolume);
        const AZStd::vector<VisibilityEntry*> positiveEntries = GatherEntries(m_octreeScene, positiveEntry.m_boundingVolume);
        ASSERT_EQ(negativeEntries.size(), 1);
        ASSERT_EQ(positiveEntries.size(), 1);
        EXPECT_EQ(negativeEntries[0], &negativeEntry);
        EXPECT_EQ(positiveEntries[0], &positiveEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
    }

    TEST_F(OctreeTests, QuadtreeGrowthKeepsOldXyPartitionAcrossExpandedZ)
    {
        SetUseQuadtree(true);
        RecreateOctreeScene();

        VisibilityEntry negativeEntry;
        negativeEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f, -0.9f, -0.1f), AZ::Vector3(-0.6f, -0.6f, 0.1f));
        VisibilityEntry positiveEntry;
        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f, 0.6f, -0.1f), AZ::Vector3(0.9f, 0.9f, 0.1f));
        VisibilityEntry farEntry;
        farEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f, 3.0f, -0.1f), AZ::Vector3(3.2f, 3.2f, 0.1f));

        m_octreeScene->InsertOrUpdateEntry(negativeEntry);
        m_octreeScene->InsertOrUpdateEntry(positiveEntry);
        m_octreeScene->InsertOrUpdateEntry(farEntry);

        VisibilityEntry highEntryInOldXyRegion;
        highEntryInOldXyRegion.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f, -0.9f, 5.9f), AZ::Vector3(-0.6f, -0.6f, 6.1f));
        m_octreeScene->InsertOrUpdateEntry(highEntryInOldXyRegion);

        const AZ::Aabb unrelatedHighQuery = AZ::Aabb::CreateFromMinMax(AZ::Vector3(5.0f, -0.9f, 5.9f), AZ::Vector3(5.2f, -0.6f, 6.1f));
        const AZStd::vector<VisibilityEntry*> unrelatedEntries = GatherEntries(m_octreeScene, unrelatedHighQuery);
        EXPECT_EQ(AZStd::find(unrelatedEntries.begin(), unrelatedEntries.end(), &highEntryInOldXyRegion), unrelatedEntries.end());

        const AZStd::vector<VisibilityEntry*> highEntries = GatherEntries(m_octreeScene, highEntryInOldXyRegion.m_boundingVolume);
        EXPECT_NE(AZStd::find(highEntries.begin(), highEntries.end(), &highEntryInOldXyRegion), highEntries.end());
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 4);
    }

    TEST_F(OctreeTests, GrowthFindsSpanningEntriesBeyondInitialRoot)
    {
        VisibilityEntry negativeEntry;
        negativeEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry positiveEntry;
        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
        m_octreeScene->InsertOrUpdateEntry(negativeEntry);
        m_octreeScene->InsertOrUpdateEntry(positiveEntry);

        VisibilityEntry spanningEntry;
        spanningEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-2.0f), AZ::Vector3(2.0f));
        m_octreeScene->InsertOrUpdateEntry(spanningEntry);

        const AZ::Aabb queryOutsideInitialRoot = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.95f), AZ::Vector3(-1.85f));
        const AZStd::vector<VisibilityEntry*> spanningEntries = GatherEntries(m_octreeScene, queryOutsideInitialRoot);
        EXPECT_NE(AZStd::find(spanningEntries.begin(), spanningEntries.end(), &spanningEntry), spanningEntries.end());
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);
    }

    TEST_F(OctreeTests, FailedGrowthDoesNotPartiallyChangeRootBounds)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry unsupportedEntry;
        const float maxCoordinate = std::numeric_limits<float>::max();
        unsupportedEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-maxCoordinate), AZ::Vector3(maxCoordinate));
        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(unsupportedEntry);

        const AZ::Aabb queryOutsideRoot = AZ::Aabb::CreateFromMinMax(AZ::Vector3(2.5f), AZ::Vector3(2.6f));
        EXPECT_TRUE(GatherEntries(m_octreeScene, queryOutsideRoot).empty());

        const AZStd::vector<VisibilityEntry*> localEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(localEntries.begin(), localEntries.end(), &localEntry), localEntries.end());
        EXPECT_EQ(unsupportedEntry.m_internalNode, nullptr);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
        m_octreeScene->EnumerateNoCull([](const IVisibilityScene::NodeData& nodeData)
            {
                EXPECT_EQ(nodeData.m_bounds.GetMin(), AZ::Vector3(-1.0f));
                EXPECT_EQ(nodeData.m_bounds.GetMax(), AZ::Vector3(1.0f));
            });
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
    }

    TEST_F(OctreeTests, GrowthInOpposingDirectionsDoesNotOmitEntries)
    {
        VisibilityEntry positiveEntry;
        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(1.1f), AZ::Vector3(1.2f));
        m_octreeScene->InsertOrUpdateEntry(positiveEntry);

        VisibilityEntry negativeEntry;
        negativeEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-2.0f), AZ::Vector3(-1.9f));
        m_octreeScene->InsertOrUpdateEntry(negativeEntry);

        const AZStd::vector<VisibilityEntry*> negativeEntries = GatherEntries(m_octreeScene, negativeEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(negativeEntries.begin(), negativeEntries.end(), &negativeEntry), negativeEntries.end());
        const AZStd::vector<VisibilityEntry*> positiveEntries = GatherEntries(m_octreeScene, positiveEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(positiveEntries.begin(), positiveEntries.end(), &positiveEntry), positiveEntries.end());
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
    }

    TEST_F(OctreeTests, NumericalGrowthFailurePreservesSplitTreeAndUnregistersUpdates)
    {
        const bool modes[] = { false, true };
        for (bool useQuadtree : modes)
        {
            SCOPED_TRACE(useQuadtree);
            SetUseQuadtree(useQuadtree);
            RecreateOctreeScene();
            VisibilityEntry localEntry;
            localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
            VisibilityEntry movingEntry;
            movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
            m_octreeScene->InsertOrUpdateEntry(localEntry);
            m_octreeScene->InsertOrUpdateEntry(movingEntry);
            VisibilityEntry spanningEntry;
            spanningEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.1f), AZ::Vector3(0.1f));
            m_octreeScene->InsertOrUpdateEntry(spanningEntry);
            const uint32_t originalNodeCount = m_octreeScene->GetNodeCount();
            AZStd::vector<AZ::Aabb> originalBounds;
            m_octreeScene->EnumerateNoCull([&originalBounds](const IVisibilityScene::NodeData& nodeData)
                {
                    originalBounds.push_back(nodeData.m_bounds);
                });

            const float maxCoordinate = std::numeric_limits<float>::max();
            VisibilityEntry unsupportedEntry;
            unsupportedEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(
                AZ::Vector3(maxCoordinate * 0.75f, maxCoordinate * 0.75f, 500.0f), AZ::Vector3(maxCoordinate, maxCoordinate, 600.0f));
            m_octreeScene->InsertOrUpdateEntry(unsupportedEntry);
            EXPECT_EQ(unsupportedEntry.m_internalNode, nullptr);
            EXPECT_EQ(m_octreeScene->GetNodeCount(), originalNodeCount);
            AZStd::vector<AZ::Aabb> currentBounds;
            m_octreeScene->EnumerateNoCull([&currentBounds](const IVisibilityScene::NodeData& nodeData)
                {
                    currentBounds.push_back(nodeData.m_bounds);
                });
            EXPECT_EQ(currentBounds, originalBounds);
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);

            movingEntry.m_boundingVolume = unsupportedEntry.m_boundingVolume;
            m_octreeScene->InsertOrUpdateEntry(movingEntry);
            EXPECT_EQ(movingEntry.m_internalNode, nullptr);
            EXPECT_EQ(movingEntry.m_internalNodeIndex, 0);
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
            movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f), AZ::Vector3(3.2f));
            m_octreeScene->InsertOrUpdateEntry(movingEntry);
            const auto foundEntries = GatherEntries(m_octreeScene, movingEntry.m_boundingVolume);
            EXPECT_NE(AZStd::find(foundEntries.begin(), foundEntries.end(), &movingEntry), foundEntries.end());
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 3);
            m_octreeScene->RemoveEntry(movingEntry);
            m_octreeScene->RemoveEntry(localEntry);
            m_octreeScene->RemoveEntry(spanningEntry);
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        }
    }

    TEST_F(OctreeTests, GrowthRejectsBoundsThatWouldOverflowChildCenters)
    {
        const bool modes[] = { false, true };
        for (bool useQuadtree : modes)
        {
            SCOPED_TRACE(useQuadtree);
            SetUseQuadtree(useQuadtree);
            m_console->PerformCommand("bg_octreeMaxWorldExtents 0.1");
            RecreateOctreeScene();
            VisibilityEntry localEntry;
            localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.09f), AZ::Vector3(-0.06f));
            VisibilityEntry farEntry;
            farEntry.m_boundingVolume = AZ::Aabb::CreateFromPoint(AZ::Vector3(1.0e38f));
            m_octreeScene->InsertOrUpdateEntry(localEntry);
            m_octreeScene->InsertOrUpdateEntry(farEntry);
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
            const uint32_t originalNodeCount = m_octreeScene->GetNodeCount();
            AZStd::vector<AZ::Aabb> originalBounds;
            m_octreeScene->EnumerateNoCull([&originalBounds](const IVisibilityScene::NodeData& nodeData)
                {
                    originalBounds.push_back(nodeData.m_bounds);
                });

            VisibilityEntry unsupportedEntry;
            unsupportedEntry.m_boundingVolume = AZ::Aabb::CreateFromPoint(AZ::Vector3(1.6e38f));
            EXPECT_TRUE(unsupportedEntry.m_boundingVolume.GetCenter().IsFinite());
            m_octreeScene->InsertOrUpdateEntry(unsupportedEntry);
            EXPECT_EQ(unsupportedEntry.m_internalNode, nullptr);
            EXPECT_EQ(m_octreeScene->GetNodeCount(), originalNodeCount);
            AZStd::vector<AZ::Aabb> currentBounds;
            m_octreeScene->EnumerateNoCull([&currentBounds](const IVisibilityScene::NodeData& nodeData)
                {
                    currentBounds.push_back(nodeData.m_bounds);
                });
            EXPECT_EQ(currentBounds, originalBounds);
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
            for (VisibilityEntry* entry : { &localEntry, &farEntry })
            {
                const auto foundEntries = GatherEntries(m_octreeScene, entry->m_boundingVolume);
                EXPECT_NE(AZStd::find(foundEntries.begin(), foundEntries.end(), entry), foundEntries.end());
                m_octreeScene->RemoveEntry(*entry);
            }
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        }
    }

    TEST_F(OctreeTests, QuadtreeRejectsZBoundsThatOverflowCenters)
    {
        SetUseQuadtree(true);
        RecreateOctreeScene();
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry movingEntry;
        movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        const uint32_t originalNodeCount = m_octreeScene->GetNodeCount();
        AZStd::vector<AZ::Aabb> originalBounds;
        m_octreeScene->EnumerateNoCull([&originalBounds](const IVisibilityScene::NodeData& nodeData)
            {
                originalBounds.push_back(nodeData.m_bounds);
            });

        VisibilityEntry unsupportedEntry;
        unsupportedEntry.m_boundingVolume = AZ::Aabb::CreateFromPoint(AZ::Vector3(0.75f, 0.75f, 2.0e38f));
        m_octreeScene->InsertOrUpdateEntry(unsupportedEntry);
        EXPECT_EQ(unsupportedEntry.m_internalNode, nullptr);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), originalNodeCount);
        AZStd::vector<AZ::Aabb> currentBounds;
        m_octreeScene->EnumerateNoCull([&currentBounds](const IVisibilityScene::NodeData& nodeData)
            {
                currentBounds.push_back(nodeData.m_bounds);
            });
        EXPECT_EQ(currentBounds, originalBounds);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);

        movingEntry.m_boundingVolume = unsupportedEntry.m_boundingVolume;
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        EXPECT_EQ(movingEntry.m_internalNode, nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        const auto foundEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(foundEntries.begin(), foundEntries.end(), &localEntry), foundEntries.end());
        m_octreeScene->RemoveEntry(localEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
    }

    TEST_F(OctreeTests, GrowthPreservesExistingNodeContainment)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry outOfRangeEntry;
        outOfRangeEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f), AZ::Vector3(3.2f));
        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(outOfRangeEntry);

        const AZStd::vector<VisibilityEntry*> gatheredEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);

        EXPECT_NE(AZStd::find(gatheredEntries.begin(), gatheredEntries.end(), &localEntry), gatheredEntries.end());
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
    }

    TEST_F(OctreeTests, InitialExtentChangesAffectNewScenesOnly)
    {
        m_console->PerformCommand("bg_octreeMaxWorldExtents 65536");

        VisibilityEntry farEntry;
        farEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f), AZ::Vector3(3.2f));
        m_octreeScene->InsertOrUpdateEntry(farEntry);

        const AZStd::vector<VisibilityEntry*> farEntries = GatherEntries(m_octreeScene, farEntry.m_boundingVolume);

        ASSERT_EQ(farEntries.size(), 1);
        EXPECT_EQ(farEntries[0], &farEntry);
        m_octreeScene->EnumerateNoCull([](const IVisibilityScene::NodeData& nodeData)
            {
                EXPECT_EQ(nodeData.m_bounds.GetExtents(), AZ::Vector3(8.0f));
            });
        m_octreeScene->RemoveEntry(farEntry);
        RecreateOctreeScene();
        m_octreeScene->InsertOrUpdateEntry(farEntry);
        m_octreeScene->EnumerateNoCull([](const IVisibilityScene::NodeData& nodeData)
            {
                EXPECT_EQ(nodeData.m_bounds.GetExtents(), AZ::Vector3(131072.0f));
            });
    }

    TEST_F(OctreeTests, GrowthSupportsLargeFiniteCoordinates)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry farEntry;
        farEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(1.0e25f), AZ::Vector3(1.001e25f));
        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(farEntry);

        const AZStd::vector<VisibilityEntry*> localEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);
        const AZStd::vector<VisibilityEntry*> farEntries = GatherEntries(m_octreeScene, farEntry.m_boundingVolume);

        ASSERT_EQ(localEntries.size(), 1);
        ASSERT_EQ(farEntries.size(), 1);
        EXPECT_EQ(localEntries[0], &localEntry);
        EXPECT_EQ(farEntries[0], &farEntry);
    }

    TEST_F(OctreeTests, InvalidInitialExtentUsesDefault)
    {
        const char* invalidValues[] = { "nan", "inf", "-1", "0", "3.4028235e38" };
        for (const char* value : invalidValues)
        {
            SCOPED_TRACE(value);
            m_console->PerformCommand(AZStd::string::format("bg_octreeMaxWorldExtents %s", value).c_str());
            RecreateOctreeScene();
            VisibilityEntry entry;
            entry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f), AZ::Vector3(3.2f));
            m_octreeScene->InsertOrUpdateEntry(entry);
            m_octreeScene->EnumerateNoCull([](const IVisibilityScene::NodeData& nodeData)
                {
                    EXPECT_EQ(nodeData.m_bounds.GetMin(), AZ::Vector3(-16384.0f));
                    EXPECT_EQ(nodeData.m_bounds.GetMax(), AZ::Vector3(16384.0f));
                });
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
            m_octreeScene->RemoveEntry(entry);
        }
    }

    TEST_F(OctreeTests, NonFiniteEntryBoundsDoNotChangeRootBounds)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        m_octreeScene->InsertOrUpdateEntry(localEntry);

        VisibilityEntry nonFiniteEntry;
        nonFiniteEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(2.0f), AZ::Vector3(std::numeric_limits<float>::infinity()));
        m_octreeScene->InsertOrUpdateEntry(nonFiniteEntry);

        const AZ::Aabb queryOutsideInitialRoot = AZ::Aabb::CreateFromMinMax(AZ::Vector3(2.5f), AZ::Vector3(2.6f));
        EXPECT_TRUE(GatherEntries(m_octreeScene, queryOutsideInitialRoot).empty());
        const AZStd::vector<VisibilityEntry*> localEntries = GatherEntries(m_octreeScene, localEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(localEntries.begin(), localEntries.end(), &localEntry), localEntries.end());
        EXPECT_EQ(nonFiniteEntry.m_internalNode, nullptr);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
    }

    TEST_F(OctreeTests, InvalidNewBoundsAreRejected)
    {
        VisibilityEntry entry;
        entry.m_boundingVolume = AZ::Aabb::CreateNull();
        m_octreeScene->InsertOrUpdateEntry(entry);
        EXPECT_EQ(entry.m_internalNode, nullptr);
        EXPECT_EQ(entry.m_internalNodeIndex, 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        m_octreeScene->RemoveEntry(entry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
    }

    TEST_F(OctreeTests, RejectedUpdateUnregistersEntryAndAllowsReinsertion)
    {
        VisibilityEntry localEntry;
        localEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry movingEntry;
        movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
        m_octreeScene->InsertOrUpdateEntry(localEntry);
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        movingEntry.m_boundingVolume = AZ::Aabb::CreateNull();
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        EXPECT_EQ(movingEntry.m_internalNode, nullptr);
        EXPECT_EQ(movingEntry.m_internalNodeIndex, 0);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        m_octreeScene->RemoveEntry(movingEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 1);
        movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(3.0f), AZ::Vector3(3.2f));
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
        const auto foundEntries = GatherEntries(m_octreeScene, movingEntry.m_boundingVolume);
        EXPECT_NE(AZStd::find(foundEntries.begin(), foundEntries.end(), &movingEntry), foundEntries.end());
        m_octreeScene->RemoveEntry(movingEntry);
        m_octreeScene->RemoveEntry(localEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
    }

    TEST_F(OctreeTests, RepeatedPointBoundsTerminateAndRemainQueryable)
    {
        const bool modes[] = { false, true };
        const float positions[] = { 1.0f, 35423.84375f };
        for (bool useQuadtree : modes)
        {
            SetUseQuadtree(useQuadtree);
            m_console->PerformCommand("bg_octreeNodeMaxEntries 64");
            m_console->PerformCommand("bg_octreeNodeMinEntries 32");
            for (float position : positions)
            {
                SCOPED_TRACE(position);
                RecreateOctreeScene();
                AZStd::array<VisibilityEntry, 65> entries;
                const AZ::Aabb point = AZ::Aabb::CreateFromPoint(AZ::Vector3(position));
                for (auto& entry : entries)
                {
                    entry.m_boundingVolume = point;
                    m_octreeScene->InsertOrUpdateEntry(entry);
                }
                ValidateEntryCountEqualsExpectedCount(m_octreeScene, aznumeric_cast<uint32_t>(entries.size()));
                EXPECT_EQ(GatherEntries(m_octreeScene, point).size(), entries.size());
                for (auto& entry : entries)
                {
                    m_octreeScene->RemoveEntry(entry);
                }
                ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
                EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
            }
        }
    }

    TEST_F(OctreeTests, ThinBoundsRemainQueryableAtFloatPrecisionLimit)
    {
        m_console->PerformCommand("bg_octreeNodeMaxEntries 64");
        m_console->PerformCommand("bg_octreeNodeMinEntries 32");
        constexpr float position = 35423.84375f;
        const AZ::Aabb bounds = AZ::Aabb::CreateFromMinMax(AZ::Vector3(position), AZ::Vector3(position + 0.00390625f));
        AZStd::array<VisibilityEntry, 65> entries;
        for (auto& entry : entries)
        {
            entry.m_boundingVolume = bounds;
            m_octreeScene->InsertOrUpdateEntry(entry);
        }
        EXPECT_EQ(GatherEntries(m_octreeScene, bounds).size(), entries.size());
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, aznumeric_cast<uint32_t>(entries.size()));
        for (auto& entry : entries)
        {
            m_octreeScene->RemoveEntry(entry);
        }
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
    }

    TEST_F(OctreeTests, QuadtreeZOnlyUpdatesPreserveXyPartitionAndMerge)
    {
        SetUseQuadtree(true);
        RecreateOctreeScene();
        VisibilityEntry movingEntry;
        movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-0.9f), AZ::Vector3(-0.6f));
        VisibilityEntry positiveEntry;
        positiveEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(AZ::Vector3(0.6f), AZ::Vector3(0.9f));
        m_octreeScene->InsertOrUpdateEntry(movingEntry);
        m_octreeScene->InsertOrUpdateEntry(positiveEntry);
        const float heights[] = { 6.0f, -60000.0f, 30.0f };
        for (float height : heights)
        {
            movingEntry.m_boundingVolume = AZ::Aabb::CreateFromMinMax(
                AZ::Vector3(-0.9f, -0.9f, height), AZ::Vector3(-0.6f, -0.6f, height + 0.25f));
            m_octreeScene->InsertOrUpdateEntry(movingEntry);
            const AZ::Aabb unrelatedQuery = AZ::Aabb::CreateFromMinMax(
                AZ::Vector3(0.6f, 0.6f, height), AZ::Vector3(0.9f, 0.9f, height + 0.25f));
            const auto unrelatedEntries = GatherEntries(m_octreeScene, unrelatedQuery);
            EXPECT_EQ(AZStd::find(unrelatedEntries.begin(), unrelatedEntries.end(), &movingEntry), unrelatedEntries.end());
            const auto foundEntries = GatherEntries(m_octreeScene, movingEntry.m_boundingVolume);
            EXPECT_NE(AZStd::find(foundEntries.begin(), foundEntries.end(), &movingEntry), foundEntries.end());
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 2);
        }
        m_octreeScene->RemoveEntry(positiveEntry);
        m_octreeScene->RemoveEntry(movingEntry);
        ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
        EXPECT_EQ(m_octreeScene->GetNodeCount(), 1);
    }

    TEST_F(OctreeTests, AlternatingGrowthMaintainsReturnedNodeBounds)
    {
        const bool modes[] = { false, true };
        for (bool useQuadtree : modes)
        {
            SetUseQuadtree(useQuadtree);
            RecreateOctreeScene();
            AZStd::array<VisibilityEntry, 8> entries;
            for (size_t index = 0; index < entries.size(); ++index)
            {
                float position = static_cast<float>(index + 1) * 10000.0f;
                if (index % 2)
                {
                    position = -position;
                }
                entries[index].m_boundingVolume = AZ::Aabb::CreateFromMinMax(
                    AZ::Vector3(position, -position, position), AZ::Vector3(position + 1.0f, -position + 1.0f, position + 1.0f));
                m_octreeScene->InsertOrUpdateEntry(entries[index]);
                ValidateEntryCountEqualsExpectedCount(m_octreeScene, static_cast<uint32_t>(index + 1));
            }
            for (auto& entry : entries)
            {
                const auto foundEntries = GatherEntries(m_octreeScene, entry.m_boundingVolume);
                EXPECT_NE(AZStd::find(foundEntries.begin(), foundEntries.end(), &entry), foundEntries.end());
                m_octreeScene->RemoveEntry(entry);
            }
            ValidateEntryCountEqualsExpectedCount(m_octreeScene, 0);
            for (const auto& entry : entries)
            {
                EXPECT_EQ(entry.m_internalNode, nullptr);
                EXPECT_EQ(entry.m_internalNodeIndex, 0);
                EXPECT_TRUE(GatherEntries(m_octreeScene, entry.m_boundingVolume).empty());
            }
        }
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
