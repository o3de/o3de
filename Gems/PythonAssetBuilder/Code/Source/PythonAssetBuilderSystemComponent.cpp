/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonAssetBuilderSystemComponent.h>
#include <PythonAssetBuilder/PythonAssetBuilderBus.h>
#include <PythonAssetBuilder/PythonBuilderRequestBus.h>

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

            behaviorContext->EBus<PythonBuilderRequestBus>("PythonBuilderRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "asset.entity")
                ->Event("WriteSliceFile", &PythonBuilderRequestBus::Events::WriteSliceFile)
                ->Event("CreateEditorEntity", &PythonBuilderRequestBus::Events::CreateEditorEntity)
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

        PythonBuilderRequestBus::Handler::BusConnect();
    }

    void PythonAssetBuilderSystemComponent::Deactivate()
    {
        PythonBuilderRequestBus::Handler::BusDisconnect();
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

    AZ::Outcome<AZ::EntityId, AZStd::string> PythonAssetBuilderSystemComponent::CreateEditorEntity(const AZStd::string& name)
    {
        AZ::EntityId entityId;
        AzToolsFramework::EditorEntityContextRequestBus::BroadcastResult(
            entityId,
            &AzToolsFramework::EditorEntityContextRequestBus::Events::CreateNewEditorEntity,
            name.c_str());

        if (entityId.IsValid() == false)
        {
            return AZ::Failure<AZStd::string>("Failed to CreateNewEditorEntity.");
        }

        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationRequests::FindEntity, entityId);

        if (entity == nullptr)
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed to find created entityId %s", entityId.ToString().c_str()));
        }

        entity->Deactivate();

        AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
            &AzToolsFramework::EditorEntityContextRequestBus::Events::AddRequiredComponents,
            *entity);

        entity->Activate();

        return AZ::Success(entityId);
    }

    AZ::Outcome<AZ::Data::AssetType, AZStd::string> PythonAssetBuilderSystemComponent::WriteSliceFile(
        AZStd::string_view filename,
        AZStd::vector<AZ::EntityId> entityList,
        bool makeDynamic)
    {
        using namespace AzToolsFramework::SliceUtilities;

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
        if (serializeContext == nullptr)
        {
            return AZ::Failure<AZStd::string>("GetSerializeContext failed");
        }

        // transaction->Commit() requires the "@user@" alias
        auto settingsRegistry = AZ::SettingsRegistry::Get();
        auto ioBase = AZ::IO::FileIOBase::GetInstance();
        if (ioBase->GetAlias("@user@") == nullptr)
        {
            if (AZ::IO::Path userPath; settingsRegistry->Get(userPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectUserPath))
            {
                userPath /= "AssetProcessorTemp";
                ioBase->SetAlias("@user@", userPath.c_str());
            }
        }

        // transaction->Commit() expects the file to exist and write-able
        AZ::IO::HandleType fileHandle;
        AZ::IO::LocalFileIO::GetInstance()->Open(filename.data(), AZ::IO::OpenMode::ModeWrite, fileHandle);
        if (fileHandle == AZ::IO::InvalidHandle)
        {
            return AZ::Failure<AZStd::string>(
                AZStd::string::format("Failed to create slice file %.*s", aznumeric_cast<int>(filename.size()), filename.data()));
        }
        AZ::IO::LocalFileIO::GetInstance()->Close(fileHandle);

        AZ::u32 creationFlags = 0;
        if (makeDynamic)
        {
            creationFlags |= SliceTransaction::CreateAsDynamic;
        }

        SliceTransaction::TransactionPtr transaction = SliceTransaction::BeginNewSlice(nullptr, serializeContext, creationFlags);

        // add entities
        for (const AZ::EntityId& entityId : entityList)
        {
            auto addResult = transaction->AddEntity(entityId, SliceTransaction::SliceAddEntityFlags::DiscardSliceAncestry);
            if (!addResult)
            {
                return AZ::Failure<AZStd::string>(AZStd::string::format("Failed slice add entity: %s", addResult.GetError().c_str()));
            }
        }

        // commit to a file
        AZ::Data::AssetType sliceAssetType;
        auto resultCommit = transaction->Commit(filename.data(), nullptr, [&sliceAssetType](
            SliceTransaction::TransactionPtr transactionPtr,
            [[maybe_unused]] const char* fullPath,
            const SliceTransaction::SliceAssetPtr& sliceAssetPtr)
            {
                sliceAssetType = sliceAssetPtr->GetType();
                return AZ::Success();
            });

        if (!resultCommit)
        {
            return AZ::Failure<AZStd::string>(AZStd::string::format("Failed commit slice: %s", resultCommit.GetError().c_str()));
        }

        return AZ::Success(sliceAssetType);
    }
}
