/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShineTest.h"

#include "UiGameEntityContext.h"
#include "UiDynamicScrollBoxComponent.h"
#include "UiScrollBoxComponent.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiCanvasComponent.h"
#include "Mocks/UiDynamicScrollBoxDataBusHandlerMock.h"
#include <AzFramework/Application/Application.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>

namespace UnitTest
{
    // Simplified version of AzFramework::Application
    class UiDynamicScrollBoxTestApplication
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
                azrtti_typeid<AZ::MemoryComponent>(),
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
            RegisterComponentDescriptor(UiScrollBoxComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiDynamicScrollBoxComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiDynamicScrollBoxDataBusHandlerMock::CreateDescriptor());
            RegisterComponentDescriptor(UiCanvasComponent::CreateDescriptor());
        }
    };

    class UiDynamicScrollBoxComponentTest
        : public UnitTest::AllocatorsTestFixture
    {
    protected:

        void SetUp() override
        {
            // start application
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create();
            AZ::ComponentApplication::Descriptor appDescriptor;
            appDescriptor.m_useExistingAllocator = true;

            m_application = aznew UiDynamicScrollBoxTestApplication();
            m_application->Start(appDescriptor, AZ::ComponentApplication::StartupParameters());
        }

        void TearDown() override
        {
            m_application->Stop();
            delete m_application;
            m_application = nullptr;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        static int FindDescendantCount(const AZ::Entity* entity)
        {
            LyShine::EntityArray children = entity->FindComponent<UiElementComponent>()->GetChildElements();
            int numDescendants = static_cast<int>(children.size());
            for(const AZ::Entity* child : children)
            {
                numDescendants += FindDescendantCount(child);
            }

            return numDescendants;
        }

        static int FindCanvasElementCount(UiCanvasComponent* uiCanvasComponent)
        {
            const LyShine::EntityArray childEntities = uiCanvasComponent->GetChildElements();
            int numCanvasElements = static_cast<int>(childEntities.size());
            for (const AZ::Entity* childEntity : childEntities)
            {
                numCanvasElements += FindDescendantCount(childEntity);
            }

            return numCanvasElements;
        }

        static AZStd::tuple<UiCanvasComponent*, UiScrollBoxComponent*, UiDynamicScrollBoxComponent*, AZ::Entity*> CreateUiCanvasWithScrollBox()
        {
            // create a canvas
            UiGameEntityContext* entityContext = new UiGameEntityContext(); //< UiCanvasComponent takes ownership of this pointer and will free this when exiting
            UiCanvasComponent* uiCanvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, false);

            // add scroll box to the canvas
            AZ::Entity* uiScrollBoxEntity = uiCanvasComponent->CreateChildElement("Ui Scroll Box");
            uiScrollBoxEntity->Deactivate(); //< deactivate so that we can add components
            uiScrollBoxEntity->CreateComponent<UiTransform2dComponent>(); //< required by UiScrollBoxComponent
            auto uiScrollBoxComponent = uiScrollBoxEntity->CreateComponent<UiScrollBoxComponent>();
            auto uiDynamicScrollBoxComponent = uiScrollBoxEntity->CreateComponent<UiDynamicScrollBoxComponent>();
            uiScrollBoxEntity->Activate();

            // create the content entity (the parent container for the scroll box items)
            AZ::Entity* contentEntity = uiScrollBoxEntity->FindComponent<UiElementComponent>()->CreateChildElement("Content");
            contentEntity->Deactivate(); // deactivate to add component
            auto contentTransform = contentEntity->CreateComponent<UiTransform2dComponent>();
            contentEntity->Activate();
            // give the content a size otherwise scroll box items won't be spawned (fill the whole canvas)
            contentTransform->SetOffsets(UiTransform2dInterface::Offsets(0.0f, 0.0f, 0.0f, 0.0f));
            contentTransform->SetAnchors(UiTransform2dInterface::Anchors(0.0f, 0.0f, 1.0f, 1.0f), false, false);
            uiScrollBoxComponent->SetContentEntity(contentEntity->GetId());
            return AZStd::make_tuple(uiCanvasComponent, uiScrollBoxComponent, uiDynamicScrollBoxComponent, contentEntity);
        }

    private:
        UiDynamicScrollBoxTestApplication* m_application = nullptr;
    };

    TEST_F(UiDynamicScrollBoxComponentTest, UiDynamicScrollBoxComponent_WillClonePrototype_FT)
    {
        auto[uiCanvasComponent, uiScrollBoxComponent, uiDynamicScrollBoxComponent, contentEntity] = CreateUiCanvasWithScrollBox();
        AZ::Entity* uiScrollBoxEntity = uiScrollBoxComponent->GetEntity();

        // Main test here: Make a scroll box with 3 items and make sure the 3 items are actually spawned
        const int numScrollBoxItems = 3;
        uiScrollBoxEntity->Deactivate();
        const auto uiDynamicScrollBoxDataBusHandlerMock = new testing::NiceMock<UiDynamicScrollBoxDataBusHandlerMock>();
        uiScrollBoxEntity->AddComponent(uiDynamicScrollBoxDataBusHandlerMock);
        uiScrollBoxEntity->Activate();

        ON_CALL(*uiDynamicScrollBoxDataBusHandlerMock, GetNumElements()).WillByDefault(testing::Return(numScrollBoxItems));

        // create a prototype element and make it a child of the scroll box's content container
        AZ::Entity* prototype = contentEntity->FindComponent<UiElementComponent>()->CreateChildElement("Prototype");
        prototype->Deactivate(); //< deactivate before adding components
        auto prototypeTransform = prototype->CreateComponent<UiTransform2dComponent>();
        prototype->Activate();
        // Give the prototype some area (1x1px) because scroll boxes won't clone zero-sized (invisible) prototypes
        prototypeTransform->SetLocalWidth(1.0f);
        prototypeTransform->SetLocalHeight(1.0f);
        prototypeTransform->SetAnchors(UiTransform2dInterface::Anchors(0.5f, 0.5f, 0.5f, 0.5f), false, false);
        uiDynamicScrollBoxComponent->SetPrototypeElement(prototype->GetId());

        // We requested 3 scroll box items, so we expect 5 element in total:
        //    (1) scroll box, (1) content entity, and (3) prototype clones. (the original prototype is deactivated and won't be counted)
        uiDynamicScrollBoxComponent->RefreshContent();
        EXPECT_EQ(FindCanvasElementCount(uiCanvasComponent), 5);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
    }

    TEST_F(UiDynamicScrollBoxComponentTest, UiDynamicScrollBoxComponent_WillNotCloneInvalidPrototype_FT)
    {
        auto[uiCanvasComponent, uiScrollBoxComponent, uiDynamicScrollBoxComponent, contentEntity] = CreateUiCanvasWithScrollBox();
        AZ::Entity* uiScrollBoxEntity = uiScrollBoxComponent->GetEntity();

        // Main test here: Set the prototype to the scroll box itself causing a circular dependency.
        // We tell the scroll box to clone 3 bad elements, none of which should actually be cloned.
        const int numScrollBoxItems = 3;
        uiScrollBoxEntity->Deactivate(); //< deactivate before adding components
        const auto uiDynamicScrollBoxDataBusHandlerMock = new testing::NiceMock<UiDynamicScrollBoxDataBusHandlerMock>();
        uiScrollBoxEntity->AddComponent(uiDynamicScrollBoxDataBusHandlerMock);
        uiScrollBoxEntity->Activate();
        ON_CALL(*uiDynamicScrollBoxDataBusHandlerMock, GetNumElements()).WillByDefault(testing::Return(numScrollBoxItems));

        uiDynamicScrollBoxComponent->SetPrototypeElement(uiScrollBoxEntity->GetId());

        // We requested 3 scroll box items, but they are invalid so we expect that only 2 entity exist on the canvas
        //    (1) scroll box and (1) content entity
        uiDynamicScrollBoxComponent->RefreshContent();
        EXPECT_EQ(FindCanvasElementCount(uiCanvasComponent), 2);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
    }
} //namespace UnitTest
