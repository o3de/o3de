/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once
#include <InputManagementFramework/InputSubComponent.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Component/TickBus.h>
#include <AzFramework/Input/Buses/Notifications/InputDeviceNotificationBus.h>
#include <AzFramework/Input/Events/InputChannelEventListener.h>
#include <InputNotificationBus.h>
#include <InputRequestBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace Input
{
    //////////////////////////////////////////////////////////////////////////
    /// Input handles raw input from any source and outputs Pressed, Held, and Released input events
    class Input
        : public InputSubComponent
        , protected AzFramework::InputChannelEventListener
        , protected AZ::GlobalInputRecordRequestBus::Handler
        , protected AZ::InputRecordRequestBus::Handler
    {
    public:
        Input();
        ~Input() override = default;
        AZ_RTTI(Input, "{546C9EBC-90EF-4F03-891A-0736BE2A487E}", InputSubComponent);
        static void Reflect(AZ::ReflectContext* reflection);

        //////////////////////////////////////////////////////////////////////////
        // InputSubComponent
        void Activate(const AZ::InputEventNotificationId& eventNotificationId) override;
        void Deactivate(const AZ::InputEventNotificationId& eventNotificationId) override;

    protected:
        AZStd::string GetEditorText() const;
        virtual const AZStd::vector<AZStd::string> GetInputDeviceTypes() const;
        virtual const AZStd::vector<AZStd::string> GetInputNamesBySelectedDevice() const;

        AZ::Crc32 OnDeviceSelected();

        //////////////////////////////////////////////////////////////////////////
        // AZ::GlobalInputRecordRequests::Handler
        void GatherEditableInputRecords(AZ::EditableInputRecords& outResults) override;

        //////////////////////////////////////////////////////////////////////////
        // AZ::EditableInputRecord::Handler
        void SetInputRecord(const AZ::EditableInputRecord& newInputRecord) override;

        //////////////////////////////////////////////////////////////////////////
        // AzFramework::InputChannelEventListener
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;

        using InputEventType = void(AZ::InputEventNotificationBus::Events::*)(float);
        virtual float CalculateEventValue(const AzFramework::InputChannel& inputChannel) const;
        void SendEventsInternal(float value, const AzFramework::LocalUserId& localUserIdOfEvent, const AZ::InputEventNotificationId busId, InputEventType eventType);

        //////////////////////////////////////////////////////////////////////////
        // Non Reflected Data
        AZ::InputEventNotificationId m_outgoingBusId;
        bool m_wasPressed = false;

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        float m_eventValueMultiplier = 1.f;
        AZStd::string m_inputName = "";
        AZStd::string m_inputDeviceType = "";
        float m_deadZone = 0.0f;
    };

    //////////////////////////////////////////////////////////////////////////
    /// ThumbstickInput handles raw input from thumbstick sources, applies any
    /// custom dead-zone or sensitivity curve calculations, and then outputs
    /// Pressed, Held, and Released input events for the specified axis
    class ThumbstickInput
        : public Input
    {
    public:
        ThumbstickInput();
        ~ThumbstickInput() override = default;
        AZ_RTTI(ThumbstickInput, "{4881FA7C-0667-476C-8C77-4DBB6C69F646}", Input);
        static void Reflect(AZ::ReflectContext* reflection);

    protected:
        //////////////////////////////////////////////////////////////////////////
        // InputSubComponent
        AZStd::string GetEditorText() const;
        const AZStd::vector<AZStd::string> GetInputDeviceTypes() const override;
        const AZStd::vector<AZStd::string> GetInputNamesBySelectedDevice() const override;
        bool OnInputChannelEventFiltered(const AzFramework::InputChannel& inputChannel) override;
        float CalculateEventValue(const AzFramework::InputChannel& inputChannel) const override;

        static AZ::Vector2 ApplyDeadZonesAndSensitivity(const AZ::Vector2& inputValues,
                                                        float innerDeadZone,
                                                        float outerDeadZone,
                                                        float axisDeadZone,
                                                        float sensitivityExponent);

        enum class OutputAxis
        {
            X,
            Y
        };

        //////////////////////////////////////////////////////////////////////////
        // Non Reflected Data
        const AzFramework::InputDeviceId* m_wasLastPressedByInputDeviceId = nullptr;

        //////////////////////////////////////////////////////////////////////////
        // Reflected Data
        float m_innerDeadZoneRadius = 0.0f;
        float m_outerDeadZoneRadius = 1.0f;
        float m_axisDeadZoneValue = 0.0f;
        float m_sensitivityExponent = 1.0f;
        OutputAxis m_outputAxis = OutputAxis::X;
    };
} // namespace Input
