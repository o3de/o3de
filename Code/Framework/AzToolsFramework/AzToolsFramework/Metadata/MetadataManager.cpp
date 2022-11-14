/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Metadata/MetadataManager.h>

namespace AzToolsFramework
{
    void MetadataManager::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<MetadataManager>()->Version(1);
        }
    }

    bool MetadataManager::Get(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId)
    {
        auto path = ToMetadataPath(file);

        auto result = AZ::JsonSerializationUtils::ReadJsonFile(path.Native());

        if (!result)
        {
            return false;
        }

        auto& document = result.GetValue();

        rapidjson_ly::Pointer pointer(key.data(), key.length());

        if (!pointer.IsValid())
        {
            return false;
        }

        const rapidjson_ly::Value* value = pointer.Get(document);

        if (!value)
        {
            return false;
        }

        auto resultCode = AZ::JsonSerialization::Load(outValue, typeId, *value);

        return resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted;
    }

    bool MetadataManager::Set(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId)
    {
        auto path = ToMetadataPath(file);

        rapidjson_ly::Pointer pointer(key.data(), key.length());
        rapidjson_ly::Pointer versionPointer(MetadataVersionKey);

        if (!pointer.IsValid())
        {
            return false;
        }

        auto result = AZ::JsonSerializationUtils::ReadJsonFile(path.Native());

        rapidjson_ly::Document document;

        if (result)
        {
            document = result.TakeValue();
        }

        AZ::JsonSerializerSettings settings;
        settings.m_reporting = [](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            if (result.GetOutcome() > AZ::JsonSerializationResult::Outcomes::PartialDefaults)
            {
                AZ_Warning(
                    "MetadataManager", false, AZ_STRING_FORMAT ": " AZ_STRING_FORMAT "\n", AZ_STRING_ARG(path), AZ_STRING_ARG(message));
            }
            return result;
        };

        rapidjson_ly::Value serializedVersion;
        AZ::JsonSerialization::Store(serializedVersion, document.GetAllocator(), &MetadataVersion, nullptr, azrtti_typeid<int>(), settings);

        rapidjson_ly::Value serializedValue;
        auto resultCode = AZ::JsonSerialization::Store(serializedValue, document.GetAllocator(), inValue, nullptr, typeId, settings);

        if (resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
        {
            rapidjson_ly::Value& versionStore = versionPointer.Create(document, document.GetAllocator());
            versionStore = AZStd::move(serializedVersion);

            rapidjson_ly::Value& store = pointer.Create(document, document.GetAllocator());
            store = AZStd::move(serializedValue);

            auto saveResult = AZ::JsonSerializationUtils::WriteJsonFile(document, path.Native());

            return saveResult.IsSuccess();
        }

        return false;
    }

    AZ::IO::Path MetadataManager::ToMetadataPath(AZ::IO::PathView file)
    {
        AZ::IO::Path path = file;

        if (path.Extension() != MetadataFileExtension)
        {
            path.ReplaceExtension(AZ::IO::PathView(AZStd::string(path.Extension().Native()) + AZStd::string(MetadataFileExtension)));
        }

        return path;
    }
}
