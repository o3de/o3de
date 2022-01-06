/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Utils/Utils.h>
#include <AzCore/base.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/ByteContainerStream.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/IO/TextStreamWriters.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Utils/Utils.h>

#include <AzCore/Serialization/Json/JsonUtils.h>

namespace AZ::JsonSerializationUtils
{
    static const char* FileTypeTag = "Type";
    static const char* FileType = "JsonSerialization";
    static const char* VersionTag = "Version";
    static const char* ClassNameTag = "ClassName";
    static const char* ClassDataTag = "ClassData";

    AZ::Outcome<void, AZStd::string> WriteJsonString(const rapidjson::Document& document, AZStd::string& jsonText, WriteJsonSettings settings)
    {
        AZ::IO::ByteContainerStream<AZStd::string> stream{&jsonText};
        return WriteJsonStream(document, stream, settings);
    }

    AZ::Outcome<void, AZStd::string> WriteJsonFile(const rapidjson::Document& document, AZStd::string_view filePath, WriteJsonSettings settings)
    {
        // Write the json into memory first and then write the file, rather than passing a file stream to rapidjson.
        // This should avoid creating a large number of micro-writes to the file.
        AZStd::string fileContent;
        auto outcome = WriteJsonString(document, fileContent, settings);
        if (!outcome.IsSuccess())
        {
            return outcome;
        }

        return AZ::Utils::WriteFile(fileContent, filePath);
    }

    AZ::Outcome<void, AZStd::string> WriteJsonStream(const rapidjson::Document& document, IO::GenericStream& stream, WriteJsonSettings settings)
    {
        AZ::IO::RapidJSONStreamWriter jsonStreamWriter(&stream);

        rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter> writer(jsonStreamWriter);

        if (settings.m_maxDecimalPlaces >= 0)
        {
            writer.SetMaxDecimalPlaces(settings.m_maxDecimalPlaces);
        }

        if (document.Accept(writer))
        {
            return AZ::Success();
        }
        else
        {
            return AZ::Failure(AZStd::string{"Json Writer failed"});
        }
    }

    AZ::Outcome<void, AZStd::string> SaveObjectToStreamByType(const void* objectPtr, const Uuid& classId, IO::GenericStream& stream,
         const void* defaultObjectPtr, const JsonSerializerSettings* settings)
    {
        if (!stream.CanWrite())
        {
            return AZ::Failure(AZStd::string("The GenericStream can't be written to"));
        }

        JsonSerializerSettings saveSettings;
        if (settings)
        {
            saveSettings = *settings;
        }

        AZ::SerializeContext* serializeContext = saveSettings.m_serializeContext;
        if (!serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!serializeContext)
            {
                return AZ::Failure(AZStd::string::format("Need SerializeContext for saving"));
            }
            saveSettings.m_serializeContext = serializeContext;
        }

        rapidjson::Document jsonDocument;
        jsonDocument.SetObject();
        jsonDocument.AddMember(rapidjson::StringRef(FileTypeTag), rapidjson::StringRef(FileType), jsonDocument.GetAllocator());

        rapidjson::Value serializedObject;

        JsonSerializationResult::ResultCode jsonResult = JsonSerialization::Store(serializedObject, jsonDocument.GetAllocator(),
            objectPtr, defaultObjectPtr, classId, saveSettings);

        if (jsonResult.GetProcessing() != JsonSerializationResult::Processing::Completed)
        {
            return AZ::Failure(jsonResult.ToString(""));
        }

        const SerializeContext::ClassData* classData = serializeContext->FindClassData(classId);

        jsonDocument.AddMember(rapidjson::StringRef(VersionTag), 1, jsonDocument.GetAllocator());
        jsonDocument.AddMember(rapidjson::StringRef(ClassNameTag), rapidjson::StringRef(classData->m_name), jsonDocument.GetAllocator());
        jsonDocument.AddMember(rapidjson::StringRef(ClassDataTag), AZStd::move(serializedObject), jsonDocument.GetAllocator());

