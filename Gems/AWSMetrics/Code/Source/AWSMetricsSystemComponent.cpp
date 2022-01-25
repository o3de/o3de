/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSCoreBus.h>
#include <AWSMetricsSystemComponent.h>
#include <MetricsManager.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

namespace AWSMetrics
{
    class AWSMetricsNotificationBusHandler
        : public AWSMetricsNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(AWSMetricsNotificationBusHandler
            , "{5329566F-3E7E-4A04-9C43-DB11232D92CA}"
            , AZ::SystemAllocator
            , OnSendMetricsSuccess
            , OnSendMetricsFailure
        );

        void OnSendMetricsSuccess(int requestId) override
        {
            Call(FN_OnSendMetricsSuccess, requestId);
        }

        void OnSendMetricsFailure(int requestId, const AZStd::string& errorMessage) override
        {
            Call(FN_OnSendMetricsFailure, requestId, errorMessage);
        }
    };

    AWSMetricsSystemComponent::AWSMetricsSystemComponent()
        : m_metricsManager(AZStd::make_unique<MetricsManager>())
    {
    }

    AWSMetricsSystemComponent::~AWSMetricsSystemComponent() = default;

    void AWSMetricsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        ReflectMetricsAttribute(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSMetricsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSMetricsSystemComponent>("AWSMetrics", "Generate and submit metrics to the metrics analytics pipeline")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System"))
                        ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<AWSMetricsRequestBus>("AWSMetricsRequestBus", "Generate and submit metrics to the metrics analytics pipeline")
                ->Attribute(AZ::Script::Attributes::Category, "AWSMetrics")
                ->Event(
                    "SubmitMetrics", &AWSMetricsRequestBus::Events::SubmitMetrics,
                    { { { "Metrics Attributes list", "The list of metrics attributes to submit." },
                        { "Event priority", "Priority of the event. Defaults to 0, which is highest priority." }, 
                        { "Event source override", "Event source used to override the default, 'AWSMetricGem'." },
                        { "Buffer metrics", "Whether to buffer metrics and send them in a batch." } } })
                ->Event("FlushMetrics", &AWSMetricsRequestBus::Events::FlushMetrics)
                ;

            behaviorContext->EBus<AWSMetricsNotificationBus>("AWSMetricsNotificationBus", "Notifications for sending metrics to the metrics analytics pipeline")
                ->Attribute(AZ::Script::Attributes::Category, "AWSMetrics")
                ->Handler<AWSMetricsNotificationBusHandler>()
                ;
        }
    }

    void AWSMetricsSystemComponent::ReflectMetricsAttribute(AZ::ReflectContext* reflection)
    {
        // Reflect MetricsAttribute
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serialize->Class<MetricsAttribute>()
                ->Version(1)
                ;
            serialize->RegisterGenericType<Attributes>();
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->Class<MetricsAttribute>("AWSMetrics_MetricsAttribute")
                ->Attribute(AZ::Script::Attributes::Category, "AWSMetrics")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Method("SetName", &MetricsAttribute::SetName,
                    {{{"Metrics Attribute Name", "Name of the metrics attribute."}}})
                ->Method("SetStrValue", (void (MetricsAttribute::*)(const AZStd::string& val)) &MetricsAttribute::SetVal,
                    {{{"Metrics Attribute Value", "String value of the metrics attribute."}}})
                ->Method("SetIntValue", (void (MetricsAttribute::*)(int)) &MetricsAttribute::SetVal,
                    {{{"Metrics Attribute Value", "Integer value of the metrics attribute."}}})
                ->Method("SetDoubleValue", (void (MetricsAttribute::*)(double)) &MetricsAttribute::SetVal,
                    {{{"Metrics Attribute Value", "Double value of the metrics attribute."}}})
                ;
        }

        AttributeSubmissionList::Reflect(reflection);
    }

    void AWSMetricsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSMetricsService"));
    }

    void AWSMetricsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSMetricsService"));
    }

    void AWSMetricsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSMetricsSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AWSMetricsSystemComponent::DumpStats(const AZ::ConsoleCommandContainer& arguments)
    {
        AZ_UNUSED(arguments);

        const GlobalStatistics& stats = m_metricsManager->GetGlobalStatistics();

        AZ_Printf("AWSMetrics", "Total number of metrics events sent to the backend/local file: %u", stats.m_numEvents.load());
        AZ_Printf("AWSMetrics", "Total number of metrics events sent to the backend/local file successfully: %u", stats.m_numSuccesses.load());
        AZ_Printf("AWSMetrics", "Total size of metrics events sent to the backend/local file successfully: %u bytes", stats.m_sendSizeInBytes.load());
        AZ_Printf("AWSMetrics", "Total number of metrics events failed to be sent to the backend/local file: %u", stats.m_numErrors.load());
        AZ_Printf("AWSMetrics", "Total number of metrics events which failed the JSON schema validation or reached the maximum number of retries : %u", stats.m_numDropped.load());
    }

    void AWSMetricsSystemComponent::EnableOfflineRecording(const AZ::ConsoleCommandContainer& arguments)
    {
        if (arguments.size() == 0 || arguments.size() > 2)
        {
            AZ_Error("AWSMetrics", false, "Invalid number of console command arguments. Please provide a boolean value to enable/disable the feature. "
                "To submit metrics recorded in the local file to the backend and delete the file, set the first argument to false and use \"submit\" as the second argument."
                "For example, AWSMetricsSystemComponent.EnableOfflineRecording false submit");
            return;
        }

        if (arguments[0] != "true" && arguments[0] != "false")
        {
            AZ_Error("AWSMetrics", false, "The first argument needs to be either true or false.");
            return;
        }

        if (arguments.size() == 2 && arguments[1] != "submit")
        {
            AZ_Error("AWSMetrics", false, "The second argument needs to be \"submit\" if exists.");
            return;
        }

        bool enable = arguments[0] == "true" ? true : false;
        bool submitLocalMetrics = arguments.size() == 2;
        m_metricsManager->UpdateOfflineRecordingStatus(enable, submitLocalMetrics);
    }

    bool AWSMetricsSystemComponent::SubmitMetrics(const AZStd::vector<MetricsAttribute>& metricsAttributes, int eventPriority,
        const AZStd::string& metricSource, bool bufferMetrics)
    {
        if (bufferMetrics)
        {
            return m_metricsManager->SubmitMetrics(metricsAttributes, eventPriority, metricSource);
        }

        return  m_metricsManager->SendMetricsAsync(metricsAttributes, eventPriority, metricSource);
    }

    void AWSMetricsSystemComponent::FlushMetrics()
    {
        m_metricsManager->FlushMetricsAsync();
    }

    void AWSMetricsSystemComponent::Init()
    {
        m_metricsManager->Init();
    }

    void AWSMetricsSystemComponent::Activate()
    {
        AWSMetricsRequestBus::Handler::BusConnect();

        m_metricsManager->StartMetrics();

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::SetAWSMetricsEnabled);
    }

    void AWSMetricsSystemComponent::Deactivate()
    {
        m_metricsManager->ShutdownMetrics();

        AWSMetricsRequestBus::Handler::BusDisconnect();
    }

    void AWSMetricsSystemComponent::AttributeSubmissionList::Reflect(AZ::ReflectContext* reflection)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(reflection))
        {
            serialize->Class<AttributeSubmissionList>()
                ->Version(1)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
        {
            behaviorContext->Class<AttributeSubmissionList>("AWSMetrics_AttributesSubmissionList")
                ->Attribute(AZ::Script::Attributes::Category, "AWSMetrics")
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Property("attributes", BehaviorValueProperty(&AttributeSubmissionList::attributes))
                ;
        }
    }
} // namespace AWSMetrics
