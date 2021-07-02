/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Serialization/Json/JsonSystemComponent.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzTest/AzTest.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Editor/Attribution/AWSCoreAttributionManager.h>
#include <Editor/Attribution/AWSCoreAttributionSystemComponent.h>
#include <TestFramework/AWSCoreFixture.h>

using namespace AWSCore;

namespace AWSCoreUnitTest
{
    class AWSCoreSystemComponentMock : public AZ::Component
    {
    public:
        AZ_COMPONENT(AWSCoreSystemComponentMock, "{5F48030D-EB59-4820-BC65-69EC7CC6C119}");

        static void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<AWSCoreSystemComponentMock, AZ::Component>()->Version(0);

                if (AZ::EditContext* ec = serialize->GetEditContext())
                {
                    ec->Class<AWSCoreSystemComponentMock>("AWSCoreMock", "Adds core support for working with AWS")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true);
                }
            }
        }

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("AWSCoreService"));
        }

        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            AZ_UNUSED(incompatible);
        }
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            AZ_UNUSED(required);
        }
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        ~AWSCoreSystemComponentMock() = default;

        MOCK_METHOD0(Init, void());
        MOCK_METHOD0(Activate, void());
        MOCK_METHOD0(Deactivate, void());
    };

    class AWSAttributionSystemComponentTest
        : public AWSCoreFixture
    {
        void SetUp() override
        {
            AWSCoreFixture::SetUp();
            m_serializeContext = AZStd::make_unique<AZ::SerializeContext>();
            m_serializeContext->CreateEditContext();
            m_behaviorContext = AZStd::make_unique<AZ::BehaviorContext>();
            AZ::JsonSystemComponent::Reflect(m_registrationContext.get());

            m_awsCoreComponentDescriptor.reset(AWSCoreSystemComponentMock::CreateDescriptor());
            m_awsCoreComponentDescriptor->Reflect(m_serializeContext.get());
            m_awsCoreComponentDescriptor->Reflect(m_behaviorContext.get());

            m_componentDescriptor.reset(AWSAttributionSystemComponent::CreateDescriptor());
            m_componentDescriptor->Reflect(m_serializeContext.get());
            m_componentDescriptor->Reflect(m_behaviorContext.get());

            m_settingsRegistry->SetContext(m_serializeContext.get());
            m_settingsRegistry->SetContext(m_registrationContext.get());

            m_entity = aznew AZ::Entity();
            m_awsCoreSystemComponentMock = aznew testing::NiceMock<AWSCoreSystemComponentMock>();
            m_entity->AddComponent(m_awsCoreSystemComponentMock);
            m_attributionSystemsComponent.reset(m_entity->CreateComponent<AWSAttributionSystemComponent>());
        }

        void TearDown() override
        {
            m_entity->Deactivate();
            m_entity->RemoveComponent(m_attributionSystemsComponent.get());
            m_entity->RemoveComponent(m_awsCoreSystemComponentMock);
            delete m_entity;
            m_entity = nullptr;

            m_attributionSystemsComponent.reset();
            delete m_awsCoreSystemComponentMock;
            m_awsCoreComponentDescriptor.reset();
            m_componentDescriptor.reset();
            m_behaviorContext.reset();
            m_registrationContext.reset();
            m_serializeContext.reset();
            AWSCoreFixture::TearDown();
        }

    public:
        AZStd::unique_ptr<AWSAttributionSystemComponent> m_attributionSystemsComponent;
        testing::NiceMock<AWSCoreSystemComponentMock>* m_awsCoreSystemComponentMock;
        AZ::Entity* m_entity;

    private:
        AZStd::unique_ptr<AZ::SerializeContext> m_serializeContext;
        AZStd::unique_ptr<AZ::BehaviorContext> m_behaviorContext;
        AZStd::unique_ptr<AZ::JsonRegistrationContext> m_registrationContext;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_componentDescriptor;
        AZStd::unique_ptr<AZ::ComponentDescriptor> m_awsCoreComponentDescriptor;
    };

    TEST_F(AWSAttributionSystemComponentTest, SystemComponentInitActivate_Success)
    {
        m_entity->Init();
        m_entity->Activate();
    }
}


