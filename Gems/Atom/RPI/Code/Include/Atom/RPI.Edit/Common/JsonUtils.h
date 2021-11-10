/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>

namespace AZ
{
    namespace RPI
    {
        namespace JsonUtils
        {
            //! Protects from allocating too much memory. The choice of a 1MB threshold is arbitrary.
            //! If you need to work with larger files, please use AZ::IO directly instead of these utility functions.
            inline constexpr size_t DefaultMaxFileSize = 1024 * 1024;

            // Declarations...

            //! Loads serialized object data from a json file at the specified path
            //! Errors will be reported using AZ trace
            template<typename ObjectType>
            bool LoadObjectFromFile(const AZStd::string& path, ObjectType& objectData);

            //! Saves serialized object data to a json file at the specified path
            //! Errors will be reported using AZ trace
            template<typename ObjectType>
            bool SaveObjectToFile(const AZStd::string& path, const ObjectType& objectData);

            // Definitions...

            template<typename ObjectType>
            bool LoadObjectFromFile(const AZStd::string& path, ObjectType& objectData)
            {
                objectData = ObjectType();

                auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(path, DefaultMaxFileSize);
                if (!loadOutcome.IsSuccess())
                {
                    AZ_Error("AZ::RPI::JsonUtils", false, "%s", loadOutcome.GetError().c_str());
                    return false;
                }

                rapidjson::Document& document = loadOutcome.GetValue();

                AZ::RPI::JsonFileLoadContext fileLoadContext;
                fileLoadContext.PushFilePath(path);

                AZ::JsonDeserializerSettings jsonSettings;
                AZ::RPI::JsonReportingHelper reportingHelper;
                reportingHelper.Attach(jsonSettings);
                jsonSettings.m_metadata.Add(AZStd::move(fileLoadContext));

                AZ::JsonSerialization::Load(objectData, document, jsonSettings);
                if (reportingHelper.ErrorsReported())
                {
                    AZ_Error("AZ::RPI::JsonUtils", false, "Failed to load object from JSON file: %s", path.c_str());
                    return false;
                }

                return true;
            }

            template<typename ObjectType>
            bool SaveObjectToFile(const AZStd::string& path, const ObjectType& objectData)
            {
                rapidjson::Document document;
                document.SetObject();

                AZ::JsonSerializerSettings settings;
                AZ::RPI::JsonReportingHelper reportingHelper;
                reportingHelper.Attach(settings);

                AZ::JsonSerialization::Store(document, document.GetAllocator(), objectData, settings);
                if (reportingHelper.ErrorsReported())
                {
                    AZ_Error("AZ::RPI::JsonUtils", false, "Failed to write object data to JSON document: %s", path.c_str());
                    return false;
                }

                AZ::JsonSerializationUtils::WriteJsonFile(document, path);
                if (reportingHelper.ErrorsReported())
                {
                    AZ_Error("AZ::RPI::JsonUtils", false, "Failed to write JSON document to file: %s", path.c_str());
                    return false;
                }

                return true;
            }

            template<typename ObjectType>
            static bool SaveObjectToJsonString(const ObjectType& objectData, AZStd::string& jsonString)
            {
                jsonString.reserve(1024);
                AZ::IO::ByteContainerStream<AZStd::string> byteStream(&jsonString);

                auto saveResult = AZ::JsonSerializationUtils::SaveObjectToStreamByType(&objectData, azrtti_typeid(objectData), byteStream, nullptr, nullptr);
                AZ_Error("AZ::RPI::JsonUtils", saveResult.IsSuccess(), "Failed to convert object to json string: %s", saveResult.GetError().c_str());
                return saveResult.IsSuccess();
            }

            template<typename ObjectType>
            static bool LoadObjectFromJsonString(const AZStd::string& jsonString, ObjectType& objectData)
            {
                IO::MemoryStream stream(jsonString.data(), jsonString.size());

                Outcome<AZStd::any, AZStd::string> loadResult = JsonSerializationUtils::LoadAnyObjectFromStream(stream);
                if (loadResult.IsSuccess())
                {
                    objectData = AZStd::any_cast<ObjectType>(loadResult.TakeValue());
                    return true;
                }
                AZ_Error("AZ::RPI::JsonUtils", false, "Failed to load object from json string: %s", loadResult.GetError().c_str());
                return false;
            }

        } // namespace JsonUtils
    } // namespace RPI
} // namespace AZ

