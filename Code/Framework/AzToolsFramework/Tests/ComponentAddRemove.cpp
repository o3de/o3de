/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>

#include <AzToolsFramework/API/EntityCompositionNotificationBus.h>
#include <AzToolsFramework/API/EntityCompositionRequestBus.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/Entity/EditorEntityActionComponent.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorComponentBase.h>
#include <AzToolsFramework/ToolsComponents/EditorDisabledCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionBus.h>
#include <AzToolsFramework/ToolsComponents/EditorPendingCompositionComponent.h>
#include <AzToolsFramework/ToolsComponents/GenericComponentWrapper.h>


namespace UnitTest
{
    using namespace AZ;
    using namespace AzToolsFramework;
    using namespace AZ::Data;
    using namespace AZ::IO;
    //
    // Declaring several clothing-themed components for use in tests.
    //

    // Shoes require socks
    class LeatherBootsComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(LeatherBootsComponent, "{C2852908-0FC6-4BF6-9907-E390840F9897}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<LeatherBootsComponent, AZ::Component>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<LeatherBootsComponent>("Leather Boots", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("ShoesService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("ShoesService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("SocksService"));
        }
    };

    // Note that WoolSocksComponent is an "editor component".
    // This is just to make sure we are testing with both "editor"
    // and "non-editor" components.
    class WoolSocksComponent
        : public AzToolsFramework::Components::EditorComponentBase
    {
    public:
        AZ_COMPONENT(WoolSocksComponent, "{6436A9A1-701E-4275-AF6F-82F53C7916C8}", EditorComponentBase);
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<WoolSocksComponent, EditorComponentBase>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<WoolSocksComponent>("Wool Socks", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("SocksService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SocksService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }
    };

    // Incompatible with socks
    class HatesSocksComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(HatesSocksComponent, "{D359D446-A172-4854-8EA9-B95073FF5709}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<HatesSocksComponent, AZ::Component>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<HatesSocksComponent>("Hates Socks", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& /*provided*/)
        {
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SocksService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }
    };

    // Pants require underwear
    class BlueJeansComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(BlueJeansComponent, "{AEA4D69E-F02B-4F6D-A793-8DEE0C0E54E3}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<BlueJeansComponent, AZ::Component>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<BlueJeansComponent>("Blue Jeans", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("TrousersService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("TrousersService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("UnderwearService"));
        }
    };

    // 1 of 2 underwear styles
    class WhiteBriefsComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(WhiteBriefsComponent, "{8B095E11-082B-4EB1-A119-D1534323C956}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<WhiteBriefsComponent, AZ::Component>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<WhiteBriefsComponent>("White Briefs", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("UnderwearService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("UnderwearService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }
    };

    // 2 of 2 underwear styles
    class HeartBoxersComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(HeartBoxersComponent, "{06071955-CC65-4C32-A4D8-1125D827C10B}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<HeartBoxersComponent, AZ::Component>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<HeartBoxersComponent>("Heart Boxers", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("UnderwearService"));
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("UnderwearService"));
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& /*required*/)
        {
        }
    };

    // Requires a belt (but no belt exists)
    class KnifeSheathComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(KnifeSheathComponent, "{D99C3EF1-592F-4744-9D07-A5F2CE679870}");
        void Activate() override {}
        void Deactivate() override {}
        static void Reflect(AZ::ReflectContext* reflection)
        {
            if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<KnifeSheathComponent, AZ::Component>();
                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<KnifeSheathComponent>("Knife Sheath", "")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& /*provided*/)
        {
        }
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& /*incompatible*/)
        {
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("BeltService"));
        }
    };

    // Count components of given type
    template <typename ComponentType>
    size_t CountComponentsOnEntity(AZ::Entity* entity)
    {
        EXPECT_NE(nullptr, entity);
        if (!entity)
        {
            return 0;
        }

        size_t count = 0;
        for (const auto component : entity->GetComponents())
        {
            if (AzToolsFramework::GetUnderlyingComponentType(*component) == azrtti_typeid<ComponentType>())
            {
                ++count;
            }
        }
        return count;
    }

    template <typename ComponentType>
    size_t CountPendingComponentsOnEntity(AZ::Entity* entity)
    {
        EXPECT_NE(nullptr, entity);
        if (!entity)
        {
            return 0;
        }

        AZ::Entity::ComponentArrayType pendingComponents;
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequestBus::Events::GetPendingComponents, pendingComponents);

        size_t count = 0;
        for (const auto pendingComponent : pendingComponents)
        {
            if (AzToolsFramework::GetUnderlyingComponentType(*pendingComponent) == azrtti_typeid<ComponentType>())
            {
                ++count;
            }
        }
        return count;
    }

    template <typename ComponentType>
    size_t CountDisabledComponentsOnEntity(AZ::Entity* entity)
    {
        EXPECT_NE(nullptr, entity);
        if (!entity)
        {
            return 0;
        }

        AZ::Entity::ComponentArrayType disabledComponents;
        AzToolsFramework::EditorDisabledCompositionRequestBus::Event(entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequestBus::Events::GetDisabledComponents, disabledComponents);

        size_t count = 0;
        for (const auto disabledComponent : disabledComponents)
        {
            if (AzToolsFramework::GetUnderlyingComponentType(*disabledComponent) == azrtti_typeid<ComponentType>())
            {
                ++count;
            }
        }
        return count;
    }

    template <typename ComponentType>
    AZ::Entity::ComponentArrayType GetComponentsForEntity(AZ::Entity* entity)
    {
        AZ::Entity::ComponentArrayType components;
        AzToolsFramework::GetAllComponentsForEntity(entity, components);

        auto itr = AZStd::remove_if(components.begin(), components.end(), [](AZ::Component* component) {return AzToolsFramework::GetUnderlyingComponentType(*component) != azrtti_typeid<ComponentType>();});
        components.erase(itr, components.end());

        return components;
    }

    bool CheckAllAreTrue(std::initializer_list<bool> booleans)
    {
        for (auto boolean : booleans)
        {
            if (!boolean)
            {
                return false;
            }
        }
        return true;
    }

    template <typename... BooleanTypes>
    bool CheckAllAreTrue(BooleanTypes... booleans)
    {
        return CheckAllAreTrue({booleans...});
    }

    bool DoesComponentListHaveComponent(const AZ::Entity::ComponentArrayType& componentList, const AZ::Uuid& componentType)
    {
        for (const auto component : componentList)
        {
            if (AzToolsFramework::GetUnderlyingComponentType(*component) == componentType)
            {
                return true;
            }
        }
        return false;
    }

    template <typename... AdditionalValidatedComponentTypes>
    struct VerifyAdditionalValidatedComponents
    {
        static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsOutcome& outcome, AZ::Entity* entity)
        {
            auto iterEntityOutcome = outcome.GetValue().find(entity->GetId());
            EXPECT_NE(iterEntityOutcome, outcome.GetValue().end());
            return CheckAllAreTrue(DoesComponentListHaveComponent(iterEntityOutcome->second.m_additionalValidatedComponents, azrtti_typeid<AdditionalValidatedComponentTypes>())...);
        }
    };

    template <typename... AddedPendingComponentTypes>
    struct VerifyAddedPendingComponents
    {
        static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsOutcome& outcome, AZ::Entity* entity)
        {
            auto iterEntityOutcome = outcome.GetValue().find(entity->GetId());
            EXPECT_NE(iterEntityOutcome, outcome.GetValue().end());
            return CheckAllAreTrue(DoesComponentListHaveComponent(iterEntityOutcome->second.m_addedPendingComponents, azrtti_typeid<AddedPendingComponentTypes>())...);
        }
    };

    template <typename... AddedValidComponentTypes>
    struct VerifyAddedValidComponents
    {
        static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsOutcome& outcome, AZ::Entity* entity)
        {
            auto iterEntityOutcome = outcome.GetValue().find(entity->GetId());
            EXPECT_NE(iterEntityOutcome, outcome.GetValue().end());
            return CheckAllAreTrue(DoesComponentListHaveComponent(iterEntityOutcome->second.m_addedValidComponents, azrtti_typeid<AddedValidComponentTypes>())...);
        }

        template <typename... AdditionalValidatedComponentTypes>
        struct AndAdditionalValidatedComponents
        {
            static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsOutcome& outcome, AZ::Entity* entity)
            {
                return CheckAllAreTrue(
                    VerifyAddedValidComponents<AddedValidComponentTypes...>::OnOutcomeForEntity(outcome, entity),
                    VerifyAdditionalValidatedComponents<AdditionalValidatedComponentTypes...>::OnOutcomeForEntity(outcome, entity)
                );
            }
        };

        template <typename... AddedPendingComponentTypes>
        struct AndAddedPendingComponents
        {
            static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsOutcome& outcome, AZ::Entity* entity)
            {
                return CheckAllAreTrue(
                    VerifyAddedValidComponents<AddedValidComponentTypes...>::OnOutcomeForEntity(outcome, entity),
                    VerifyAddedPendingComponents<AddedPendingComponentTypes...>::OnOutcomeForEntity(outcome, entity)
                );
            }

            template <typename... AdditionalValidatedComponentTypes>
            struct AndAdditionalValidatedComponents
            {
                static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::AddComponentsOutcome& outcome, AZ::Entity* entity)
                {
                    return CheckAllAreTrue(
                            VerifyAddedValidComponents<AddedValidComponentTypes...>::OnOutcomeForEntity(outcome, entity),
                            VerifyAddedPendingComponents<AddedPendingComponentTypes...>::OnOutcomeForEntity(outcome, entity),
                            VerifyAdditionalValidatedComponents<AdditionalValidatedComponentTypes...>::OnOutcomeForEntity(outcome, entity)
                    );
                }
            };
        };
    };

    template <typename... InvalidatedComponentTypes>
    struct VerifyRemovalInvalidatedComponents
    {
        static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::RemoveComponentsOutcome& outcome, AZ::Entity* entity)
        {
            auto iterEntityOutcome = outcome.GetValue().find(entity->GetId());
            EXPECT_NE(iterEntityOutcome, outcome.GetValue().end());
            return CheckAllAreTrue(DoesComponentListHaveComponent(iterEntityOutcome->second.m_invalidatedComponents, azrtti_typeid<InvalidatedComponentTypes>())...);
        }
    };

    template <typename... ValidatedComponentTypes>
    struct VerifyRemovalValidatedComponents
    {
        static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::RemoveComponentsOutcome& outcome, AZ::Entity* entity)
        {
            auto iterEntityOutcome = outcome.GetValue().find(entity->GetId());
            EXPECT_NE(iterEntityOutcome, outcome.GetValue().end());
            return CheckAllAreTrue(DoesComponentListHaveComponent(iterEntityOutcome->second.m_validatedComponents, azrtti_typeid<ValidatedComponentTypes>())...);
        }

        template <typename... InvalidatedComponentTypes>
        struct AndInvalidatedComponents
        {
            static bool OnOutcomeForEntity(const AzToolsFramework::EntityCompositionRequestBus::Events::RemoveComponentsOutcome& outcome, AZ::Entity* entity)
            {
                return CheckAllAreTrue(
                        VerifyRemovalValidatedComponents<ValidatedComponentTypes...>::OnOutcomeForEntity(outcome, entity),
                        VerifyRemovalInvalidatedComponents<InvalidatedComponentTypes...>::OnOutcomeForEntity(outcome, entity)
                );
            }
        };
    };

    class EntityComponentCounter
    {
    public:
        void SetEntity(const AZ::Entity* entity)
        {
            m_entity = entity;
        }

        size_t GetCount() const
        {
            if (!m_entity)
            {
                return 0;
            }

            return GetComponentCount() - m_lastResetCount;
        }

        size_t GetPendingCount() const
        {
            if (!m_entity)
            {
                return 0;
            }

            return GetPendingComponentCount() - m_lastPendingResetCount;
        }

        size_t GetDisabledCount() const
        {
            if (!m_entity)
            {
                return 0;
            }

            return GetDisabledComponentCount() - m_lastDisabledResetCount;
        }

        void Reset()
        {
            m_lastResetCount = GetComponentCount();
            m_lastPendingResetCount = GetPendingComponentCount();
            m_lastDisabledResetCount = GetDisabledComponentCount();
        }

    private:

        size_t GetComponentCount() const
        {
            return m_entity->GetComponents().size();
        }

        size_t GetPendingComponentCount() const
        {
            AZ::Entity::ComponentArrayType pendingComponents;
            AzToolsFramework::EditorPendingCompositionRequestBus::Event(m_entity->GetId(), &AzToolsFramework::EditorPendingCompositionRequestBus::Events::GetPendingComponents, pendingComponents);
            return pendingComponents.size();
        }

        size_t GetDisabledComponentCount() const
        {
            AZ::Entity::ComponentArrayType disabledComponents;
            AzToolsFramework::EditorDisabledCompositionRequestBus::Event(m_entity->GetId(), &AzToolsFramework::EditorDisabledCompositionRequestBus::Events::GetDisabledComponents, disabledComponents);
            return disabledComponents.size();
        }

        size_t m_lastResetCount = 0;
        size_t m_lastPendingResetCount = 0;
        size_t m_lastDisabledResetCount = 0;
        const AZ::Entity* m_entity;
    };

    class AddComponentsTest
        : public LeakDetectionFixture
    {
    public:
        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            AZ::SettingsRegistryInterface* registry = AZ::SettingsRegistry::Get();
            auto projectPathKey =
                AZ::SettingsRegistryInterface::FixedValueString(AZ::SettingsRegistryMergeUtils::BootstrapSettingsRootKey) + "/project_path";
            AZ::IO::FixedMaxPath enginePath;
            registry->Get(enginePath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            registry->Set(projectPathKey, (enginePath / "AutomatedTesting").Native());
            AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_AddRuntimeFilePaths(*registry);

            AzFramework::Application::Descriptor descriptor;
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_app.Start(descriptor, startupParameters);

            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);

            m_app.RegisterComponentDescriptor(LeatherBootsComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(WoolSocksComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(HatesSocksComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(BlueJeansComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(WhiteBriefsComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(HeartBoxersComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(KnifeSheathComponent::CreateDescriptor());

            m_entity1 = new AZ::Entity("Entity1");
            m_entity1Counter.SetEntity(m_entity1);
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *m_entity1);
            m_entity1->Init();
            m_entity1Counter.Reset();

            m_entity2 = new AZ::Entity("Entity2");
            m_entity2Counter.SetEntity(m_entity2);
            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(&AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents, *m_entity2);
            m_entity2->Init();
            m_entity2Counter.Reset();
        }

        void TearDown() override
        {
            m_app.Stop();

            LeakDetectionFixture::TearDown();
        }

        AzToolsFramework::ToolsApplication m_app;

        AZ::Entity* m_entity1 = nullptr;
        EntityComponentCounter m_entity1Counter;

        AZ::Entity* m_entity2 = nullptr;
        EntityComponentCounter m_entity2Counter;

        size_t m_initialComponentCount = 0;
    };

    TEST_F(AddComponentsTest, AddOneComponentToOneEntity)
    {
        auto outcome = AzToolsFramework::AddComponents<WoolSocksComponent>::ToEntities(m_entity1);
        // Verify success
        ASSERT_TRUE(outcome.IsSuccess());

        // Check that the returned result was what we expected
        ASSERT_TRUE(VerifyAddedValidComponents<WoolSocksComponent>::OnOutcomeForEntity(outcome, m_entity1));

        // Check that we have the component added as expected
        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));

        // We do separate count checks in case some other random components were added or something unexpected occurred
        ASSERT_EQ(1, m_entity1Counter.GetCount());

        // Verify nothing is pending
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());

        // Verify nothing is disabled
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        auto addComponentsResults = outcome.GetValue()[m_entity1->GetId()];
        AzToolsFramework::DisableComponents(addComponentsResults.m_addedValidComponents);
        ASSERT_EQ(0, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(1, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetDisabledCount());

        AzToolsFramework::EnableComponents(addComponentsResults.m_addedValidComponents);
        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());
    }

    TEST_F(AddComponentsTest, AddOneComponentToMultipleEntities)
    {
        // have one entity activated, we must ensure that it is still activated after add operation
        m_entity1->Activate();

        // add a component to both the activated and inactive entities
        auto outcome = AzToolsFramework::AddComponents<WoolSocksComponent>::ToEntities(m_entity1, m_entity2);

        // Verify outcome
        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedValidComponents<WoolSocksComponent>::OnOutcomeForEntity(outcome, m_entity1));
        ASSERT_TRUE(VerifyAddedValidComponents<WoolSocksComponent>::OnOutcomeForEntity(outcome, m_entity2));

        // Should always be on entity, not pending since services are met
        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity2));
        ASSERT_EQ(1, m_entity1Counter.GetCount());
        ASSERT_EQ(1, m_entity2Counter.GetCount());

        // Nothing is pending
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity2));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity2Counter.GetPendingCount());

        // Nothing is disabled
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity2));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());
        ASSERT_EQ(0, m_entity2Counter.GetDisabledCount());

        // Still in original states
        ASSERT_EQ(AZ::Entity::State::Active, m_entity1->GetState());
        ASSERT_EQ(AZ::Entity::State::Init, m_entity2->GetState());
    }

    // Add a component which requires another component
    TEST_F(AddComponentsTest, ComponentRequiresService)
    {
        auto outcome = AzToolsFramework::AddComponents<LeatherBootsComponent>::ToEntities(m_entity1);

        // Verify outcome
        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedPendingComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity1));

        // This will be pending since it is missing a socks service
        ASSERT_EQ(0, CountComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetCount());
        ASSERT_EQ(1, CountPendingComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        // Satisfy the pending component with wool socks
        outcome = AzToolsFramework::AddComponents<WoolSocksComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedValidComponents<WoolSocksComponent>::AndAdditionalValidatedComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity1));

        // Should have both on the entity now and no pending
        ASSERT_EQ(1, CountComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        // LeatherBootsComponent should be wrapped in a GenericComponentWrapper
        // because it is not an "editor component".
        auto* wrapper = m_entity1->FindComponent<AzToolsFramework::Components::GenericComponentWrapper>();

        ASSERT_TRUE(wrapper && wrapper->GetTemplate());
        ASSERT_EQ(azrtti_typeid<LeatherBootsComponent>(), azrtti_typeid(wrapper->GetTemplate()));

        // Try adding a component which requires a service
        // that no other component provides.
        outcome = AzToolsFramework::AddComponents<KnifeSheathComponent>::ToEntities(m_entity2);

        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedPendingComponents<KnifeSheathComponent>::OnOutcomeForEntity(outcome, m_entity2));

        // This one will always be pending, never on entity as it will never be satisfied
        ASSERT_EQ(0, CountComponentsOnEntity<KnifeSheathComponent>(m_entity2));
        ASSERT_EQ(0, m_entity2Counter.GetCount());
        ASSERT_EQ(1, CountPendingComponentsOnEntity<KnifeSheathComponent>(m_entity2));
        ASSERT_EQ(1, m_entity2Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<KnifeSheathComponent>(m_entity2));
        ASSERT_EQ(0, m_entity2Counter.GetDisabledCount());

        // Check pending status
        auto component = outcome.GetValue()[m_entity2->GetId()].m_addedPendingComponents[0];
        AzToolsFramework::EntityCompositionRequestBus::Events::PendingComponentInfo pendingComponentInfo;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequestBus::Events::GetPendingComponentInfo, component);
        // Should have one missing service
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices.size(), 1);
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible.size(), 0);
        ASSERT_EQ(pendingComponentInfo.m_pendingComponentsWithRequiredServices.size(), 0);
        // And that missing service should be the BeltService
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices[0], AZ_CRC_CE("BeltService"));

        // Entity 1 should remain untouched
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());
    }

    // Add a component (jeans) which requires a service (underwear),
    // and there are two viable options (boxers or briefs).
    TEST_F(AddComponentsTest, ComponentRequiresServiceWithTwoViableOptions)
    {
        auto outcome = AzToolsFramework::AddComponents<BlueJeansComponent>::ToEntities(m_entity1);

        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedPendingComponents<BlueJeansComponent>::OnOutcomeForEntity(outcome, m_entity1));

        ASSERT_EQ(0, CountComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetCount());
        ASSERT_EQ(1, CountPendingComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        // Check pending status
        auto component = outcome.GetValue()[m_entity1->GetId()].m_addedPendingComponents[0];
        AzToolsFramework::EntityCompositionRequestBus::Events::PendingComponentInfo pendingComponentInfo;
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequestBus::Events::GetPendingComponentInfo, component);
        // Should have one missing service
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices.size(), 1);
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible.size(), 0);
        ASSERT_EQ(pendingComponentInfo.m_pendingComponentsWithRequiredServices.size(), 0);
        // And that missing service should be the "UnderwearService"
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices[0], AZ_CRC_CE("UnderwearService"));

        outcome = AzToolsFramework::AddComponents<WhiteBriefsComponent>::ToEntities(m_entity1);

        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedValidComponents<WhiteBriefsComponent>::AndAdditionalValidatedComponents<BlueJeansComponent>::OnOutcomeForEntity(outcome, m_entity1));

        // Save this for later checks
        auto whiteBriefsComponent = outcome.GetValue()[m_entity1->GetId()].m_addedValidComponents[0];

        ASSERT_EQ(1, CountComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        // Now try adding the second kind of underwear
        // (it should be pending because entity already has underwear)
        outcome = AzToolsFramework::AddComponents<HeartBoxersComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedPendingComponents<HeartBoxersComponent>::OnOutcomeForEntity(outcome, m_entity1));

        ASSERT_EQ(1, CountComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(1, CountPendingComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        // Check pending status
        component = outcome.GetValue()[m_entity1->GetId()].m_addedPendingComponents[0];
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequestBus::Events::GetPendingComponentInfo, component);
        // Should have one incompatible component
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices.size(), 0);
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible.size(), 1);
        ASSERT_EQ(pendingComponentInfo.m_pendingComponentsWithRequiredServices.size(), 0);
        // And that incompatible component should be the WhiteBriefsComponent from earlier
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible[0], whiteBriefsComponent);

        // disable white briefs component, which should resolve heart briefs, and check container counts
        AzToolsFramework::DisableComponents({ whiteBriefsComponent });
        ASSERT_EQ(1, CountComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(1, CountDisabledComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetDisabledCount());

        // re-enable white briefs component which is now pending because it's re-added after heart boxers was resolved
        AzToolsFramework::EnableComponents({ whiteBriefsComponent });
        ASSERT_EQ(1, CountComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(1, CountPendingComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        // Try removing pending component (should be uneventful, but it is a branch internally)
        auto removalOutcome = AzToolsFramework::RemoveComponents({ component });
        ASSERT_TRUE(removalOutcome.IsSuccess());
        ASSERT_EQ(1, CountComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<BlueJeansComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WhiteBriefsComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<HeartBoxersComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());
    }

    // Add a component to two entities, where the component requires a service,
    // and one entity already has that service, but the other entity does not.
    TEST_F(AddComponentsTest, TwoEntitiesWhereOneHasRequiredServiceAndOneDoesNot)
    {
        // entity1 already has socks
        auto outcome = AzToolsFramework::AddComponents<WoolSocksComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedValidComponents<WoolSocksComponent>::OnOutcomeForEntity(outcome, m_entity1));

        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(1, m_entity1Counter.GetCount());
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<LeatherBootsComponent>::ToEntities(m_entity1, m_entity2);
        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedValidComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity1));
        ASSERT_TRUE(VerifyAddedPendingComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity2));

        ASSERT_EQ(1, CountComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(1, CountComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(2, m_entity1Counter.GetCount());

        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());

        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity1));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<LeatherBootsComponent>(m_entity1));
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        ASSERT_EQ(0, CountComponentsOnEntity<LeatherBootsComponent>(m_entity2));
        ASSERT_EQ(0, CountComponentsOnEntity<WoolSocksComponent>(m_entity2));
        ASSERT_EQ(0, m_entity2Counter.GetCount());

        ASSERT_EQ(1, CountPendingComponentsOnEntity<LeatherBootsComponent>(m_entity2));
        ASSERT_EQ(0, CountPendingComponentsOnEntity<WoolSocksComponent>(m_entity2));
        ASSERT_EQ(1, m_entity2Counter.GetPendingCount());

        ASSERT_EQ(0, CountDisabledComponentsOnEntity<LeatherBootsComponent>(m_entity2));
        ASSERT_EQ(0, CountDisabledComponentsOnEntity<WoolSocksComponent>(m_entity2));
        ASSERT_EQ(0, m_entity2Counter.GetDisabledCount());
    }

    // Test adding a component which requires a service,
    // but all candidates which provide that service conflicts with some existing component.
    TEST_F(AddComponentsTest, RequiredServiceConflictsWithExistingComponents)
    {
        auto outcome = AzToolsFramework::AddComponents<HatesSocksComponent>::ToEntities(m_entity1);

        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedValidComponents<HatesSocksComponent>::OnOutcomeForEntity(outcome, m_entity1));

        // Save this for tests and removal later
        AZ::Component* hatesSocksComponent = outcome.GetValue()[m_entity1->GetId()].m_addedValidComponents[0];

        // Adding boots
        outcome = AzToolsFramework::AddComponents<LeatherBootsComponent>::ToEntities(m_entity1, m_entity2);

        ASSERT_TRUE(outcome.IsSuccess());
        ASSERT_TRUE(VerifyAddedPendingComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity1));
        ASSERT_TRUE(VerifyAddedPendingComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity2));
        auto leatherBootsComponent = outcome.GetValue()[m_entity1->GetId()].m_addedPendingComponents[0];

        // Add socks to make it valid, but incompatible with HatesSocks on entity 1
        outcome = AzToolsFramework::AddComponents<WoolSocksComponent>::ToEntities(m_entity1, m_entity2);
        ASSERT_TRUE(VerifyAddedPendingComponents<WoolSocksComponent>::OnOutcomeForEntity(outcome, m_entity1));
        // Socks will work on entity 2 and leather boots should be valid now because of it
        ASSERT_TRUE(VerifyAddedValidComponents<WoolSocksComponent>::AndAdditionalValidatedComponents<LeatherBootsComponent>::OnOutcomeForEntity(outcome, m_entity2));

        // Save this component for later
        auto woolSocksComponent = outcome.GetValue()[m_entity1->GetId()].m_addedPendingComponents[0];

        // Check pending status
        AzToolsFramework::EntityCompositionRequestBus::Events::PendingComponentInfo pendingComponentInfo;

        // First check leather boots, it should indicate it is waiting on woolSocksComponent
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequestBus::Events::GetPendingComponentInfo, leatherBootsComponent);
        // Should have one pending component
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices.size(), 0);
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible.size(), 0);
        ASSERT_EQ(pendingComponentInfo.m_pendingComponentsWithRequiredServices.size(), 1);
        // And that pending component should be the woolSocksComponent
        ASSERT_EQ(pendingComponentInfo.m_pendingComponentsWithRequiredServices[0], woolSocksComponent);

        // Now check the wool socks, they should be incompatible with Hates Socks
        AzToolsFramework::EntityCompositionRequestBus::BroadcastResult(pendingComponentInfo, &AzToolsFramework::EntityCompositionRequestBus::Events::GetPendingComponentInfo, woolSocksComponent);
        // Should have one incompatible component
        ASSERT_EQ(pendingComponentInfo.m_missingRequiredServices.size(), 0);
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible.size(), 1);
        ASSERT_EQ(pendingComponentInfo.m_pendingComponentsWithRequiredServices.size(), 0);
        // And that incompatible component should be the hatesSocksComponent
        ASSERT_EQ(pendingComponentInfo.m_validComponentsThatAreIncompatible[0], hatesSocksComponent);

        // Remove HatesSocks from entity 1 to valid the entire entity
        auto removalOutcome = AzToolsFramework::RemoveComponents({ hatesSocksComponent });
        ASSERT_TRUE(removalOutcome.IsSuccess());
        ASSERT_TRUE((VerifyRemovalValidatedComponents<WoolSocksComponent, LeatherBootsComponent>::OnOutcomeForEntity(removalOutcome, m_entity1)));
    }

    // Test adding, enabling, disabling several components
    TEST_F(AddComponentsTest, EnableDisableConflictingServices)
    {
        auto outcome = AzToolsFramework::AddComponents<LeatherBootsComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(0, m_entity1Counter.GetCount());
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<HatesSocksComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(1, m_entity1Counter.GetCount());
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<BlueJeansComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(1, m_entity1Counter.GetCount());
        ASSERT_EQ(2, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<WhiteBriefsComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(3, m_entity1Counter.GetCount());
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<HeartBoxersComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(3, m_entity1Counter.GetCount());
        ASSERT_EQ(2, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<KnifeSheathComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(3, m_entity1Counter.GetCount());
        ASSERT_EQ(3, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        outcome = AzToolsFramework::AddComponents<WoolSocksComponent>::ToEntities(m_entity1);
        ASSERT_TRUE(outcome.IsSuccess());

        ASSERT_EQ(3, m_entity1Counter.GetCount());
        ASSERT_EQ(4, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(0, m_entity1Counter.GetDisabledCount());

        AzToolsFramework::DisableComponents(GetComponentsForEntity<HatesSocksComponent>(m_entity1));

        ASSERT_EQ(4, m_entity1Counter.GetCount());
        ASSERT_EQ(2, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(1, m_entity1Counter.GetDisabledCount());

        AzToolsFramework::DisableComponents(GetComponentsForEntity<HeartBoxersComponent>(m_entity1));

        ASSERT_EQ(4, m_entity1Counter.GetCount());
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(2, m_entity1Counter.GetDisabledCount());

        AzToolsFramework::DisableComponents(GetComponentsForEntity<KnifeSheathComponent>(m_entity1));

        ASSERT_EQ(4, m_entity1Counter.GetCount());
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(3, m_entity1Counter.GetDisabledCount());

        AzToolsFramework::DisableComponents(GetComponentsForEntity<WhiteBriefsComponent>(m_entity1));

        ASSERT_EQ(2, m_entity1Counter.GetCount());
        ASSERT_EQ(1, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(4, m_entity1Counter.GetDisabledCount());

        AzToolsFramework::EnableComponents(GetComponentsForEntity<HeartBoxersComponent>(m_entity1));

        ASSERT_EQ(4, m_entity1Counter.GetCount());
        ASSERT_EQ(0, m_entity1Counter.GetPendingCount());
        ASSERT_EQ(3, m_entity1Counter.GetDisabledCount());
    }

    // A reusable testing fixture that ensures basic application services are mocked.
    // Provided:
    //      Basic Component descriptor functionality
    //      Memory
    //      Serialize (and Edit) contexts
    class MockApplicationFixture
        : public LeakDetectionFixture
        , public AZ::ComponentApplicationBus::Handler
    {
    public:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::vector<const ComponentDescriptor*> m_descriptors;

        //////////////////////////////////////////////////////////////////////////
        // ComponentApplicationMessages
        ComponentApplication* GetApplication() override { return nullptr; }
        void RegisterComponentDescriptor(const ComponentDescriptor* descriptor) override
        {
            m_descriptors.push_back(descriptor);
            descriptor->Reflect(m_serializeContext.get());
        }

        void UnregisterComponentDescriptor(const ComponentDescriptor*) override {}
        void RegisterEntityAddedEventHandler(EntityAddedEvent::Handler&) override {}
        void RegisterEntityRemovedEventHandler(EntityRemovedEvent::Handler&) override {}
        void RegisterEntityActivatedEventHandler(EntityActivatedEvent::Handler&) override {}
        void RegisterEntityDeactivatedEventHandler(EntityDeactivatedEvent::Handler&) override {}
        void SignalEntityActivated(Entity*) override {}
        void SignalEntityDeactivated(Entity*) override {}
        bool AddEntity(Entity*) override { return true; }
        bool RemoveEntity(Entity*) override { return true; }
        bool DeleteEntity(const EntityId&) override { return true; }
        Entity* FindEntity(const EntityId&) override { return nullptr; }
        SerializeContext* GetSerializeContext() override { return m_serializeContext.get(); }
        BehaviorContext*  GetBehaviorContext() override { return nullptr; }
        JsonRegistrationContext* GetJsonRegistrationContext() override { return nullptr; }
        const char* GetEngineRoot() const override { return nullptr; }
        const char* GetExecutableFolder() const override { return nullptr; }
        void EnumerateEntities(const EntityCallback& /*callback*/) override {}
        void QueryApplicationType(AZ::ApplicationTypeQuery& /*appType*/) const override {}
        //////////////////////////////////////////////////////////////////////////

        MockApplicationFixture()
            : LeakDetectionFixture()
        {
        }

        void SetUp() override
        {
            LeakDetectionFixture::SetUp();

            ComponentApplicationBus::Handler::BusConnect();
            AZ::Interface<AZ::ComponentApplicationRequests>::Register(this);
            m_serializeContext.reset(aznew AZ::SerializeContext(true, true));

            Entity::Reflect(m_serializeContext.get());
        }

        void TearDown() override
        {
            for (auto descriptor : m_descriptors)
            {
                delete descriptor;
            }
            m_descriptors.set_capacity(0);

            m_serializeContext.reset();
            AZ::Interface<AZ::ComponentApplicationRequests>::Unregister(this);
            ComponentApplicationBus::Handler::BusDisconnect();

            LeakDetectionFixture::TearDown();
        }
    };

    // add scrubbing capability to the above fixture by adding some components to it.
    class EntityTest_Scrubbing : public MockApplicationFixture
    {
    protected:
        AZStd::unique_ptr<Entity> m_fakeSystemEntity;

    public:
        class VisibleComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_COMPONENT(VisibleComponent, "{6CEC2D1E-08CF-4609-9BEE-BA9D32B4C223}", AzToolsFramework::Components::EditorComponentBase);

            void Activate() override {}
            void Deactivate() override {}

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("ValidComponentService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("ValidComponentService"));
            }

            static void Reflect(ReflectContext* reflection)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
                if (serializeContext)
                {
                    serializeContext->Class<VisibleComponent, AzToolsFramework::Components::EditorComponentBase>();

                    AZ::EditContext* ec = serializeContext->GetEditContext();
                    ec->Class<VisibleComponent>("Visible Component", "A class that should show up in the property editor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Show)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"));
                }
            }
        };

        class HiddenComponent
            : public AzToolsFramework::Components::EditorComponentBase
        {
        public:
            AZ_COMPONENT(HiddenComponent, "{E4D2AD8B-3930-46FC-837A-8DDFCA0FB1AF}", AzToolsFramework::Components::EditorComponentBase);

            static Component* s_wasDeleted;
            ~HiddenComponent() override
            {
                s_wasDeleted = this;
            }

            void Activate() override {}
            void Deactivate() override {}

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("HiddenComponentService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("HiddenComponentService"));
            }

            static void Reflect(ReflectContext* reflection)
            {
                SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(reflection);
                if (serializeContext)
                {
                    serializeContext->Class<HiddenComponent, AzToolsFramework::Components::EditorComponentBase>();

                    AZ::EditContext* ec = serializeContext->GetEditContext();
                    ec->Class<HiddenComponent>("Hidden Component", "A class that should not show up in the property editor")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::HideIcon, true)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::HideOnAdd | AZ::Edit::SliceFlags::PushWhenHidden);
                }
            }
        };


        void SetUp() override
        {
            MockApplicationFixture::SetUp();
            RegisterComponentDescriptor(VisibleComponent::CreateDescriptor());
            RegisterComponentDescriptor(HiddenComponent::CreateDescriptor());
            RegisterComponentDescriptor(AzToolsFramework::Components::EditorPendingCompositionComponent::CreateDescriptor());
            RegisterComponentDescriptor(AzToolsFramework::Components::EditorEntityActionComponent::CreateDescriptor());

            m_fakeSystemEntity.reset(aznew Entity());
            m_fakeSystemEntity->CreateComponent<AzToolsFramework::Components::EditorEntityActionComponent>();
            m_fakeSystemEntity->Init();
            m_fakeSystemEntity->Activate();
        }

        void TearDown() override
        {
            m_fakeSystemEntity->Deactivate();
            m_fakeSystemEntity.reset();
            MockApplicationFixture::TearDown();
        }
    };

    Component* EntityTest_Scrubbing::HiddenComponent::s_wasDeleted = nullptr;

    TEST_F(EntityTest_Scrubbing, ConflictingVisibleComponents_AreInvalidated)
    {
        // in this test we make sure that visible components (ones which show up on the UI)
        // that conflict with each other are properly disabled and moved to the pending list during scrubbing.

        // Component setup:
        AZ::Entity newEntity;

        AZ::Component* firstValidComponent = aznew VisibleComponent();
        newEntity.AddComponent(firstValidComponent);
        AZ::Component* conflictingVisibleComponent = aznew VisibleComponent();
        newEntity.AddComponent(conflictingVisibleComponent);

        EntityList entities;
        entities.push_back(&newEntity);
        EntityCompositionRequests::ScrubEntitiesOutcome resultValue = AZ::Failure<AZStd::string>("Didn't get called");
        EntityCompositionRequestBus::BroadcastResult(resultValue, &EntityCompositionRequestBus::Events::ScrubEntities, entities);

        ASSERT_TRUE(resultValue.IsSuccess());
        ASSERT_EQ(resultValue.GetValue().size(), 1);

        EntityCompositionRequests::ScrubEntityResults& resultForThisEntity = resultValue.GetValue()[entities[0]->GetId()];
        
        EXPECT_EQ(resultForThisEntity.m_invalidatedComponents.size(), 1);
        EXPECT_TRUE(AZStd::find(resultForThisEntity.m_invalidatedComponents.begin(), resultForThisEntity.m_invalidatedComponents.end(), conflictingVisibleComponent) != resultForThisEntity.m_invalidatedComponents.end());

        // The "Validated components" array should be empty becuase it should only list previously invalid components that were somehow validated by the scrubbing.
        EXPECT_EQ(resultForThisEntity.m_validatedComponents.size(), 0);

        // make sure the valid visible one wasn't removed:
        EXPECT_EQ(newEntity.FindComponent(azrtti_typeid<VisibleComponent>()), firstValidComponent);
    }

    TEST_F(EntityTest_Scrubbing, ConflictingHiddenComponents_AreDeleted)
    {
        // in this test we make sure that when an entity contains a conflicting hidden component
        // the hidden component is deleted as part of thes scrubbing.

        // Component setup:
        AZ::Entity newEntity;

        AZ::Component* validHiddenComponent = aznew HiddenComponent();
        AZ::Component* conflictingHiddenComponent = aznew HiddenComponent();
        newEntity.AddComponent(validHiddenComponent);
        newEntity.AddComponent(conflictingHiddenComponent);

        EntityTest_Scrubbing::HiddenComponent::s_wasDeleted = nullptr;

        EntityList entities;
        entities.push_back(&newEntity);
        EntityCompositionRequests::ScrubEntitiesOutcome resultValue = AZ::Failure<AZStd::string>("Didn't get called");
        EntityCompositionRequestBus::BroadcastResult(resultValue, &EntityCompositionRequestBus::Events::ScrubEntities, entities);

        // we cannnot test anything further if the array is empty or it failed.
        ASSERT_TRUE(resultValue.IsSuccess());
        ASSERT_EQ(resultValue.GetValue().size(), 1);

        EntityCompositionRequests::ScrubEntityResults& resultForThisEntity = resultValue.GetValue()[entities[0]->GetId()];

        // we must NOT find the conflicting component - should have been deleted.
        EXPECT_EQ(EntityTest_Scrubbing::HiddenComponent::s_wasDeleted, conflictingHiddenComponent);

        // we must also not find it on the invalidated list, since it has been deleted.
        EXPECT_EQ(resultForThisEntity.m_invalidatedComponents.size(), 0);

        // The "Validated components" array should be empty becuase it should only list previously invalid components that were somehow validated by the scrubbing.
        EXPECT_EQ(resultForThisEntity.m_validatedComponents.size(), 0);
        // make sure the remaining component on the entity is the correct hidden component 
        EXPECT_EQ(newEntity.FindComponents(azrtti_typeid<HiddenComponent>()).size(), 1); // there should only be one of them.
        EXPECT_EQ(newEntity.FindComponent(azrtti_typeid<HiddenComponent>()), validHiddenComponent);
        
    }

    TEST_F(EntityTest_Scrubbing, NonConflictingVisibleComponents_AreReinstated)
    {
        // in this test we make sure that if a pending component (inactive due to prior problems)
        // no longer has those problems, it is made valid and active when scrubbing occurs.

        // Component setup:
        AZ::Entity newEntity;

        AZ::Component* firstValidComponent = aznew VisibleComponent();
        AZ::Component* conflictingVisibleComponent = aznew VisibleComponent();
        newEntity.AddComponent(firstValidComponent);
        newEntity.AddComponent(conflictingVisibleComponent);

        EntityList entities;
        entities.push_back(&newEntity);
        {
            EntityCompositionRequests::ScrubEntitiesOutcome resultValue = AZ::Failure<AZStd::string>("Didn't get called");
            EntityCompositionRequestBus::BroadcastResult(resultValue, &EntityCompositionRequestBus::Events::ScrubEntities, entities);

            ASSERT_TRUE(resultValue.IsSuccess());
            ASSERT_EQ(resultValue.GetValue().size(), 1);
            // note that the actual results of the above operation are already verified in another test.  now we go further with this
            // and actually delete the original component so that the second one can become valid.
        }

        newEntity.RemoveComponent(firstValidComponent);
        delete firstValidComponent;

        // now re-scrub and expect to see the previously disabled conflicting one become the active one:
        {
            EntityCompositionRequests::ScrubEntitiesOutcome resultValue = AZ::Failure<AZStd::string>("Didn't get called");
            EntityCompositionRequestBus::BroadcastResult(resultValue, &EntityCompositionRequestBus::Events::ScrubEntities, entities);
            ASSERT_TRUE(resultValue.IsSuccess());
            ASSERT_EQ(resultValue.GetValue().size(), 1);
            EntityCompositionRequests::ScrubEntityResults& resultForThisEntity = resultValue.GetValue()[entities[0]->GetId()];

            // nothing should be invalidated
            EXPECT_EQ(resultForThisEntity.m_invalidatedComponents.size(), 0);
            // the visible component should now be activated.
            ASSERT_EQ(resultForThisEntity.m_validatedComponents.size(), 1);
            EXPECT_EQ(resultForThisEntity.m_validatedComponents[0], conflictingVisibleComponent);

            // make sure its actually active, on the entity.
            EXPECT_EQ(newEntity.FindComponent(azrtti_typeid<VisibleComponent>()), conflictingVisibleComponent);

        }
    }

    TEST_F(EntityTest_Scrubbing, InactiveEntityWithInvalidComponents_AreValidatedByPendingComponents)
    {
        // This test takes an entity with known invalid component setup that has not been activated
        // Adds pending components which will satisfy the invalid component setup
        // And expects scrub entities to succeed in this case.
        // Note - this is an edge case when deserializing a module entity or system entity from the app descriptor
        RegisterComponentDescriptor(LeatherBootsComponent::CreateDescriptor());
        RegisterComponentDescriptor(WoolSocksComponent::CreateDescriptor());
        RegisterComponentDescriptor(AzToolsFramework::Components::GenericComponentWrapper::CreateDescriptor());

        AZ::Entity* testEntity = aznew AZ::Entity("Test Scrubbing Entity");
        testEntity->CreateComponent<AzToolsFramework::Components::EditorPendingCompositionComponent>();
        testEntity->Init();  // init to kick off the pending composition request bus, but don't activate because we have invalid components

        EntityList entities;
        entities.push_back(testEntity);

        // Manually add boots component that requires the socks component, to simulate this being read out of app descriptor
        testEntity->CreateComponent<AzToolsFramework::Components::GenericComponentWrapper>(aznew LeatherBootsComponent());
        EntityCompositionRequests::ScrubEntitiesOutcome scrubResults = AZ::Failure<AZStd::string>("Didn't get called");
        EntityCompositionRequestBus::BroadcastResult(scrubResults, &EntityCompositionRequestBus::Events::ScrubEntities, entities);
        ASSERT_TRUE(scrubResults.IsSuccess());

        // If component is invalidated by scrubbing, it should now be in the pending set
        AZ::Entity::ComponentArrayType pendingComponents;
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(testEntity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
        ASSERT_EQ(pendingComponents.size(), 1);

        // the boots component should be flagged as invalid, since our entity was not activated
        // Note that this copies the invalidated / validated components array into resultForTestEntity, rather than references
        // as the reference will later become invalid as we reset scrubResults.
        EntityCompositionRequests::ScrubEntityResults resultForTestEntity = scrubResults.GetValue()[entities[0]->GetId()];
        ASSERT_EQ(resultForTestEntity.m_invalidatedComponents.size(), 1);

        // Don't actually want to keep the component in the pending set, so that we can validate the initial problem, so add it back onto the entity
        pendingComponents.clear();
        AZ::Component* invalidComponent = resultForTestEntity.m_invalidatedComponents[0];
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(testEntity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::RemovePendingComponent, invalidComponent);
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(testEntity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
        ASSERT_TRUE(pendingComponents.empty());
        delete invalidComponent;

        // now add a socks component to the pending set which will fulfill the boots' dependency
        testEntity->CreateComponent<AzToolsFramework::Components::GenericComponentWrapper>(aznew LeatherBootsComponent());
        WoolSocksComponent* woolSocksComponent = aznew WoolSocksComponent();
        woolSocksComponent->SetSerializedIdentifier("WoolSocksComponent"); // pending composition component cannot store an empty serialized identifier.
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(testEntity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::AddPendingComponent, woolSocksComponent);

        pendingComponents.clear();
        AzToolsFramework::EditorPendingCompositionRequestBus::Event(testEntity->GetId(), &AzToolsFramework::EditorPendingCompositionRequests::GetPendingComponents, pendingComponents);
        ASSERT_EQ(pendingComponents.size(), 1);

        scrubResults = AZ::Failure<AZStd::string>("Didn't get called");
        EntityCompositionRequestBus::BroadcastResult(scrubResults, &EntityCompositionRequestBus::Events::ScrubEntities, entities);
        ASSERT_TRUE(scrubResults.IsSuccess());

        resultForTestEntity = scrubResults.GetValue()[entities[0]->GetId()];
        ASSERT_TRUE(resultForTestEntity.m_invalidatedComponents.empty());

        // this should now succeed because the wool socks component is on the entity
        testEntity->Activate();
        ASSERT_EQ(testEntity->GetState(), Entity::State::Active);

        delete testEntity;
    }
} // namespace UnitTest
