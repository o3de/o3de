/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Interface/Interface.h>

namespace AZ::RPI
{
    //! This class contains the interface related to XR but significant to RPI level functionality
    class XRRenderingInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(XRRenderingInterface, AZ::SystemAllocator, 0);
        AZ_RTTI(XRRenderingInterface, "{18177EAF-3014-4349-A28F-BF58442FFC2B}");

        XRRenderingInterface() = default;
        virtual ~XRRenderingInterface() = default;

        //! This api is use to create a XRInstance
        virtual AZ::RHI::ResultCode InitInstance() = 0;
    };

    //! This class contains the interface that will be used to register the XR system with RPI and RHI
    class IXRRegisterInterface
    {
    public:
        AZ_CLASS_ALLOCATOR(IXRRegisterInterface, AZ::SystemAllocator, 0);
        AZ_RTTI(IXRRegisterInterface, "{89FA72F6-EA61-43AA-B129-7DC63959D5EA}");

        IXRRegisterInterface() = default;
        virtual ~IXRRegisterInterface() = default;

        //! Register XR system with RPI and RHI
        virtual void RegisterXRInterface(XRRenderingInterface*) = 0;

        //! UnRegister XR system with RPI and RHI
        virtual void UnRegisterXRInterface() = 0;
    };
    using XRRegisterInterface = AZ::Interface<IXRRegisterInterface>;

}
