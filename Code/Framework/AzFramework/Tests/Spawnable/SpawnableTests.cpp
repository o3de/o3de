/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzFramework/Spawnable/Spawnable.h>
#include <AzTest/AzTest.h>

namespace UnitTest
{
    class SpawnableTest : public LeakDetectionFixture
    {
    public:
        static constexpr size_t DefaultEntityAliasTestCount = 8;

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            m_spawnable = aznew AzFramework::Spawnable();
        }

        void TearDown() override
        {
            delete m_spawnable;
            m_spawnable = nullptr;

            LeakDetectionFixture::TearDown();
        }

        void InsertEntities(size_t count)
        {
            AzFramework::Spawnable::EntityList& entities = m_spawnable->GetEntities();
            entities.reserve(entities.size() + count);
            for (size_t i = 0; i < count; ++i)
            {
                entities.emplace_back(AZStd::make_unique<AZ::Entity>());
            }
        }

        template<size_t Count>
        void InsertEntityAliases(
            const AZStd::array<uint32_t, Count>& sourceIds,
            const AZStd::array<uint32_t, Count>& targetIds,
            const AZStd::array<AzFramework::Spawnable::EntityAliasType, Count>& aliasTypes,
            bool queueLoad = false)
        {
            AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();

            for (uint32_t i = 0; i < Count; ++i)
            {
                AZ::Data::Asset<AzFramework::Spawnable> spawnable(
                    AZ::Data::AssetId(AZ::Uuid("{4CBEC17A-52D6-42D5-9037-F4C05B9CE1D9}"), i), azrtti_typeid<AzFramework::Spawnable>());
                visitor.AddAlias(spawnable, AZ::Crc32(i), sourceIds[i], targetIds[i], aliasTypes[i], queueLoad);
            }
        }

        template<size_t Count>
        void InsertEntityAliases(bool queueLoad)
        {
            using namespace AzFramework;

            AZStd::array<uint32_t, Count> ids;
            for (uint32_t i=0; i<aznumeric_cast<uint32_t>(Count); ++i)
            {
                ids[i] = i;
            }

            AZStd::array<AzFramework::Spawnable::EntityAliasType, Count> aliasTypes;
            for (uint32_t i = 0; i < aznumeric_cast<uint32_t>(Count); ++i)
            {
                aliasTypes[i] = Spawnable::EntityAliasType::Replace;
            }

            InsertEntityAliases<Count>(ids, ids, aliasTypes, queueLoad);
        }

        template<size_t Count>
        void InsertEntityAliases()
        {
            InsertEntityAliases<Count>(false);
        }

