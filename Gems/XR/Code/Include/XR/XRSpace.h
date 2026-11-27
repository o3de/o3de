/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <Atom/RHI/ValidationLayer.h>
#include <XR/XRBase.h>
#include <XR/XRObject.h>

namespace XR
{
    //! Base XR Space class which will provide access to the back-end concrete object
    class Space
        : public ::XR::Object
    {
    public:
        AZ_CLASS_ALLOCATOR(Space, AZ::SystemAllocator);
        AZ_RTTI(Space, "{A78A37F1-8861-4EB4-8FC6-0E9C11394EF1}");

        struct Descriptor
        {
            AZ::RHI::ValidationMode m_validationMode = AZ::RHI::ValidationMode::Disabled;
        };

        AZ::RHI::ResultCode Init(Descriptor descriptor);
        const Space::Descriptor& GetDescriptor() const;

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
