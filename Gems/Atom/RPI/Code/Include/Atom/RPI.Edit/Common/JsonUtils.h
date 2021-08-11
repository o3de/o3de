/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/document.h>
#include <AzCore/JSON/filereadstream.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>

#include <AtomCore/Serialization/Json/JsonUtils.h>

#include <Atom/RHI/CpuProfilerImpl.h>
#include <Atom/RPI.Edit/Common/JsonFileLoadContext.h>
#include <Atom/RPI.Edit/Common/JsonReportingHelper.h>

namespace AZ
{
    namespace RPI
    {
        namespace JsonUtils
        {
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

                auto loadOutcome = AZ::JsonSerializationUtils::ReadJsonFile(path);
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

            using DeserializedCpuData = AZStd::vector<RHI::CpuProfilingStatisticsSerializer::CpuProfilingStatisticsSerializerEntry>;
            inline Outcome<DeserializedCpuData, AZStd::string> LoadSavedCpuProfilingStatistics(const AZStd::string& capturePath)
            {
                const auto* base = IO::FileIOBase::GetInstance();

                char resolvedPath[IO::MaxPathLength];
                base->ResolvePath(capturePath.c_str(), resolvedPath, IO::MaxPathLength);

                // NOTE: this uses raw file pointers over the abstractions and utility functions provided by AZ::JsonSerializationUtils because
                // saved profiling captures can be upwards of 400 MB. This necessitates a buffered approach to avoid allocating huge chunks of memory.
                FILE* fp = nullptr;
                azfopen(&fp, resolvedPath, "rb");
                if (!fp)
                {
                    return Failure(AZStd::string::format("Could not fopen file %s, is the path correct?\n", resolvedPath));
                }

                constexpr AZStd::size_t BufSize = 65536;
                char* buf = reinterpret_cast<char*>(azmalloc(BufSize));

                rapidjson::Document document;
                rapidjson::FileReadStream inputStream(fp, buf, BufSize);
                document.ParseStream(inputStream);

                azfree(buf);
                fclose(fp);

                if (document.HasParseError())
                {
                    const auto pe = document.GetParseError();
                    return Failure(AZStd::string::format(
                        "Rapidjson could not parse the document with ParseErrorCode %u. See 3rdParty/rapidjson/error.h for definitions.\n", pe));
                }

                if (!document.IsObject() || !document.HasMember("ClassData"))
                {
                    return Failure(AZStd::string::format(
                        "Error in loading saved capture: top-level object does not have a ClassData field. Did the serialization format change recently?\n"));
                }

                AZ_TracePrintf("JsonUtils", "Successfully loaded JSON into memory.\n");

                const auto& root = document["ClassData"];
                RHI::CpuProfilingStatisticsSerializer serializer;
                const JsonSerializationResult::ResultCode deserializationResult = JsonSerialization::Load(serializer, root);
                if (deserializationResult.GetProcessing() == JsonSerializationResult::Processing::Halted
                    || serializer.m_cpuProfilingStatisticsSerializerEntries.empty())
                {
                    return Failure(AZStd::string::format("Error in deserializing document: %s\n", deserializationResult.ToString(capturePath.c_str()).c_str()));
                }

                AZ_TracePrintf("JsonUtils", "Successfully loaded CPU profiling data with %zu profiling entries.\n",
                     serializer.m_cpuProfilingStatisticsSerializerEntries.size());

                return Success(AZStd::move(serializer.m_cpuProfilingStatisticsSerializerEntries));
            }
        } // namespace JsonUtils
    } // namespace RPI
} // namespace AZ

