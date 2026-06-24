/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Tests/SerializeContextFixture.h>

#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityActiveSystemComponent.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/Serialization/Utils.h>

namespace UnitTest
{
    // Used to verify that components are serialized on entities in a stable order, smallest to largest.
    // This component has the UUID with the smallest value.
    class SortOrderTestFirstComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SortOrderTestFirstComponent, "{00000000-0000-0000-0000-000000000010}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestFirstComponent, AZ::Component>(); 
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("SortOrderTestFirstService"));
        }
    };

    // This component has the UUID with the second smallest value.
    class SortOrderTestSecondComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SortOrderTestSecondComponent, "{00000000-0000-0000-0000-000000000020}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestSecondComponent, AZ::Component>();
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("SortOrderTestSecondService"));
        }
    };

    // This component has the UUID with the largest value.
    class SortOrderTestThirdComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SortOrderTestThirdComponent, "{00000000-0000-0000-0000-000000000030}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestThirdComponent, AZ::Component>();
            }
        }
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("SortOrderTestThirdService"));
        }
    };

    // Used to verify that components are always sorted after their dependencies.
    class SortOrderTestRequiresFirstComponent
        : public AZ::Component
    {
    public:
        // Purposely give this a UUID lower than its dependency.
        AZ_COMPONENT(SortOrderTestRequiresFirstComponent, "{00000000-0000-0000-0000-000000000001}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestRequiresFirstComponent, AZ::Component>();
            }
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("SortOrderTestFirstService"));
        }
    };

    // Used to verify that components are always sorted after their dependencies.
    class SortOrderTestRequiresSecondComponent
        : public AZ::Component
    {
    public:
        // Purposely give this a UUID lower than its dependency.
        AZ_COMPONENT(SortOrderTestRequiresSecondComponent, "{00000000-0000-0000-0000-000000000002}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestRequiresSecondComponent, AZ::Component>();
            }
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("SortOrderTestSecondService"));
        }
    };

    // Used to verify that components are always sorted after their dependencies.
    class SortOrderTestRequiresSecondAndThirdComponent
        : public AZ::Component
    {
    public:
        // Purposely give this a UUID between its dependencies.
        AZ_COMPONENT(SortOrderTestRequiresSecondAndThirdComponent, "{00000000-0000-0000-0000-000000000025}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestRequiresSecondAndThirdComponent, AZ::Component>();
            }
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("SortOrderTestThirdService"));
            services.push_back(AZ_CRC_CE("SortOrderTestSecondService"));
        }
    };

    // Used to verify that components that do not provide services are sorted after components that do.
    class SortOrderTestNoService
        : public AZ::Component
    {
    public:
        // Purposely give this a UUID lower than the test components that provide services.
        AZ_COMPONENT(SortOrderTestNoService, "{00000000-0000-0000-0000-000000000003}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestNoService, AZ::Component>();
            }
        }
    };

    // This component wraps the component base class, like GenericComponentWrapper.
    // GenericComponentWrapper is used in the editor for components that don't have specific editor representations.
    // Its usage depends on other editor systems being setup, so this is meant to simulate its usage.
    class SortOrderTestComponentWrapper
        : public AZ::Component
    {
    public:
        AZ_CLASS_ALLOCATOR(SortOrderTestComponentWrapper, AZ::SystemAllocator);
        AZ_RTTI(SortOrderTestComponentWrapper, "{00000000-0000-0000-0000-000000000011}", AZ::Component);

        SortOrderTestComponentWrapper() { }

        SortOrderTestComponentWrapper(AZ::Component* wrappedComponent)
        {
            if (wrappedComponent)
            {
                m_id = wrappedComponent->GetId();
                m_wrappedComponent = wrappedComponent;
            }
        }

        ~SortOrderTestComponentWrapper() override
        {
            delete m_wrappedComponent;
            m_wrappedComponent = nullptr;
        }

        AZ::Component* GetWrappedComponent() const { return m_wrappedComponent; }

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        AZ::TypeId GetUnderlyingComponentType() const override
        {
            if (m_wrappedComponent)
            {
                return m_wrappedComponent->RTTI_GetType();
            }

            return RTTI_GetType();
        }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<SortOrderTestComponentWrapper, AZ::Component>()
                    ->Field("m_wrappedComponent", &SortOrderTestComponentWrapper::m_wrappedComponent);
            }
        }

        static AZ::ComponentDescriptor* CreateDescriptor();

        // Provide a service so that this component sorts with all other components that provide services.
        // This is used to test that the wrapped component sorts with what it wraps, and it is used
        // here to wrap components that provide services.
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("TestWrapperService"));
        }
    private:
        AZ::Component* m_wrappedComponent = nullptr;
    };

    // This is based on GenericComponentWrapperDescriptor, and it is meant to pass through services.
    class SortOrderComponentWrapperDescriptor
        : public AZ::ComponentDescriptorHelper<SortOrderTestComponentWrapper>
    {
    public:
        AZ_CLASS_ALLOCATOR(SortOrderComponentWrapperDescriptor, AZ::SystemAllocator);
        AZ_TYPE_INFO(SortOrderComponentWrapperDescriptor, "{58A6544E-9476-4A93-AB6E-768B7326494B}");

        AZ::ComponentDescriptor* GetTemplateDescriptor(const AZ::Component* instance) const
        {
            AZ::ComponentDescriptor* templateDescriptor = nullptr;

            const SortOrderTestComponentWrapper* wrapper = azrtti_cast<const SortOrderTestComponentWrapper*>(instance);
            if (wrapper && wrapper->GetWrappedComponent())
            {
                AZ::ComponentDescriptorBus::EventResult(
                    templateDescriptor,
                    wrapper->GetWrappedComponent()->RTTI_GetType(),
                    &AZ::ComponentDescriptorBus::Events::GetDescriptor);
            }

            return templateDescriptor;
        }

        void Reflect(AZ::ReflectContext* reflection) const override
        {
            SortOrderTestComponentWrapper::Reflect(reflection);
        }

        void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided, const AZ::Component* instance) const override
        {
            const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
            if (templateDescriptor)
            {
                templateDescriptor->GetProvidedServices(provided, instance);
            }
        }

        void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent, const AZ::Component* instance) const override
        {
            const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
            if (templateDescriptor)
            {
                templateDescriptor->GetDependentServices(dependent, instance);
            }
        }

        void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required, const AZ::Component* instance) const override
        {
            const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
            if (templateDescriptor)
            {
                templateDescriptor->GetRequiredServices(required, instance);
            }
        }

        void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible, const AZ::Component* instance) const override
        {
            const AZ::ComponentDescriptor* templateDescriptor = GetTemplateDescriptor(instance);
            if (templateDescriptor)
            {
                templateDescriptor->GetIncompatibleServices(incompatible, instance);
            }
        }
    };

    AZ::ComponentDescriptor* SortOrderTestComponentWrapper::CreateDescriptor()
    {
        AZ::ComponentDescriptor* descriptor = nullptr;
        AZ::ComponentDescriptorBus::EventResult(
            descriptor, SortOrderTestComponentWrapper::RTTI_Type(), &AZ::ComponentDescriptorBus::Events::GetDescriptor);

        return descriptor ? descriptor : aznew SortOrderComponentWrapperDescriptor();
    }


    // Used to verify that components that accidentally provide the same service twice initialize correctly and ignore the duplication.
    class DuplicateProvidedServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(DuplicateProvidedServiceComponent, "{D39D65A9-6A26-40A6-99FB-586E3AC14B56}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<DuplicateProvidedServiceComponent, AZ::Component>();
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("DuplicatedService"));
            services.push_back(AZ_CRC_CE("DuplicatedService"));
        }
    };

    // Used to verify that components that accidentally provide the same service twice initialize correctly and ignore the duplication.
    // Previously, a crash occured when a component depended on a service that was provided multiple times by another component.
    class DependsOnDuplicateProvidedServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(DependsOnDuplicateProvidedServiceComponent, "{1B78B608-AECB-44CE-9060-53A1998AB1D4}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<DependsOnDuplicateProvidedServiceComponent, AZ::Component>();
            }
        }

        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("DuplicatedService"));
        }
    };

    // Used to verify that components that accidentally provide the same service twice initialize correctly and ignore the duplication.
    // Previously, a crash occured when a component required a service that was provided multiple times by another component.
    class RequiresDuplicateProvidedServiceComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(RequiresDuplicateProvidedServiceComponent, "{9AACE495-0E45-4DF0-B362-43CE12AE2F33}");

        ///////////////////////////////////////
        // Component overrides
        void Activate() override { }
        void Deactivate() override { }
        ///////////////////////////////////////

        static void Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<RequiresDuplicateProvidedServiceComponent, AZ::Component>();
            }
        }

        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("DuplicatedService"));
        }
    };

    class EntityTests : public UnitTest::SerializeContextFixture
    {
        void SetUp() override
        {
            SerializeContextFixture::SetUp();

            m_sortFirstDescriptor = SortOrderTestFirstComponent::CreateDescriptor();
            m_sortFirstDescriptor->Reflect(m_serializeContext);
            m_sortSecondDescriptor = SortOrderTestSecondComponent::CreateDescriptor();
            m_sortSecondDescriptor->Reflect(m_serializeContext);
            m_sortThirdDescriptor = SortOrderTestThirdComponent::CreateDescriptor();
            m_sortThirdDescriptor->Reflect(m_serializeContext);
            m_sortFirstDependencyDescriptor = SortOrderTestRequiresFirstComponent::CreateDescriptor();
            m_sortFirstDependencyDescriptor->Reflect(m_serializeContext);
            m_sortSecondDependencyDescriptor = SortOrderTestRequiresSecondComponent::CreateDescriptor();
            m_sortSecondDependencyDescriptor->Reflect(m_serializeContext);
            m_sortSecondAndThirdDependencyDescriptor = SortOrderTestRequiresSecondAndThirdComponent::CreateDescriptor();
            m_sortSecondAndThirdDependencyDescriptor->Reflect(m_serializeContext);
            m_sortWrapperDescriptor = SortOrderTestComponentWrapper::CreateDescriptor();
            m_sortWrapperDescriptor->Reflect(m_serializeContext);
            m_sortNoServiceDescriptor = SortOrderTestNoService::CreateDescriptor();
            m_sortNoServiceDescriptor->Reflect(m_serializeContext);

            m_duplicateProvidedServiceComponentDescriptor = DuplicateProvidedServiceComponent::CreateDescriptor();
            m_duplicateProvidedServiceComponentDescriptor->Reflect(m_serializeContext);
            m_dependsOnDuplicateServiceComponentDescriptor = DependsOnDuplicateProvidedServiceComponent::CreateDescriptor();
            m_dependsOnDuplicateServiceComponentDescriptor->Reflect(m_serializeContext);
            m_requiresDuplicateServiceComponentDescriptor = RequiresDuplicateProvidedServiceComponent::CreateDescriptor();
            m_requiresDuplicateServiceComponentDescriptor->Reflect(m_serializeContext);

            AZ::Entity::Reflect(m_serializeContext);
        }

        void TearDown() override
        {
            m_requiresDuplicateServiceComponentDescriptor->ReleaseDescriptor();
            m_dependsOnDuplicateServiceComponentDescriptor->ReleaseDescriptor();
            m_duplicateProvidedServiceComponentDescriptor->ReleaseDescriptor();

            m_sortNoServiceDescriptor->ReleaseDescriptor();
            m_sortWrapperDescriptor->ReleaseDescriptor();
            m_sortSecondAndThirdDependencyDescriptor->ReleaseDescriptor();
            m_sortSecondDependencyDescriptor->ReleaseDescriptor();
            m_sortFirstDependencyDescriptor->ReleaseDescriptor();
            m_sortThirdDescriptor->ReleaseDescriptor();
            m_sortSecondDescriptor->ReleaseDescriptor();
            m_sortFirstDescriptor->ReleaseDescriptor();

            UnitTest::SerializeContextFixture::TearDown();
        }

    protected:
        // Make sure the component list is sorted, and has the expected number of components.
        void ValidateComponentList(const AZ::Entity& entity1, const AZ::Entity& entity2, int expectedComponentListSize)
        {
            const AZ::Entity::ComponentArrayType& components1 = entity1.GetComponents();
            const AZ::Entity::ComponentArrayType& components2 = entity2.GetComponents();

            EXPECT_EQ(components1.size(), expectedComponentListSize);
            EXPECT_EQ(components2.size(), expectedComponentListSize);
            for (int i = 0; i < expectedComponentListSize; ++i)
            {
                EXPECT_EQ(components1[i]->GetUnderlyingComponentType(), components2[i]->GetUnderlyingComponentType());
            }
        }

        AZ::ComponentDescriptor* m_sortFirstDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortSecondDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortThirdDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortFirstDependencyDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortSecondDependencyDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortSecondAndThirdDependencyDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortWrapperDescriptor = nullptr;
        AZ::ComponentDescriptor* m_sortNoServiceDescriptor = nullptr;
        AZ::ComponentDescriptor* m_duplicateProvidedServiceComponentDescriptor = nullptr;
        AZ::ComponentDescriptor* m_dependsOnDuplicateServiceComponentDescriptor = nullptr;
        AZ::ComponentDescriptor* m_requiresDuplicateServiceComponentDescriptor = nullptr;
    };

    TEST_F(EntityTests, EntityComponentList_OutOfOrderUUIDs_ComponentListIsSorted)
    {
        AZ::Entity entity1;
        entity1.CreateComponent<SortOrderTestFirstComponent>();
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        entity1.CreateComponent<SortOrderTestThirdComponent>();
        entity1.EvaluateDependencies();

        AZ::Entity entity2;
        entity2.CreateComponent<SortOrderTestSecondComponent>();
        entity2.CreateComponent<SortOrderTestThirdComponent>();
        entity2.CreateComponent<SortOrderTestFirstComponent>();
        entity2.EvaluateDependencies();

        ValidateComponentList(entity1, entity2, 3);
    }

    TEST_F(EntityTests, EntityComponentList_OutOfOrderUUIDs_NoServiceComponentsAreSortedLast)
    {
        AZ::Entity entity1;
        entity1.CreateComponent<SortOrderTestFirstComponent>();
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        entity1.CreateComponent<SortOrderTestNoService>();
        entity1.EvaluateDependencies();

        AZ::Entity entity2;

        entity2.CreateComponent<SortOrderTestSecondComponent>();
        entity2.CreateComponent<SortOrderTestNoService>();
        entity2.CreateComponent<SortOrderTestFirstComponent>();
        entity2.EvaluateDependencies();

        ValidateComponentList(entity1, entity2, 3);
    }

    TEST_F(EntityTests, EntityComponentList_DuplicateAndOutOfOrderUUIDs_ComponentListIsSorted)
    {
        AZ::Entity entity1;
        entity1.CreateComponent<SortOrderTestFirstComponent>();
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        // Create a second copy of the second component, to verify that ordering works with duplicates.
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        entity1.CreateComponent<SortOrderTestThirdComponent>();
        entity1.EvaluateDependencies();

        AZ::Entity entity2;
        entity2.CreateComponent<SortOrderTestThirdComponent>();
        entity2.CreateComponent<SortOrderTestSecondComponent>();
        // Create a second copy of the second component, to verify that ordering works with duplicates.
        entity2.CreateComponent<SortOrderTestSecondComponent>();
        entity2.CreateComponent<SortOrderTestFirstComponent>();
        entity2.EvaluateDependencies();

        ValidateComponentList(entity1, entity2, 4);
    }

    // Verify that the entity's component sorting uses the correct ID, the GetUnderlyingComponentType instead
    // of the component's base ID. This ensures that component wrappers like GenericComponentWrapper
    // will sort based on what they hold and not their own ID.
    TEST_F(EntityTests, EntityComponentList_WrappedOutOfOrderUUIDs_ComponentListIsSorted)
    {
        AZ::Entity entity1;
        entity1.CreateComponent<SortOrderTestFirstComponent>();
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        entity1.CreateComponent<SortOrderTestThirdComponent>();
        entity1.EvaluateDependencies();

        AZ::Entity entity2;
        entity2.CreateComponent<SortOrderTestSecondComponent>();
        entity2.CreateComponent<SortOrderTestComponentWrapper>(aznew SortOrderTestThirdComponent());
        entity2.CreateComponent<SortOrderTestComponentWrapper>(aznew SortOrderTestFirstComponent());
        entity2.EvaluateDependencies();

        ValidateComponentList(entity1, entity2, 3);
    }

    TEST_F(EntityTests, EntityComponentList_OutOfOrderUUIDsWithDependencies_ComponentListIsSorted)
    {
        AZ::Entity entity1;
        entity1.CreateComponent<SortOrderTestFirstComponent>();
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        entity1.CreateComponent<SortOrderTestThirdComponent>();
        entity1.CreateComponent<SortOrderTestRequiresFirstComponent>();
        entity1.CreateComponent<SortOrderTestRequiresSecondComponent>();
        entity1.CreateComponent<SortOrderTestRequiresSecondAndThirdComponent>();
        entity1.EvaluateDependencies();

        AZ::Entity entity2;
        entity2.CreateComponent<SortOrderTestSecondComponent>();
        entity2.CreateComponent<SortOrderTestRequiresSecondAndThirdComponent>();
        entity2.CreateComponent<SortOrderTestRequiresSecondComponent>();
        entity2.CreateComponent<SortOrderTestThirdComponent>();
        entity2.CreateComponent<SortOrderTestRequiresFirstComponent>();
        entity2.CreateComponent<SortOrderTestFirstComponent>();
        entity2.EvaluateDependencies();

        ValidateComponentList(entity1, entity2, 6);
    }

    TEST_F(EntityTests, EntityComponentList_WrappedOutOfOrderUUIDsWithDependencies_ComponentListIsSorted)
    {
        AZ::Entity entity1;
        entity1.CreateComponent<SortOrderTestFirstComponent>();
        entity1.CreateComponent<SortOrderTestSecondComponent>();
        entity1.CreateComponent<SortOrderTestThirdComponent>();
        entity1.CreateComponent<SortOrderTestRequiresFirstComponent>();
        entity1.CreateComponent<SortOrderTestRequiresSecondComponent>();
        entity1.CreateComponent<SortOrderTestRequiresSecondAndThirdComponent>();
        entity1.EvaluateDependencies();

        AZ::Entity entity2;
        entity2.CreateComponent<SortOrderTestSecondComponent>();
        entity2.CreateComponent<SortOrderTestRequiresSecondAndThirdComponent>();
        entity2.CreateComponent<SortOrderTestRequiresSecondComponent>();
        entity2.CreateComponent<SortOrderTestComponentWrapper>(aznew SortOrderTestThirdComponent());
        entity2.CreateComponent<SortOrderTestComponentWrapper>(aznew SortOrderTestRequiresFirstComponent());
        entity2.CreateComponent<SortOrderTestFirstComponent>();
        entity2.EvaluateDependencies();

        ValidateComponentList(entity1, entity2, 6);
    }

    TEST_F(EntityTests, EntityComponentList_ComponentWithDuplicateProvidedService_EntityInitializesCorrectly)
    {
        AZ::Entity entity;
        DuplicateProvidedServiceComponent* duplicateServiceComponent = aznew DuplicateProvidedServiceComponent();
        entity.AddComponent(duplicateServiceComponent);
        // No test condition here, EvaluateDependencies was previously crashing when duplicate services were provided.
        // The crash would be caught by the unit test system.
        entity.EvaluateDependencies();
    }

    TEST_F(EntityTests, EntityComponentList_ComponentDependingOnComponentWithDuplicateProvidedService_EntityInitializesCorrectly)
    {
        AZ::Entity entity;
        DuplicateProvidedServiceComponent* duplicateServiceComponent = aznew DuplicateProvidedServiceComponent();
        DependsOnDuplicateProvidedServiceComponent* dependantService = aznew DependsOnDuplicateProvidedServiceComponent();
        entity.AddComponent(duplicateServiceComponent);
        entity.AddComponent(dependantService);
        // No test condition here, EvaluateDependencies was previously crashing when duplicate services were provided.
        // The crash would be caught by the unit test system.
        entity.EvaluateDependencies();
    }

    TEST_F(EntityTests, EntityComponentList_ComponentRequiringComponentWithDuplicateProvidedService_EntityInitializesCorrectly)
    {
        AZ::Entity entity;
        DuplicateProvidedServiceComponent* duplicateServiceComponent = aznew DuplicateProvidedServiceComponent();
        RequiresDuplicateProvidedServiceComponent* dependantService = aznew RequiresDuplicateProvidedServiceComponent();
        entity.AddComponent(duplicateServiceComponent);
        entity.AddComponent(dependantService);
        // No test condition here, EvaluateDependencies was previously crashing when duplicate services were provided.
        // The crash would be caught by the unit test system.
        entity.EvaluateDependencies();
    }

    TEST_F(EntityTests, EntityIsMoveConstructed)
    {
        static_assert(!AZStd::is_copy_constructible<AZ::Entity>::value, "Entity is dangerous to copy construct.");
        static_assert(!AZStd::is_copy_assignable<AZ::Entity>::value, "Entity is dangerous to copy assign.");

        {
            AZ::Entity entity1;
            entity1.CreateComponent<SortOrderTestFirstComponent>();
            AZ::Entity entity2(AZStd::move(entity1));
            EXPECT_EQ(entity1.GetComponents().size(), 0);
            EXPECT_EQ(entity2.GetComponents().size(), 1);
        } // there will be a crash here if they go out of scope if they weren't properly moved.
    }

    TEST_F(EntityTests, EntityIsMoveAssigned)
    {
        {
            AZ::Entity entity1;
            entity1.CreateComponent<SortOrderTestFirstComponent>();
            AZ::Entity entity2;
            entity2 = AZStd::move(entity1);
            EXPECT_EQ(entity1.GetComponents().size(), 0);
            EXPECT_EQ(entity2.GetComponents().size(), 1);
        } // there will be a crash here if they go out of scope if they weren't properly moved.
    }

    // Must match Entity::s_maxStateFlags / EntityActiveSystemComponent::s_maxStateFlags. (If the
    // 16/16 footprint fold ever lands, update this to the new activation-layer capacity.)
    static constexpr size_t s_activationMaxStateFlags = 32;

    // EntityActiveSystemComponent hands out one bitmask index per registered active-type name, with
    // index 0 pre-reserved for "Entity". Verifies registration, the cap, and - importantly - that an
    // already-registered name still resolves once the registry is full.
    TEST_F(EntityTests, EntityActiveSystemComponent_RegistersTypesAndResolvesWhenFull)
    {
        AZ::EntityActiveSystemComponent activeSystem;

        // "Entity" is pre-registered at index 0.
        EXPECT_EQ(activeSystem.GetActiveTypeIndexByName("Entity"), 0);

        // With "Entity" occupying slot 0, there are (max - 1) further slots: indices 1 .. max-1.
        for (size_t i = 1; i < s_activationMaxStateFlags; ++i)
        {
            const AZStd::string name = AZStd::string::format("Active%d", static_cast<int>(i));
            EXPECT_EQ(activeSystem.GetActiveTypeIndexByName(name), i);
        }

        // The registry is now full - registering one more NEW name must fail.
        EXPECT_EQ(
            activeSystem.GetActiveTypeIndexByName("ActiveOverflow"),
            AZ::EntityActiveSystemComponent::kInvalidIndex);

        // ...but every already-registered name must still resolve to its index even when full.
        EXPECT_EQ(activeSystem.GetActiveTypeIndexByName("Entity"), 0);
        for (size_t i = 1; i < s_activationMaxStateFlags; ++i)
        {
            const AZStd::string name = AZStd::string::format("Active%d", static_cast<int>(i));
            EXPECT_EQ(activeSystem.GetActiveTypeIndexByName(name), i);
        }

        // Re-querying an existing name does not consume a slot or change its index.
        EXPECT_EQ(activeSystem.GetActiveTypeIndexByName("Active1"), 1);
    }

    // Entity packs an "active by type" bitmask; the entity is effectively active only when every
    // layer is set. SetEffectiveActiveLayerByTypeIndex returns whether the EFFECTIVE active state
    // flipped (Active<->Inactive), not merely whether a bit changed.
    TEST_F(EntityTests, Entity_ActiveLayerBitmask_EffectiveStateTransitions)
    {
        AZ::Entity entity;
        EXPECT_TRUE(entity.IsEffectivelyActive()); // all layers on by default

        // Toggling a single layer off then on flips effective state both ways (it is the only change).
        for (size_t i = 0; i < s_activationMaxStateFlags; ++i)
        {
            EXPECT_TRUE(entity.SetEffectiveActiveLayerByTypeIndex(i, false)); // Active -> Inactive
            EXPECT_FALSE(entity.IsEffectivelyActive());
            EXPECT_TRUE(entity.SetEffectiveActiveLayerByTypeIndex(i, true));  // Inactive -> Active
            EXPECT_TRUE(entity.IsEffectivelyActive());
        }

        // An index at/over the cap is rejected (no change, returns false); out-of-range reads as active.
        // (SetEffectiveActiveLayerByTypeIndex emits an informational AZ_Warning here; it does not fail.)
        EXPECT_FALSE(entity.SetEffectiveActiveLayerByTypeIndex(s_activationMaxStateFlags, false));
        EXPECT_TRUE(entity.IsEffectivelyActive());
        EXPECT_TRUE(entity.GetEffectiveActiveLayerByTypeIndex(s_activationMaxStateFlags));

        // Multiple layers off: only the FIRST clear flips effective state; the rest are already inactive.
        EXPECT_TRUE(entity.SetEffectiveActiveLayerByTypeIndex(3, false));   // Active -> Inactive
        EXPECT_FALSE(entity.SetEffectiveActiveLayerByTypeIndex(7, false));  // still inactive
        EXPECT_FALSE(entity.SetEffectiveActiveLayerByTypeIndex(15, false)); // still inactive
        EXPECT_FALSE(entity.IsEffectivelyActive());
        EXPECT_FALSE(entity.GetEffectiveActiveLayerByTypeIndex(7));

        // Re-enabling: only the LAST re-enable restores effective active.
        EXPECT_FALSE(entity.SetEffectiveActiveLayerByTypeIndex(3, true));  // 7,15 still off
        EXPECT_FALSE(entity.SetEffectiveActiveLayerByTypeIndex(7, true));  // 15 still off
        EXPECT_TRUE(entity.SetEffectiveActiveLayerByTypeIndex(15, true));  // now all on
        EXPECT_TRUE(entity.IsEffectivelyActive());

        // SetEntityActive is the index-0 ("Entity") wrapper.
        EXPECT_TRUE(entity.SetEntityActive(false));
        EXPECT_FALSE(entity.IsEffectivelyActive());
        EXPECT_TRUE(entity.SetEntityActive(true));
        EXPECT_TRUE(entity.IsEffectivelyActive());
    }

    // Records the entity's State at the instant each lifecycle notification fires. Connect while the
    // entity is still Constructed so the EntityBus connection policy (which replays OnEntityExists/
    // OnEntityActivated for an already-registered Init/Active entity) has nothing to replay here.
    class LifecycleStateProbe
        : public AZ::EntityBus::Handler
    {
    public:
        explicit LifecycleStateProbe(AZ::Entity* entity)
            : m_entity(entity)
        {
            AZ::EntityBus::Handler::BusConnect(entity->GetId());
        }
        ~LifecycleStateProbe() override
        {
            AZ::EntityBus::Handler::BusDisconnect();
        }

        void OnEntityExists(const AZ::EntityId&) override      { m_stateAtExists = m_entity->GetState();      ++m_existsCount; }
        void OnEntityActivated(const AZ::EntityId&) override   { m_stateAtActivated = m_entity->GetState();   ++m_activatedCount; }
        void OnEntityDeactivated(const AZ::EntityId&) override { m_stateAtDeactivated = m_entity->GetState(); ++m_deactivatedCount; }

        AZ::Entity* m_entity = nullptr;
        AZ::Entity::State m_stateAtExists = AZ::Entity::State::Constructed;
        AZ::Entity::State m_stateAtActivated = AZ::Entity::State::Constructed;
        AZ::Entity::State m_stateAtDeactivated = AZ::Entity::State::Constructed;
        int m_existsCount = 0;
        int m_activatedCount = 0;
        int m_deactivatedCount = 0;
    };

    // Entity lifecycle notifications must observe the TRANSITION state, not the settled state, so a
    // listener can tell "transitioning now" apart from "already settled" when it gates work on
    // GetState() (the pattern TransformComponent relies on). Init/Activate set the settled state
    // AFTER notifying; Deactivate sets Deactivating BEFORE notifying. Guards the AZ::Entity ordering
    // from PR #19856 - this FAILS on a tree without that ordering fix, which is the intended alarm.
    TEST_F(EntityTests, EntityLifecycle_NotificationsObserveTransitionState)
    {
        AZ::Entity entity("LifecycleProbe");
        LifecycleStateProbe probe(&entity); // connect while Constructed -> no connection-policy replay

        entity.Init();
        EXPECT_EQ(probe.m_existsCount, 1);
        EXPECT_EQ(probe.m_stateAtExists, AZ::Entity::State::Initializing); // not Init
        EXPECT_EQ(entity.GetState(), AZ::Entity::State::Init);             // settles after notify

        entity.Activate();
        EXPECT_EQ(probe.m_activatedCount, 1);
        EXPECT_EQ(probe.m_stateAtActivated, AZ::Entity::State::Activating); // not Active
        EXPECT_EQ(entity.GetState(), AZ::Entity::State::Active);

        entity.Deactivate();
        EXPECT_EQ(probe.m_deactivatedCount, 1);
        EXPECT_EQ(probe.m_stateAtDeactivated, AZ::Entity::State::Deactivating); // not Active
        EXPECT_EQ(entity.GetState(), AZ::Entity::State::Init);              // Deactivate returns to Init
    }
} // namespace UnitTest
