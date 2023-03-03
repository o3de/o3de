/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AWSCoreBus.h>

#include <AWSMetricsEditorSystemComponent.h>
#include <MetricsManager.h>

#include <AzCore/IO/FileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include <AzToolsFramework/ActionManager/Action/ActionManager.h>
#include <AzToolsFramework/ActionManager/Action/ActionManagerInterface.h>
#include <AzToolsFramework/ActionManager/Menu/MenuManagerInterface.h>

#include <QDesktopServices>
#include <QUrl>

namespace AWSMetrics
{

    void AWSMetricsEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AWSMetricsEditorSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AWSMetricsEditorSystemComponent>("AWSMetricsEditor", "Generate and submit metrics to the metrics analytics pipeline")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AWSMetricsEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AWSMetricsServiceEditor"));
    }

    void AWSMetricsEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AWSMetricsServiceEditor"));
    }

    void AWSMetricsEditorSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("AWSCoreService"));
    }

    void AWSMetricsEditorSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType&)
    {
    }

    void AWSMetricsEditorSystemComponent::Activate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusConnect();

    }

    void AWSMetricsEditorSystemComponent::OnMenuBindingHook()
    {
        constexpr const char* AWSMetrics[] =
        {
             "Metrics Gem" ,
             "aws_metrics_gem" ,
             ":/Notifications/download.svg",
             ""
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::CreateSubMenu, AWSCore::AWSMenuIdentifier, AWSMetrics, 200);

        const auto& submenuIdentifier = AWSMetrics[1];

        static constexpr const char* AWSMetricsGemOverview[] =
        {
             "Metrics Gem overview" ,
             "aws_metrics_gem_overview" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-metrics/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSMetricsGemOverview, 0);

        static constexpr const char* AWSMetricsSetupGem[] =
        {
             "Setup Metrics Gem" ,
             "aws_setup_metrics_gem" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-metrics/setup/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSMetricsSetupGem, 0);

        static constexpr const char* AWSMetricsScripting[] =
        {
             "Scripting Reference" ,
             "aws_metrics_scripting_reference" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-metrics/scripting/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSMetricsScripting, 0);

        static constexpr const char* AWSMetricsAPIReference[] =
        {
             "API Reference" ,
             "aws_metrics_api_reference" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-metrics/cpp-api/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSMetricsAPIReference, 0);

        static constexpr const char* AWSMetricsAdvancedTopics[] =
        {
             "Advanced Topics" ,
             "aws_metrics_advanced_topics" ,
             ":/Notifications/link.svg",
             "https://o3de.org/docs/user-guide/gems/reference/aws/aws-metrics/advanced-topics/"
        };

        AWSCore::AWSCoreEditorRequestBus::Broadcast(&AWSCore::AWSCoreEditorRequests::AddExternalLinkAction, submenuIdentifier, AWSMetricsAdvancedTopics, 0);

        constexpr const char metricsSettingsIdentifier[] = "aws_metrics_settings";

        AZStd::string priorAlias = AZ::IO::FileIOBase::GetInstance()->GetAlias("@engroot@");
        AZStd::string configFilePath = priorAlias + "\\Gems\\AWSMetrics\\Code\\" + AZ::SettingsRegistryInterface::RegistryFolder;
        AzFramework::StringFunc::Path::Normalize(configFilePath);

        auto actionManagerInterface = AZ::Interface<AzToolsFramework::ActionManagerInterface>::Get();
        AZ_Assert(actionManagerInterface, "AWSMetricsEditorSystemComponent - could not get ActionManagerInterface");

        AzToolsFramework::ActionProperties actionProperties;
        actionProperties.m_name = "Metrics Settings";
        auto outcome = actionManagerInterface->RegisterAction(AWSCore::ActionContext, metricsSettingsIdentifier, actionProperties,
            [configFilePath]()
            {
                QDesktopServices::openUrl(QUrl::fromLocalFile(configFilePath.c_str()));
            });
        AZ_Assert(outcome.IsSuccess(), "Failed to register action %s", metricsSettingsIdentifier);

        auto menuManagerInterface = AZ::Interface<AzToolsFramework::MenuManagerInterface>::Get();
        AZ_Assert(menuManagerInterface, "AWSCoreEditorSystemComponent - could not get MenuManagerInterface");

        menuManagerInterface->AddActionToMenu(submenuIdentifier, metricsSettingsIdentifier, 0);

    }

    void AWSMetricsEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::ActionManagerRegistrationNotificationBus::Handler::BusDisconnect();
    }

} // namespace AWSMetrics