        AZ::IO::RapidJSONStreamWriter jsonStreamWriter(&stream);
        rapidjson::PrettyWriter<AZ::IO::RapidJSONStreamWriter> writer(jsonStreamWriter);
        bool jsonWriteResult = jsonDocument.Accept(writer);
        if (!jsonWriteResult)
        {
            return AZ::Failure(AZStd::string::format("Unable to write class %s with json serialization format'",
                classId.ToString<AZStd::string>().data()));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> SaveObjectToFileByType(const void* classPtr, const Uuid& classId, const AZStd::string& filePath,
        const void* defaultClassPtr, const JsonSerializerSettings* settings)
    {
        AZStd::vector<char> buffer;
        buffer.reserve(1024);
        AZ::IO::ByteContainerStream<AZStd::vector<char> > byteStream(&buffer);
        auto saveResult = SaveObjectToStreamByType(classPtr, classId, byteStream, defaultClassPtr, settings);
        if (saveResult.IsSuccess())
        {
            AZ::IO::FileIOStream outputFileStream;
            if (!outputFileStream.Open(filePath.c_str(), AZ::IO::OpenMode::ModeWrite | AZ::IO::OpenMode::ModeCreatePath | AZ::IO::OpenMode::ModeText))
            {
                return AZ::Failure(AZStd::string::format("Error opening file '%s' for writing", filePath.c_str()));
            }
            outputFileStream.Write(buffer.size(), buffer.data());
        }
        return saveResult;
    }

    // Helper function to check whether the load outcome was success (for loading json serialization file)
    bool WasLoadSuccess(JsonSerializationResult::Outcomes outcome)
    {
        return (outcome == JsonSerializationResult::Outcomes::Success
            || outcome == JsonSerializationResult::Outcomes::DefaultsUsed
            || outcome == JsonSerializationResult::Outcomes::PartialDefaults);
    }

    AZ::Outcome<void, AZStd::string> PrepareDeserializerSettings(const JsonDeserializerSettings* inputSettings, JsonDeserializerSettings& returnSettings
        , AZStd::string& deserializeError)
    {
        if (inputSettings)
        {
            returnSettings = *inputSettings;
        }

        if (!returnSettings.m_serializeContext)
        {
            AZ::ComponentApplicationBus::BroadcastResult(returnSettings.m_serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
            if (!returnSettings.m_serializeContext)
            {
                return AZ::Failure(AZStd::string("Need SerializeContext for loading"));
            }
        }

        // Report unused data field as error by default
        auto reporting = returnSettings.m_reporting;
        auto issueReportingCallback = [&deserializeError, reporting](AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view target) -> JsonSerializationResult::ResultCode
        {
            using namespace JsonSerializationResult;

            if (!WasLoadSuccess(result.GetOutcome()))
            {
                // This if is a hack around fault in the JSON serialization system
                // Jira: LY-106587
                if (message != "No part of the string could be interpreted as a uuid.")
                {
                    deserializeError.append(message);
                    deserializeError.append(AZStd::string::format(" '%s' \n", target.data()));
                }
            }

            if (reporting)
            {
                result = reporting(message, result, target);
            }

            return result;
        };

        returnSettings.m_reporting = issueReportingCallback;

        return AZ::Success();
    }


    AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonString(AZStd::string_view jsonText)
    {
        if (jsonText.empty())
        {
            return AZ::Failure(AZStd::string("Failed to parse JSON: input string is empty."));
        }

        rapidjson::Document jsonDocument;
        jsonDocument.Parse<rapidjson::kParseCommentsFlag>(jsonText.data(), jsonText.size());
        if (jsonDocument.HasParseError())
        {
            size_t lineNumber = 1;

            const size_t errorOffset = jsonDocument.GetErrorOffset();
            for (size_t searchOffset = jsonText.find('\n');
                searchOffset < errorOffset && searchOffset < AZStd::string::npos;
                searchOffset = jsonText.find('\n', searchOffset + 1))
            {
                lineNumber++;
            }

            return AZ::Failure(AZStd::string::format("JSON parse error at line %zu: %s", lineNumber, rapidjson::GetParseError_En(jsonDocument.GetParseError())));
        }
        else
        {
            return AZ::Success(AZStd::move(jsonDocument));
        }
    }

    AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonStream(IO::GenericStream& stream)
    {
        IO::SizeType length = stream.GetLength();

        AZStd::vector<char> memoryBuffer;
        memoryBuffer.resize_no_construct(static_cast<AZStd::vector<char>::size_type>(static_cast<AZStd::vector<char>::size_type>(length) + 1));

        IO::SizeType bytesRead = stream.Read(length, memoryBuffer.data());
        if (bytesRead != length)
        {
            return AZ::Failure(AZStd::string{"Cannot to read input stream."});
        }

        memoryBuffer.back() = 0;

        return ReadJsonString(AZStd::string_view{memoryBuffer.data(), memoryBuffer.size()});
    }

    AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFile(AZStd::string_view filePath, size_t maxFileSize)
    {
        // Read into memory first and then parse the json, rather than passing a file stream to rapidjson.
        // This should avoid creating a large number of micro-reads from the file.

        auto readResult = AZ::Utils::ReadFile<AZStd::string>(filePath, maxFileSize);
        if(!readResult.IsSuccess())
        {
            return AZ::Failure(readResult.GetError());
        }

        AZStd::string jsonContent = readResult.TakeValue();

        auto result = ReadJsonString(jsonContent);
        if (!result.IsSuccess())
        {
            return AZ::Failure(AZStd::string::format("Failed to load '%.*s'. %s", AZ_STRING_ARG(filePath), result.GetError().c_str()));
        }
        else
        {
            return result;
        }
    }

    // Helper function to validate the JSON is structured with the standard header for a generic class
    AZ::Outcome<void, AZStd::string> ValidateJsonClassHeader(const rapidjson::Document& jsonDocument)
    {
        auto typeItr = jsonDocument.FindMember(FileTypeTag);
        if (typeItr == jsonDocument.MemberEnd() || !typeItr->value.IsString() || azstricmp(typeItr->value.GetString(), FileType) != 0)
        {
            return AZ::Failure(AZStd::string::format("Not a valid JsonSerialization file"));
        }

        auto nameItr = jsonDocument.FindMember(ClassNameTag);
        if (nameItr == jsonDocument.MemberEnd() || !nameItr->value.IsString())
        {
            return AZ::Failure(AZStd::string::format("File should contain ClassName"));
        }

        auto dataItr = jsonDocument.FindMember(ClassDataTag);
        // data can be empty but it should be an object
        if (dataItr != jsonDocument.MemberEnd() && !dataItr->value.IsObject())
        {
            return AZ::Failure(AZStd::string::format("ClassData should be an object"));
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> LoadObjectFromStringByType(void* objectToLoad, const Uuid& classId, AZStd::string_view stream,
        const JsonDeserializerSettings* settings)
    {
        JsonDeserializerSettings loadSettings;
        AZStd::string deserializeErrors;
        auto prepare = PrepareDeserializerSettings(settings, loadSettings, deserializeErrors);
        if (!prepare.IsSuccess())
        {
            return AZ::Failure(prepare.GetError());
        }

        auto parseResult = ReadJsonString(stream);
        if (!parseResult.IsSuccess())
        {
            return AZ::Failure(parseResult.GetError());
        }

        const rapidjson::Document& jsonDocument = parseResult.GetValue();

        auto validateResult = ValidateJsonClassHeader(jsonDocument);
        if (!validateResult.IsSuccess())
        {
            return AZ::Failure(validateResult.GetError());
        }

        const char* className = jsonDocument.FindMember(ClassNameTag)->value.GetString();

        // validate class name
        auto classData = loadSettings.m_serializeContext->FindClassData(classId);
        if (!classData)
        {
            return AZ::Failure(AZStd::string::format("Try to load class from Id %s", classId.ToString<AZStd::string>().c_str()));
        }

        if (azstricmp(classData->m_name, className) != 0)
        {
            return AZ::Failure(AZStd::string::format("Try to load class %s from class %s data", classData->m_name, className));
        }

        JsonSerializationResult::ResultCode result = JsonSerialization::Load(objectToLoad, classId, jsonDocument.FindMember(ClassDataTag)->value, loadSettings);

        if (!WasLoadSuccess(result.GetOutcome()) || !deserializeErrors.empty())
        {
            return AZ::Failure(deserializeErrors);
        }

        return AZ::Success();
    }

    AZ::Outcome<void, AZStd::string> LoadObjectFromStreamByType(void* objectToLoad, const Uuid& classId, IO::GenericStream& stream,
        const JsonDeserializerSettings* settings)
    {
        JsonDeserializerSettings loadSettings;
        AZStd::string deserializeErrors;
        auto prepare = PrepareDeserializerSettings(settings, loadSettings, deserializeErrors);
        if (!prepare.IsSuccess())
        {
            return AZ::Failure(prepare.GetError());
        }

        auto parseResult = ReadJsonStream(stream);
        if (!parseResult.IsSuccess())
        {
            return AZ::Failure(parseResult.GetError());
        }

        const rapidjson::Document& jsonDocument = parseResult.GetValue();

        auto validateResult = ValidateJsonClassHeader(jsonDocument);
        if (!validateResult.IsSuccess())
        {
            return AZ::Failure(validateResult.GetError());
        }

        const char* className = jsonDocument.FindMember(ClassNameTag)->value.GetString();

        // validate class name
        auto classData = loadSettings.m_serializeContext->FindClassData(classId);
        if (!classData)
        {
            return AZ::Failure(AZStd::string::format("Try to load class from Id %s", classId.ToString<AZStd::string>().c_str()));
        }

        if (azstricmp(classData->m_name, className) != 0)
        {
            return AZ::Failure(AZStd::string::format("Try to load class %s from class %s data", classData->m_name, className));
        }

        JsonSerializationResult::ResultCode result = JsonSerialization::Load(objectToLoad, classId, jsonDocument.FindMember(ClassDataTag)->value, loadSettings);

        if (!WasLoadSuccess(result.GetOutcome()) || !deserializeErrors.empty())
        {
            return AZ::Failure(deserializeErrors);
        }

        return AZ::Success();
    }

    AZ::Outcome<AZStd::any, AZStd::string> LoadAnyObjectFromStream(IO::GenericStream& stream, const JsonDeserializerSettings* settings)
    {
        JsonDeserializerSettings loadSettings;
        AZStd::string deserializeErrors;
        auto prepare = PrepareDeserializerSettings(settings, loadSettings, deserializeErrors);
        if (!prepare.IsSuccess())
        {
            return AZ::Failure(prepare.GetError());
        }

        auto parseResult = ReadJsonStream(stream);
        if (!parseResult.IsSuccess())
        {
            return AZ::Failure(parseResult.GetError());
        }

        const rapidjson::Document& jsonDocument = parseResult.GetValue();

        auto validateResult = ValidateJsonClassHeader(jsonDocument);
        if (!parseResult.IsSuccess())
        {
            return AZ::Failure(parseResult.GetError());
        }

        const char* className = jsonDocument.FindMember(ClassNameTag)->value.GetString();
        AZStd::vector<AZ::Uuid> ids = loadSettings.m_serializeContext->FindClassId(AZ::Crc32(className));

        // Load with first found class id
        if (ids.size() >= 1)
        {
            auto classId = ids[0];
            AZStd::any anyData = loadSettings.m_serializeContext->CreateAny(classId);
            auto& objectData = jsonDocument.FindMember(ClassDataTag)->value;
            JsonSerializationResult::ResultCode result = JsonSerialization::Load(AZStd::any_cast<void>(&anyData), classId, objectData, loadSettings);

            if (!WasLoadSuccess(result.GetOutcome()) || !deserializeErrors.empty())
            {
                return AZ::Failure(deserializeErrors);
            }

            return AZ::Success(anyData);
        }

        return AZ::Failure(AZStd::string::format("Can't find serialize context for class %s", className));
    }

    AZ::Outcome<AZStd::any, AZStd::string> LoadAnyObjectFromFile(const AZStd::string& filePath, const JsonDeserializerSettings* settings)
    {
        AZ::IO::FileIOStream inputFileStream;
        if (!inputFileStream.Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText))
        {
            return AZ::Failure(AZStd::string::format("Error opening file '%s' for reading", filePath.c_str()));
        }
        return LoadAnyObjectFromStream(inputFileStream, settings);
    }

} // namespace AZ::JsonSerializationUtils
