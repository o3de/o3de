/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonAssetBuilderSystemComponent.h>
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/StringFunc/StringFunc.h>

#include <AzFramework/API/ApplicationAPI.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <AssetBuilderSDK/AssetBuilderSDK.h>
#include <AzToolsFramework/API/EditorPythonConsoleBus.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/Slice/SliceTransaction.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>
#include <EditorPythonBindings/EditorPythonBindingsSymbols.h>

#include <Source/PythonBuilderWorker.h>
#include <Source/PythonBuilderMessageSink.h>
#include <Source/PythonBuilderNotificationHandler.h>

namespace PythonAssetBuilder
{
    void PythonAssetBuilderSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        PythonBuilderNotificationHandler::Reflect(context);
        PythonBuilderWorker::Reflect(context);

        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            const AZStd::vector<AZ::Crc32> systemTags({ AssetBuilderSDK::ComponentTags::AssetBuilder });

            serialize->Class<PythonAssetBuilderSystemComponent, AZ::Component>()
                ->Version(0)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, systemTags)
                ;
        }

        if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<PythonAssetBuilderRequestBus>("PythonAssetBuilderRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "asset.builder")
                ->Event("RegisterAssetBuilder", &PythonAssetBuilderRequestBus::Events::RegisterAssetBuilder)
                ->Event("GetExecutableFolder", &PythonAssetBuilderRequestBus::Events::GetExecutableFolder)
                ;
        }
    }

    void PythonAssetBuilderSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("PythonAssetBuilderService"));
    }

    void PythonAssetBuilderSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("PythonAssetBuilderService"));
    }

    void PythonAssetBuilderSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        dependent.push_back(EditorPythonBindings::PythonMarshalingService);
        dependent.push_back(EditorPythonBindings::PythonReflectionService);
        dependent.push_back(EditorPythonBindings::PythonEmbeddedService);
    }

    void PythonAssetBuilderSystemComponent::Init()
    {
        m_messageSink = AZStd::make_shared<PythonBuilderMessageSink>();
    }

    void PythonAssetBuilderSystemComponent::Activate()
    {
        PythonAssetBuilderRequestBus::Handler::BusConnect();

        if (auto&& pythonInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get())
        {
            pythonInterface->StartPython(true);
        }
    }

    void PythonAssetBuilderSystemComponent::Deactivate()
    {
        m_messageSink.reset();

        if (PythonAssetBuilderRequestBus::HasHandlers())
        {
            PythonAssetBuilderRequestBus::Handler::BusDisconnect();

            if (auto&& pythonInterface = AZ::Interface<AzToolsFramework::EditorPythonEventsInterface>::Get())
            {
                pythonInterface->StopPython(true);
            }
        }
    }

    AZ::Outcome<bool, AZStd::string> PythonAssetBuilderSystemComponent::RegisterAssetBuilder(const AssetBuilderSDK::AssetBuilderDesc& desc)
    {
        const AZ::Uuid busId = desc.m_busId;
        if (m_pythonBuilderWorkerMap.find(busId) != m_pythonBuilderWorkerMap.end())
        {
            AZStd::string busIdString(busId.ToString<AZStd::string>());
            AZStd::string failMessage = AZStd::string::format("Asset Builder of JobId:%s has already been created.", busIdString.c_str());
            AZ_Warning(PythonBuilder, false, failMessage.c_str());
            return AZ::Failure(failMessage);
        }

        // create a PythonBuilderWorker instance
        auto worker = AZStd::make_shared<PythonBuilderWorker>();
        if (worker->ConfigureBuilderInformation(desc) == false)
        {
            return AZ::Failure(AZStd::string::format("Failed to configure builderId:%s", busId.ToString<AZStd::string>().c_str()));
        }
        m_pythonBuilderWorkerMap[busId] = worker;
        return AZ::Success(true);
    }

    AZ::Outcome<AZStd::string, AZStd::string> PythonAssetBuilderSystemComponent::GetExecutableFolder() const
    {
        const char* exeFolderName = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(exeFolderName, &AZ::ComponentApplicationRequests::GetExecutableFolder);
        if (exeFolderName)
        {
            return AZ::Success(AZStd::string(exeFolderName));
        }
        return AZ::Failure(AZStd::string("GetExecutableFolder access is missing."));
    }
}