    protected:
        AzFramework::Spawnable* m_spawnable;
    };


    //
    // TryGetAliasesConst
    //

    TEST_F(SpawnableTest, TryGetAliasesConst_GetVisitor_VisitorDataIsAvailable)
    {
        AzFramework::Spawnable::EntityAliasConstVisitor visitor = m_spawnable->TryGetAliasesConst();
        EXPECT_TRUE(visitor.IsValid());
    }

    TEST_F(SpawnableTest, TryGetAliasesConst_VisitorThatIsNotReadShared_VisitorDataIsNotAvailable)
    {
        AzFramework::Spawnable::EntityAliasVisitor readWriteVisitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(readWriteVisitor.IsValid());

        AzFramework::Spawnable::EntityAliasConstVisitor visitor = m_spawnable->TryGetAliasesConst();
        EXPECT_FALSE(visitor.IsValid());
    }

    TEST_F(SpawnableTest, TryGetAliasesConst_VisitorThatIsAlreadyReadShared_VisitorDataIsAvailable)
    {
        AzFramework::Spawnable::EntityAliasConstVisitor readVisitor = m_spawnable->TryGetAliasesConst();
        ASSERT_TRUE(readVisitor.IsValid());

        AzFramework::Spawnable::EntityAliasConstVisitor visitor = m_spawnable->TryGetAliasesConst();
        EXPECT_TRUE(visitor.IsValid());
    }


    //
    // TryGetAliases
    //

    TEST_F(SpawnableTest, TryGetAliases_GetVisitor_VisitorDataIsAvailable)
    {
        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        EXPECT_TRUE(visitor.IsValid());
    }

    TEST_F(SpawnableTest, TryGetAliasesConst_VisitorThatIsAlreadyShared_VisitorDataNotIsAvailable)
    {
        AzFramework::Spawnable::EntityAliasConstVisitor readVisitor = m_spawnable->TryGetAliasesConst();
        ASSERT_TRUE(readVisitor.IsValid());

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        EXPECT_FALSE(visitor.IsValid());
    }


    //
    // EntityAliasVisitor
    //


    //
    // HasAliases
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_HasAliases_EmptyAliasList_ReturnsFalse)
    {
        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_FALSE(visitor.HasAliases());
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_HasAliases_FilledInAliasList_ReturnsTrue)
    {
        InsertEntities(8);
        InsertEntityAliases<8>();
        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_TRUE(visitor.HasAliases());
    }


    //
    // Optimize
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_Optimize_SortEntityAliases_AliasesAreSortedBySourceAndTargetId)
    {
        InsertEntities(DefaultEntityAliasTestCount);
        InsertEntityAliases<DefaultEntityAliasTestCount>();
        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        // Optimize doesn't need to be explicitly called because the setup of the aliases will cause the alias list to be sorted and optimized.

        uint32_t sourceIndex = 0;
        uint32_t targetIndex = 0;
        for (const AzFramework::Spawnable::EntityAlias& alias : visitor)
        {
            if (alias.m_sourceIndex != sourceIndex)
            {
                ASSERT_LE(sourceIndex, alias.m_sourceIndex);
            }
            else
            {
                ASSERT_LE(targetIndex, alias.m_targetIndex);
            }
            sourceIndex = alias.m_sourceIndex;
            targetIndex = alias.m_targetIndex;
        }
    }

    TEST_F(
        SpawnableTest, EntityAliasVisitor_Optimize_RemoveUnused_OnlySecondToLastAliasRemains)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Disable,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Disable,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Original });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_EQ(1, AZStd::distance(visitor.begin(), visitor.end()));
        EXPECT_EQ(Spawnable::EntityAliasType::Replace, visitor.begin()->m_aliasType);
        EXPECT_EQ(6, visitor.begin()->m_targetIndex);
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_Optimize_AddAdditional_ThreeAdditionalAliasesAreAdded)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 0, 0, 0, 1, 2, 2, 2 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional,
              Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional,
              Spawnable::EntityAliasType::Additional, Spawnable::EntityAliasType::Additional });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_EQ(11, AZStd::distance(visitor.begin(), visitor.end()));
        EXPECT_EQ(Spawnable::EntityAliasType::Original, visitor.begin()->m_aliasType);
        EXPECT_EQ(Spawnable::EntityAliasType::Original, visitor.begin()[5].m_aliasType);
        EXPECT_EQ(Spawnable::EntityAliasType::Original, visitor.begin()[7].m_aliasType);
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_Optimize_OriginalsOnly_AliasListIsEmpty)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 0, 0, 0, 0, 0, 0, 0 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original,
              Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original,
              Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_EQ(0, AZStd::distance(visitor.begin(), visitor.end()));
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_Optimize_MixedOriginals_AllOriginalsRemoved)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 0, 0, 1, 1, 2, 2, 2 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original,
              Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Disable, Spawnable::EntityAliasType::Original,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Original });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_EQ(2, AZStd::distance(visitor.begin(), visitor.end()));
        EXPECT_EQ(Spawnable::EntityAliasType::Disable, visitor.begin()->m_aliasType);
        EXPECT_EQ(Spawnable::EntityAliasType::Replace, visitor.begin()[1].m_aliasType);
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_Optimize_MergeAfterOriginal_NoAdditionalOriginalIsInserted)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 0, 1, 1, 2, 2, 2, 2 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Merge, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Merge, Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original,
              Spawnable::EntityAliasType::Original, Spawnable::EntityAliasType::Original });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_EQ(4, AZStd::distance(visitor.begin(), visitor.end()));
        EXPECT_EQ(Spawnable::EntityAliasType::Original, visitor.begin()->m_aliasType);
        EXPECT_EQ(Spawnable::EntityAliasType::Merge, visitor.begin()[1].m_aliasType);
        EXPECT_EQ(Spawnable::EntityAliasType::Replace, visitor.begin()[2].m_aliasType);
        EXPECT_EQ(Spawnable::EntityAliasType::Merge, visitor.begin()[3].m_aliasType);
    }


    //
    // UpdateAliasType
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_UpdateAliasType_AllToOriginal_NoAliasesAfterOptimization)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 1, 2, 3, 4, 5, 6, 7 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        for (uint32_t i = 0; i < 8; ++i)
        {
            visitor.UpdateAliasType(i, Spawnable::EntityAliasType::Original);
        }

        for (const Spawnable::EntityAlias& alias : visitor)
        {
            EXPECT_EQ(Spawnable::EntityAliasType::Original, alias.m_aliasType);
        }

        visitor.Optimize();

        EXPECT_EQ(0, AZStd::distance(visitor.begin(), visitor.end()));
    }


    //
    // UpdateAliases
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_UpdateAliases_AllToOriginal_NoAliasesAfterOptimization)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 1, 2, 3, 4, 5, 6, 7 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        auto callback =
            [](Spawnable::EntityAliasType& aliasType, bool& /*queueLoad*/, const AZ::Data::Asset<Spawnable>& /*aliasedSpawnable*/,
               const AZ::Crc32 /*tag*/, const uint32_t /*sourceIndex*/, const uint32_t /*targetIndex*/)
            {
                aliasType = Spawnable::EntityAliasType::Original;
            };
        visitor.UpdateAliases(AZStd::move(callback));

        for (const Spawnable::EntityAlias& alias : visitor)
        {
            EXPECT_EQ(Spawnable::EntityAliasType::Original, alias.m_aliasType);
        }

        visitor.Optimize();

        EXPECT_EQ(0, AZStd::distance(visitor.begin(), visitor.end()));
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_UpdateAliases_FilterByTag_OnlyOneAliasUpdated)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(
            { 0, 1, 2, 3, 4, 5, 6, 7 }, { 0, 1, 2, 3, 4, 5, 6, 7 },
            { Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace,
              Spawnable::EntityAliasType::Replace, Spawnable::EntityAliasType::Replace });

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        bool correctTag = false;
        size_t numberOfUpdates = 0;
        auto callback = [&correctTag, &numberOfUpdates](Spawnable::EntityAliasType& aliasType, bool& /*queueLoad*/,
                           const AZ::Data::Asset<Spawnable>& /*aliasedSpawnable*/, const AZ::Crc32 tag, const uint32_t /*sourceIndex*/,
                           const uint32_t /*targetIndex*/)
        {
            correctTag = (tag == AZ::Crc32(3));
            numberOfUpdates++;
            aliasType = Spawnable::EntityAliasType::Original;
        };
        visitor.UpdateAliases(AZ::Crc32(3), AZStd::move(callback));

        EXPECT_EQ(Spawnable::EntityAliasType::Original, visitor.begin()[3].m_aliasType);
    }


    //
    // AreAllSpawnablesReady
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_AreAllSpawnablesReady_CheckFakeLoadedAssets_ReturnsTrue)
    {
        using namespace AzFramework;
        InsertEntities(DefaultEntityAliasTestCount);
        InsertEntityAliases<DefaultEntityAliasTestCount>();

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_TRUE(visitor.AreAllSpawnablesReady());
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_AreAllSpawnablesReady_CheckFakeNotLoadedAssets_ReturnsFalse)
    {
        using namespace AzFramework;
        InsertEntities(8);
        InsertEntityAliases<8>(true);

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        EXPECT_FALSE(visitor.AreAllSpawnablesReady());
    }


    //
    // ListTargetSpawnables
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_ListTargetSpawnables_ListAllTargetAssets_AllTargetsListed)
    {
        using namespace AzFramework;
        InsertEntities(DefaultEntityAliasTestCount);
        InsertEntityAliases<DefaultEntityAliasTestCount>();

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        size_t count = 0;
        bool correctAssets = true;
        auto callback = [&count, &correctAssets](const AZ::Data::Asset<Spawnable>& targetSpawnable)
        {
            correctAssets = correctAssets && (targetSpawnable.GetId().m_subId == count);
            count++;
        };
        visitor.ListTargetSpawnables(callback);

        EXPECT_EQ(8, count);
        EXPECT_TRUE(correctAssets);
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_ListTargetSpawnables_ListTaggedTargetAssets_OneAssetListed)
    {
        using namespace AzFramework;
        InsertEntities(DefaultEntityAliasTestCount);
        InsertEntityAliases<DefaultEntityAliasTestCount>();

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        size_t count = 0;
        bool correctAsset = false;
        auto callback = [&count, &correctAsset](const AZ::Data::Asset<Spawnable>& targetSpawnable)
        {
            correctAsset = (targetSpawnable.GetId().m_subId == 3);
            count++;
        };
        visitor.ListTargetSpawnables(AZ::Crc32(3), callback);

        EXPECT_EQ(1, count);
        EXPECT_TRUE(correctAsset);
    }


    //
    // ListSpawnablesRequiringLoad
    //

    TEST_F(SpawnableTest, EntityAliasVisitor_ListSpawnablesRequiringLoad_AllSetToLoaded_AllTargetsListed)
    {
        using namespace AzFramework;
        InsertEntities(DefaultEntityAliasTestCount);
        InsertEntityAliases<DefaultEntityAliasTestCount>(true);

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        size_t count = 0;
        bool correctAssets = true;
        auto callback = [&count, &correctAssets](const AZ::Data::Asset<Spawnable>& targetSpawnable)
        {
            correctAssets = correctAssets && (targetSpawnable.GetId().m_subId == count);
            count++;
        };
        visitor.ListSpawnablesRequiringLoad(callback);

        EXPECT_EQ(8, count);
        EXPECT_TRUE(correctAssets);
    }

    TEST_F(SpawnableTest, EntityAliasVisitor_ListSpawnablesRequiringLoad_AllSetToNotLoaded_NoTargetsListed)
    {
        using namespace AzFramework;
        InsertEntities(DefaultEntityAliasTestCount);
        InsertEntityAliases<DefaultEntityAliasTestCount>(false);

        AzFramework::Spawnable::EntityAliasVisitor visitor = m_spawnable->TryGetAliases();
        ASSERT_TRUE(visitor.IsValid());

        size_t count = 0;
        auto callback = [&count](const AZ::Data::Asset<Spawnable>& /*targetSpawnable*/)
        {
            count++;
        };
        visitor.ListSpawnablesRequiringLoad(callback);

        EXPECT_EQ(0, count);
    }
} // namespace UnitTest
