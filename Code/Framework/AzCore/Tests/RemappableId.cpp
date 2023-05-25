/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Serialization/IdUtils.h>

namespace UnitTest
{
    using namespace AZ;

    //! Unit Test for IdUtils GenerateNewIdsAndFixRefs functions for generating new unique ids
    class RemappableIdTest
        : public LeakDetectionFixture
    {
    public:
        RemappableIdTest()
            : LeakDetectionFixture()
        {
        }

        ~RemappableIdTest() override
        {
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();
            m_serializeContext.reset(aznew AZ::SerializeContext());

            Entity::Reflect(m_serializeContext.get());
            RemapIdData::Reflect(*m_serializeContext);
            RemapUuidAndEntityIdData::Reflect(*m_serializeContext);
            ParentEntityContainer::Reflect(*m_serializeContext);
            RootParentElementWrapper::Reflect(*m_serializeContext);
        }

        void TearDown() override
        {
            m_serializeContext.reset();

            LeakDetectionFixture::TearDown();
        }

        AZStd::unique_ptr<SerializeContext> m_serializeContext;

        struct RemapIdData
        {
            AZ_TYPE_INFO(RemapIdData, "{0431A85A-C2CC-453A-92A7-807BCEAB46D7}");
            AZ_CLASS_ALLOCATOR(RemapIdData, AZ::SystemAllocator);

            static void Reflect(AZ::SerializeContext& serializeContext)
            {
                serializeContext.Class<RemapIdData>()
                    ->Field("m_remappableUuid", &RemapIdData::m_remappableUuid)
                        ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Uuid::CreateRandom)
                    ->Field("m_uuid1", &RemapIdData::m_uuid1)
                    ->Field("m_uuid2", &RemapIdData::m_uuid2)
                    ;
            }

            Uuid m_remappableUuid;
            Uuid m_uuid1;
            Uuid m_uuid2;
        };

        struct RemapUuidAndEntityIdData
        {
            AZ_TYPE_INFO(RemapUuidAndEntityIdData, "{6951A116-505C-47E0-9310-2E60D040F390}");
            AZ_CLASS_ALLOCATOR(RemapUuidAndEntityIdData, AZ::SystemAllocator);

            // copying entities is forbidden.
            RemapUuidAndEntityIdData()
            {
            }

            RemapUuidAndEntityIdData(const RemapUuidAndEntityIdData& other)
            {
                m_remappableUuid = other.m_remappableUuid;
                m_uuid1 = other.m_uuid1;
                m_uuid2 = other.m_uuid2;
                m_remappableEntityId = other.m_remappableEntityId;
                m_entityId1 = other.m_entityId1;
                m_entityId2 = other.m_entityId2;
                m_entity.SetId(other.m_entity.GetId()); // for this test, we only copy the ID.
                m_entityRef = other.m_entityRef;
            }

