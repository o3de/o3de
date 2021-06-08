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
