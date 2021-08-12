/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Edit/Common/JsonUtils.h>

#include <AzCore/JSON/filereadstream.h>

namespace AZ
{
    namespace RPI
    {
        namespace JsonUtils
        {
            Outcome<DeserializedCpuData, AZStd::string> LoadSavedCpuProfilingStatistics(const AZStd::string& capturePath)
            {
                auto* base = IO::FileIOBase::GetInstance();

                char resolvedPath[IO::MaxPathLength];
                if (!base->ResolvePath(capturePath.c_str(), resolvedPath, IO::MaxPathLength))
                {
                    return Failure(AZStd::string::format("Could not resolve the path to file %s, is the path correct?", resolvedPath));
                }

                u64 captureSizeBytes;
                const IO::Result fileSizeResult = base->Size(resolvedPath, captureSizeBytes);
                if (!fileSizeResult)
                {
                    return Failure(AZStd::string::format("Could not read the size of file %s, is the path correct?", resolvedPath));
                }

                // NOTE: this uses raw file pointers over the abstractions and utility functions provided by AZ::JsonSerializationUtils because
                // saved profiling captures can be upwards of 400 MB. This necessitates a buffered approach to avoid allocating huge chunks of memory.
                FILE* fp = nullptr;
                azfopen(&fp, resolvedPath, "rb");
                if (!fp)
                {
                    return Failure(AZStd::string::format("Could not fopen file %s, is the path correct?\n", resolvedPath));
                }

                constexpr AZStd::size_t MaxBufSize = 65536;
                const AZStd::size_t bufSize = AZStd::min(MaxBufSize, captureSizeBytes);
                char* buf = reinterpret_cast<char*>(azmalloc(bufSize));

                rapidjson::Document document;
                rapidjson::FileReadStream inputStream(fp, buf, bufSize);
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
