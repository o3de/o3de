/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Metadata/UuidUtils.h>
#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <AzToolsFramework/Metadata/MetaUuidEntry.h>

namespace AzToolsFramework
{
    void UuidUtilComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<UuidUtilComponent, AZ::Component>();
        }

        MetaUuidEntry::Reflect(context);
    }

    AZ::Outcome<void, AZStd::string> UuidUtilComponent::CreateSourceUuid(AZ::IO::PathView absoluteFilePath, AZ::Uuid uuid)
    {
        auto* metadataInterface = AZ::Interface<IMetadataRequests>::Get();

        if (!metadataInterface)
        {
            AZ_Assert(metadataInterface, "Programmer Error - IMetadataRequests interface is not available");
            return AZ::Failure(AZStd::string("Programmer Error - IMetadataRequests interface is not available"));
        }

        AzToolsFramework::MetaUuidEntry entry;
        auto result = metadataInterface->GetValue(absoluteFilePath, UuidKey, entry);

        if (!result)
        {
            return AZ::Failure(result.GetError());
        }

        if (!entry.m_uuid.IsNull())
        {
            return AZ::Failure(AZStd::string::format(
                "CreateSourceUuid failed - metadata file already exists and contains a UUID for " AZ_STRING_FORMAT " " AZ_STRING_FORMAT,
                AZ_STRING_ARG(absoluteFilePath.Native()),
                AZ_STRING_ARG(entry.m_uuid.ToFixedString())));
        }

        entry.m_uuid = uuid;

        return metadataInterface->SetValue(absoluteFilePath, UuidKey, entry);
    }

    AZ::Outcome<AZ::Uuid, AZStd::string> UuidUtilComponent::CreateSourceUuid(AZ::IO::PathView absoluteFilePath)
    {
        auto uuid = AZ::Uuid::CreateRandom();

        if(auto result = CreateSourceUuid(absoluteFilePath, uuid); !result)
        {
            return AZ::Failure(result.GetError());
        }

        return AZ::Success(uuid);
    }
} // namespace AzToolsFramework
