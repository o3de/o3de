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
#include <AzToolsFramework/UnitTest/ToolsTestApplication.h>

// Inspector Test Includes
#include <AzToolsFramework/UI/ComponentPalette/ComponentPaletteUtil.hxx>

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
            services.push_back(AZ_CRC("InspectorTestService1"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("InspectorTestService1"));
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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("Game"))
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
            services.push_back(AZ_CRC("InspectorTestService2"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("InspectorTestService2"));
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
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
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
            services.push_back(AZ_CRC("InspectorTestService3"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& services)
        {
            services.push_back(AZ_CRC("InspectorTestService3"));
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
        : public AllocatorsTestFixture
    {
    public:
        ComponentPaletteTests()
            : AllocatorsTestFixture()
        { }

        void SetUp() override
        {
            AZ::ComponentApplication::Descriptor componentApplicationDesc;
            componentApplicationDesc.m_useExistingAllocator = true;
            m_application = aznew ToolsTestApplication("ComponentPaletteTests");
            m_application->Start(componentApplicationDesc);
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
}
