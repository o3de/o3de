/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineTest.h"

#include "UiGameEntityContext.h"
#include "UiScrollBarComponent.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiCanvasComponent.h"
#include "UiImageComponent.h"
#include <AzFramework/Application/Application.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>

namespace UnitTest
{
    class UiScrollBarTestApplication
        : public AzFramework::Application
    {
        void Reflect(AZ::ReflectContext* context) override
        {
            AzFramework::Application::Reflect(context);
            UiSerialize::ReflectUiTypes(context); //< needed to serialize ui Anchor and Offset
        }

        // override and only include system components required for tests.
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<AZ::AssetManagerComponent>(),
                azrtti_typeid<AZ::JobManagerComponent>(),
                azrtti_typeid<AZ::StreamerComponent>(),
                azrtti_typeid<AZ::SliceSystemComponent>(),
                azrtti_typeid<AzFramework::GameEntityContextComponent>(),
                azrtti_typeid<AzFramework::AssetSystem::AssetSystemComponent>(),
            };
        }

        void RegisterCoreComponents() override
        {
            AzFramework::Application::RegisterCoreComponents();
            RegisterComponentDescriptor(UiTransform2dComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiElementComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiScrollBarComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiImageComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiCanvasComponent::CreateDescriptor());
        }
    };

    class UiScrollBarComponentTest
        : public UnitTest::LeakDetectionFixture
    {
    protected:

        void SetUp() override
        {
            // start application

            AZ::ComponentApplication::Descriptor appDescriptor;
            appDescriptor.m_useExistingAllocator = true;

            m_application = aznew UiScrollBarTestApplication();
            AZ::ComponentApplication::StartupParameters startupParameters;
            startupParameters.m_loadSettingsRegistry = false;
            m_application->Start(appDescriptor, startupParameters);
        }

        void TearDown() override
        {
            m_application->Stop();
            delete m_application;
            m_application = nullptr;
        }

        static AZStd::tuple<UiCanvasComponent*, UiScrollBarComponent*> CreateUiCanvasWithScrollBar()
        {
            // create a canvas
            UiGameEntityContext* entityContext = new UiGameEntityContext(); //< UiCanvasComponent takes ownership of this pointer and will free this when exiting
            UiCanvasComponent* uiCanvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, false);

            // add scroll bar to the canvas
            AZ::Entity* uiScrollBarEntity = uiCanvasComponent->CreateChildElement("Ui Scroll Bar");
            uiScrollBarEntity->Deactivate(); //< deactivate so that we can add components
            uiScrollBarEntity->CreateComponent<UiTransform2dComponent>(); //< required by UiScrollBarComponent
            uiScrollBarEntity->CreateComponent<UiImageComponent>(); //< required by UiScrollBarComponent
            auto uiScrollBarComponent = uiScrollBarEntity->CreateComponent<UiScrollBarComponent>();
            uiScrollBarEntity->Activate();

            // create the handle entity
            AZ::Entity* handleEntity = uiScrollBarEntity->FindComponent<UiElementComponent>()->CreateChildElement("Handle");
            handleEntity->Deactivate(); // deactivate to add component
            handleEntity->CreateComponent<UiTransform2dComponent>();
            handleEntity->CreateComponent<UiImageComponent>();
            handleEntity->Activate();

            // give the handle a size otherwise scroll box items won't be spawned (fill the whole canvas)
            uiScrollBarComponent->SetHandleEntity(handleEntity->GetId());


            return AZStd::make_tuple(uiCanvasComponent, uiScrollBarComponent);
        }

    private:
        UiScrollBarTestApplication* m_application = nullptr;
    };

    TEST_F(UiScrollBarComponentTest, UiScrollBarComponent_WillFadeAfterDelay)
    {
        UiCanvasComponent* uiCanvasComponent;
        UiScrollBarComponent* uiScrollBarComponent;
        std::tie(uiCanvasComponent, uiScrollBarComponent) = CreateUiCanvasWithScrollBar();
        AZ::Entity* uiScrollBarEntity = uiScrollBarComponent->GetEntity();

        // test: move the scrollbar, wait 2 seconds and see if the alpha has faded to 0
        UiScrollBarBus::Event(uiScrollBarEntity->GetId(), &UiScrollBarBus::Events::SetAutoFadeEnabled, true);
        uiScrollBarComponent->SetValue(0.5f); // move the scrollbar
        uiScrollBarComponent->Update(2.0f);// wait two seconds

        EXPECT_EQ(uiScrollBarEntity->FindComponent<UiImageComponent>()->GetAlpha(), 0.0f);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
    }
} //namespace UnitTest

