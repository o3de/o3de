/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "LyShine_precompiled.h"
#include "LyShineTest.h"

#include <Mocks/IRendererMock.h>
#include "UiGameEntityContext.h"
#include "UiElementComponent.h"
#include "UiTransform2dComponent.h"
#include "UiCanvasComponent.h"
#include "UiTextInputComponent.h"
#include <AzFramework/Application/Application.h>
#include <AzFramework/Entity/GameEntityContextComponent.h>
#include <AzFramework/Asset/AssetSystemComponent.h>
#include <AzFramework/Input/Buses/Requests/InputTextEntryRequestBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Memory/MemoryComponent.h>
#include <AzCore/Asset/AssetManagerComponent.h>
#include <AzCore/IO/Streamer/StreamerComponent.h>
#include <AzCore/Jobs/JobManagerComponent.h>
#include <AzCore/Slice/SliceSystemComponent.h>

namespace UnitTest
{
    class UiTextInputTestApplication
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
            RegisterComponentDescriptor(UiTextInputComponent::CreateDescriptor());
            RegisterComponentDescriptor(UiCanvasComponent::CreateDescriptor());
        }
    };

    class InputDeviceMock
        : public AzFramework::InputTextEntryRequestBus::Handler
    {
    public:
        InputDeviceMock()
            : m_textEntryStarted(false)
            , m_mockId("MockInputDevice")
        {
            AzFramework::InputTextEntryRequestBus::Handler::BusConnect(m_mockId);
        }

        ~InputDeviceMock()
        {
            AzFramework::InputTextEntryRequestBus::Handler::BusDisconnect(m_mockId);
        }

        bool HasTextEntryStarted() const override
        {
            return m_textEntryStarted;
        }

        void TextEntryStart([[maybe_unused]] const VirtualKeyboardOptions& options) override
        {
            m_textEntryStarted = true;
        }

        void TextEntryStop() override
        {
            m_textEntryStarted = false;
        }

    private:
        bool m_textEntryStarted;
        AzFramework::InputDeviceId m_mockId;
    };

    class UiTextInputComponentTest
        : public ::testing::Test
    {
    protected:

        void SetUp() override
        {
            // Start application
            AZ::AllocatorInstance<AZ::SystemAllocator>::Create(AZ::SystemAllocator::Descriptor());

            AZ::ComponentApplication::Descriptor appDescriptor;
            appDescriptor.m_useExistingAllocator = true;

            m_applicationPtr = aznew UiTextInputTestApplication();
            m_applicationPtr->Start(appDescriptor, AZ::ComponentApplication::StartupParameters());

            // Create mock gEnv for mock renderer
            m_env = AZStd::make_unique<StubEnv>();
            memset(&m_env->m_stubEnv, 0, sizeof(SSystemGlobalEnvironment));
            m_priorEnv = gEnv;
            gEnv = &m_env->m_stubEnv;

            m_data = AZStd::make_unique<DataMembers>();
            m_env->m_stubEnv.pRenderer = &m_data->m_renderer;
        }

        void TearDown() override
        {
            m_data.reset();
            m_env.reset();
            gEnv = m_priorEnv;

            m_applicationPtr->Stop();
            delete m_applicationPtr;
            m_applicationPtr = nullptr;
            AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
        }

    private:
        struct StubEnv
        {
            SSystemGlobalEnvironment m_stubEnv;
        };

        struct DataMembers
        {
            testing::NiceMock<IRendererMock> m_renderer;
        };

        AZStd::unique_ptr<DataMembers> m_data;
        AZStd::unique_ptr<StubEnv> m_env;

        SSystemGlobalEnvironment* m_priorEnv = nullptr;
        UiTextInputTestApplication* m_applicationPtr = nullptr;
    };

    TEST_F(UiTextInputComponentTest, UiTextInputComponent_CanForceFocus_FT)
    {
        // create a canvas
        UiGameEntityContext* entityContext = new UiGameEntityContext(); //< UiCanvasComponent takes ownership of this pointer and will free this when exiting
        UiCanvasComponent* uiCanvasComponent = UiCanvasComponent::CreateCanvasInternal(entityContext, false);

        // add text input to the canvas
        AZ::Entity* uiTextInputEntity = uiCanvasComponent->CreateChildElement("Ui Text Input");
        uiTextInputEntity->Deactivate(); //< deactivate so that we can add components
        uiTextInputEntity->CreateComponent<UiTransform2dComponent>(); //< required by UiTextInputComponent
        uiTextInputEntity->CreateComponent<UiTextInputComponent>();
        uiTextInputEntity->Activate();

        AZ::EntityId uiTextInputEntityId = uiTextInputEntity->GetId();
        InputDeviceMock mockInputDevice = InputDeviceMock();

        // Make sure text entry has not already started
        bool textEntryStarted = mockInputDevice.HasTextEntryStarted();
        EXPECT_FALSE(textEntryStarted);

        uiCanvasComponent->ForceFocusInteractable(uiTextInputEntityId);

        // Make sure Text Input is active and requesting input
        textEntryStarted = mockInputDevice.HasTextEntryStarted();
        EXPECT_TRUE(textEntryStarted);

        // clean up the canvas
        delete uiCanvasComponent->GetEntity();
    }
} //namespace UnitTest

