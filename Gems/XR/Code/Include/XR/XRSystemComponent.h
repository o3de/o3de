/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <XR/XRSystem.h>
#include <AzCore/Component/Component.h>

namespace XR
{
    //! This class is the component related to the common XR module.
    class SystemComponent final
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(SystemComponent, "{93A3A7E7-5188-49CD-8C37-26816476A3AE}");

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void Reflect(AZ::ReflectContext* context);

        SystemComponent() = default;
        ~SystemComponent() override = default;

        //////////////////////////////////////////////////////////////////////////
        // Component
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:
        Ptr<XR::System> m_xrSystem;
    };
}