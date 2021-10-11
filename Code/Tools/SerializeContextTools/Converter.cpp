/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/Entity.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/JSON/prettywriter.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/Utils.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Settings/SettingsRegistryImpl.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/Utils/Utils.h>
#include <Application.h>
#include <Converter.h>
#include <Utilities.h>

namespace AZ
{
    namespace SerializeContextTools
    {
        bool Converter::ConvertObjectStreamFiles(Application& application)
        {
            using namespace AZ::JsonSerializationResult;

            const AZ::CommandLine* commandLine = application.GetAzCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return false;
            }
            
            JsonSerializerSettings convertSettings;
            convertSettings.m_keepDefaults = commandLine->HasSwitch("keepdefaults");
            convertSettings.m_registrationContext = application.GetJsonRegistrationContext();
            convertSettings.m_serializeContext = application.GetSerializeContext();
            if (!convertSettings.m_serializeContext)
            {
                AZ_Error("Convert", false, "No serialize context found.");
                return false;
            }
            if (!convertSettings.m_registrationContext)
            {
                AZ_Error("Convert", false, "No json registration context found.");
                return false;
            }
            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            if (!commandLine->HasSwitch("ext"))
            {
                AZ_Error("Convert", false, "No extension provided through the 'ext' argument.");
                return false;
            }

            const AZStd::string& extension = commandLine->GetSwitchValue("ext", 0);
            bool isDryRun = commandLine->HasSwitch("dryrun");
            bool skipVerify = commandLine->HasSwitch("skipverify");

            JsonDeserializerSettings verifySettings;
            if (!skipVerify)
            {
                verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
                verifySettings.m_serializeContext = application.GetSerializeContext();
                SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);
            }


            bool result = true;
            rapidjson::StringBuffer scratchBuffer;

            AZStd::vector<AZStd::string> fileList = Utilities::ReadFileListFromCommandLine(application, "files");
            for (AZStd::string& filePath : fileList)
            {
                AZ_Printf("Convert", "Converting '%s'\n", filePath.c_str());

                PathDocumentContainer documents;
                auto callback = [&result, &documents, &convertSettings, &verifySettings, skipVerify]
                    (void* classPtr, const Uuid& classId, SerializeContext* /*context*/)
                {
                    rapidjson::Document document;
                    ResultCode parseResult = JsonSerialization::Store(document.SetObject(), document.GetAllocator(), classPtr, nullptr, classId, convertSettings);
                    if (parseResult.GetProcessing() != Processing::Halted)
                    {
                        if (skipVerify || VerifyConvertedData(document, classPtr, classId, verifySettings))
                        {
                            if (parseResult.GetOutcome() == Outcomes::DefaultsUsed)
                            {
                                AZ_Printf("Convert", "  File not converted as only default values were found.\n");
                            }
                            else
                            {
                                documents.emplace_back(GetClassName(classId, convertSettings.m_serializeContext), AZStd::move(document));
                            }
                        }
                        else
                        {
                            AZ_Printf("Convert", "  Verification of the converted file failed.\n");
                            result = false;
                        }
                    }
                    else
                    {
                        AZ_Printf("Convert", "  Conversion to JSON failed.\n");
                        result = false;
                    }
                    return true;
                };
                if (!Utilities::InspectSerializedFile(filePath.c_str(), convertSettings.m_serializeContext, callback))
                {
                    AZ_Warning("Convert", false, "Failed to load '%s'. File may not contain an object stream.", filePath.c_str());
                    result = false;
                }
                
                // If there's only one file, then use the original name instead of the extended name
                AZ::StringFunc::Path::ReplaceExtension(filePath, extension.c_str());
                if (documents.size() == 1)
                {
                    AZ_Printf("Convert", "  Exporting to '%s'\n", filePath.c_str());
                    if (!isDryRun)
                    {
                        AZStd::string jsonDocumentRootPrefix;
                        if (commandLine->HasSwitch("json-prefix"))
                        {
                            jsonDocumentRootPrefix = commandLine->GetSwitchValue("json-prefix", 0);
                        }

                        result = WriteDocumentToDisk(filePath, documents[0].second, jsonDocumentRootPrefix, scratchBuffer) && result;
                        scratchBuffer.Clear();
                    }
                }
                else
                {
                    AZStd::string fileName;
                    AZ::StringFunc::Path::GetFileName(filePath.c_str(), fileName);
                    for (PathDocumentPair& document : documents)
                    {
                        AZStd::string fileNameExtended = fileName;
                        fileNameExtended += '_';
                        fileNameExtended += document.first;
                        Utilities::SanitizeFilePath(fileNameExtended);
                        AZStd::string finalFilePath = filePath;
                        AZ::StringFunc::Path::ReplaceFullName(finalFilePath, fileNameExtended.c_str(), extension.c_str());

                        AZ_Printf("Convert", "  Exporting to '%s'\n", finalFilePath.c_str());
                        if (!isDryRun)
                        {
                            AZStd::string jsonDocumentRootPrefix;
                            if (commandLine->HasSwitch("json-prefix"))
                            {
                                jsonDocumentRootPrefix = commandLine->GetSwitchValue("json-prefix", 0);
                            }
                            result = WriteDocumentToDisk(finalFilePath, document.second, jsonDocumentRootPrefix, scratchBuffer) && result;
                            scratchBuffer.Clear();
                        }
                    }
                }
            }

