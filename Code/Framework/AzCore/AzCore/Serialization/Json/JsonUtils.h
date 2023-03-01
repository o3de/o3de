/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Serialization/ObjectStream.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/std/string/string.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/writer.h>
#include <AzCore/Serialization/Json/JsonSerializationResult.h>

namespace AZ
{
    namespace IO
    {
        class GenericStream;
    }

    struct JsonSerializerSettings;
    struct JsonDeserializerSettings;

    // Utility functions which use json serializer/deserializer to save/load object to file/stream 
    namespace JsonSerializationUtils
    {
        struct WriteJsonSettings
        {
            int m_maxDecimalPlaces = -1; // -1 means use default
        };

        ///////////////////////////////////////////////////////////////////////////////////
        // Save functions

        //! Save a json document to text. Otherwise returns a failure with error message.
        AZ::Outcome<void, AZStd::string> WriteJsonString(const rapidjson::Document& document, AZStd::string& jsonText, WriteJsonSettings settings = WriteJsonSettings{});

        //! Save a json document to a file. Otherwise returns a failure with error message.
        AZ::Outcome<void, AZStd::string> WriteJsonFile(const rapidjson::Document& document, AZStd::string_view filePath, WriteJsonSettings settings = WriteJsonSettings{});

        //! Save a json document to a stream. Otherwise returns a failure with error message.
        AZ::Outcome<void, AZStd::string> WriteJsonStream(const rapidjson::Document& document, IO::GenericStream& stream, WriteJsonSettings settings = WriteJsonSettings{});

        AZ::Outcome<void, AZStd::string> SaveObjectToStreamByType(const void* objectPtr, const Uuid& objectType, IO::GenericStream& stream,
            const void* defaultObjectPtr = nullptr, const JsonSerializerSettings* settings = nullptr);
        AZ::Outcome<void, AZStd::string> SaveObjectToFileByType(const void* objectPtr, const Uuid& objectType, const AZStd::string& filePath,
            const void* defaultObjectPtr = nullptr, const JsonSerializerSettings* settings = nullptr);

        template <typename ObjectType>
        AZ::Outcome<void, AZStd::string> SaveObjectToStream(const ObjectType* classPtr, IO::GenericStream& stream,
            const ObjectType* defaultClassPtr = nullptr, const JsonSerializerSettings* settings = nullptr)
        {
            return SaveObjectToStreamByType(classPtr, AzTypeInfo<ObjectType>::Uuid(), stream, defaultClassPtr, settings);
        }

        template <typename ObjectType>
        AZ::Outcome<void, AZStd::string> SaveObjectToFile(const ObjectType* classPtr, const AZStd::string& filePath,
            const ObjectType* defaultClassPtr = nullptr, const JsonSerializerSettings* settings = nullptr)
        {
            return SaveObjectToFileByType(classPtr, AzTypeInfo<ObjectType>::Uuid(), filePath, defaultClassPtr, settings);
        }

        ///////////////////////////////////////////////////////////////////////////////////
        // Load functions

        //! Parse json text. Returns a failure with error message if the content is not valid JSON.
        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonString(AZStd::string_view jsonText);

        //! Parse a json file. Returns a failure with error message if the content is not valid JSON or if
        //! the file size is larger than the max file size provided.
        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonFile(
            AZStd::string_view filePath, size_t maxFileSize = AZStd::numeric_limits<size_t>::max());

        //! Parse a json stream. Returns a failure with error message if the content is not valid JSON.
        AZ::Outcome<rapidjson::Document, AZStd::string> ReadJsonStream(IO::GenericStream& stream);
        
        //! Load object with known class type
        //! Even if errorsOut contains errors, the load to an object could have succeeded
        AZ::Outcome<void, AZStd::string> LoadObjectFromStringByType(void* objectToLoad, const Uuid& objectType, AZStd::string_view source,
            AZStd::string& errorsOut, const JsonDeserializerSettings* settings = nullptr);

        AZ::Outcome<void, AZStd::string> LoadObjectFromStreamByType(void* objectToLoad, const Uuid& objectType, IO::GenericStream& stream,
            AZStd::string& errorsOut, const JsonDeserializerSettings* settings = nullptr);

        template <typename ObjectType>
        AZ::Outcome<void, AZStd::string> LoadObjectFromStream(ObjectType& objectToLoad, IO::GenericStream& stream
            , AZStd::string& errorsOut, const JsonDeserializerSettings* settings = nullptr)
        {
            return LoadObjectFromStreamByType(&objectToLoad, AzTypeInfo<ObjectType>::Uuid(), stream, settings);
        }

