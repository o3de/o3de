/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Component/EditorComponentAPIBus.h>
#include <AzToolsFramework/Entity/EditorEntitySearchComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/UnitTest/AzToolsFrameworkTestHelpers.h>
#include <AzToolsFramework/ToolsComponents/TransformComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorLockComponent.h>
#include <AzToolsFramework/ToolsComponents/EditorVisibilityComponent.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

namespace AzToolsFramework
{
    // Test components used to test component filters
    class EntitySearch_TestComponent1
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(EntitySearch_TestComponent1, "{D8ABC8F6-E43B-4ED9-AABE-BA8905D4099D}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntitySearch_TestComponent1, AZ::Component>()
                    ->Version(1)
                    ->Field("Bool Value", &EntitySearch_TestComponent1::m_boolValue)
                    ->Field("Int Value", &EntitySearch_TestComponent1::m_intValue)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EntitySearch_TestComponent1>("SearchTestComponent1", "Component 1 for Entity Search Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::Category, "Entity Search Test Components")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Tag.png")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Tag.png")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EntitySearch_TestComponent1::m_boolValue, "Bool", "")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EntitySearch_TestComponent1::m_intValue, "Int", "")
                    ;
                }
            }
        }

        static constexpr bool DefaultBoolValue = true;

        EntitySearch_TestComponent1() = default;

        EntitySearch_TestComponent1(int intValue, bool boolValue)
            : m_boolValue(boolValue)
            , m_intValue(intValue)
        {
        }

        ~EntitySearch_TestComponent1() override
        {}

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}

        int m_intValue = 0;
        bool m_boolValue = DefaultBoolValue;
    };

    class EntitySearch_TestComponent2
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(EntitySearch_TestComponent2, "{E50A848D-64C3-4445-A21B-D8F9C96972FE}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<EntitySearch_TestComponent2, AZ::Component>()
                    ->Version(1)
                    ->Field("Float Value", &EntitySearch_TestComponent2::m_floatValue)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<EntitySearch_TestComponent2>("SearchTestComponent2", "Component 2 for Entity Search Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                            ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                            ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                            ->Attribute(AZ::Edit::Attributes::Category, "Entity Search Test Components")
                            ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Tag.png")
                            ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Tag.png")
                            ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                            ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &EntitySearch_TestComponent2::m_floatValue, "Float", "")
                    ;
                }
            }
        }

        const static float DefaultFloatValue;

        EntitySearch_TestComponent2() = default;

        EntitySearch_TestComponent2(float floatValue)
            : m_floatValue(floatValue)
        {
        }

        ~EntitySearch_TestComponent2() override
        {}

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}

        float m_floatValue = DefaultFloatValue;
    };

    const float EntitySearch_TestComponent2::DefaultFloatValue = 5.0f;

    class EditorEntitySearchComponentTests
        : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(m_descriptor, startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            RegisterComponents();
            GenerateTestHierarchy();
        }

        void RegisterComponents()
        {
            // Register our test components (This process also reflects them to the appropriate contexts)
            auto* EntitySearch_TestComponent1Descriptor = EntitySearch_TestComponent1::CreateDescriptor();
            auto* EntitySearch_TestComponent2Descriptor = EntitySearch_TestComponent2::CreateDescriptor();

            m_app.RegisterComponentDescriptor(EntitySearch_TestComponent1Descriptor);
            m_app.RegisterComponentDescriptor(EntitySearch_TestComponent2Descriptor);

            m_testComponentType1 = azrtti_typeid<EntitySearch_TestComponent1>();
            m_testComponentType2 = azrtti_typeid<EntitySearch_TestComponent2>();
        }

        void GenerateTestHierarchy() 
        {
            /*
            *   City
            *   |_  Street              (Test Component 2)
            *       |_  Car
            *       |   |_ Passenger    (Test Component 1, Test Component 2)
            *       |   |_ Passenger
            *       |_  Car             (Test Component 1)
            *       |   |_ Passenger
            *       |_  SportsCar
            *           |_ Passenger    (Test Component 2)
            *           |_ Passenger
            */

            m_testComponentType1Count = 0;

            m_entityMap["cityId"] =         CreateEditorEntity("City",      AZ::EntityId());
            m_entityMap["streetId"] =       CreateEditorEntity("Street",    m_entityMap["cityId"],      false,  true);
            m_entityMap["carId1"] =         CreateEditorEntity("Car",       m_entityMap["streetId"]);
            m_entityMap["passengerId1"] =   CreateEditorEntity("Passenger", m_entityMap["carId1"],      true,   true);
            m_entityMap["passengerId2"] =   CreateEditorEntity("Passenger", m_entityMap["carId1"]);
            m_entityMap["carId2"] =         CreateEditorEntity("Car",       m_entityMap["streetId"],    true);
            m_entityMap["passengerId3"] =   CreateEditorEntity("Passenger", m_entityMap["carId2"]);
            m_entityMap["sportsCarId"] =    CreateEditorEntity("SportsCar", m_entityMap["streetId"]);
            m_entityMap["passengerId4"] =   CreateEditorEntity("Passenger", m_entityMap["sportsCarId"], false,  true);
            m_entityMap["passengerId5"] =   CreateEditorEntity("Passenger", m_entityMap["sportsCarId"]);

            // Add some Components
        }

        AZ::EntityId CreateEditorEntity(const char* name, AZ::EntityId parentId, bool addTestComponent1 = false, bool addTestComponent2 = false)
        {
            AZ::Entity* entity = nullptr;
            UnitTest::CreateDefaultEditorEntity(name, &entity);

            entity->Deactivate();

            if (addTestComponent1)
            {
                entity->CreateComponent<EntitySearch_TestComponent1>(m_testComponentType1Count++, EntitySearch_TestComponent1::DefaultBoolValue);
            }

            if (addTestComponent2)
            {
                entity->CreateComponent<EntitySearch_TestComponent2>(EntitySearch_TestComponent2::DefaultFloatValue);
            }

            entity->Activate();

            // Parent
            AZ::TransformBus::Event(entity->GetId(), &AZ::TransformInterface::SetParent, parentId);

            return entity->GetId();
        }

        void TearDown() override
        {
            m_app.Stop();
        }

        UnitTest::ToolsTestApplication m_app{ "EditorEntitySearchComponentTests" };
        AZ::ComponentApplication::Descriptor m_descriptor;
        AZStd::unordered_map<AZStd::string, AZ::EntityId> m_entityMap;

        AZ::Uuid m_testComponentType1;
        AZ::Uuid m_testComponentType2;

        int m_testComponentType1Count;
    };

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_RootEntities)
    {
        {
            EntityIdList rootEntities;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(rootEntities, &AzToolsFramework::EditorEntitySearchRequests::GetRootEditorEntities);

            EXPECT_EQ(rootEntities.size(), 1);
            EXPECT_EQ(rootEntities[0], m_entityMap["cityId"]);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByName_Base)
    {
        {
            // No filters - return all entities

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, EntitySearchFilter());

            EXPECT_EQ(searchResults.size(), m_entityMap.size());
        }

        {
            // Filter by name - single entity

            EntitySearchFilter filter;
            filter.m_names.push_back("Street");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            // Filter by name - multiple entities

            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 5);
        }

        {
            // Filter by name - multiple names

            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");
            filter.m_names.push_back("Street");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 6);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByName_Wildcard)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Str*et");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }
        
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("St*t");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Str?et");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Str?t");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("C*");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("*");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), m_entityMap.size());
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByName_CaseSensitive)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByPath_Base)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|SportsCar");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["sportsCarId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|Car|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByPath_Wildcard)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|*|SportsCar");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["sportsCarId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|*|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 5);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|*Car|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 5);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|Sport*|Passenger");

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByPath_CaseSensitive)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("city|street");
            filter.m_namesCaseSensitive = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["streetId"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("city|street");
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByComponent_Base)
    {
        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType2);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByComponent_Multiple)
    {
        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1);
            filter.m_components.emplace(m_testComponentType2);
            filter.m_mustMatchAllComponents = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 4);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1);
            filter.m_components.emplace(m_testComponentType2);
            filter.m_mustMatchAllComponents = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["passengerId1"]);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByComponent_MatchProperty)
    {
        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            
            EXPECT_EQ(searchResults.size(), 2);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Int", 0 } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 1);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", !EntitySearch_TestComponent1::DefaultBoolValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Int", m_entityMap.size() } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Float", 0.0f } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByComponent_MatchMultipleProperties)
    {
        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = true;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue } });
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 1);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = false;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue } });
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 4);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = true;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue }, { "Int", 0 } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 1);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = false;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue }, { "Int", 0 } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 2);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = false;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue }, { "Int", 0 } });
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 4);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = true;
            filter.m_components.emplace(m_testComponentType1, EntitySearchFilter::ComponentProperties{ { "Bool", EntitySearch_TestComponent1::DefaultBoolValue }, { "Int", 0 } });
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 1);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = false;
            filter.m_components.emplace(m_testComponentType1);
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 4);
        }

        {
            EntitySearchFilter filter;
            filter.m_mustMatchAllComponents = true;
            filter.m_components.emplace(m_testComponentType1);
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);


            EXPECT_EQ(searchResults.size(), 1);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_SearchByAabb_Base)
    {
        {
            // No filters - return all entities

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, EntitySearchFilter());

            EXPECT_EQ(searchResults.size(), m_entityMap.size());
        }

        {
            // Filter by huge AABB - return all entities

            EntitySearchFilter filter;
            filter.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateZero(), 1000.0f);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), m_entityMap.size());
        }

        {
            // Filter by small AABB - return no entity

            EntitySearchFilter filter;
            filter.m_aabb = AZ::Aabb::CreateCenterRadius(AZ::Vector3::CreateOne(), 0.1f);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_Search_Roots_Base)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Passenger");
            filter.m_roots.push_back(m_entityMap["carId2"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["passengerId3"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("SportsCar");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("City|Street|SportsCar|Passenger");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["carId1"]);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 2);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_Search_Roots_NamesAreRootBased)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            // No root - Default
            filter.m_namesAreRootBased = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            // No root - Default
            filter.m_namesAreRootBased = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["streetId"]);
            filter.m_namesAreRootBased = false; // Default

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["streetId"]);
            filter.m_namesAreRootBased = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 3);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car|Passenger");
            filter.m_roots.push_back(m_entityMap["carId2"]);
            filter.m_namesAreRootBased = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 0);
        }
    }

    TEST_F(EditorEntitySearchComponentTests, EditorEntitySearchTests_Search_MultipleFilters)
    {
        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Car");
            filter.m_aabb = AZ::Aabb::CreateFromMinMax(AZ::Vector3(-1.0f), AZ::Vector3(1.0f));
            filter.m_components.emplace(m_testComponentType1);

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["carId2"]);
        }

        {
            EntitySearchFilter filter;
            filter.m_names.push_back("Pass*");
            filter.m_roots.push_back(m_entityMap["sportsCarId"]);
            filter.m_components.emplace(m_testComponentType2, EntitySearchFilter::ComponentProperties{ { "Float", EntitySearch_TestComponent2::DefaultFloatValue } });
            filter.m_namesAreRootBased = true;
            filter.m_namesCaseSensitive = true;

            EntityIdList searchResults;
            AzToolsFramework::EditorEntitySearchBus::BroadcastResult(searchResults, &AzToolsFramework::EditorEntitySearchRequests::SearchEntities, filter);

            EXPECT_EQ(searchResults.size(), 1);
            EXPECT_EQ(searchResults[0], m_entityMap["passengerId4"]);
        }
    }
}
