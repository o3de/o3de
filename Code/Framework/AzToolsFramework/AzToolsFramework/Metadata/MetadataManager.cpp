/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Metadata/MetadataManager.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/IO/FileReader.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/StackedString.h>
#include <AzCore/JSON/pointer.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AzToolsFramework
{
    void MetadataManager::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            // Note the version here is not the same as the MetadataFile version, since this is a separate serialization
            serializeContext->Class<MetadataManager, AZ::Component>()->Version(1);
        }
    }

    AZ::Outcome<bool, AZStd::string> MetadataManager::GetValue(AZ::IO::PathView file, AZStd::string_view key, void* outValue, AZ::Uuid typeId)
    {
        rapidjson::Document value;

        if (auto result = GetJson(file, key, value); !result || !result.GetValue())
        {
            return result;
        }

        // Deserialize the JSON into the outValue parameter
        auto resultCode = AZ::JsonSerialization::Load(outValue, typeId, value);

        if(resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
        {
            return AZ::Success(true);
        }

        return AZ::Failure(
            AZStd::string::format("Failed to deserialize JSON for metadata file " AZ_STRING_FORMAT, AZ_STRING_ARG(file.Native())));
    }

    AZ::Outcome<bool, AZStd::string> MetadataManager::GetJson(AZ::IO::PathView file, AZStd::string_view key, rapidjson::Document& outValue)
    {
        auto path = ToMetadataPath(file);

        // Make a JSONPath pointer and validate it
        rapidjson::Pointer pointer(key.data(), key.length());

        if (!pointer.IsValid())
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid JSONPath key `" AZ_STRING_FORMAT "` provided for file " AZ_STRING_FORMAT,
                AZ_STRING_ARG(key),
                AZ_STRING_ARG(file.Native())));
        }

        // Load the JSON document into memory
        auto result = AZ::JsonSerializationUtils::ReadJsonFile(path.Native());

        if (!result)
        {
            AZ::u64 size = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(path.Native().c_str(), size);

            if (size > 0)
            {
                // Its only an error if the file actually exists, is non-empty and there was a failure reading it
                return AZ::Failure(AZStd::string::format(
                    "Metadata file `" AZ_STRING_FORMAT "`: Failed to load metadata JSON - %s",
                    AZ_STRING_ARG(file.Native()),
                    result.GetError().c_str()));
            }

            return AZ::Success(false);
        }

        auto& document = result.GetValue();

        // Use the pointer to find the value we're trying to read
        rapidjson::Value* value = pointer.Get(document);

        if (!value)
        {
            return AZ::Failure(
                AZStd::string::format("Metadata file `" AZ_STRING_FORMAT "`: JSONPath key " AZ_STRING_FORMAT " does not exist",
                AZ_STRING_ARG(file.Native()),
                AZ_STRING_ARG(key)));
        }

        outValue = rapidjson::Document(); // Make sure to release any existing memory if the document happens to be non-empty
        outValue.CopyFrom(*value, outValue.GetAllocator());
        return AZ::Success(true);
    }

    AZ::Outcome<bool, AZStd::string> MetadataManager::GetValueVersion(AZ::IO::PathView file, AZStd::string_view key, int& version)
    {
        rapidjson::Document value;
        if (auto result = GetJson(file, key, value); !result || !result.GetValue())
        {
            return result;
        }

        if(!value.IsObject())
        {
            return AZ::Failure(AZStd::string::format(
                "Cannot get version for key " AZ_STRING_FORMAT " in file " AZ_STRING_FORMAT ".  Stored value is not an object",
                AZ_STRING_ARG(key),
                AZ_STRING_ARG(file.Native())));
        }

        if(!value.GetObject().HasMember(MetadataObjectVersionField))
        {
            return AZ::Failure(AZStd::string::format(
                "Cannot get version for key " AZ_STRING_FORMAT " in file " AZ_STRING_FORMAT ".  Stored value does not have a version",
                AZ_STRING_ARG(key),
                AZ_STRING_ARG(file.Native())));
        }

        auto itr = value.GetObject().FindMember(MetadataObjectVersionField);
        version = itr->value.GetUint();

        return AZ::Success(true);
    }

    AZ::Outcome<void, AZStd::string> MetadataManager::SetValue(AZ::IO::PathView file, AZStd::string_view key, const void* inValue, AZ::Uuid typeId)
    {
        auto path = ToMetadataPath(file);

        // Make a JSONPath pointer and validate it
        rapidjson::Pointer pointer(key.data(), key.length());
        rapidjson::Pointer versionPointer(MetadataVersionKey);

        if (!pointer.IsValid())
        {
            return AZ::Failure(AZStd::string::format(
                "Invalid JSONPath key `" AZ_STRING_FORMAT "` provided for file " AZ_STRING_FORMAT,
                AZ_STRING_ARG(key),
                AZ_STRING_ARG(file.Native())));
        }

        // If there is an existing metadata file already, read the JSON document into memory.
        // Otherwise, just start a new, blank document
        auto result = AZ::JsonSerializationUtils::ReadJsonFile(path.Native());

        rapidjson::Document document;

        if (result)
        {
            document = result.TakeValue();
        }
        else
        {
            // Ensure the failure was due to the file not existing rather than failing to read the metadata file.
            // It would be bad to continue and overwrite an existing metadata file.
            // Check the size because an empty file is fine to overwrite.
            AZ::u64 size = 0;
            AZ::IO::FileIOBase::GetInstance()->Size(path.Native().c_str(), size);

            if (size > 0)
            {
                return AZ::Failure(AZStd::string::format(
                    "Metadata file `" AZ_STRING_FORMAT
                    "` exists on disk but failed to read.  Metadata file may be corrupt.  Aborting Set for " AZ_STRING_FORMAT
                    ".  Error - " AZ_STRING_FORMAT,
                    AZ_STRING_ARG(file.Native()),
                    AZ_STRING_ARG(key),
                    AZ_STRING_ARG(result.GetError())));
            }
        }

        // Grab the class data so the type version can be written out
        auto classData = GetClassData(typeId);

        if (!classData)
        {
            return AZ::Failure(classData.GetError());
        }

        int currentVersion = classData.GetValue()->m_version;

        AZ::JsonSerializerSettings settings;
        settings.m_reporting = [&file]([[maybe_unused]] AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, [[maybe_unused]] AZStd::string_view path)
        {
            AZ_UNUSED(file); // AZ_Warning doesn't exist on release builds so file may not be used in that case

            if (result.GetOutcome() > AZ::JsonSerializationResult::Outcomes::PartialDefaults)
            {
                AZ_Warning(
                    "MetadataManager",
                    false,
                    "Metadata file `" AZ_STRING_FORMAT "`: Path " AZ_STRING_FORMAT " - " AZ_STRING_FORMAT "\n",
                    AZ_STRING_ARG(file.Native()),
                    AZ_STRING_ARG(path),
                    AZ_STRING_ARG(message));
            }

            return result;
        };

        // Encode the version into JSON
        rapidjson::Value serializedVersion;
        AZ::JsonSerialization::Store(serializedVersion, document.GetAllocator(), &MetadataVersion, nullptr, azrtti_typeid<int>(), settings);

        // Encode the value into JSON
        rapidjson::Value serializedValue;
        auto resultCode = AZ::JsonSerialization::Store(serializedValue, document.GetAllocator(), inValue, nullptr, typeId, settings);

        // Try to insert the version of the type being serialized into the serialized data
        if (serializedValue.IsObject())
        {
            if(serializedValue.GetObject().HasMember(MetadataObjectVersionField))
            {
                AZ_Warning(
                    "MetadataManager",
                    false,
                    "Type %s (%s) already has reserved field %s which is intended for version tracking",
                    classData.GetValue()->m_name,
                    classData.GetValue()->m_typeId.ToFixedString().c_str(),
                    MetadataObjectVersionField);
            }
            else
            {
                rapidjson::Value versionValue;
                versionValue.SetInt(currentVersion);
                serializedValue.GetObject().AddMember(
                    rapidjson::Value(MetadataObjectVersionField, document.GetAllocator()).Move(), versionValue, document.GetAllocator());
            }
        }
        else
        {
            // In this case, the serialized type is not an object (likely the type is just a primitive), so skip trying to save a version for it
        }

        if (resultCode.GetProcessing() != AZ::JsonSerializationResult::Processing::Halted)
        {
            // Create the file version JSON entry in the document and store the encoded value
            // This will update the saved version to the current version
            // Note that this is the version of the entire saved document (the metadata version), not the version of the type being saved
            rapidjson::Value& versionStore = versionPointer.Create(document, document.GetAllocator());
            versionStore = AZStd::move(serializedVersion);

            // Create the user JSON entry in the document and store the encoded value
            rapidjson::Value& store = pointer.Create(document, document.GetAllocator());
            store = AZStd::move(serializedValue);

            // Save JSON to disk
            return AZ::JsonSerializationUtils::WriteJsonFile(document, path.Native());
        }

        return AZ::Failure(AZStd::string::format(
            "Metadata file " AZ_STRING_FORMAT " - failed to serialize " AZ_STRING_FORMAT " value to JSON: " AZ_STRING_FORMAT,
            AZ_STRING_ARG(file.Native()),
            AZ_STRING_ARG(key),
            AZ_STRING_ARG(resultCode.ToString(""))));
    }

    AZ::IO::Path MetadataManager::ToMetadataPath(AZ::IO::Path path)
    {
        if (path.Extension() != MetadataFileExtension)
        {
            path.ReplaceExtension(AZ::IO::PathView(AZStd::string(path.Extension().Native()) + AZStd::string(MetadataFileExtension)));
        }

        return path;
    }

    AZ::Outcome<const AZ::SerializeContext::ClassData*, AZStd::string> MetadataManager::GetClassData(AZ::Uuid typeId)
    {
        AZ::SerializeContext* serializeContext{};
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

        if (!serializeContext)
        {
            return AZ::Failure(AZStd::string("Could not retrieve SerializeContext"));
        }

        auto* classData = serializeContext->FindClassData(typeId);

        if (!classData)
        {
            return AZ::Failure(AZStd::string::format("Type " AZ_STRING_FORMAT " must be registered with serialize context.", AZ_STRING_ARG(typeId.ToFixedString())));
        }

        return AZ::Success(classData);
    }
}
