/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <XR/XRBase.h>
#include <XR/XRDevice.h>
#include <XR/XRObject.h>

namespace XR
{
    // This class will be responsible for creating XR::Input
    // which manage event queue or poll actions
    class Input
        : public XR::Object
    {
    public:
        AZ_CLASS_ALLOCATOR(Input, AZ::SystemAllocator);
        AZ_RTTI(Input, "{DCDFC6A7-B457-414B-BC24-0831C2AC628B}");

        struct Descriptor
        {
            Ptr<Instance> m_instance;
            Ptr<Device> m_device;
            Ptr<Session> m_session;
        };

        AZ::RHI::ResultCode Init(Descriptor descriptor);
        const Descriptor& GetDescriptor() const;

    private:
        ///////////////////////////////////////////////////////////////////
        // XR::Object
        void Shutdown() override;
        ///////////////////////////////////////////////////////////////////

        //! Called when the XR instance is being shutdown.
        virtual void ShutdownInternal() = 0;
        virtual AZ::RHI::ResultCode InitInternal() = 0;

        Descriptor m_descriptor;
    };
} // namespace XR
