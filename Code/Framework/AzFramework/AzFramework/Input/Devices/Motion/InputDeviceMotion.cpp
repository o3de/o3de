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

#include <AzFramework/Input/Devices/Motion/InputDeviceMotion.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/RTTI/BehaviorContext.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AzFramework
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDeviceId InputDeviceMotion::Id("motion");

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotion::IsMotionDevice(const InputDeviceId& inputDeviceId)
    {
        return (inputDeviceId.GetNameCrc32() == Id.GetNameCrc32());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId InputDeviceMotion::Acceleration::Gravity("motion_acceleration_gravity");
    const InputChannelId InputDeviceMotion::Acceleration::Raw("motion_acceleration_raw");
    const InputChannelId InputDeviceMotion::Acceleration::User("motion_acceleration_user");
    const AZStd::array<InputChannelId, 3> InputDeviceMotion::Acceleration::All =
    {{
        Gravity,
        Raw,
        User
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId InputDeviceMotion::RotationRate::Raw("motion_rotation_rate_raw");
    const InputChannelId InputDeviceMotion::RotationRate::Unbiased("motion_rotation_rate_unbiased");
    const AZStd::array<InputChannelId, 2> InputDeviceMotion::RotationRate::All =
    {{
        Raw,
        Unbiased
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId InputDeviceMotion::MagneticField::North("motion_magnetic_field_north");
    const InputChannelId InputDeviceMotion::MagneticField::Raw("motion_magnetic_field_raw");
    const InputChannelId InputDeviceMotion::MagneticField::Unbiased("motion_magnetic_field_unbiased");
    const AZStd::array<InputChannelId, 3> InputDeviceMotion::MagneticField::All =
    {{
        North,
        Raw,
        Unbiased
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputChannelId InputDeviceMotion::Orientation::Current("motion_orientation_current");
    const AZStd::array<InputChannelId, 1> InputDeviceMotion::Orientation::All =
    {{
        Current
    }};

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            // Unfortunately it doesn't seem possible to reflect anything through BehaviorContext
            // using lambdas which capture variables from the enclosing scope. So we are manually
            // reflecting all input channel names, instead of just iterating over them like this:
            //
            //  auto classBuilder = behaviorContext->Class<InputDeviceMotion>();
            //  for (const InputChannelId& channelId : Acceleration::All)
            //  {
            //      const char* channelName = channelId.GetName();
            //      classBuilder->Constant(channelName, [channelName]() { return channelName; });
            //  }

            behaviorContext->Class<InputDeviceMotion>()
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::RuntimeOwn)
                ->Constant("name", BehaviorConstant(Id.GetName()))

                ->Constant(Acceleration::Gravity.GetName(), BehaviorConstant(Acceleration::Gravity.GetName()))
                ->Constant(Acceleration::Raw.GetName(), BehaviorConstant(Acceleration::Raw.GetName()))
                ->Constant(Acceleration::User.GetName(), BehaviorConstant(Acceleration::User.GetName()))

                ->Constant(RotationRate::Raw.GetName(), BehaviorConstant(RotationRate::Raw.GetName()))
                ->Constant(RotationRate::Unbiased.GetName(), BehaviorConstant(RotationRate::Unbiased.GetName()))

                ->Constant(MagneticField::North.GetName(), BehaviorConstant(MagneticField::North.GetName()))
                ->Constant(MagneticField::Raw.GetName(), BehaviorConstant(MagneticField::Raw.GetName()))
                ->Constant(MagneticField::Unbiased.GetName(), BehaviorConstant(MagneticField::Unbiased.GetName()))

                ->Constant(Orientation::Current.GetName(), BehaviorConstant(Orientation::Current.GetName()))
            ;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotion::InputDeviceMotion()
        : InputDevice(Id)
        , m_allChannelsById()
        , m_accelerationChannelsById()
        , m_rotationRateChannelsById()
        , m_magneticFieldChannelsById()
        , m_orientationChannelsById()
        , m_enabledMotionChannelIds()
        , m_pimpl()
        , m_implementationRequestHandler(*this)
    {
        // Create all acceleration input channels
        for (AZ::u32 i = 0; i < Acceleration::All.size(); ++i)
        {
            const InputChannelId& channelId = Acceleration::All[i];
            InputChannelAxis3D* channel = aznew InputChannelAxis3D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_accelerationChannelsById[channelId] = channel;
        }

        // Create all rotation rate input channels
        for (AZ::u32 i = 0; i < RotationRate::All.size(); ++i)
        {
            const InputChannelId& channelId = RotationRate::All[i];
            InputChannelAxis3D* channel = aznew InputChannelAxis3D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_rotationRateChannelsById[channelId] = channel;
        }

        // Create all magnetic field input channels
        for (AZ::u32 i = 0; i < MagneticField::All.size(); ++i)
        {
            const InputChannelId& channelId = MagneticField::All[i];
            InputChannelAxis3D* channel = aznew InputChannelAxis3D(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_magneticFieldChannelsById[channelId] = channel;
        }

        // Create all orientation input channels
        for (AZ::u32 i = 0; i < Orientation::All.size(); ++i)
        {
            const InputChannelId& channelId = Orientation::All[i];
            InputChannelQuaternion* channel = aznew InputChannelQuaternion(channelId, *this);
            m_allChannelsById[channelId] = channel;
            m_orientationChannelsById[channelId] = channel;
        }

        // Create the platform specific implementation
        m_pimpl.reset(Implementation::Create(*this));

        // Connect to the motion sensor request bus
        InputMotionSensorRequestBus::Handler::BusConnect(GetInputDeviceId());
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotion::~InputDeviceMotion()
    {
        // Disconnect from the motion sensor request bus
        InputMotionSensorRequestBus::Handler::BusDisconnect(GetInputDeviceId());

        // Destroy the platform specific implementation
        m_pimpl.reset();

        // Destroy all orientation input channels
        for (const auto& channelById : m_orientationChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all magnetic field input channels
        for (const auto& channelById : m_magneticFieldChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all rotation rate input channels
        for (const auto& channelById : m_rotationRateChannelsById)
        {
            delete channelById.second;
        }

        // Destroy all acceleration input channels
        for (const auto& channelById : m_accelerationChannelsById)
        {
            delete channelById.second;
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    const InputDevice::InputChannelByIdMap& InputDeviceMotion::GetInputChannelsById() const
    {
        return m_allChannelsById;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotion::IsSupported() const
    {
        return m_pimpl != nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotion::IsConnected() const
    {
        return m_pimpl ? m_pimpl->IsConnected() : false;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::TickInputDevice()
    {
        if (m_pimpl)
        {
            m_pimpl->TickInputDevice();
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::SetInputChannelEnabled(const InputChannelId& channelId, bool enabled)
    {
        if (!m_pimpl)
        {
            return;
        }

        if (m_allChannelsById.find(channelId) == m_allChannelsById.end())
        {
            // The channel id is not recognized by this device
            return;
        }

        const bool isEnabled = m_enabledMotionChannelIds.find(channelId) != m_enabledMotionChannelIds.end();
        if (enabled == isEnabled)
        {
            // The channel is already enabled or disabled as requested
            return;
        }

        if (enabled)
        {
            m_enabledMotionChannelIds.insert(channelId);
        }
        else
        {
            m_enabledMotionChannelIds.erase(channelId);
        }

        m_pimpl->RefreshMotionSensors(m_enabledMotionChannelIds);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    bool InputDeviceMotion::GetInputChannelEnabled(const InputChannelId& channelId)
    {
        if (m_allChannelsById.find(channelId) == m_allChannelsById.end())
        {
            // The channel id is not recognized by this device
            return false;
        }

        return m_enabledMotionChannelIds.find(channelId) != m_enabledMotionChannelIds.end();
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::OnApplicationSuspended(Event /*lastEvent*/)
    {
        if (m_pimpl)
        {
            m_pimpl->RefreshMotionSensors(InputChannelIdSet());
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::OnApplicationResumed(Event /*lastEvent*/)
    {
        if (m_pimpl)
        {
            m_pimpl->RefreshMotionSensors(m_enabledMotionChannelIds);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotion::Implementation::Implementation(InputDeviceMotion& inputDevice)
        : m_inputDevice(inputDevice)
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    InputDeviceMotion::Implementation::~Implementation()
    {
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::Implementation::ProcessAccelerationData(const InputChannelId& channelId,
                                                                    const AZ::Vector3& data)
    {
        if (m_inputDevice.m_enabledMotionChannelIds.find(channelId) != m_inputDevice.m_enabledMotionChannelIds.end())
        {
            m_inputDevice.m_accelerationChannelsById[channelId]->ProcessRawInputEvent(data);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::Implementation::ProcessRotationRateData(const InputChannelId& channelId,
                                                                    const AZ::Vector3& data)
    {
        if (m_inputDevice.m_enabledMotionChannelIds.find(channelId) != m_inputDevice.m_enabledMotionChannelIds.end())
        {
            m_inputDevice.m_rotationRateChannelsById[channelId]->ProcessRawInputEvent(data);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::Implementation::ProcessMagneticFieldData(const InputChannelId& channelId,
                                                                     const AZ::Vector3& data)
    {
        if (m_inputDevice.m_enabledMotionChannelIds.find(channelId) != m_inputDevice.m_enabledMotionChannelIds.end())
        {
            m_inputDevice.m_magneticFieldChannelsById[channelId]->ProcessRawInputEvent(data);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::Implementation::ProcessOrientationData(const InputChannelId& channelId,
                                                                   const AZ::Quaternion& data)
    {
        if (m_inputDevice.m_enabledMotionChannelIds.find(channelId) != m_inputDevice.m_enabledMotionChannelIds.end())
        {
            m_inputDevice.m_orientationChannelsById[channelId]->ProcessRawInputEvent(data);
        }
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    void InputDeviceMotion::Implementation::ResetInputChannelStates()
    {
        m_inputDevice.ResetInputChannelStates();
    }
} // namespace AzFramework
