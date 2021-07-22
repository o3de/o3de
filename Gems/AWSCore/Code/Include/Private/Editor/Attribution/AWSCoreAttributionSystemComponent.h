/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AWSCore
{
    class AWSAttributionManager;

    //! Attribution System Component. Responsible for instantiating and managing AWS Attribution Manager
    class AWSAttributionSystemComponent:
        public AZ::Component
    {
    public:
        AZ_COMPONENT(AWSAttributionSystemComponent, "{366861EC-8337-4180-A202-4E4DF082A3A8}");

        AWSAttributionSystemComponent();
        ~AWSAttributionSystemComponent() = default;

        static void Reflect(AZ::ReflectContext* context);\

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        AZStd::unique_ptr<AWSAttributionManager> m_manager; //!< pointer to the attribution manager which handles operational metrics
    };
} // namespace AWSCore
