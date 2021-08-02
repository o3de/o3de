/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/IO/Path/Path_fwd.h>
#include <AzCore/JSON/document.h>
#include <AzCore/JSON/stringbuffer.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/utils.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class CommandLine;
    class Entity;
    class ModuleEntity;
    class SerializeContext;
    struct Uuid;

    namespace SerializeContextTools
    {
        class Application;

        class Converter
        {
        public:
            static bool ConvertObjectStreamFiles(Application& application);
            static bool ConvertApplicationDescriptor(Application& application);
            //! Converts Windows INI Style File
            //! Can be used to convert *.ini and *.cfg files
            static bool ConvertConfigFile(Application& application);

        protected:
            using PathDocumentPair = AZStd::pair<AZStd::string, rapidjson::Document>;
            using PathDocumentContainer = AZStd::vector<PathDocumentPair>;

            static bool ConvertSystemSettings(PathDocumentContainer& documents, const ComponentApplication::Descriptor& descriptor, 
                const AZStd::string& configurationName, const AZ::IO::PathView& projectFolder, const AZStd::string& applicationRoot);
            static bool ConvertSystemComponents(PathDocumentContainer& documents, const Entity& entity,
                const AZStd::string& configurationName, const AZ::IO::PathView& projectFolder,
                const JsonSerializerSettings& convertSettings, const JsonDeserializerSettings& verifySettings);
            static bool ConvertModuleComponents(PathDocumentContainer& documents, const ModuleEntity& entity, const AZStd::string& configurationName,
                const JsonSerializerSettings& convertSettings, const JsonDeserializerSettings& verifySettings);
            static bool VerifyConvertedData(rapidjson::Value& convertedData, const void* original, const Uuid& originalType,
                const JsonDeserializerSettings& settings);

            static AZStd::string GetClassName(const Uuid& classId, SerializeContext* context);
            static bool WriteDocumentToDisk(const AZStd::string& filename, const rapidjson::Document& document, AZStd::string_view documentRoot,
                rapidjson::StringBuffer& scratchBuffer);

            static void SetupLogging(AZStd::string& scratchBuffer, JsonSerializationResult::JsonIssueCallback& callback,
                const AZ::CommandLine& commandLine);
            static JsonSerializationResult::ResultCode VerboseLogging(AZStd::string& scratchBuffer, AZStd::string_view message,
                JsonSerializationResult::ResultCode result, AZStd::string_view target);
            static AZ::JsonSerializationResult::ResultCode SimpleLogging(AZStd::string& scratchBuffer, AZStd::string_view message,
                JsonSerializationResult::ResultCode result, AZStd::string_view target);
        };
    } // namespace SerializeContextTools
} // namespace AZ