            static void Reflect(AZ::SerializeContext& serializeContext)
            {
                serializeContext.Class<RemapUuidAndEntityIdData>()
                    ->Field("m_remappableUuid", &RemapUuidAndEntityIdData::m_remappableUuid)
                        ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Uuid::CreateRandom)
                    ->Field("m_uuid1", &RemapUuidAndEntityIdData::m_uuid1)
                    ->Field("m_uuid2", &RemapUuidAndEntityIdData::m_uuid2)
                    ->Field("m_remappbleEntityId", &RemapUuidAndEntityIdData::m_remappableEntityId)
                        ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Entity::MakeId)
                    ->Field("m_entityId1", &RemapUuidAndEntityIdData::m_entityId1)
                    ->Field("m_entityId2", &RemapUuidAndEntityIdData::m_entityId2)
                    ->Field("m_entity", &RemapUuidAndEntityIdData::m_entity)
                    ->Field("m_entityRef", &RemapUuidAndEntityIdData::m_entityRef)
                    ;
            }

            Uuid m_remappableUuid;
            Uuid m_uuid1;
            Uuid m_uuid2;
            AZ::EntityId m_remappableEntityId;
            AZ::EntityId m_entityId1;
            AZ::EntityId m_entityId2;
            AZ::Entity m_entity;
            AZ::EntityId m_entityRef;
        };

        struct ParentEntityContainer
        {
            AZ_TYPE_INFO(ParentEntityContainer, "{60EFD610-4B9A-444C-9004-A651B772927F}");
            AZ_CLASS_ALLOCATOR(ParentEntityContainer, AZ::SystemAllocator);

            static void Reflect(AZ::SerializeContext& serializeContext);

            AZ::EntityId m_remappableEntityId1;
            AZ::EntityId m_remappableEntityId2;

            int m_beginWriteCount = 0;
            int m_endWriteCount = 0;
        };

        struct RootParentElementWrapper
        {
            AZ_TYPE_INFO(RootParentElementWrapper, "{BFC46F3C-D696-4A34-B824-F18428CAA910}");
            AZ_CLASS_ALLOCATOR(RootParentElementWrapper, AZ::SystemAllocator);

            static void Reflect(AZ::SerializeContext& serializeContext)
            {
                serializeContext.Class<RootParentElementWrapper>()
                    ->Field("m_parentEntityContainer", &RootParentElementWrapper::m_parentEntityContainer)
                    ;
            }

            ~RootParentElementWrapper()
            {
                delete m_parentEntityContainer;
            }

            ParentEntityContainer* m_parentEntityContainer = nullptr;
        };

        struct ParentEntityContainerEventHandler
            : public AZ::SerializeContext::IEventHandler
        {
            static bool s_isCheckingCallbackOrder;

            void OnWriteBegin(void* classPtr) override
            {
                auto parentEntityContainer = reinterpret_cast<ParentEntityContainer*>(classPtr);

                if (s_isCheckingCallbackOrder)
                {
                    EXPECT_EQ(parentEntityContainer->m_beginWriteCount, parentEntityContainer->m_endWriteCount);
                }

                parentEntityContainer->m_beginWriteCount++;
            }

            void OnWriteEnd(void* classPtr) override
            {
                auto parentEntityContainer = reinterpret_cast<ParentEntityContainer*>(classPtr);

                if (s_isCheckingCallbackOrder)
                {
                    EXPECT_EQ(parentEntityContainer->m_beginWriteCount, parentEntityContainer->m_endWriteCount + 1);
                }

                parentEntityContainer->m_endWriteCount++;
            }
        };

    };

    bool RemappableIdTest::ParentEntityContainerEventHandler::s_isCheckingCallbackOrder = false;

    void RemappableIdTest::ParentEntityContainer::Reflect(AZ::SerializeContext& serializeContext)
    {
        serializeContext.Class<ParentEntityContainer>()
            ->EventHandler<ParentEntityContainerEventHandler>()
            ->Field("m_remappableEntityId1", &ParentEntityContainer::m_remappableEntityId1)
                ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Entity::MakeId)
            ->Field("m_remappableEntityId2", &ParentEntityContainer::m_remappableEntityId2)
                ->Attribute(AZ::Edit::Attributes::IdGeneratorFunction, &AZ::Entity::MakeId)
            ;
    }

    TEST_F(RemappableIdTest, UuidRemapTest)
    {
        RemapIdData testData;
        testData.m_remappableUuid = AZ::Uuid::CreateRandom();
        testData.m_uuid1 = testData.m_remappableUuid;
        testData.m_uuid2 = AZ::Uuid::CreateRandom();
        EXPECT_EQ(testData.m_remappableUuid, testData.m_uuid1);
        EXPECT_NE(testData.m_remappableUuid, testData.m_uuid2);

        RemapIdData originalData = testData;
        EXPECT_EQ(originalData.m_remappableUuid, testData.m_remappableUuid);
        EXPECT_EQ(originalData.m_uuid1, testData.m_uuid1);
        EXPECT_EQ(originalData.m_uuid2, testData.m_uuid2);

        AZStd::unordered_map<Uuid, Uuid> remapIds;
        IdUtils::Remapper<Uuid>::GenerateNewIdsAndFixRefs(&testData, remapIds, m_serializeContext.get());
        EXPECT_NE(originalData.m_remappableUuid, testData.m_remappableUuid);
        EXPECT_NE(originalData.m_uuid1, testData.m_uuid1);
        // Uuid2 never gets remapped so it should remain the same throughout the test
        EXPECT_EQ(originalData.m_uuid2, testData.m_uuid2);

        // Uuid1 has the same value as the RemappableUuid so it gets updated as well
        EXPECT_EQ(testData.m_remappableUuid, testData.m_uuid1);
        auto remapIdsIt = remapIds.find(originalData.m_remappableUuid);
        EXPECT_NE(remapIds.end(), remapIdsIt);
        EXPECT_EQ(testData.m_remappableUuid, remapIdsIt->second);
    }

    TEST_F(RemappableIdTest, UuidAndEntityIdRemapTest)
    {
        RemapUuidAndEntityIdData testData;
        testData.m_remappableUuid = AZ::Uuid::CreateRandom();
        testData.m_uuid1 = testData.m_remappableUuid;
        testData.m_uuid2 = AZ::Uuid::CreateRandom();
        testData.m_remappableEntityId = AZ::Entity::MakeId();
        testData.m_entityId1 = testData.m_remappableEntityId;
        testData.m_entityId2 = AZ::Entity::MakeId();
        testData.m_entityRef = testData.m_entity.GetId();

        //Uuid Compare
        EXPECT_EQ(testData.m_remappableUuid, testData.m_uuid1);
        EXPECT_NE(testData.m_remappableUuid, testData.m_uuid2);
        // EntityId Compare
        EXPECT_EQ(testData.m_remappableEntityId, testData.m_entityId1);
        EXPECT_NE(testData.m_remappableEntityId, testData.m_entityId2);

        RemapUuidAndEntityIdData originalData = testData;
        EXPECT_EQ(originalData.m_remappableUuid, testData.m_remappableUuid);
        EXPECT_EQ(originalData.m_uuid1, testData.m_uuid1);
        EXPECT_EQ(originalData.m_uuid2, testData.m_uuid2);
        EXPECT_EQ(originalData.m_remappableEntityId, testData.m_remappableEntityId);
        EXPECT_EQ(originalData.m_entityId1, testData.m_entityId1);
        EXPECT_EQ(originalData.m_entityId2, testData.m_entityId2);
        EXPECT_EQ(originalData.m_entity.GetId(), testData.m_entity.GetId());
        EXPECT_EQ(originalData.m_entityRef, testData.m_entityRef);

        // Remap Uuids
        AZStd::unordered_map<Uuid, Uuid> remapUuids;
        IdUtils::Remapper<Uuid>::GenerateNewIdsAndFixRefs(&testData, remapUuids, m_serializeContext.get());
        
        // Remap EntityIds
        AZStd::unordered_map<EntityId, EntityId> remapEntityIds;
        IdUtils::Remapper<EntityId>::GenerateNewIdsAndFixRefs(&testData, remapEntityIds, m_serializeContext.get());

        // Remapped Uuid Compare
        // Uuid1 has the same value as the RemappableUuid so it gets updated as well
        EXPECT_NE(originalData.m_remappableUuid, testData.m_remappableUuid);
        EXPECT_NE(originalData.m_uuid1, testData.m_uuid1);
        EXPECT_EQ(testData.m_remappableUuid, testData.m_uuid1);
        // Uuid2 never gets remapped so it should remain the same throughout the test
        EXPECT_EQ(originalData.m_uuid2, testData.m_uuid2);

        // Remapped EntityId Compare
        // EntityId1 has the same value as the RemappableEntityId so it gets updated as well
        EXPECT_NE(originalData.m_remappableEntityId, testData.m_remappableEntityId);
        EXPECT_NE(originalData.m_entityId1, testData.m_entityId1);
        EXPECT_EQ(testData.m_remappableEntityId, testData.m_entityId1);
        
        EXPECT_NE(originalData.m_entity.GetId(), testData.m_entity.GetId());
        EXPECT_NE(originalData.m_entityRef, testData.m_entityRef);
        EXPECT_EQ(testData.m_entity.GetId(), testData.m_entityRef);
        // EntityId does not match the RemappableEntityId so it does not get remapped
        EXPECT_EQ(originalData.m_entityId2, testData.m_entityId2);

        auto remapUuidIt = remapUuids.find(originalData.m_remappableUuid);
        EXPECT_NE(remapUuids.end(), remapUuidIt);
        EXPECT_EQ(testData.m_remappableUuid, remapUuidIt->second);

        auto remapEntityIdIt = remapEntityIds.find(originalData.m_remappableEntityId);
        EXPECT_NE(remapEntityIds.end(), remapEntityIdIt);
        EXPECT_EQ(testData.m_remappableEntityId, remapEntityIdIt->second);
    }

    TEST_F(RemappableIdTest, CloneObjectAndRemapUuidAndEntityIdTest)
    {
        RemapUuidAndEntityIdData testData;
        testData.m_remappableUuid = AZ::Uuid::CreateRandom();
        testData.m_uuid1 = testData.m_remappableUuid;
        testData.m_uuid2 = AZ::Uuid::CreateRandom();
        testData.m_remappableEntityId = AZ::Entity::MakeId();
        testData.m_entityId1 = testData.m_remappableEntityId;
        testData.m_entityId2 = AZ::Entity::MakeId();
        testData.m_entityRef = testData.m_entity.GetId();

        //Uuid Compare
        EXPECT_EQ(testData.m_remappableUuid, testData.m_uuid1);
        EXPECT_NE(testData.m_remappableUuid, testData.m_uuid2);
        // EntityId Compare
        EXPECT_EQ(testData.m_remappableEntityId, testData.m_entityId1);
        EXPECT_NE(testData.m_remappableEntityId, testData.m_entityId2);

        // Clone Object and Remap Uuids
        AZStd::unordered_map<Uuid, Uuid> remapUuids;
        RemapUuidAndEntityIdData* cloneData = IdUtils::Remapper<Uuid>::CloneObjectAndGenerateNewIdsAndFixRefs(&testData, remapUuids, m_serializeContext.get());
        // Remap EntityIds
        AZStd::unordered_map<EntityId, EntityId> remapEntityIds;
        IdUtils::Remapper<EntityId>::GenerateNewIdsAndFixRefs(cloneData, remapEntityIds, m_serializeContext.get());

        // Remapped Uuid Compare
        // Uuid1 has the same value as the RemappableUuid so it gets updated as well
        EXPECT_NE(testData.m_remappableUuid, cloneData->m_remappableUuid);
        EXPECT_NE(testData.m_uuid1, cloneData->m_uuid1);
        EXPECT_EQ(cloneData->m_remappableUuid, cloneData->m_uuid1);
        // Uuid2 never gets remapped so it should remain the same throughout the test
        EXPECT_EQ(testData.m_uuid2, cloneData->m_uuid2);

        // Remapped EntityId Compare
        // EntityId1 has the same value as the RemappableEntityId so it gets updated as well
        EXPECT_NE(testData.m_remappableEntityId, cloneData->m_remappableEntityId);
        EXPECT_NE(testData.m_entityId1, cloneData->m_entityId1);
        EXPECT_EQ(cloneData->m_remappableEntityId, cloneData->m_entityId1);

        EXPECT_NE(testData.m_entity.GetId(), cloneData->m_entity.GetId());
        EXPECT_NE(testData.m_entityRef, cloneData->m_entityRef);
        EXPECT_EQ(cloneData->m_entity.GetId(), cloneData->m_entityRef);
        // EntityId does not match the RemappableEntityId so it does not get remapped
        EXPECT_EQ(testData.m_entityId2, cloneData->m_entityId2);

        auto remapUuidIt = remapUuids.find(testData.m_remappableUuid);
        EXPECT_NE(remapUuids.end(), remapUuidIt);
        EXPECT_EQ(cloneData->m_remappableUuid, remapUuidIt->second);

        auto remapEntityIdIt = remapEntityIds.find(testData.m_remappableEntityId);
        EXPECT_NE(remapEntityIds.end(), remapEntityIdIt);
        EXPECT_EQ(cloneData->m_remappableEntityId, remapEntityIdIt->second);

        delete cloneData;
    }

    // Test that the IdUtils::RemapIds function does not crash when invoking the IEventHandler function
    // on a pointer to a class element(i.e ClassType**)
    TEST_F(RemappableIdTest, OnWriteCallbacks_WhenParentHasPointer_DoesNotCrash)
    {
        RootParentElementWrapper rootWrapper;
        rootWrapper.m_parentEntityContainer = aznew ParentEntityContainer;

        auto entityIdMapper = [](const AZ::EntityId&, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator& idGenerator)
        {
            return idGenerator();
        };

        AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&rootWrapper, entityIdMapper, m_serializeContext.get(), true);
        EXPECT_NE(0, rootWrapper.m_parentEntityContainer->m_beginWriteCount); // check that callbacks were invoked at all
    }

    // Test that each OnWriteBegin is paired with an OnWriteEnd
    // and that we don't get two OnWriteBegins in a row
    TEST_F(RemappableIdTest, OnWriteCallbacks_CalledInCorrectOrder)
    {
        ParentEntityContainerEventHandler::s_isCheckingCallbackOrder = true;

        RootParentElementWrapper rootWrapper;
        rootWrapper.m_parentEntityContainer = aznew ParentEntityContainer;

        auto entityIdMapper = [](const AZ::EntityId&, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator& idGenerator)
        {
            return idGenerator();
        };

        AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&rootWrapper, entityIdMapper, m_serializeContext.get(), true);

        EXPECT_EQ(rootWrapper.m_parentEntityContainer->m_beginWriteCount, rootWrapper.m_parentEntityContainer->m_endWriteCount);

        ParentEntityContainerEventHandler::s_isCheckingCallbackOrder = false;
    }

    // Test that we only call OnWriteBegin once per node, even if multiple IDs on child nodes are remapped
    TEST_F(RemappableIdTest, OnWriteCallbacks_WhenMultipleChildrenChanged_CalledOnceOnParent)
    {
        RootParentElementWrapper rootWrapper;
        rootWrapper.m_parentEntityContainer = aznew ParentEntityContainer;

        auto entityIdMapper = [](const AZ::EntityId&, bool, const AZ::IdUtils::Remapper<AZ::EntityId>::IdGenerator& idGenerator)
        {
            return idGenerator();
        };

        unsigned int remappedIdCount = AZ::IdUtils::Remapper<AZ::EntityId>::RemapIds(&rootWrapper, entityIdMapper, m_serializeContext.get(), true);

        EXPECT_EQ(2, remappedIdCount);
        EXPECT_EQ(1, rootWrapper.m_parentEntityContainer->m_beginWriteCount);
    }

} // namespace UnitTest
