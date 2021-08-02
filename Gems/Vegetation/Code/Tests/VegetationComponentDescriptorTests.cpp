/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "VegetationTest.h"
#include "VegetationMocks.h"

#include <AzCore/Component/Entity.h>
#include <AzTest/AzTest.h>
#include <AzCore/UnitTest/TestTypes.h>

#include <Source/Components/DescriptorListCombinerComponent.h>
#include <Source/Components/DescriptorListComponent.h>
#include <Source/Components/DescriptorWeightSelectorComponent.h>

namespace UnitTest
{
    struct VegetationComponentTestsDescriptor
        : public VegetationComponentTests
    {
        void RegisterComponentDescriptors() override
        {
        }

        AZStd::unique_ptr<AZ::Entity> CreateDescriptorProvider(size_t count)
        {
            Vegetation::DescriptorListConfig config;
            for (int i = 0; i < count; ++i)
            {
                Vegetation::Descriptor descriptor;
                config.m_descriptors.emplace_back(descriptor);
            }

            Vegetation::DescriptorListComponent* component = nullptr;
            auto entity = CreateEntity(config, &component);

            return entity;
        }
    };

    TEST_F(VegetationComponentTestsDescriptor, DescriptorListCombinerComponent)
    {
        MockDescriptorBus mockDescriptorBus;

        auto eProvider1 = CreateDescriptorProvider(2);
        auto eProvider2 = CreateDescriptorProvider(40);

        Vegetation::DescriptorListCombinerConfig config;
        config.m_descriptorProviders.push_back(eProvider1->GetId());
        config.m_descriptorProviders.push_back(eProvider2->GetId());

        Vegetation::DescriptorListCombinerComponent* component = nullptr;
        auto entity = CreateEntity(config, &component);

        Vegetation::DescriptorPtrVec descriptorCollection;
        Vegetation::DescriptorProviderRequestBus::Event(entity->GetId(), &Vegetation::DescriptorProviderRequestBus::Events::GetDescriptors, descriptorCollection);

        EXPECT_EQ(42, descriptorCollection.size());
    }

    TEST_F(VegetationComponentTestsDescriptor, DescriptorListComponent_External)
    {
        MockDescriptorBus mockDescriptorBus;

        Vegetation::Descriptor descriptor;
        descriptor.m_weight = 123.0f;

        Vegetation::DescriptorListAsset descriptorListAssetData;
        descriptorListAssetData.m_descriptors.push_back(descriptor);
        reinterpret_cast<MockAssetData*>(&descriptorListAssetData)->SetId(AZ::Uuid::CreateRandom());

        AZ::Data::Asset<Vegetation::DescriptorListAsset> descriptorListAsset(&descriptorListAssetData, AZ::Data::AssetLoadBehavior::Default);

        Vegetation::DescriptorListConfig config;
        config.m_sourceType = Vegetation::DescriptorListSourceType::EXTERNAL;
        config.m_descriptorListAsset = descriptorListAsset;

        Vegetation::DescriptorListComponent* component = nullptr;
        auto entity = CreateEntity(config, &component);

        component->OnAssetReady(descriptorListAsset);

        Vegetation::DescriptorPtrVec descriptorCollection;
        Vegetation::DescriptorProviderRequestBus::Event(entity->GetId(), &Vegetation::DescriptorProviderRequestBus::Events::GetDescriptors, descriptorCollection);

        EXPECT_EQ(1, descriptorCollection.size());

        // clear up memory before leaving
        reinterpret_cast<MockAsset<Vegetation::DescriptorListAsset>&>(descriptorListAsset).ClearData();
        mockDescriptorBus.m_descriptorSet.clear();
    }

    TEST_F(VegetationComponentTestsDescriptor, DescriptorListComponent_Embedded)
    {
        MockDescriptorBus mockDescriptorBus;

        Vegetation::Descriptor descriptor;
        descriptor.m_weight = 123.0f;

        Vegetation::DescriptorListConfig config;
        config.m_sourceType = Vegetation::DescriptorListSourceType::EMBEDDED;
        config.m_descriptors.emplace_back(descriptor);

        Vegetation::DescriptorListComponent* component = nullptr;
        auto entity = CreateEntity(config, &component);

        EXPECT_EQ(1, mockDescriptorBus.m_descriptorSet.size());

        Vegetation::DescriptorPtrVec descriptorCollection;
        Vegetation::DescriptorProviderRequestBus::Event(entity->GetId(), &Vegetation::DescriptorProviderRequestBus::Events::GetDescriptors, descriptorCollection);

        ASSERT_EQ(1, descriptorCollection.size());
        EXPECT_EQ(123.0f, descriptorCollection[0]->m_weight);

        // clear up memory before leaving
        mockDescriptorBus.m_descriptorSet.clear();
    }

    TEST_F(VegetationComponentTestsDescriptor, DescriptorWeightSelectorComponent)
    {
        MockGradientRequestHandler mockGradientRequestBusHandler;

        Vegetation::DescriptorWeightSelectorConfig config;
        config.m_sortBehavior = Vegetation::SortBehavior::Descending;
        config.m_gradientSampler.m_gradientId = mockGradientRequestBusHandler.m_entity.GetId();

        Vegetation::DescriptorWeightSelectorComponent* component = nullptr;
        auto entity = CreateEntity(config, &component);

        Vegetation::DescriptorSelectorParams descriptorSelectorParams;

        Vegetation::DescriptorPtrVec descriptors;
        for (int i = 1; i < 5; ++i)
        {
            auto descriptor = AZStd::make_shared<Vegetation::Descriptor>();
            descriptor->m_weight = i / 1000.0f;
            descriptors.emplace_back(descriptor);
        }

        Vegetation::DescriptorSelectorRequestBus::Event(
            entity->GetId(),
            &Vegetation::DescriptorSelectorRequestBus::Events::SelectDescriptors,
            descriptorSelectorParams,
            descriptors);

        EXPECT_GT(mockGradientRequestBusHandler.m_count, 0);
        ASSERT_EQ(4, descriptors.size());
        EXPECT_GT(descriptors[0]->m_weight, descriptors[1]->m_weight);
        EXPECT_GT(descriptors[1]->m_weight, descriptors[2]->m_weight);
        EXPECT_GT(descriptors[2]->m_weight, descriptors[3]->m_weight);
    }
}
