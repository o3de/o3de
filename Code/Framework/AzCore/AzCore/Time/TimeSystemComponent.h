/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Time/ITime.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>

namespace AZ
{
    //! Implementation of the ITime system interface.
    class TimeSystemComponent
        : public AZ::Component
        , public ITimeRequestBus::Handler
    {
    public:

        AZ_COMPONENT(TimeSystemComponent, "{CE1C5E4F-7DC1-4248-B10C-AC55E8924A48}");

        static void Reflect(AZ::ReflectContext* context);
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        TimeSystemComponent();
        virtual ~TimeSystemComponent();

        //! AZ::Component overrides.
        //! @{
        void Activate() override;
        void Deactivate() override;
        //! @}

        //! ITime overrides.
        //! @{
        TimeMs GetElapsedTimeMs() const override;
        //! @}

    private:

        mutable TimeMs m_lastInvokedTimeMs = TimeMs{0};
        mutable TimeMs m_accumulatedTimeMs = TimeMs{0};
    };
}
