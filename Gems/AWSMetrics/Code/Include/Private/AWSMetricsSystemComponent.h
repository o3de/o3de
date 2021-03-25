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

#include <AWSMetricsBus.h>

#include <AzCore/Component/Component.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

namespace AWSMetrics
{
    class MetricsManager;

    //! Gem System Component. Responsible for instantiating and managing the Metrics Manager
    class AWSMetricsSystemComponent
        : public AZ::Component
        , protected AWSMetricsRequestBus::Handler
    {
    public:
        AZ_COMPONENT(AWSMetricsSystemComponent, "{D6252A35-6A8E-4E8B-BFC6-FCBE80E5A626}");

        AWSMetricsSystemComponent();
        ~AWSMetricsSystemComponent();

        static void Reflect(AZ::ReflectContext* context);
        static void ReflectMetricsAttribute(AZ::ReflectContext* reflection);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

    protected:
        ////////////////////////////////////////////////////////////////////////
        // AWSMetricsRequestBus interface implementation
        bool SubmitMetrics(const AZStd::vector<MetricsAttribute>& metricsAttributes, int eventPriority = 0,
            const AZStd::string& eventSourceOverride = "", bool bufferMetrics = true) override;
        void FlushMetrics() override;
        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

        using Attributes = AZStd::vector<MetricsAttribute>;

        struct AttributeSubmissionList
        {
            AZ_TYPE_INFO(AttributeSubmissionList, "{B1106C14-D22B-482F-B33E-B6E154A53798}");

            static void Reflect(AZ::ReflectContext* reflection);

            Attributes attributes;
        };

    private:
        AZ_CONSOLEFUNC(AWSMetricsSystemComponent, DumpStats, AZ::ConsoleFunctorFlags::Null, "Dumps stats for sending metrics");
        AZ_CONSOLEFUNC(
            AWSMetricsSystemComponent, EnableOfflineRecording, AZ::ConsoleFunctorFlags::Null, "Enable/disable the offline recording");

        //! Console commands.
        void DumpStats(const AZ::ConsoleCommandContainer& arguments);
        void EnableOfflineRecording(const AZ::ConsoleCommandContainer& arguments);

        AZStd::unique_ptr<MetricsManager> m_metricsManager; //!< Pointer to the metrics manager which handles the metrics submission
    };
} // namespace AWSMetrics
