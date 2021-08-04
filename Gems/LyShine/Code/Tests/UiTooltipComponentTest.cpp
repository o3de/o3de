/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "LyShineTest.h"
#include <ISerialize.h>
#include "UiGameEntityContext.h"
#include "UiButtonComponent.h"
#include "UiTooltipComponent.h"
#include "UiTooltipDisplayComponent.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiCanvasComponent.h"
#include "UiImageComponent.h"
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
    class MockTimer : public ITimer
    {
        public:
            mutable float m_timer_count = 1.0f;
            MOCK_METHOD0(ResetTimer, void());
            MOCK_METHOD0(UpdateOnFrameStart, void());
            float GetCurrTime([[maybe_unused]] ETimer which) const
            {
                m_timer_count += 1.0f;
                return m_timer_count;
            }
            MOCK_CONST_METHOD1(GetFrameStartTime, CTimeValue&(ETimer));
            MOCK_CONST_METHOD0(GetAsyncTime, CTimeValue());
            MOCK_METHOD0(GetAsyncCurTime, float());
            MOCK_CONST_METHOD1(GetFrameTime, float(ETimer));
            MOCK_CONST_METHOD0(GetRealFrameTime, float());
            MOCK_CONST_METHOD0(GetTimeScale, float());
            MOCK_CONST_METHOD1(GetTimeScale, float(uint32));
            MOCK_METHOD0(ClearTimeScales, void());
            MOCK_METHOD2(SetTimeScale, void(float, uint32));
            MOCK_METHOD1(EnableTimer, void(bool));
            MOCK_CONST_METHOD0(IsTimerEnabled, bool());
            MOCK_METHOD0(GetFrameRate, float());
            MOCK_METHOD2(GetProfileFrameBlending, float(float*, int*));
            MOCK_METHOD1(Serialize, void(TSerialize));
            MOCK_METHOD2(PauseTimer, bool(ETimer, bool));
            MOCK_METHOD1(IsTimerPaused, bool(ETimer));
            MOCK_METHOD2(SetTimer, bool(ETimer, float));
            MOCK_METHOD2(SecondsToDateUTC, void(time_t, struct tm&));
            MOCK_METHOD1(DateToSecondsUTC, time_t(struct tm&));
            MOCK_METHOD1(TicksToSeconds, float(int64));
            MOCK_METHOD0(GetTicksPerSecond, int64());
            MOCK_METHOD0(CreateNewTimer, ITimer*());
            MOCK_METHOD2(EnableFixedTimeMode, void(bool, float));
    };

    class UiTooltipTestApplication
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
            RegisterComponentDescriptor(UiButtonComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiTooltipComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiTooltipDisplayComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiImageComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiCanvasComponent::CreateDescriptor());
        }
    };

    class UiTooltipComponentTest
        : public testing::Test
    {
    protected:
        UiTooltipTestApplication* m_application = nullptr;

    protected:

        void SetUp() override
        {
            // start application
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(AZ::SystemAllocator::Descriptor());

            AZ::ComponentApplication::Descriptor appDescriptor;
            appDescriptor.m_useExistingAllocator = true;

            m_application = aznew UiTooltipTestApplication();
            m_application->Start(appDescriptor, AZ::ComponentApplication::StartupParameters());
        }

        void TearDown() override
        {
            m_application->Stop();
            delete m_application;
            m_application = nullptr;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

        AZStd::tuple<UiCanvasComponent*, UiTooltipDisplayComponent*, UiTooltipComponent*> CreateUiCanvasWithTooltip()
        {
            // create a canvas
            UiGameEntityContext* entityContext = new UiGameEntityContext(); //< UiCanvasComponent takes ownership of this pointer and will free this when exiting
            UiCanvasComponent* uiCanvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, false);

            // add scroll bar to the canvas
            AZ::Entity* tooltipDisplayEntity = uiCanvasComponent->CreateChildElement("Ui Tooltip");
            tooltipDisplayEntity->Deactivate(); //< deactivate so that we can add components
            tooltipDisplayEntity->CreateComponent<UiTransform2dComponent>(); //< required by UiTooltipDisplauComponent
            tooltipDisplayEntity->CreateComponent<UiImageComponent>(); //< required by UiTooltipDisplayComponent
            auto uiTooltipDisplayComponent = tooltipDisplayEntity->CreateComponent<UiTooltipDisplayComponent>();
            tooltipDisplayEntity->Activate();
            uiCanvasComponent->SetTooltipDisplayElement(tooltipDisplayEntity->GetId());

            // create the button entity
            AZ::Entity* buttonEntity = uiCanvasComponent->CreateChildElement("Ui Button");
            buttonEntity->Deactivate(); // deactivate to add component
            buttonEntity->CreateComponent<UiTransform2dComponent>();
            buttonEntity->CreateComponent<UiButtonComponent>();
            buttonEntity->CreateComponent<UiImageComponent>();
            auto uiTooltipComponent = buttonEntity->CreateComponent<UiTooltipComponent>();
            buttonEntity->Activate();

            return AZStd::make_tuple(uiCanvasComponent, uiTooltipDisplayComponent, uiTooltipComponent);
        }

    };

    TEST_F(UiTooltipComponentTest, UiTooltipComponent_WillAppearOnHover)
    {
        MockTimer m_timer = MockTimer();
        SSystemGlobalEnvironment env;
        SSystemGlobalEnvironment* prevEnv = gEnv;
        gEnv = &env;
        gEnv->pTimer = &m_timer;
        gEnv->pLyShine = nullptr;

        auto [uiCanvasComponent, uiTooltipDisplayComponent, uiTooltipComponent] = CreateUiCanvasWithTooltip();
        uiTooltipDisplayComponent->SetTriggerMode(UiTooltipDisplayInterface::TriggerMode::OnHover);
        AZ::Entity* uiTooltipEntity = uiTooltipComponent->GetEntity();

        // test: verify tooltip is hidden by default, hover over button for tooltip to appear, hover off for tooltip to disappear
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnHoverStart);
        AZ::EntityId canvasEntityId;
        UiElementBus::EventResult(canvasEntityId, uiTooltipComponent->GetEntityId(), &UiElementBus::Events::GetCanvasEntityId);
        uiTooltipDisplayComponent->Update();
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Shown);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnHoverEnd);
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
        gEnv = prevEnv;
    }

    TEST_F(UiTooltipComponentTest, UiTooltipComponent_HoverTooltipDisappearsOnPress)
    {
        MockTimer m_timer = MockTimer();
        SSystemGlobalEnvironment env;
        SSystemGlobalEnvironment* prevEnv = gEnv;
        gEnv = &env;
        gEnv->pTimer = &m_timer;
        gEnv->pLyShine = nullptr;

        auto [uiCanvasComponent, uiTooltipDisplayComponent, uiTooltipComponent] = CreateUiCanvasWithTooltip();
        uiTooltipDisplayComponent->SetTriggerMode(UiTooltipDisplayInterface::TriggerMode::OnHover);
        AZ::Entity* uiTooltipEntity = uiTooltipComponent->GetEntity();

        // test: verify tooltip is hidden by default, hover over button for tooltip to appear, press button for tooltip to disappear
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnHoverStart);
        uiTooltipDisplayComponent->Update();
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Shown);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnPressed);
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
        gEnv = prevEnv;
    }

    TEST_F(UiTooltipComponentTest, UiTooltipComponent_TooltipAppearsOnPress)
    {
        MockTimer m_timer = MockTimer();
        SSystemGlobalEnvironment env;
        SSystemGlobalEnvironment* prevEnv = gEnv;
        gEnv = &env;
        gEnv->pTimer = &m_timer;
        gEnv->pLyShine = nullptr;

        auto [uiCanvasComponent, uiTooltipDisplayComponent, uiTooltipComponent] = CreateUiCanvasWithTooltip();
        uiTooltipDisplayComponent->SetTriggerMode(UiTooltipDisplayInterface::TriggerMode::OnPress);
        AZ::Entity* uiTooltipEntity = uiTooltipComponent->GetEntity();

        // test: verify tooltip is hidden by default, press button for tooltip to appear, release button for tooltip to disappear
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnPressed);
        uiTooltipDisplayComponent->Update();
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Shown);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnReleased);
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
        gEnv = prevEnv;
    }

    TEST_F(UiTooltipComponentTest, UiTooltipComponent_TooltipDisappearsOnCanvasPrimaryRelease)
    {
        MockTimer m_timer = MockTimer();
        SSystemGlobalEnvironment env;
        SSystemGlobalEnvironment* prevEnv = gEnv;
        gEnv = &env;
        gEnv->pTimer = &m_timer;
        gEnv->pLyShine = nullptr;

        auto [uiCanvasComponent, uiTooltipDisplayComponent, uiTooltipComponent] = CreateUiCanvasWithTooltip();
        uiTooltipDisplayComponent->SetTriggerMode(UiTooltipDisplayInterface::TriggerMode::OnPress);
        AZ::Entity* uiTooltipEntity = uiTooltipComponent->GetEntity();

        // test: verify tooltip is hidden by default, press button for tooltip to appear, release mouse on canvas (not on button) for tooltip to disappear
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnPressed);
        uiTooltipDisplayComponent->Update();
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Shown);
        UiCanvasInputNotificationBus::Event(uiCanvasComponent->GetEntityId(), &UiCanvasInputNotificationBus::Events::OnCanvasPrimaryReleased, uiCanvasComponent->GetEntityId());
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
        gEnv = prevEnv;
    }

    TEST_F(UiTooltipComponentTest, UiTooltipComponent_TooltipAppearsOnClick)
    {
        MockTimer m_timer = MockTimer();
        SSystemGlobalEnvironment env;
        SSystemGlobalEnvironment* prevEnv = gEnv;
        gEnv = &env;
        gEnv->pTimer = &m_timer;
        gEnv->pLyShine = nullptr;

        auto [uiCanvasComponent, uiTooltipDisplayComponent, uiTooltipComponent] = CreateUiCanvasWithTooltip();
        uiTooltipDisplayComponent->SetTriggerMode(UiTooltipDisplayInterface::TriggerMode::OnClick);
        AZ::Entity* uiTooltipEntity = uiTooltipComponent->GetEntity();

        // test: verify tooltip is hidden by default, click button for tooltip to appear, click off of button for tooltip to disappear
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnPressed);
        UiInteractableNotificationBus::Event(uiTooltipEntity->GetId(), &UiInteractableNotificationBus::Events::OnReleased);
        uiTooltipDisplayComponent->Update();
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Shown);
        UiCanvasInputNotificationBus::Event(uiCanvasComponent->GetEntityId(), &UiCanvasInputNotificationBus::Events::OnCanvasPrimaryReleased, uiCanvasComponent->GetEntityId());
        EXPECT_EQ(uiTooltipDisplayComponent->GetState(), UiTooltipDisplayComponent::State::Hidden);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
        gEnv = prevEnv;
    }
} //namespace UnitTest