        //! Load object with known class type
        //! returns a failure on any error encountered during loading
        AZ::Outcome<void, AZStd::string> LoadObjectFromStringByType(void* objectToLoad, const Uuid& objectType, AZStd::string_view source,
            const JsonDeserializerSettings* settings = nullptr);

        AZ::Outcome<void, AZStd::string> LoadObjectFromStreamByType(void* objectToLoad, const Uuid& objectType, IO::GenericStream& stream,
            const JsonDeserializerSettings* settings = nullptr);

        template <typename ObjectType>
        AZ::Outcome<void, AZStd::string> LoadObjectFromStream(ObjectType& objectToLoad, IO::GenericStream& stream, const JsonDeserializerSettings* settings = nullptr)
        {
            return LoadObjectFromStreamByType(&objectToLoad, AzTypeInfo<ObjectType>::Uuid(), stream, settings);
        }

        template <typename ObjectType>
        AZ::Outcome<void, AZStd::string> LoadObjectFromFile(ObjectType& objectToLoad, const AZStd::string& filePath, const JsonDeserializerSettings* settings = nullptr)
        {
            AZ::IO::FileIOStream inputFileStream;
            if (!inputFileStream.Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeText))
            {
                return AZ::Failure(AZStd::string::format("Error opening file '%s' for reading", filePath.c_str()));
            }
            return LoadObjectFromStream(objectToLoad, inputFileStream, settings);
        }

        //! Load any object
        AZ::Outcome<AZStd::any, AZStd::string> LoadAnyObjectFromStream(IO::GenericStream& stream, const JsonDeserializerSettings* settings = nullptr);
        AZ::Outcome<AZStd::any, AZStd::string> LoadAnyObjectFromFile(const AZStd::string& filePath,const JsonDeserializerSettings* settings = nullptr);

        //! Provides a common way to report errors and warnings when processing Atom assets with JsonSerialization.
        class JsonReportingHelper
        {
        public:
            //! Attach this helper to the JsonSerializerSettings reporting callback
            void Attach(JsonSerializerSettings& settings)
            {
                settings.m_reporting = [this](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, AZStd::string_view path)
                {
                    return Reporting(message, resultCode, path);
                };
            }

            //! Attach this helper to the JsonDeserializerSettings reporting callback
            void Attach(JsonDeserializerSettings& settings)
            {
                settings.m_reporting = [this](AZStd::string_view message, AZ::JsonSerializationResult::ResultCode resultCode, AZStd::string_view path)
                {
                    return Reporting(message, resultCode, path);
                };
            }

            bool WarningsReported() const
            {
                return m_warningsReported;
            }

            bool ErrorsReported() const
            {
                return m_errorsReported;
            }

            AZStd::string GetErrorMessage() const
            {
                return m_firstErrorMessage;
            }

        private:
            AZ::JsonSerializationResult::ResultCode Reporting(AZStd::string_view message, AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
            {
                if (result.GetOutcome() == JsonSerializationResult::Outcomes::Skipped)
                {
                    m_warningsReported = true;
                    AZ_Warning("JSON", false, "Skipped unrecognized field '%.*s'", AZ_STRING_ARG(path));
                }
                else if (result.GetProcessing() != JsonSerializationResult::Processing::Completed ||
                    result.GetOutcome() >= JsonSerializationResult::Outcomes::Unavailable)
                {
                    if (result.GetOutcome() >= JsonSerializationResult::Outcomes::Catastrophic)
                    {
                        m_errorsReported = true;
                        AZ_Error("JSON", false, "'%.*s': %.*s - %s", AZ_STRING_ARG(path), AZ_STRING_ARG(message), result.ToString("").c_str());

                        if (m_firstErrorMessage.empty())
                        {
                            m_firstErrorMessage = message;
                        }
                    }
                    else
                    {
                        m_warningsReported = true;
                        AZ_Warning("JSON", false, "'%.*s': %.*s - %s", AZ_STRING_ARG(path), AZ_STRING_ARG(message), result.ToString("").c_str());
                    }
                }

                return result;
            }

            bool m_warningsReported = false;
            bool m_errorsReported = false;
            AZStd::string m_firstErrorMessage;
        };

    } // namespace JsonSerializationUtils
} // namespace Az
