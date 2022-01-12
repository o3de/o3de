/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Input/Contexts/InputContext.h>
#include <AzFramework/Input/Mappings/InputMapping.h>
#include <AzFramework/Input/Mappings/InputMappingAnd.h>
#include <AzFramework/Input/Mappings/InputMappingOr.h>
#include <AzFramework/Input/Devices/Gamepad/InputDeviceGamepad.h>
#include <AzFramework/Input/Devices/Keyboard/InputDeviceKeyboard.h>
#include <AzFramework/Input/System/InputSystemComponent.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/UnitTest/TestTypes.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace InputUnitTests
{
    using namespace AZ;
    using namespace AzFramework;
    using namespace UnitTest;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    class InputTest : public ScopedAllocatorSetupFixture
    {
    public:
        InputTest() : ScopedAllocatorSetupFixture()
        {
            // Many input tests are only valid if the GamePad device is supported on this platform.
            m_gamepadSupported = InputDeviceGamepad::GetMaxSupportedGamepads() > 0;
        }

    protected:
        ////////////////////////////////////////////////////////////////////////////////////////////
        void SetUp() override
        {
            m_inputSystemComponent = AZStd::make_unique<InputSystemComponent>();
            m_inputSystemComponent->Activate();
        }

        ////////////////////////////////////////////////////////////////////////////////////////////
        void TearDown() override
        {
            m_inputSystemComponent->Deactivate();
            m_inputSystemComponent.reset();
        }
        
        ////////////////////////////////////////////////////////////////////////////////////////////
        AZStd::unique_ptr<InputSystemComponent> m_inputSystemComponent;
        bool m_gamepadSupported;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputChannelId_ConstExpression_CopyConstructorSuccessfull)
    {
        constexpr InputChannelId testInputChannelId1("TestInputChannelId");
        constexpr InputChannelId testInputChannelId2(testInputChannelId1);
        static_assert(testInputChannelId1 == testInputChannelId2);
        EXPECT_EQ(testInputChannelId1, testInputChannelId2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputDeviceId_ConstExpression_CopyConstructorSuccessfull)
    {
        constexpr InputDeviceId testInputDeviceId1("TestInputDeviceId");
        constexpr InputDeviceId testInputDeviceId2(testInputDeviceId1);
        static_assert(testInputDeviceId1 == testInputDeviceId2);
        EXPECT_EQ(testInputDeviceId1, testInputDeviceId2);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_InitWithDataStruct_InitializationSuccessfull)
    {
        // Create an input context using an init data struct.
        constexpr AZ::s32 testPriority = 9;
        InputContext::InitData initData;
        initData.priority = testPriority;
        InputContext inputContext("TestInputContext", initData);
        EXPECT_EQ(inputContext.GetPriority(), testPriority);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_ActivateDeactivate_Successfull)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputContext_ActivateDeactivate_Successfull";
        #else
            SUCCEED() << "Skipping test InputContext_ActivateDeactivate_Successfull";
        #endif
            return;
        }
        // Create an input context (they are inactive by default).
        InputContext inputContext("TestInputContext");

        // Create an input mapping, add a single source input, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Reset the button, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Activate the input context, then validate the state of the mapping is unchanged.
        inputContext.Activate();
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping has changed.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Deactivate the input context, then validate the state of the mapping is unchanged.
        inputContext.Deactivate();
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button release, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_AddRemoveInputMapping_Successfull)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputContext_AddRemoveInputMapping_Successfull";
        #else
            SUCCEED() << "Skipping test InputContext_AddRemoveInputMapping_Successfull";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping and add a single source input.
        const InputChannelId inputMappingId = InputChannelId("TestInputMapping");
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(inputMappingId, inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Reset the button, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Add the mapping to the context, then validate the state of the mapping is unchanged.
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping has changed.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Remove the mapping from the context, then validate the state of the mapping is unchanged.
        EXPECT_TRUE(inputContext.RemoveInputMapping(inputMappingId));
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button release, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_AddSameMappingTwice_Unsuccessful)
    {
        // Validate that we can't add the same input mapping more than one to the same input context.
        InputContext inputContext("TestInputContext");
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));
        EXPECT_FALSE(inputContext.AddInputMapping(inputMapping));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_RemoveMappingThatHasNotBeenAdded_Unsuccessful)
    {
        // Validate that we can't remove an input mapping that has not been added to an input context.
        InputContext inputContext("TestInputContext");
        EXPECT_FALSE(inputContext.RemoveInputMapping(InputChannelId("TestInputMapping")));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_AddMappingToContextThatIsNotParent_Unsuccessful)
    {
        // Validate that we can't add a mapping to a context that is not its parent.
        InputContext inputContext("TestInputContext");
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);

        InputContext otherInputContext("TestInputContext");
        EXPECT_FALSE(otherInputContext.AddInputMapping(inputMapping));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_AddNullMapping_Unsuccessful)
    {
        // Validate that we can't add a null mapping to an input context.
        InputContext inputContext("TestInputContext");
        EXPECT_FALSE(inputContext.AddInputMapping(nullptr));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_ConsumeProcessedInput_Consumed)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputContext_ConsumeProcessedInput_Consumed";
        #else
            SUCCEED() << "Skipping test InputContext_ConsumeProcessedInput_Consumed";
        #endif
            return;
        }

        InputContext::InitData initData;

        // Create a high priority input context that consumes input processed by any of its mappings.
        initData.autoActivate = true;
        initData.consumesProcessedInput = true;
        initData.priority = InputChannelEventListener::GetPriorityFirst();
        InputContext inputContextPriorityHigh("TestInputContextPriorityHigh", initData);

        // Create a default priority input context that does not consume any input.
        initData.autoActivate = true;
        initData.consumesProcessedInput = false;
        initData.priority = InputChannelEventListener::GetPriorityDefault();
        InputContext inputContextPriorityDefault("TestInputContextPriorityDefault", initData);

        // Create a low priority input context that does not consume any input.
        initData.autoActivate = true;
        initData.consumesProcessedInput = false;
        initData.priority = InputChannelEventListener::GetPriorityLast();
        InputContext inputContextPriorityLow("TestInputContextPriorityLow", initData);

        // Create an input mapping to add to the high priority input context.
        AZStd::shared_ptr<InputMappingOr> inputMappingPriorityHigh = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMappingPriorityHigh"), inputContextPriorityHigh);
        EXPECT_TRUE(inputMappingPriorityHigh->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContextPriorityHigh.AddInputMapping(inputMappingPriorityHigh));

        // Create the same input mapping to add to the default and low priority input contexts.
        AZStd::shared_ptr<InputMappingOr> inputMappingPriorityDefault = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMappingPriorityDefault"), inputContextPriorityDefault);
        EXPECT_TRUE(inputMappingPriorityDefault->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContextPriorityDefault.AddInputMapping(inputMappingPriorityDefault));
        AZStd::shared_ptr<InputMappingOr> inputMappingPriorityLow = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMappingPriorityLow"), inputContextPriorityLow);
        EXPECT_TRUE(inputMappingPriorityLow->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContextPriorityLow.AddInputMapping(inputMappingPriorityLow));

        // Simulate a button press, then validate that it was consumed by the high priority context.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);

        EXPECT_TRUE(inputMappingPriorityHigh->IsActive());
        EXPECT_TRUE(inputMappingPriorityHigh->IsStateBegan());
        EXPECT_EQ(inputMappingPriorityHigh->GetValue(), 1.0f);
        EXPECT_EQ(inputMappingPriorityHigh->GetDelta(), 1.0f);

        EXPECT_FALSE(inputMappingPriorityDefault->IsActive());
        EXPECT_TRUE(inputMappingPriorityDefault->IsStateIdle());
        EXPECT_EQ(inputMappingPriorityDefault->GetValue(), 0.0f);
        EXPECT_EQ(inputMappingPriorityDefault->GetDelta(), 0.0f);

        EXPECT_FALSE(inputMappingPriorityLow->IsActive());
        EXPECT_TRUE(inputMappingPriorityLow->IsStateIdle());
        EXPECT_EQ(inputMappingPriorityLow->GetValue(), 0.0f);
        EXPECT_EQ(inputMappingPriorityLow->GetDelta(), 0.0f);

        // Create different input mappings to add to the default and low priority input contexts.
        AZStd::shared_ptr<InputMappingOr> otherInputMappingPriorityDefault = AZStd::make_shared<InputMappingOr>(InputChannelId("OtherTestInputMappingPriorityDefault"), inputContextPriorityDefault);
        EXPECT_TRUE(otherInputMappingPriorityDefault->AddSourceInput(InputDeviceGamepad::Button::B));
        EXPECT_TRUE(inputContextPriorityDefault.AddInputMapping(otherInputMappingPriorityDefault));
        AZStd::shared_ptr<InputMappingOr> otherInputMappingPriorityLow = AZStd::make_shared<InputMappingOr>(InputChannelId("OtherTestInputMappingPriorityLow"), inputContextPriorityLow);
        EXPECT_TRUE(otherInputMappingPriorityLow->AddSourceInput(InputDeviceGamepad::Button::B));
        EXPECT_TRUE(inputContextPriorityLow.AddInputMapping(otherInputMappingPriorityLow));

        // Simulate a button press, then validate that it was not consumed.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::B,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);

        EXPECT_TRUE(otherInputMappingPriorityDefault->IsActive());
        EXPECT_TRUE(otherInputMappingPriorityDefault->IsStateBegan());
        EXPECT_EQ(otherInputMappingPriorityDefault->GetValue(), 1.0f);
        EXPECT_EQ(otherInputMappingPriorityDefault->GetDelta(), 1.0f);

        EXPECT_TRUE(otherInputMappingPriorityLow->IsActive());
        EXPECT_TRUE(otherInputMappingPriorityLow->IsStateBegan());
        EXPECT_EQ(otherInputMappingPriorityLow->GetValue(), 1.0f);
        EXPECT_EQ(otherInputMappingPriorityLow->GetDelta(), 1.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputContext_FilteredInput_Mapped)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputContext_FilteredInput_Mapped";
        #else
            SUCCEED() << "Skipping test InputContext_FilteredInput_Mapped";
        #endif
            return;
        }

        // Create an input context that initially only listens for keyboard input.
        InputContext::InitData initData;
        initData.autoActivate = true;
        AZStd::shared_ptr<InputChannelEventFilterInclusionList> inclusionFilter = AZStd::make_shared<InputChannelEventFilterInclusionList>();
        inclusionFilter->IncludeDeviceName(InputDeviceKeyboard::Id.GetNameCrc32());
        initData.filter = inclusionFilter;
        InputContext inputContext("TestInputContext", initData);

        // Create an input mapping and add multiple source inputs from different input devices.
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceKeyboard::Key::AlphanumericA));
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a gamepad button press, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a keyboard key press, then validate the state of the mapping is updated.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Reset the button and key, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA, &AzFramework::InputChannelRequests::ResetState);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Tick the input system, then validate the state of the mapping reset.
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Update the filter so it also listens for gamepad input.
        inclusionFilter->IncludeDeviceName(InputDeviceGamepad::IdForIndex0.GetNameCrc32());

        // Simulate a gamepad button press, then validate the state of the mapping is updated.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingOr_AddRemoveSourceInput_Successful)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingOr_AddRemoveSourceInput_Successful";
        #else
            SUCCEED() << "Skipping test InputMappingOr_AddRemoveSourceInput_Successful";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping and add it to the context.
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Reset the button, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Add a source input to the mapping, then validate the state of the mapping is unchanged.
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping has changed.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Remove the source input from the mapping, then validate the state of the mapping is unchanged.
        EXPECT_TRUE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button release, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Validate that we can't add a source input twice.
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_FALSE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::A));

        // Validate that we can't remove a source input that has not been added.
        EXPECT_FALSE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::B));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingOr_SingleSourceInput_Mapped)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingOr_SingleSourceInput_Mapped";
        #else
            SUCCEED() << "Skipping test InputMappingOr_SingleSourceInput_Mapped";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping, add a single source input, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button hold, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Simulate a tick of the input system, then validate the expected state of the mapping.
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Remove the source input, simulate a button press, then validate the expected state of the mapping.
        EXPECT_TRUE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::A));
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingOr_MultipleSourceInputs_Mapped)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingOr_MultipleSourceInputs_Mapped";
        #else
            SUCCEED() << "Skipping test InputMappingOr_MultipleSourceInputs_Mapped";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping, add multiple source inputs, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingOr> inputMapping = AZStd::make_shared<InputMappingOr>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceKeyboard::Key::AlphanumericA));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button hold, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Simulate a tick of the input system, then validate the expected state of the mapping.
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a key press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a key hold, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a key release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Simulate a tick of the input system, then validate the expected state of the mapping.
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingAnd_AddRemoveSourceInput_Successful)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingAnd_AddRemoveSourceInput_Successful";
        #else
            SUCCEED() << "Skipping test InputMappingAnd_AddRemoveSourceInput_Successful";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping and add it to the context.
        AZStd::shared_ptr<InputMappingAnd> inputMapping = AZStd::make_shared<InputMappingAnd>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Reset the button, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Add a source input to the mapping, then validate the state of the mapping is unchanged.
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the state of the mapping has changed.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Remove the source input from the mapping, then validate the state of the mapping is unchanged.
        EXPECT_TRUE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button release, then validate the state of the mapping is unchanged.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Validate that we can't add a source input twice.
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_FALSE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::A));

        // Validate that we can't remove a source input that has not been added.
        EXPECT_FALSE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::B));
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingAnd_SingleSourceInput_Mapped)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingAnd_SingleSourceInput_Mapped";
        #else
            SUCCEED() << "Skipping test InputMappingAnd_SingleSourceInput_Mapped";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping, add a single source input, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingAnd> inputMapping = AZStd::make_shared<InputMappingAnd>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button hold, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Simulate a tick of the input system, then validate the expected state of the mapping.
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Remove the source input, simulate a button press, then validate the expected state of the mapping.
        EXPECT_TRUE(inputMapping->RemoveSourceInput(InputDeviceGamepad::Button::A));
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingAnd_MultipleSourceInputs_Mapped)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingAnd_MultipleSourceInputs_Mapped";
        #else
            SUCCEED() << "Skipping test InputMappingAnd_MultipleSourceInputs_Mapped";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping, add multiple source inputs, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingAnd> inputMapping = AZStd::make_shared<InputMappingAnd>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Button::A));
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceKeyboard::Key::AlphanumericA));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a key press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a button hold, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Reset the button and key, tick the input system, then validate the state of the mapping reset.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA, &AzFramework::InputChannelRequests::ResetState);
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a key press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a button press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate a key hold, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate a key release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);

        // Reset the button and key, tick the input system, then validate the state of the mapping reset.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Button::A, &AzFramework::InputChannelRequests::ResetState);
        AzFramework::InputChannelRequestBus::Event(InputDeviceKeyboard::Key::AlphanumericA, &AzFramework::InputChannelRequests::ResetState);
        AzFramework::InputSystemRequestBus::Broadcast(&InputSystemRequests::TickInput);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingAnd_MultipleSourceInputsWithDifferentValues_ValuesAveraged)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingAnd_MultipleSourceInputsWithDifferentValues_ValuesAveraged";
        #else
            SUCCEED() << "Skipping test InputMappingAnd_MultipleSourceInputsWithDifferentValues_ValuesAveraged";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping, add multiple source inputs, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingAnd> inputMapping = AZStd::make_shared<InputMappingAnd>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Trigger::L2));
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Trigger::R2));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate an L2 trigger press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Trigger::L2,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.25f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate an R2 trigger press, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Trigger::R2,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.75f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 0.5f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.5f);

        // Simulate an L2 trigger value change, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Trigger::L2,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.75f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateUpdated());
        EXPECT_EQ(inputMapping->GetValue(), 0.75f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.25f);

        // Simulate an R2 trigger release, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputDeviceGamepad::Trigger::R2,
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -0.75f);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    TEST_F(InputTest, InputMappingAnd_MultipleSourceInputsFromTheSameInputDeviceTypeWithDifferentIndicies_NotMapped)
    {
        if (!m_gamepadSupported)
        {
        #if defined(GTEST_SKIP)
            GTEST_SKIP() << "Skipping test InputMappingAnd_MultipleSourceInputsFromTheSameInputDeviceTypeWithDifferentIndicies_NotMapped";
        #else
            SUCCEED() << "Skipping test InputMappingAnd_MultipleSourceInputsFromTheSameInputDeviceTypeWithDifferentIndicies_NotMapped";
        #endif
            return;
        }

        // Create an input context and activate it.
        InputContext inputContext("TestInputContext");
        inputContext.Activate();

        // Create an input mapping, add multiple source inputs, and add the mapping to the context.
        AZStd::shared_ptr<InputMappingAnd> inputMapping = AZStd::make_shared<InputMappingAnd>(InputChannelId("TestInputMapping"), inputContext);
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Trigger::L2));
        EXPECT_TRUE(inputMapping->AddSourceInput(InputDeviceGamepad::Trigger::R2));
        EXPECT_TRUE(inputContext.AddInputMapping(inputMapping));

        // Validate the initial state of the mapping.
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate an L2 trigger press from device index 0, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::L2, 0),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate an L2 trigger press from device index 1, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::L2, 1),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate an R2 trigger press from device index 1, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::R2, 1),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   1.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateIdle());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 0.0f);

        // Simulate an R2 trigger press from device index 0, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::R2, 0),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                  1.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate an L2 trigger release from device index 1, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::L2, 1),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate an R2 trigger release from device index 1, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::R2, 1),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_TRUE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateBegan());
        EXPECT_EQ(inputMapping->GetValue(), 1.0f);
        EXPECT_EQ(inputMapping->GetDelta(), 1.0f);

        // Simulate an R2 trigger release from device index 0, then validate the expected state of the mapping.
        AzFramework::InputChannelRequestBus::Event(InputChannelRequests::BusIdType(InputDeviceGamepad::Trigger::R2, 0),
                                                   &AzFramework::InputChannelRequests::SimulateRawInput,
                                                   0.0f);
        EXPECT_FALSE(inputMapping->IsActive());
        EXPECT_TRUE(inputMapping->IsStateEnded());
        EXPECT_EQ(inputMapping->GetValue(), 0.0f);
        EXPECT_EQ(inputMapping->GetDelta(), -1.0f);
    }

} // namespace UnitTest
