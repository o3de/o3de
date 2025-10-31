/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// Test Environment
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/UserSettings/UserSettingsComponent.h>
#include <AzToolsFramework/Application/ToolsApplication.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Entity/EditorEntityHelpers.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponent.h>
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>
#include <AzToolsFramework/ToolsComponents/EditorInspectorComponentBus.h>

// Inspector Test Includes
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>

#include <gmock/gmock.h>


namespace UnitTest
{
    // Test component that is NOT available for a user to interact with
    // It does not appear in the Add Component menu in the Editor
    // It is not a system or game component
    class Inspector_TestComponent1
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(Inspector_TestComponent1, "{BD25A077-DF38-4B67-BEA5-F4587A747A36}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Inspector_TestComponent1, AZ::Component>()
                    ->Field("Data", &Inspector_TestComponent1::m_data)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Inspector_TestComponent1>("InspectorTestComponent1", "Component 1 for AZ Tools Framework Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, false)
                        ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::Hide)
                        ->Attribute(AZ::Edit::Attributes::SliceFlags, AZ::Edit::SliceFlags::NotPushable)
                        ->Attribute(AZ::Edit::Attributes::HideIcon, true);
                }
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("InspectorTestService1"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("InspectorTestService1"));
        }

        ~Inspector_TestComponent1() override
        {
        }

        void SetData(int data)
        {
            m_data = data;
        };

        int GetData()
        {
            return m_data;
        }

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}

        /// Whether this entity is locked
        int m_data = 0;
    };

    // Test component that IS available for a user to interact with
    // It does appear in the Add Component menu in the editor and is a game component
    class Inspector_TestComponent2
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(Inspector_TestComponent2, "{57D1C818-FD31-4FCD-A4DB-705EABF4E98B}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Inspector_TestComponent2, AZ::Component>()
                    ->Field("Data", &Inspector_TestComponent2::m_data)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Inspector_TestComponent2>("InspectorTestComponent2", "Component 2 for AZ Tools Framework Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                        ->Attribute(AZ::Edit::Attributes::Category, "Inspector Test Components")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Inspector_TestComponent2::m_data, "Data", "The component's Data");
                }
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("InspectorTestService2"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("InspectorTestService2"));
        }

        ~Inspector_TestComponent2() override
        {
        }

        void SetData(int data)
        {
            m_data = data;
        };

        int GetData()
        {
            return m_data;
        }

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}

        /// Whether this entity is locked
        int m_data = 0;
    };

    // Test component that IS available for a user to interact with
    // It does appear in an Add Component menu and is a system component
    class Inspector_TestComponent3
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(Inspector_TestComponent3, "{552CCFB1-135E-4B02-A492-25A3BBDFA381}", AZ::Component);
        static void Reflect(AZ::ReflectContext* context)
        {
            if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<Inspector_TestComponent3, AZ::Component>()
                    ->Field("Data", &Inspector_TestComponent3::m_data)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Inspector_TestComponent3>("InspectorTestComponent3", "Component 3 for AZ Tools Framework Unit Tests")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                        ->Attribute(AZ::Edit::Attributes::Category, "Inspector Test Components")
                        ->Attribute(AZ::Edit::Attributes::Icon, "Icons/Components/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::ViewportIcon, "Icons/Components/Viewport/Tag.png")
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                        ->Attribute(AZ::Edit::Attributes::HelpPageURL, "https://o3de.org/docs/user-guide/components")
                        ->DataElement(AZ::Edit::UIHandlers::Default, &Inspector_TestComponent3::m_data, "Data", "The component's Data");
                }
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("InspectorTestService3"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC_CE("InspectorTestService3"));
        }

        ~Inspector_TestComponent3() override
        {
        }

        void SetData(int data)
        {
            m_data = data;
        };

        int GetData()
        {
            return m_data;
        }

    private:
        void Init() override
        {}

        void Activate() override
        {}

        void Deactivate() override
        {}

        /// Whether this entity is locked
        int m_data = 0;
    };

    // Component Filters for Testing
    bool Filter_IsTestComponent1(const AZ::SerializeContext::ClassData& classData)
    {
        AZ::Uuid testComponent1_typeId = azrtti_typeid<Inspector_TestComponent1>();
        return classData.m_typeId == testComponent1_typeId;
    }

    // Component Filters for Testing
    bool Filter_IsTestComponent2(const AZ::SerializeContext::ClassData& classData)
    {
        AZ::Uuid testComponent2_typeId = azrtti_typeid<Inspector_TestComponent2>();
        return classData.m_typeId == testComponent2_typeId;
    }

    // Component Filters for Testing
    bool Filter_IsTestComponent3(const AZ::SerializeContext::ClassData& classData)
    {
        AZ::Uuid testComponent3_typeId = azrtti_typeid<Inspector_TestComponent2>();
        return classData.m_typeId == testComponent3_typeId;
    }

    class ComponentPaletteTests
        : public LeakDetectionFixture
    {
    public:
        ComponentPaletteTests()
            : LeakDetectionFixture()
        { }

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor componentApplicationDesc;
            componentApplicationDesc.m_useExistingAllocator = true;
            m_application = aznew ToolsTestApplication("ComponentPaletteTests");
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            startupParameters.m_loadAssetCatalog = false;
            m_application->Start(componentApplicationDesc, startupParameters);
            // Without this, the user settings component would attempt to save on finalize/shutdown. Since the file is
            // shared across the whole engine, if multiple tests are run in parallel, the saving could cause a crash 
            // in the unit tests.
            AZ::UserSettingsComponentRequestBus::Broadcast(&AZ::UserSettingsComponentRequests::DisableSaveOnFinalize);
        }

        void TearDown() override
        {
            // Release all slice asset references, so AssetManager doens't complain.
            delete m_application;
        }

    public:
        ToolsTestApplication* m_application = nullptr;
    };

    // Test pushing slices to create news slices that could result in cyclic 
    // dependency, e.g. push slice1 => slice2 and slice2 => slice1 at the same
    // time.
    TEST_F(ComponentPaletteTests, TestComponentPalleteUtilities)
    {
        AZ::SerializeContext* context = m_application->GetSerializeContext();

        // Register our test components (This process also reflects them to the appropriate contexts)
        auto* Inspector_TestComponent1Descriptor = Inspector_TestComponent1::CreateDescriptor();
        auto* Inspector_TestComponent2Descriptor = Inspector_TestComponent2::CreateDescriptor();
        auto* Inspector_TestComponent3Descriptor = Inspector_TestComponent3::CreateDescriptor();

        m_application->RegisterComponentDescriptor(Inspector_TestComponent1Descriptor);
        m_application->RegisterComponentDescriptor(Inspector_TestComponent2Descriptor);
        m_application->RegisterComponentDescriptor(Inspector_TestComponent3Descriptor);

        AZ::Uuid testComponent1_typeId = azrtti_typeid<Inspector_TestComponent1>();
        AZ::Uuid testComponent2_typeId = azrtti_typeid<Inspector_TestComponent2>();

        //////////////////////////////////////////////////////////////////////////
        // TEST OffersRequiredServices()
        //////////////////////////////////////////////////////////////////////////

        // Verify that OffersRequiredServices returns true with the services provided by the component.
        AZ::ComponentDescriptor::DependencyArrayType testComponent1_ProvidedServices;
        Inspector_TestComponent1::GetProvidedServices(testComponent1_ProvidedServices);
        AZ_TEST_ASSERT(testComponent1_ProvidedServices.size() == 1);
        const AZ::SerializeContext::ClassData* testComponent1_ClassData = context->FindClassData(testComponent1_typeId);

        EXPECT_TRUE(AzToolsFramework::OffersRequiredServices(testComponent1_ClassData, testComponent1_ProvidedServices));

        // Verify that OffersRequiredServices returns when given services provided by a different component
        AZ::ComponentDescriptor::DependencyArrayType testComponent2_ProvidedServices;
        Inspector_TestComponent2::GetProvidedServices(testComponent2_ProvidedServices);
        AZ_TEST_ASSERT(testComponent2_ProvidedServices.size() == 1);
        AZ_TEST_ASSERT(testComponent1_ProvidedServices != testComponent2_ProvidedServices);
        EXPECT_FALSE(AzToolsFramework::OffersRequiredServices(testComponent1_ClassData, testComponent2_ProvidedServices));

        // verify that OffersRequiredServices returns true when provided with an empty list of services
        EXPECT_TRUE(AzToolsFramework::OffersRequiredServices(testComponent1_ClassData, AZ::ComponentDescriptor::DependencyArrayType()));

        //////////////////////////////////////////////////////////////////////////
        // TEST IsAddableByUser()
        //////////////////////////////////////////////////////////////////////////

        // Verify that IsAddableByUser returns false when given a component that is not editable or viewable by the user
        EXPECT_FALSE(AzToolsFramework::ComponentPaletteUtil::IsAddableByUser(testComponent1_ClassData));

        // Verify that IsAddableByUser returns true when given a component that has the appropriate edit context reflection
        const AZ::SerializeContext::ClassData* testComponent2_ClassData = context->FindClassData(testComponent2_typeId);
        EXPECT_TRUE(AzToolsFramework::ComponentPaletteUtil::IsAddableByUser(testComponent2_ClassData));

        //////////////////////////////////////////////////////////////////////////
        // TEST ContainsEditableComponents()
        //////////////////////////////////////////////////////////////////////////

        // Remove reflection of Test Component 2 for the first test
        m_application->UnregisterComponentDescriptor(Inspector_TestComponent2Descriptor);
        context->EnableRemoveReflection();
        Inspector_TestComponent2::Reflect(context);
        context->DisableRemoveReflection();

        // Verify that there are no components that satisfy the AppearsInGameComponentMenu filter without service dependency conditions
        EXPECT_FALSE(AzToolsFramework::ComponentPaletteUtil::ContainsEditableComponents(context, &Filter_IsTestComponent2, AZ::ComponentDescriptor::DependencyArrayType()));

        // Reflect Test Component 2 for subsequent tests
        m_application->RegisterComponentDescriptor(Inspector_TestComponent2Descriptor);

        // Verify that there is now a component that satisfies the AppearsInGameComponentMenu filter without service dependency conditions
        EXPECT_TRUE(AzToolsFramework::ComponentPaletteUtil::ContainsEditableComponents(context, &Filter_IsTestComponent2, AZ::ComponentDescriptor::DependencyArrayType()));

        // Verify that true is returned here because test component 2 is editable and provides test component 2 services
        EXPECT_TRUE(AzToolsFramework::ComponentPaletteUtil::ContainsEditableComponents(context, &Filter_IsTestComponent2, testComponent2_ProvidedServices));

        // Verify that false is returned here because test component 2 does not provide any of the required services
        EXPECT_FALSE(AzToolsFramework::ComponentPaletteUtil::ContainsEditableComponents(context, &Filter_IsTestComponent2, testComponent1_ProvidedServices));

        // Verify that even though Test Component 1 exists and is returned by the filter and there are no services to match, false is returned
        // because Test Component 1 is not editable.
        EXPECT_FALSE(AzToolsFramework::ComponentPaletteUtil::ContainsEditableComponents(context, &Filter_IsTestComponent1, AZ::ComponentDescriptor::DependencyArrayType()));

        // Verify that true is returned here when a system component is editable
        EXPECT_TRUE(AzToolsFramework::ComponentPaletteUtil::ContainsEditableComponents(context, &Filter_IsTestComponent3, AZ::ComponentDescriptor::DependencyArrayType()));
    }

    // helperfunction to reflect serialize for all these components to keep code short
    template<typename ComponentType>
    void RegisterSerialize(AZ::ReflectContext* context, bool visible, const char* iconPath, int fixedIndex = -1)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<ComponentType, AZ::Component>();

            if (AZ::EditContext* editContext = serializeContext->GetEditContext())
            {
                auto reflectionBuilder = editContext->Class<ComponentType>(AZ_STRINGIZE(ComponentType), AZ_STRINGIZE(ComponentType));
                auto classElement = reflectionBuilder->ClassElement(AZ::Edit::ClassElements::EditorData, "");
                classElement->Attribute(AZ::Edit::Attributes::AddableByUser, true)
                    ->Attribute(AZ::Edit::Attributes::Visibility, visible ? AZ::Edit::PropertyVisibility::Show :  AZ::Edit::PropertyVisibility::Hide)
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("Game"))
                    ->Attribute(AZ::Edit::Attributes::Category, "Inspector Test Components")
                    ->Attribute(AZ::Edit::Attributes::Icon, iconPath)
                    ->Attribute(AZ::Edit::Attributes::ViewportIcon, iconPath);

                if (fixedIndex != -1)
                {
                    classElement->Attribute(AZ::Edit::Attributes::FixedComponentListIndex, fixedIndex);
                }
            }
        }
    }
    
    class EditorInspectorTestComponentBase : public AZ::Component
    {
    public:
        // These functions are mandatory to provide but are of no use in this case.
        void Activate() override {}
        void Deactivate() override {}
    };
    
    class EditorInspectorTestComponent1 : public EditorInspectorTestComponentBase
    {
        public:
            AZ_COMPONENT(EditorInspectorTestComponent1, "{EF3D8047-4FAA-4615-93E1-C2B5B6EB3C08}", AZ::Component);
            static void Reflect(AZ::ReflectContext* context)
            {
                // A component that is user movable and is visible
                RegisterSerialize<EditorInspectorTestComponent1>(context, true, "Component1.png");
            }
    };

    class EditorInspectorTestComponent2 : public EditorInspectorTestComponentBase
    {
        public:
            AZ_COMPONENT(EditorInspectorTestComponent2, "{42BE5BEE-A7B9-4D8D-8F61-C0E0FDAA1450}", AZ::Component);
            static void Reflect(AZ::ReflectContext* context)
            {
                // A component that is not movable, but is visible
                RegisterSerialize<EditorInspectorTestComponent2>(context, true, "Component2.png", 0);
            }
    };

    class EditorInspectorTestComponent3 : public EditorInspectorTestComponentBase
    {
        public:
            AZ_COMPONENT(EditorInspectorTestComponent3, "{71329B94-76B3-4C8B-AF4B-159D51BDE820}", AZ::Component);
            static void Reflect(AZ::ReflectContext* context)
            {
                // A component that is not visible
                RegisterSerialize<EditorInspectorTestComponent3>(context, false, "Component3.png");
            }
    };

    class EditorInspectorTestComponent4 : public EditorInspectorTestComponentBase
    {
        public:
            AZ_COMPONENT(EditorInspectorTestComponent4, "{10385AEF-88AA-4682-AF1E-3EBE21E4632B}", AZ::Component);
            static void Reflect(AZ::ReflectContext* context)
            {
                // Another component that is visible and movable
                RegisterSerialize<EditorInspectorTestComponent4>(context, true, "Component4.png");
            }
    };
    
    class MockEditorInspectorNotificationBusHandler : public AzToolsFramework::EditorInspectorComponentNotificationBus::Handler
    {
    public:
        MOCK_METHOD0(OnComponentOrderChanged, void());
    };

    class InspectorComponentOrderingTest
        : public ComponentPaletteTests
    {
        void SetUp() override
        {
            ComponentPaletteTests::SetUp();
            m_application->RegisterComponentDescriptor(EditorInspectorTestComponent1::CreateDescriptor());
            m_application->RegisterComponentDescriptor(EditorInspectorTestComponent2::CreateDescriptor());
            m_application->RegisterComponentDescriptor(EditorInspectorTestComponent3::CreateDescriptor());
            m_application->RegisterComponentDescriptor(EditorInspectorTestComponent4::CreateDescriptor());
            m_mockedInspectorBusHandler = AZStd::make_unique<::testing::NiceMock<MockEditorInspectorNotificationBusHandler>>();
        }

        void TearDown() override
        {
            m_mockedInspectorBusHandler->BusDisconnect();
            m_mockedInspectorBusHandler.reset();
            ComponentPaletteTests::TearDown();
        }

        protected:
        AZStd::unique_ptr<::testing::NiceMock<MockEditorInspectorNotificationBusHandler>> m_mockedInspectorBusHandler;
    };

    // Makes sure that the inspector component (responsible for keeping track of any order overrides of components on it)
    // only stores data and only emits events when the components are in a non default order.
    // Also makes sure (since it invokes them) that the actual ordering utility functions, such as RemoveHiddenComponents,
    // SortComponentsByPriority, and the functions they call, all work as expected.
    TEST_F(InspectorComponentOrderingTest, AddingComponents_InspectorComponent_PersistsDataOnlyIfDifferentFromDefault)
    {
        using namespace AzToolsFramework;

        AZ::EntityId entityId(123);

        AZ::Entity testEntity(entityId);
        testEntity.AddComponent(aznew EditorInspectorTestComponent1);
        testEntity.AddComponent(aznew EditorInspectorTestComponent2);
        testEntity.AddComponent(aznew EditorInspectorTestComponent3);
        testEntity.AddComponent(aznew EditorInspectorTestComponent4);
        testEntity.AddComponent(aznew Components::EditorInspectorComponent);

        m_mockedInspectorBusHandler->BusConnect(entityId);

        // activating the entity should not invoke the component order change bus at all, anything that cares about activation should listen for activation, not reorder.
        EXPECT_CALL(*m_mockedInspectorBusHandler, OnComponentOrderChanged()).Times(0);
        
        // Activating an entity does reorder the actual components on the entity itself.
        // They will not be in the order added.
        // The actual order on the entity is not relevant to this test, but the stable sort function itself will place
        // the components that provide services (EditorInspectorComponent) in this case, ahead of ones which don't, and
        // if there is a tie, it will sort them by their typeid (their GUID).  In this case, it means the order will be:
        // * EditorInspectorComponent (because it has services provided)
        // * EditorInspectorTestComponent4 // TypeID starts with 10385AEF
        // * EditorInspectorTestComponent2 // TypeID starts with 42BE5BEE
        // * EditorInspectorTestComponent3 // TypeID starts with 71329B94
        // * EditorInspectorTestComponent1 // TypeID starts with EF3D8047

        testEntity.Init();
        testEntity.Activate();

        AZ::Entity::ComponentArrayType componentsOnEntity = testEntity.GetComponents();

        EXPECT_EQ(componentsOnEntity.size(), 5);

        // An empty component order array sent to an already empty entity should result in no callbacks.
        ComponentOrderArray componentOrderArray;
        EditorInspectorComponentRequestBus::Event(entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, componentOrderArray);
        EditorInspectorComponentRequestBus::EventResult(componentOrderArray, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);
        EXPECT_TRUE(componentOrderArray.empty());

        // Setting an empty component order when its already empty should result in no calls to the "Component order changed!" event.
        EXPECT_CALL(*m_mockedInspectorBusHandler, OnComponentOrderChanged()).Times(0);

        // Setting the component order array to what is already the default order should result in no callbacks:
        AZ::Entity::ComponentArrayType components;
        components = testEntity.GetComponents();
        EXPECT_EQ(components.size(), 5);
        RemoveHiddenComponents(components);
        EXPECT_EQ(components.size(), 3); // the inspector component and the test Component 3 are hidden.
        SortComponentsByPriority(components);
        ASSERT_EQ(components.size(), 3); // sorting components should not change the number of components.

        // After the sort, the first one in the array should be the fixed order one, that says it "must be in position 0"
        EXPECT_EQ(components[0]->RTTI_GetType(), azrtti_typeid<EditorInspectorTestComponent2>());

        // the others should remain in their original order, but be after it:
        EXPECT_EQ(components[1]->RTTI_GetType(), azrtti_typeid<EditorInspectorTestComponent4>()); // Note the above, 4 comes before 1 due to the stable sort
        EXPECT_EQ(components[2]->RTTI_GetType(), azrtti_typeid<EditorInspectorTestComponent1>());

        // convert the vector of Component* to a vector of ComponentId
        ComponentOrderArray defaultComponentOrder;
        for (const AZ::Component* component : components)
        {
            defaultComponentOrder.push_back(component->GetId());
        }
        
        // set the order.  Since its the default order, this should again not emit an event, nor update any data that would be persisted:
        EditorInspectorComponentRequestBus::Event(entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, defaultComponentOrder);
        EditorInspectorComponentRequestBus::EventResult(componentOrderArray, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);
        EXPECT_TRUE(componentOrderArray.empty());

        // Setting the component order array to a different order than default should result in a callback and result in data to save.
        // in this case, we swap the order of element [1] and [2], so that the final order should be [Component2, Component1, Component4]
        ComponentOrderArray nonDefaultOrder = defaultComponentOrder;
        AZStd::iter_swap(nonDefaultOrder.begin() + 1, nonDefaultOrder.begin() + 2);
        EXPECT_CALL(*m_mockedInspectorBusHandler, OnComponentOrderChanged()).Times(1);
        EditorInspectorComponentRequestBus::Event(entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, nonDefaultOrder);
        EditorInspectorComponentRequestBus::EventResult(componentOrderArray, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);
        EXPECT_EQ(componentOrderArray.size(), 3);
        EXPECT_EQ(componentOrderArray, nonDefaultOrder);

        // Setting the component order array back to default, should result in it emptying it out and notifying since its changing (from non-default to default)
        EXPECT_CALL(*m_mockedInspectorBusHandler, OnComponentOrderChanged()).Times(1);
        EditorInspectorComponentRequestBus::Event(entityId, &EditorInspectorComponentRequests::SetComponentOrderArray, defaultComponentOrder);
        EditorInspectorComponentRequestBus::EventResult(componentOrderArray, entityId, &EditorInspectorComponentRequests::GetComponentOrderArray);
        EXPECT_TRUE(componentOrderArray.empty());

        m_mockedInspectorBusHandler->BusDisconnect();
        testEntity.Deactivate();
    }
}