            return result;
        }

        bool Converter::ConvertApplicationDescriptor(Application& application)
        {
            const AZ::CommandLine* commandLine = application.GetAzCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return false;
            }

            JsonSerializerSettings convertSettings;
            convertSettings.m_keepDefaults = commandLine->HasSwitch("keepdefaults");
            convertSettings.m_registrationContext = application.GetJsonRegistrationContext();
            convertSettings.m_serializeContext = application.GetSerializeContext();
            if (!convertSettings.m_serializeContext)
            {
                AZ_Error("Convert", false, "No serialize context found.");
                return false;
            }
            if (!convertSettings.m_registrationContext)
            {
                AZ_Error("Convert", false, "No json registration context found.");
                return false;
            }
            AZStd::string logggingScratchBuffer;
            SetupLogging(logggingScratchBuffer, convertSettings.m_reporting, *commandLine);

            JsonDeserializerSettings verifySettings;
            verifySettings.m_registrationContext = application.GetJsonRegistrationContext();
            verifySettings.m_serializeContext = application.GetSerializeContext();
            SetupLogging(logggingScratchBuffer, verifySettings.m_reporting, *commandLine);

            bool skipGems = commandLine->HasSwitch("skipgems");
            bool skipSystem = commandLine->HasSwitch("skipsystem");
            bool isDryRun = commandLine->HasSwitch("dryrun");

            const char* appRoot = const_cast<const Application&>(application).GetAppRoot();

            PathDocumentContainer documents;
            bool result = true;
            const AZStd::string& filePath = application.GetConfigFilePath();
            AZ_Printf("Convert", "Reading '%s' for conversion.\n", filePath.c_str());
            AZStd::string configurationName;
            if (!AZ::StringFunc::Path::GetFileName(filePath.c_str(), configurationName) ||
                configurationName.empty())
            {
                AZ_Error("Convert", false, "Unable to extract configuration from '%s'.", filePath.c_str());
                return false;
            }
            // Most folder names start with a capital letter, but most files with lower case. As the configuration name
            // will be used as a folder, turn the first letter into a capital one.
            AZStd::to_upper(configurationName.begin(), configurationName.begin() + 1);

            AZ::IO::FixedMaxPath sourceGameFolder;
            if (auto settingsRegistry = AZ::SettingsRegistry::Get();
                !settingsRegistry
                || !settingsRegistry->Get(sourceGameFolder.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_ProjectPath))
            {
                AZ_Error("Serialize Context Tools", false, "Unable to determine the game root automatically. "
                    "Make sure a default project has been set or provide a default option on the command line. (See -help for more info.)");
                return false;
            }
            
            auto callback = 
                [&result, skipGems, skipSystem, &configurationName, sourceGameFolder, &appRoot, &documents, &convertSettings, &verifySettings]
                (void* classPtr, const Uuid& classId, SerializeContext* context)
            {
                if (classId == azrtti_typeid<AZ::ComponentApplication::Descriptor>())
                {
                    if (!skipSystem)
                    {
                        result = ConvertSystemSettings(documents, *reinterpret_cast<AZ::ComponentApplication::Descriptor*>(classPtr), 
                            configurationName, sourceGameFolder, appRoot) && result;
                    }

                    // Cleanup the Serialized Element to allow any classes within the element's hierarchy to delete
                    // memory allocated by the SerializeContext
                    const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId);
                    if (classData)
                    {
                        classData->m_factory->Destroy(classPtr);
                    }
                }
                else if (classId == azrtti_typeid<Entity>())
                {
                    if (!skipSystem)
                    {
                        result = ConvertSystemComponents(documents, *reinterpret_cast<Entity*>(classPtr), configurationName,
                            sourceGameFolder, convertSettings, verifySettings) && result;
                    }
                    const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId);
                    if (classData)
                    {
                        classData->m_factory->Destroy(classPtr);
                    }
                }
                else if (classId == azrtti_typeid<ModuleEntity>())
                {
                    if (!skipGems)
                    {
                        result = ConvertModuleComponents(documents, *reinterpret_cast<ModuleEntity*>(classPtr), configurationName, 
                            convertSettings, verifySettings) && result;
                    }
                    const AZ::SerializeContext::ClassData* classData = context->FindClassData(classId);
                    if (classData)
                    {
                        classData->m_factory->Destroy(classPtr);
                    }
                }
                else
                {
                    AZ_Warning("Convert", false, "Unable to process component in Application Descriptor of type '%s'.", 
                        classId.ToString<AZStd::string>().c_str());
                    result = false;
                }
                return true;
            };
            if (!Utilities::InspectSerializedFile(filePath.c_str(), convertSettings.m_serializeContext, callback))
            {
                AZ_Warning("Convert", false, "Failed to load '%s'. File may not contain an object stream.", filePath.c_str());
                result = false;
            }
            
            if (!isDryRun)
            {
                AZStd::string jsonDocumentRootPrefix;
                if (commandLine->HasSwitch("json-prefix"))
                {
                    jsonDocumentRootPrefix = commandLine->GetSwitchValue("json-prefix", 0);
                }

                rapidjson::StringBuffer scratchBuffer;
                for (auto& pathDocPair : documents)
                {
                    result = WriteDocumentToDisk(pathDocPair.first, pathDocPair.second, jsonDocumentRootPrefix, scratchBuffer) && result;
                    scratchBuffer.Clear();
                }
            }

            return result;
        }

        bool Converter::ConvertConfigFile(Application& application)
        {
            bool result = true;
            const AZ::CommandLine* commandLine = application.GetAzCommandLine();
            if (!commandLine)
            {
                AZ_Error("SerializeContextTools", false, "Command line not available.");
                return false;
            }

            AZStd::string outputExtension;
            if (!commandLine->HasSwitch("ext"))
            {
                AZ_TracePrintf("Convert", "No extension provided through the 'ext' argument.\nThe extension of .setreg will be used instead\n");
                outputExtension = "setreg";
            }
            else
            {
                outputExtension = commandLine->GetSwitchValue("ext", 0);
            }

            const bool isDryRun = commandLine->HasSwitch("dryrun");

            // The AZ CommandLine internally splits switches on <comma> and semicolon
            AZStd::vector<AZStd::string_view> fileList;
            size_t filesToConvert = commandLine->GetNumSwitchValues("files");
            for (size_t fileIndex{}; fileIndex < filesToConvert; ++fileIndex)
            {
                fileList.emplace_back(commandLine->GetSwitchValue("files", fileIndex));
            }

            // Gather list of INI style files to convert using the SystemFile::FindFiles function
            PathDocumentContainer documents;
            for (AZStd::string_view configFileView : fileList)
            {
                // Convert the supplied file list to an absolute path
                AZStd::optional<AZ::IO::FixedMaxPathString> absFilePath = AZ::Utils::ConvertToAbsolutePath(configFileView);
                AZ::IO::FixedMaxPath configFilePath = absFilePath ? *absFilePath : configFileView;
                auto callback = [&documents, &configFilePath](AZ::IO::PathView configFileView, bool isFile) -> bool
                {
                    if (configFileView == "." || configFileView == "..")
                    {
                        return true;
                    }

                    if (isFile)
                    {
                        AZ::IO::FixedMaxPath foundFilePath{ configFilePath.ParentPath() };
                        foundFilePath /= configFileView;
                        // Initialize added documents with an empty JSON object(instead of a JSON null)
                        // This prevents a JSON document from being output with just null when there
                        // are no configuration entries
                        documents.emplace_back(foundFilePath.String(), rapidjson::Document{rapidjson::kObjectType});
                    }
                    return true;
                };
                AZ::IO::SystemFile::FindFiles(configFilePath.c_str(), callback);
            }

            // JSON pointer prefix to use as a temporary root for merging the config file to the settings registry
            // and dumping it to a rapidjson document. The prefix is used to make sure other settings outside
            // of the config settings are not output
            constexpr AZStd::string_view ConvertJsonPointer = "/Amazon/Config/Root";
            for (auto&& [iniFilename, iniJsonDocument] : documents)
            {
                // Local Settings Registry is used to contain only the converted INI-style file settings
                AZ::SettingsRegistryImpl settingsRegistry;
                AZ::SettingsRegistryMergeUtils::ConfigParserSettings configParserSettings;
                configParserSettings.m_commentPrefixFunc = [](AZStd::string_view line) -> AZStd::string_view
                {
                    constexpr AZStd::string_view commentPrefixes[]{ "--", ";","#" };
                    for (AZStd::string_view commentPrefix : commentPrefixes)
                    {
                        if (size_t commentOffset = line.find(commentPrefix); commentOffset != AZStd::string_view::npos)
                        {
                            return line.substr(0, commentOffset);
                        }
                    }
                    return line;
                };
                configParserSettings.m_registryRootPointerPath = ConvertJsonPointer;
                if (!AZ::SettingsRegistryMergeUtils::MergeSettingsToRegistry_ConfigFile(settingsRegistry, iniFilename, configParserSettings))
                {
                    AZ_TracePrintf("Convert", "Merging of config file %s has failed. It will be skipped", iniFilename.c_str());
                    result = false;
                    continue;
                }

                // If the Config file contained no settings, then Settings Registry contains no settings to dump at the JSON Pointer
                // In this scenario there are no settings to dump so continue to the next iteration
                if (settingsRegistry.GetType(ConvertJsonPointer) == AZ::SettingsRegistryInterface::Type::Object)
                {
                    // Dump the Settings Registry to a string that can be stored in a rapidjson::Document
                    AZ::SettingsRegistryMergeUtils::DumperSettings dumperSettings;
                    dumperSettings.m_prettifyOutput = true;
                    AZStd::string configJson;
                    AZ::IO::ByteContainerStream configJsonStream(&configJson);
                    if (!AZ::SettingsRegistryMergeUtils::DumpSettingsRegistryToStream(settingsRegistry, ConvertJsonPointer, configJsonStream, dumperSettings))
                    {
                        AZ_TracePrintf("Convert", "Config Settings for file %s cannot be queried from the Setting Registry", iniFilename.c_str());
                        result = false;
                        continue;
                    }
                    iniJsonDocument.Parse(configJson.c_str());
                }
                else
                {
                    AZ_TracePrintf("Convert", "Config file %s contained no convertible settings, an empty JSON object anchored"
                        " at the -json-prefix will be output", iniFilename.c_str());
                }
            }

            if (!isDryRun)
            {
                AZStd::string jsonDocumentRootPrefix;
                if (commandLine->GetNumSwitchValues("json-prefix") > 0)
                {
                    jsonDocumentRootPrefix = commandLine->GetSwitchValue("json-prefix", 0);
                }

                rapidjson::StringBuffer scratchBuffer;
                for (auto&& [iniFilename, iniJsonDocument] : documents)
                {
                    // Update the extension on the the input filename at this point
                    AZ::IO::Path outputFilename{ AZStd::move(iniFilename) };
                    outputFilename.ReplaceExtension(AZ::IO::PathView(outputExtension));
                    result = WriteDocumentToDisk(outputFilename.Native(), iniJsonDocument, jsonDocumentRootPrefix, scratchBuffer) && result;
                    scratchBuffer.Clear();
                }
            }

            return result;
        }

        bool Converter::ConvertSystemSettings(PathDocumentContainer& documents, const ComponentApplication::Descriptor& descriptor, 
            const AZStd::string& configurationName, const AZ::IO::PathView& projectFolder, [[maybe_unused]] const AZStd::string& applicationRoot)
        {
            AZ::IO::FixedMaxPath memoryFilePath{ projectFolder };
            memoryFilePath /= "Registry";
            
            AZ::IO::FixedMaxPath modulesFilePath = memoryFilePath;
            AZStd::string configurationNameLower = configurationName;
            AZStd::to_lower(configurationNameLower.begin(), configurationNameLower.end());
            modulesFilePath /= AZ::IO::FixedMaxPathString::format("module.%s.setreg", configurationNameLower.c_str());
            memoryFilePath /= AZ::IO::FixedMaxPathString::format("memory.%s.setreg", configurationNameLower.c_str());

            AZ_Printf("Convert", "  Exporting application descriptor to '%s' and '%s'.\n", memoryFilePath.c_str(), modulesFilePath.c_str());
            
            rapidjson::Document modulesDoc;
            modulesDoc.SetObject();
            rapidjson::Value moduleList(rapidjson::kArrayType);
            for (auto& module : descriptor.m_modules)
            {
                moduleList.PushBack(rapidjson::StringRef(module.m_dynamicLibraryPath.c_str()), modulesDoc.GetAllocator());
            }
            modulesDoc.AddMember(rapidjson::StringRef("Modules"), AZStd::move(moduleList), modulesDoc.GetAllocator());

            struct GemVisitor
                : public AZ::SettingsRegistryInterface::Visitor
            {
                GemVisitor(rapidjson::Value& gemSourcePaths, rapidjson::Document& modulesDoc)
                    : m_gemSourcePaths{ gemSourcePaths }
                    , m_modulesDoc{ modulesDoc }
                {}

                AZ::SettingsRegistryInterface::VisitResponse Traverse([[maybe_unused]] AZStd::string_view path, [[maybe_unused]] AZStd::string_view valueName,
                    AZ::SettingsRegistryInterface::VisitAction action, [[maybe_unused]] AZ::SettingsRegistryInterface::Type type) override
                {
                    if (valueName == "SourcePaths")
                    {
                        if (action == AZ::SettingsRegistryInterface::VisitAction::Begin)
                        {
                            // Allows merging of the registry folders within the gem source path array
                            // via the Visit function
                            m_processingSourcePathKey = true;
                        }
                        else if (action == AZ::SettingsRegistryInterface::VisitAction::End)
                        {
                            // The end of the gem source path array has been reached
                            m_processingSourcePathKey = false;
                        }
                    }

                    return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                }

                using AZ::SettingsRegistryInterface::Visitor::Visit;
                void Visit(AZStd::string_view, [[maybe_unused]] AZStd::string_view valueName, AZ::SettingsRegistryInterface::Type, AZStd::string_view value) override
                {
                    if (m_processingSourcePathKey)
                    {
                        m_gemSourcePaths.PushBack(rapidjson::StringRef(value.data(), value.size()), m_modulesDoc.GetAllocator());
                    }
                }

                rapidjson::Value& m_gemSourcePaths;
                rapidjson::Document& m_modulesDoc;
                bool m_processingSourcePathKey{};
            };

            // Visit each gem target "SourcePaths" entry within the settings registry
            rapidjson::Value gemPathList(rapidjson::kArrayType);
            GemVisitor visitor{ gemPathList, modulesDoc };
            const auto gemListKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems",
                AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
            AZ::SettingsRegistryInterface& registry = *AZ::SettingsRegistry::Get();
            registry.Visit(visitor, gemListKey);

            modulesDoc.AddMember(rapidjson::StringRef("GemFolders"), AZStd::move(gemPathList), modulesDoc.GetAllocator());
            documents.emplace_back(AZStd::move(modulesFilePath.Native()), AZStd::move(modulesDoc));

            rapidjson::Document memoryDoc;
            memoryDoc.SetObject();
            memoryDoc.AddMember(rapidjson::StringRef("useExistingAllocator"),
                rapidjson::Value(descriptor.m_useExistingAllocator), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("grabAllMemory"),
                rapidjson::Value(descriptor.m_grabAllMemory), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("allocationRecords"),
                rapidjson::Value(descriptor.m_allocationRecords), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("allocationRecordsSaveNames"),
                rapidjson::Value(descriptor.m_allocationRecordsSaveNames), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("allocationRecordsAttemptDecodeImmediately"),
                rapidjson::Value(descriptor.m_allocationRecordsAttemptDecodeImmediately), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("recordingMode"),
                rapidjson::Value(descriptor.m_recordingMode), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("stackRecordLevels"),
                rapidjson::Value(descriptor.m_stackRecordLevels), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("autoIntegrityCheck"),
                rapidjson::Value(descriptor.m_autoIntegrityCheck), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("markUnallocatedMemory"),
                rapidjson::Value(descriptor.m_markUnallocatedMemory), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("doNotUsePools"),
                rapidjson::Value(descriptor.m_doNotUsePools), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("enableScriptReflection"),
                rapidjson::Value(descriptor.m_enableScriptReflection), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("pageSize"),
                rapidjson::Value(descriptor.m_pageSize), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("poolPageSize"),
                rapidjson::Value(descriptor.m_poolPageSize), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("blockAlignment"),
                rapidjson::Value(descriptor.m_memoryBlockAlignment), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("blockSize"),
                rapidjson::Value(descriptor.m_memoryBlocksByteSize), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("reservedOS"),
                rapidjson::Value(descriptor.m_reservedOS), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("reservedDebug"),
                rapidjson::Value(descriptor.m_reservedDebug), memoryDoc.GetAllocator());
            memoryDoc.AddMember(rapidjson::StringRef("enableDrilling"),
                rapidjson::Value(descriptor.m_enableDrilling), memoryDoc.GetAllocator());
            documents.emplace_back(AZStd::move(memoryFilePath.Native()), AZStd::move(memoryDoc));
            
            return true;
        }

        bool Converter::ConvertSystemComponents(PathDocumentContainer& documents, const Entity& entity, const AZStd::string& configurationName, 
            const AZ::IO::PathView& projectFolder, const JsonSerializerSettings& convertSettings, const JsonDeserializerSettings& verifySettings)
        {
            using namespace AZ::JsonSerializationResult;

            AZ::IO::FixedMaxPath systemFilePath{ projectFolder };
            systemFilePath /= "Registry";
            AZStd::string configurationNameLower = configurationName;
            AZStd::to_lower(configurationNameLower.begin(), configurationNameLower.end());
            systemFilePath /= AZ::IO::FixedMaxPathString::format("system.%s.setreg", configurationNameLower.c_str());
            AZ_Printf("Convert", "  Exporting Entity to '%s'\n", systemFilePath.c_str());

            rapidjson::Document systemSettings;
            ResultCode result = JsonSerialization::Store(systemSettings.SetObject(), systemSettings.GetAllocator(), entity, convertSettings);
            if (result.GetProcessing() != Processing::Halted)
            {
                if (!VerifyConvertedData(systemSettings, &entity, azrtti_typeid(entity), verifySettings))
                {
                    // Errors will already be reported by VerifyConvertedData.
                    return false;
                }

                if (result.GetProcessing() != Processing::Halted)
                {
                    if (result.GetOutcome() == Outcomes::DefaultsUsed)
                    {
                        AZ_Printf("Convert", "  System settings not exported as only default values were found.\n");
                    }
                    else
                    {
                        documents.emplace_back(AZStd::move(systemFilePath.Native()), AZStd::move(systemSettings));
                    }
                }
                else
                {
                    AZ_Printf("Convert", "  System settings not exported.\n");
                }
                return true;
            }
            else
            {
                // Other errors will already have been reported by the JsonSerialierManager.
                return false;
            }
        }

        bool Converter::ConvertModuleComponents(PathDocumentContainer& documents, const ModuleEntity& entity, 
            const AZStd::string& configurationName, const JsonSerializerSettings& convertSettings, const JsonDeserializerSettings& verifySettings)
        {
            using namespace AZ::JsonSerializationResult;

            AZStd::fixed_string<128> gemName;
            AZStd::vector<AZ::IO::FixedMaxPath> gemModuleSourcePaths;
            AZ::ModuleManagerRequestBus::Broadcast([&gemModuleSourcePaths, &gemName, gemModuleClassId = entity.m_moduleClassId](AZ::ModuleManagerRequests* request)
            {
                request->EnumerateModules([&gemModuleSourcePaths, &gemName, &gemModuleClassId](const AZ::ModuleData& moduleData) -> bool
                {
                    AZ::Module* moduleInst = moduleData.GetModule();
                    if (moduleInst && AZ::RttiTypeId(*moduleInst) == gemModuleClassId)
                    {
                        struct GemBuildSystemVisitor
                            : AZ::SettingsRegistryInterface::Visitor
                        {
                            GemBuildSystemVisitor(AZStd::string_view moduleFilename, AZStd::vector<AZ::IO::FixedMaxPath>& gemSourcePaths)
                                : m_gemModuleFilename(moduleFilename)
                                , m_gemSourcePaths(gemSourcePaths)
                            {}

                            AZ::SettingsRegistryInterface::VisitResponse Traverse([[maybe_unused]] AZStd::string_view path, AZStd::string_view valueName,
                                AZ::SettingsRegistryInterface::VisitAction action, AZ::SettingsRegistryInterface::Type) override
                            {
                                if (m_gemSourcePathStored)
                                {
                                    return AZ::SettingsRegistryInterface::VisitResponse::Done;
                                }

                                // Store off the name of the Gem target when it is parsed underneath the /Amazon/Gems JSON pointer path
                                // The names of gems are keys on the /Amazon/Gems JSON object which is at a depth of 1
                                if (m_keyDepthIndex == 1)
                                {
                                    m_gemName = valueName;
                                }

                                if (action == SettingsRegistryInterface::VisitAction::Begin)
                                {
                                    ++m_keyDepthIndex;
                                }
                                else if (action == SettingsRegistryInterface::VisitAction::End)
                                {
                                    --m_keyDepthIndex;
                                }

                                return AZ::SettingsRegistryInterface::VisitResponse::Continue;
                            }

                            using AZ::SettingsRegistryInterface::Visitor::Visit;
                            void Visit(AZStd::string_view path, AZStd::string_view valueName, [[maybe_unused]] AZ::SettingsRegistryInterface::Type type,
                                AZStd::string_view value) override
                            {
                                if (valueName == "Module" && m_gemModuleFilename.find(value) != AZStd::string_view::npos)
                                {
                                    m_moduleFilenameMatches = true;
                                }
                                else if (m_moduleFilenameMatches && path.find("SourcePaths") != AZStd::string_view::npos)
                                {
                                    m_gemSourcePaths.emplace_back(value);
                                    m_gemSourcePathStored = true;
                                    m_moduleFilenameMatches = false;
                                }
                            }
                            AZStd::string_view m_gemModuleFilename;
                            AZStd::vector<AZ::IO::FixedMaxPath>& m_gemSourcePaths;
                            AZStd::fixed_string<128> m_gemName;
                            bool m_moduleFilenameMatches{};
                            bool m_gemSourcePathStored{};
                            int32_t m_keyDepthIndex{};
                        };

                        GemBuildSystemVisitor visitor{ AZStd::string_view{moduleData.GetDynamicModuleHandle()->GetFilename()}, gemModuleSourcePaths };
                        const auto gemListKey = AZ::SettingsRegistryInterface::FixedValueString::format("%s/Gems",
                            AZ::SettingsRegistryMergeUtils::OrganizationRootKey);
                        AZ::SettingsRegistry::Get()->Visit(visitor, gemListKey);
                        gemName = visitor.m_gemName;
                    }
                    return true;
                });
            });

            if (gemModuleSourcePaths.empty())
            {
                AZ_Warning("Convert", false, "Unable to find a gem folder to write output registry for module entity '%s'.", entity.GetName().c_str());
                return false;
            }

            AZ::SettingsRegistryInterface* settingsRegistry = AZ::SettingsRegistry::Get();

            AZ::IO::FixedMaxPath registryPath;
            if (!settingsRegistry->Get(registryPath.Native(), AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder))
            {
                AZ_Warning("Convert", false, "Unable To find Engine Root Path at key '%s' in the Settings Registry",
                    AZ::SettingsRegistryMergeUtils::FilePathKey_EngineRootFolder);
            }
            registryPath /= gemModuleSourcePaths.front();
            registryPath /= "Registry";
            AZStd::string configurationNameLower = configurationName;
            AZStd::to_lower(configurationNameLower.begin(), configurationNameLower.end());
            registryPath /= AZ::IO::FixedMaxPathString::format("gem.%s.setreg", configurationNameLower.c_str());
            AZ_Printf("Convert", "  Exporting ModuleEntity to '%s'\n", registryPath.c_str());

            rapidjson::Document moduleSettings;
            moduleSettings.SetObject().AddMember(rapidjson::Value(gemName.c_str(),
                aznumeric_cast<rapidjson::SizeType>(gemName.size()), moduleSettings.GetAllocator()),
                rapidjson::Value(rapidjson::kObjectType).Move(), moduleSettings.GetAllocator());
            rapidjson::Value& moduleSettingsValue = moduleSettings[gemName.c_str()];
            ResultCode result = JsonSerialization::Store(moduleSettingsValue, moduleSettings.GetAllocator(), entity, convertSettings);
            if (result.GetProcessing() != Processing::Halted)
            {
                if (!VerifyConvertedData(moduleSettingsValue, &entity, azrtti_typeid(entity), verifySettings))
                {
                    // Errors will already be reported by VerifyConvertedData.
                    return false;
                }

                if (result.GetProcessing() != Processing::Halted)
                {
                    if (result.GetOutcome() == Outcomes::DefaultsUsed)
                    {
                        AZ_Printf("Convert", "  Gem settings not exported as only default values were found.\n");
                    }
                    else
                    {
                        // Add Converted module settings in a JSON pointer path underneath the Gem Name
                        documents.emplace_back(AZStd::move(registryPath.Native()), AZStd::move(moduleSettings));
                    }
                }
                else
                {
                    AZ_Printf("Convert", "  Gem settings not exported.\n");
                }
                return true;
            }
            else
            {
                // Other errors will already have been reported by the JsonSerialization.
                return false;
            }
        }

        bool Converter::VerifyConvertedData(rapidjson::Value& convertedData, const void* original, const Uuid& originalType, 
            const JsonDeserializerSettings& settings)
        {
            using namespace AZ::JsonSerializationResult;

            // Need special handling if the original type is `any', because `CreateAny' creates an empty `any' in that case,
            // because it's not possible to store an any inside an any
            const bool originalTypeIsAny = originalType == azrtti_typeid<AZStd::any>();

            AZStd::any convertedDeserialized = settings.m_serializeContext->CreateAny(originalType);
            if (!originalTypeIsAny && convertedDeserialized.empty())
            {
                AZ_Printf("Convert", "  Failed to deserialized from converted document.\n");
                return false;
            }

            // Get a storage suitable to hold this data.
            void* objectPtr = originalTypeIsAny ? &convertedDeserialized : AZStd::any_cast<void>(&convertedDeserialized);

            ResultCode loadResult = JsonSerialization::Load(objectPtr, originalType, convertedData, settings);
            if (loadResult.GetProcessing() == Processing::Halted)
            {
                AZ_Printf("Convert", "  Failed to verify converted document because it couldn't be loaded.\n");
                return false;
            }

            const SerializeContext::ClassData* data = settings.m_serializeContext->FindClassData(originalType);
            if (!data)
            {
                AZ_Printf("Convert", "  Failed to find serialization information for type '%s'.\n",
                    originalType.ToString<AZStd::string>().c_str());
                return false;
            }
            
            bool result = false;
            if (data->m_serializer)
            {
                result = data->m_serializer->CompareValueData(original, objectPtr);
            }
            else
            {
                AZStd::vector<AZ::u8> originalData;
                AZ::IO::ByteContainerStream<decltype(originalData)> orignalStream(&originalData);
                AZ::Utils::SaveObjectToStream(orignalStream, AZ::ObjectStream::ST_BINARY, original, originalType);

                AZStd::vector<AZ::u8> loadedData;
                AZ::IO::ByteContainerStream<decltype(loadedData)> loadedStream(&loadedData);
                AZ::Utils::SaveObjectToStream(loadedStream, AZ::ObjectStream::ST_BINARY,
                    objectPtr, originalType);
                
                result = 
                    (originalData.size() == loadedData.size()) &&
                    (memcmp(originalData.data(), loadedData.data(), originalData.size()) == 0);
            }

            if (!result)
            {
                AZ_Printf("Convert", "  Differences found between the original and converted data.\n");
            }
            return result;
        }

        AZStd::string Converter::GetClassName(const Uuid& classId, SerializeContext* context)
        {
            const SerializeContext::ClassData* data = context->FindClassData(classId);
            if (data)
            {
                if (data->m_editData)
                {
                    return data->m_editData->m_name;
                }
                else
                {
                    return data->m_name;
                }
            }
            else
            {
                return classId.ToString<AZStd::string>();
            }
        }

        bool Converter::WriteDocumentToDisk(const AZStd::string& filename, const rapidjson::Document& document,
            AZStd::string_view pointerRoot, rapidjson::StringBuffer& scratchBuffer)
        {
            IO::SystemFile outputFile;
            if (!outputFile.Open(filename.c_str(),
                IO::SystemFile::OpenMode::SF_OPEN_CREATE |
                IO::SystemFile::OpenMode::SF_OPEN_CREATE_PATH |
                IO::SystemFile::OpenMode::SF_OPEN_WRITE_ONLY))
            {
                AZ_Error("SerializeContextTools", false, "Unable to open output file '%s'.", filename.c_str());
                return false;
            }

            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(scratchBuffer);

            // rapidjson::Pointer constructor attempts to dereference the const char* index 0 even if the size is 0
            // so make sure an string_view isn't referencing a nullptr
            rapidjson::Pointer jsonPointerAnchor(pointerRoot.data() ? pointerRoot.data() : "", pointerRoot.size());

            // Anchor the content in the Json Document under the Json Pointer root path
            rapidjson::Document rootDocument;
            rapidjson::SetValueByPointer(rootDocument, jsonPointerAnchor, document);
            rootDocument.Accept(writer);

            outputFile.Write(scratchBuffer.GetString(), scratchBuffer.GetSize());
            outputFile.Close();

            scratchBuffer.Clear();
            return true;
        }

        void Converter::SetupLogging(AZStd::string& scratchBuffer, JsonSerializationResult::JsonIssueCallback& callback,
            const AZ::CommandLine& commandLine)
        {
            if (commandLine.HasSwitch("verbose"))
            {
                callback = [&scratchBuffer](
                    AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view path)
                    ->JsonSerializationResult::ResultCode
                {
                    return VerboseLogging(scratchBuffer, message, result, path);
                };
            }
            else
            {
                callback = [&scratchBuffer](
                    AZStd::string_view message, JsonSerializationResult::ResultCode result, AZStd::string_view path)
                    ->JsonSerializationResult::ResultCode
                {
                    return SimpleLogging(scratchBuffer, message, result, path);
                };
            }
        }

        AZ::JsonSerializationResult::ResultCode Converter::VerboseLogging(AZStd::string& scratchBuffer, AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            scratchBuffer.append(message.begin(), message.end());
            scratchBuffer.append("\n    Reason: ");
            result.AppendToString(scratchBuffer, path);
            scratchBuffer.append(".\n");
            AZ_Printf("SerializeContextTools", "%s", scratchBuffer.c_str());
            scratchBuffer.clear();
            
            return result;
        }

        AZ::JsonSerializationResult::ResultCode Converter::SimpleLogging(AZStd::string& scratchBuffer, AZStd::string_view message,
            AZ::JsonSerializationResult::ResultCode result, AZStd::string_view path)
        {
            using namespace JsonSerializationResult;

            if (result.GetProcessing() != Processing::Completed)
            {
                scratchBuffer.append(message.begin(), message.end());
                scratchBuffer.append(" @ ");
                scratchBuffer.append(path.begin(), path.end());
                scratchBuffer.append(".\n");
                AZ_Printf("SerializeContextTools", "%s", scratchBuffer.c_str());

                scratchBuffer.clear();
            }
            return result;
        }
    } // namespace SerializeContextTools
} // namespace AZ
