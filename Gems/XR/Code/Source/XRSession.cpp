/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <XR/XRSession.h>
#include <XR/XRFactory.h>
#include <XR/XRDevice.h>
#include <XR/XRInput.h>

namespace XR
{
    AZ::RHI::ResultCode Session::Init(const Descriptor& descriptor)
    {
        AZ::RHI::ResultCode result = AZ::RHI::ResultCode::Fail;
        m_descriptor = descriptor;

        m_space = Factory::Get().CreateSpace();
        AZ_Error("XR", m_space, "XR Space was not created");
        if (m_space)
        {
            result = m_space->Init(Space::Descriptor{ m_descriptor.m_validationMode});
            AZ_Error("XR", result == AZ::RHI::ResultCode::Success, "XR Space was not initialized");
            RETURN_RESULTCODE_IF_UNSUCCESSFUL(result);
        }

        m_input = Factory::Get().CreateInput();
        AZ_Error("XR", m_input, "XR Input was not created");
        if (m_input)
        {
            result = m_input->Init(Input::Descriptor{ m_descriptor.m_instance, m_descriptor.m_device, this });
            AZ_Error("XR", result == AZ::RHI::ResultCode::Success, "XR Input was not initialized");
            RETURN_RESULTCODE_IF_UNSUCCESSFUL(result);
        }
        m_descriptor.m_device->RegisterSession(this);
        return AZ::RHI::ResultCode::Success;
    }

    const Session::Descriptor& Session::GetDescriptor() const
    {
        return m_descriptor;
    }

    void Session::Shutdown()
    {
        m_descriptor.m_device->UnRegisterSession();
        ShutdownInternal();
    }

    Input* Session::GetInput() const
    {
        return m_input.get();
    }

    Space* Session::GetSpace() const
    {
        return m_space.get();
    }
} // namespace XR
