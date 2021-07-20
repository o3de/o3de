/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Component/Component.h>
#include "StatisticalProfilerProxy.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
namespace AZ
{
    namespace Statistics
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////
        //! This system component manages the globally unique StatisticalProfilerProxy instance.
        //! And this is all this component does... it simply makes sure the StatisticalProfilerProxy exists.
        class StatisticalProfilerProxySystemComponent : public AZ::Component
        {
        public:
            ////////////////////////////////////////////////////////////////////////////////////////////
            // AZ::Component Setup
            AZ_COMPONENT(StatisticalProfilerProxySystemComponent, "{1E15565F-A5C1-4BF2-8AEE-D3880AC9E1EB}")

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AZ::ComponentDescriptor::Reflect
            static void Reflect(AZ::ReflectContext* reflection);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AZ::ComponentDescriptor::GetProvidedServices
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AZ::ComponentDescriptor::GetIncompatibleServices
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Constructor
            StatisticalProfilerProxySystemComponent();

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! Destructor
            ~StatisticalProfilerProxySystemComponent() override;

        protected:
            ////////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AZ::Component::Activate
            void Activate() override;

            ////////////////////////////////////////////////////////////////////////////////////////////
            //! \ref AZ::Component::Deactivate
            void Deactivate() override;

        private:
            ////////////////////////////////////////////////////////////////////////////////////////////
            // Disable copy constructor
            StatisticalProfilerProxySystemComponent(const StatisticalProfilerProxySystemComponent&) = delete;

            ////////////////////////////////////////////////////////////////////////////////////////////
            // The one and only StatisticalProfilerProxy (Which is itself an AZ::Interface<>)
            StatisticalProfilerProxy* m_StatisticalProfilerProxy;
        };
    } //namespace Statistics
} // namespace AZ
