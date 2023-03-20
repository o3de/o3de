/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Utils/Utils.h>
#include <AzFramework/IO/FileOperations.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>
#include <AzToolsFramework/Entity/SliceEditorEntityOwnershipServiceBus.h>
#include <AzToolsFramework/Slice/SliceRequestComponent.h>
#include <AzToolsFramework/Slice/SliceUtilities.h>

namespace AzToolsFramework
{
    void SliceRequestComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<SliceRequestComponent, AZ::Component>();
        }

        if (auto behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->EBus<SliceRequestBus>("SliceRequestBus")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Category, "Slice")
                ->Attribute(AZ::Script::Attributes::Module, "slice")
                ->Event("IsSliceDynamic", &SliceRequests::IsSliceDynamic)
                ->Event("SetSliceDynamic", &SliceRequests::SetSliceDynamic)
                ->Event("InstantiateSliceFromAssetId", &SliceRequests::InstantiateSliceFromAssetId)
                ->Event("CreateNewSlice", &SliceRequests::CreateNewSlice)
                ->Event("ShowPushDialog", &SliceRequests::ShowPushDialog)
                ;
        }
    }

    void SliceRequestComponent::Activate()
    {
        SliceRequestBus::Handler::BusConnect();
    }

    void SliceRequestComponent::Deactivate()
    {
        SliceRequestBus::Handler::BusDisconnect();
    }

    bool SliceRequestComponent::IsSliceDynamic(const AZ::Data::AssetId& assetId)
    {
        return SliceUtilities::IsDynamic(assetId);
    }

    void SliceRequestComponent::SetSliceDynamic(const AZ::Data::AssetId& assetId, bool isDynamic)
    {
        SliceUtilities::SetIsDynamic(assetId, isDynamic);
    }

    AzFramework::SliceInstantiationTicket SliceRequestComponent::InstantiateSliceFromAssetId(const AZ::Data::AssetId& assetId, const AZ::Transform& transform)
    {
        AZ::Data::Asset<AZ::SliceAsset> sliceAsset;
        sliceAsset.Create(assetId, true);

        AzFramework::SliceInstantiationTicket ticket;
        SliceEditorEntityOwnershipServiceRequestBus::BroadcastResult(ticket,
            &SliceEditorEntityOwnershipServiceRequests::InstantiateEditorSlice, sliceAsset, transform);

        return ticket;
    }

    bool SliceRequestComponent::CreateNewSlice(const AZ::EntityId& entityId, const char* assetPath)
    {
        // Expand the list of entities to include all transform descendant entities
        AzToolsFramework::EntityIdSet entitiesAndDescendants;
        AzToolsFramework::ToolsApplicationRequestBus::BroadcastResult(entitiesAndDescendants,
            &AzToolsFramework::ToolsApplicationRequestBus::Events::GatherEntitiesAndAllDescendents, AzToolsFramework::EntityIdList{ entityId });

        // Join our relative path with the game folder to get a full path to the desired asset
        AZ::IO::FixedMaxPath assetFullPath = AZ::Utils::GetProjectPath();
        assetFullPath /= assetPath;

        // Call SliceUtilities::MakeNewSlice with all user input prompts disabled
        bool success = AzToolsFramework::SliceUtilities::MakeNewSlice(entitiesAndDescendants,
            assetFullPath.c_str(),
            true  /*inheritSlices*/,
            false /*setAsDynamic*/,
            true  /*acceptDefaultPath*/,
            true  /*defaultMoveExternalRefs*/,
            true  /*defaultGenerateSharedRoot*/,
            true  /*silenceWarningPopups*/);

        return success;
    }

    void SliceRequestComponent::ShowPushDialog(const EntityIdList& entityIds)
    {
        QWidget* mainWindow = nullptr;
        EditorRequests::Bus::BroadcastResult(mainWindow, &EditorRequests::Bus::Events::GetMainWindow);
        SliceUtilities::PushEntitiesModal(mainWindow, entityIds, nullptr);
    }
} // namespace AzToolsFramework
