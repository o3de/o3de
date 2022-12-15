/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AWSMetricsBus.h>
#include <AWSMetricsSystemComponent.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <AzToolsFramework/ActionManager/ActionManagerRegistrationNotificationBus.h>

namespace AWSMetrics
{
    class MetricsManager;

    //! Gem System Component. Responsible for instantiating and managing the Metrics Manager
    class AWSMetricsEditorSystemComponent
        : public AWSMetricsSystemComponent
        , private AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler
    {
    public:
        AZ_COMPONENT(AWSMetricsEditorSystemComponent, "{6144EDF6-12A6-4C3B-ADF1-7AA3C421BA68}", AWSMetricsSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&);

    protected:

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        // ActionManagerRegistrationNotificationBus implementation
        void OnMenuBindingHook() override;

    };
} // namespace AWSMetrics
