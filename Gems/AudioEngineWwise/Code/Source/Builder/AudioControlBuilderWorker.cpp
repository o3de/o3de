/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Builder/AudioControlBuilderWorker.h>

#include <AzCore/AzCore_Traits_Platform.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/JSON/rapidjson.h>
#include <AzCore/JSON/document.h>
#include <AzCore/PlatformId/PlatformId.h>
#include <AzCore/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>

#include <ATLCommon.h>
#include <Common_wwise.h>
#include <Config_wwise.h>

namespace AudioControlBuilder
{
    namespace Internal
    {
        const char JsonEventsKey[] = "includedEvents";
        const char SoundbankDependencyFileExtension[] = ".bankdeps";

        const char NodeDoesNotExistMessage[] = "%s node does not exist. Please be sure that you have defined at least one %s for this Audio Control file.\n";
        const char MalformedNodeMissingAttributeMessage[] = "%s node is malformed: does not have an attribute %s defined. This is likely the result of manual editing. Please resave the Audio Control file.\n";
        const char MalformedNodeMissingChildNodeMessage[] = "%s node does not contain a child %s node. This is likely the result of manual editing. Please resave the Audio Control file.\n";

        namespace Legacy
        {
            using AtlConfigGroupMap = AZStd::unordered_map<AZStd::string, const AZ::rapidxml::xml_node<char>*>;

            AZStd::string GetAtlPlatformName(const AZStd::string& requestPlatform)
            {
                AZStd::string atlPlatform;
                AZStd::string platform = requestPlatform;

                // When debugging a builder using a debug task, it replaces platform tags with "debug platform". Make sure the builder
                //  actually uses the host platform identifier when going through this function in this case.
                if (platform == "debug platform")
                {
                    atlPlatform = AZ_TRAIT_OS_PLATFORM_NAME;
                    AZStd::to_lower(atlPlatform.begin(), atlPlatform.end());
                }
                else
                {
                    if (platform == "pc")
                    {
                        atlPlatform = "windows";
                    }
                    else if (platform == "android")
                    {
                        atlPlatform = "android";
                    }
                    else if (platform == "mac")
                    {
                        atlPlatform = "mac";
                    }
                    else
                    {
                        atlPlatform = AZStd::move(platform);
                    }
                }
                return AZStd::move(atlPlatform);
            }

            AZ::Outcome<void, AZStd::string> BuildConfigGroupMap(const AZ::rapidxml::xml_node<char>* preloadRequestNode, AtlConfigGroupMap& configGroupMap)
            {
                AZ_Assert(preloadRequestNode != nullptr, NodeDoesNotExistMessage, Audio::ATLXmlTags::ATLPreloadRequestTag, "preload request");

                auto configGroupNode = preloadRequestNode->first_node(Audio::ATLXmlTags::ATLConfigGroupTag);
                while (configGroupNode)
                {
                    // Populate the config group map by iterating over all ATLConfigGroup nodes, and place each one in the map keyed by the group's atl_name attribute...
                    if (const auto configGroupNameAttr = configGroupNode->first_attribute(Audio::ATLXmlTags::ATLNameAttribute))
                    {
                        configGroupMap.emplace(configGroupNameAttr->value(), configGroupNode);
                    }
                    else
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage,
                            Audio::ATLXmlTags::ATLConfigGroupTag, Audio::ATLXmlTags::ATLNameAttribute));
                    }

                    configGroupNode = configGroupNode->next_sibling(Audio::ATLXmlTags::ATLConfigGroupTag);
                }

                // If no config groups are defined, this is an empty preload request with no banks referenced, which is valid.
                return AZ::Success();
            }

            AZ::Outcome<void, AZStd::string> GetBanksFromAtlPreloads(const AZ::rapidxml::xml_node<char>* preloadsNode, const AZStd::string& atlPlatformIdentifier, AZStd::vector<AZStd::string>& banksReferenced)
            {
                AZ_Assert(preloadsNode != nullptr, NodeDoesNotExistMessage, Audio::ATLXmlTags::PreloadsNodeTag, "preload request");

                auto preloadRequestNode = preloadsNode->first_node(Audio::ATLXmlTags::ATLPreloadRequestTag);
                if (!preloadRequestNode)
                {
                    return AZ::Failure(AZStd::string::format(NodeDoesNotExistMessage,
                        Audio::ATLXmlTags::ATLPreloadRequestTag, "preload request"));
                }

                // For each preload request in the control file, determine which config group is used for this platform and register each
                // bank listed in that preload request as a dependency.
                while (preloadRequestNode)
                {
                    AtlConfigGroupMap configGroupMap;
                    auto configGroupMapResult = BuildConfigGroupMap(preloadRequestNode, configGroupMap);

                    // If the returned map is empty, there are not banks referenced in the preload request, return the result here.
                    if (!configGroupMapResult.IsSuccess() || configGroupMap.size() == 0)
                    {
                        return configGroupMapResult;
                    }

                    const auto platformsNode = preloadRequestNode->first_node(Audio::ATLXmlTags::ATLPlatformsTag);
                    if (!platformsNode)
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage,
                            Audio::ATLXmlTags::ATLPreloadRequestTag, Audio::ATLXmlTags::ATLPlatformsTag));
                    }

                    auto platformNode = platformsNode->first_node(Audio::ATLXmlTags::PlatformNodeTag);
                    if (!platformNode)
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage,
                            Audio::ATLXmlTags::ATLPlatformsTag, Audio::ATLXmlTags::PlatformNodeTag));
                    }

                    AZStd::string configGroupName;
                    // For each platform node in the platform list, check the atl_name to see if it matches the platform the request is
                    //  intended for. If it is, grab the name of the config group that is used for that platform to load it.
                    while (platformNode)
                    {
                        const auto atlNameAttr = platformNode->first_attribute(Audio::ATLXmlTags::ATLNameAttribute);
                        if (!atlNameAttr)
                        {
                            return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage,
                                Audio::ATLXmlTags::PlatformNodeTag, Audio::ATLXmlTags::ATLNameAttribute));
                        }
                        else if (atlPlatformIdentifier == atlNameAttr->value())
                        {
                            // We've found the right platform that matches the request, so grab the group name and stop looking through
                            //  the list
                            const auto configGroupNameAttr = platformNode->first_attribute(Audio::ATLXmlTags::ATLConfigGroupAttribute);
                            if (!configGroupNameAttr)
                            {
                                return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage,
                                    Audio::ATLXmlTags::PlatformNodeTag, Audio::ATLXmlTags::ATLConfigGroupAttribute));
                            }
                            configGroupName = configGroupNameAttr->value();
                            break;
                        }

                        platformNode = platformNode->next_sibling(Audio::ATLXmlTags::PlatformNodeTag);
                    }

                    const AZ::rapidxml::xml_node<char>* configGroupNode = configGroupMap[configGroupName];
                    if (!configGroupNode)
                    {
                        // The config group this platform uses isn't defined in the control file. This might be intentional, so just
                        //  generate a warning and keep going to the next preload node.
                        AZ_TracePrintf("Audio Control Builder", "%s node for config group %s is not defined, so no banks are referenced.",
                            Audio::ATLXmlTags::ATLConfigGroupTag, configGroupName.c_str());
                    }
                    else
                    {
                        auto wwiseFileNode = configGroupNode->first_node(Audio::WwiseXmlTags::WwiseFileTag);
                        if (!wwiseFileNode)
                        {
                            return AZ::Failure(AZStd::string::format(MalformedNodeMissingChildNodeMessage,
                                Audio::ATLXmlTags::ATLConfigGroupTag, Audio::WwiseXmlTags::WwiseFileTag));
                        }

                        // For each WwiseFile (soundbank) referenced in the config group, grab the file name and add it to the reference list
                        while (wwiseFileNode)
                        {
                            const auto bankNameAttribute = wwiseFileNode->first_attribute(Audio::WwiseXmlTags::WwiseNameAttribute);
                            if (!bankNameAttribute)
                            {
                                return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage,
                                    Audio::WwiseXmlTags::WwiseFileTag, Audio::WwiseXmlTags::WwiseNameAttribute));
                            }

                            // Prepend the bank name with the relative path to the wwise sounds folder to get relative path to the bank from
                            //  the @products@ alias and push that into the list of banks referenced.
                            AZStd::string soundsPrefix = Audio::Wwise::DefaultBanksPath;
                            banksReferenced.emplace_back(soundsPrefix + bankNameAttribute->value());

                            wwiseFileNode = wwiseFileNode->next_sibling(Audio::WwiseXmlTags::WwiseFileTag);
                        }
                    }

                    preloadRequestNode = preloadRequestNode->next_sibling(Audio::ATLXmlTags::ATLPreloadRequestTag);
                }

                return AZ::Success();
            }

        } // namespace Legacy


        AZ::Outcome<void, AZStd::string> BuildAtlEventList(const AZ::rapidxml::xml_node<char>* triggersNode, AZStd::vector<AZStd::string>& eventNames)
        {
            AZ_Assert(triggersNode != nullptr, NodeDoesNotExistMessage, Audio::ATLXmlTags::TriggersNodeTag, "trigger");

            auto triggerNode = triggersNode->first_node(Audio::ATLXmlTags::ATLTriggerTag);
            while (triggerNode)
            {
                // For each audio trigger, push the name of the Wwise event (if assigned) into the list.
                // It's okay for an ATLTrigger node to not have a Wwise event associated with it.
                if (const auto eventNode = triggerNode->first_node(Audio::WwiseXmlTags::WwiseEventTag))
                {
                    if (const auto eventNameAttr = eventNode->first_attribute(Audio::WwiseXmlTags::WwiseNameAttribute))
                    {
                        eventNames.push_back(eventNameAttr->value());
                    }
                    else
                    {
                        return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage,
                            Audio::WwiseXmlTags::WwiseEventTag, Audio::WwiseXmlTags::WwiseNameAttribute));
                    }
                }

                triggerNode = triggerNode->next_sibling(Audio::ATLXmlTags::ATLTriggerTag);
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetBanksFromAtlPreloads(const AZ::rapidxml::xml_node<char>* preloadsNode, AZStd::vector<AZStd::string>& banksReferenced)
        {
            AZ_Assert(preloadsNode != nullptr, NodeDoesNotExistMessage, Audio::ATLXmlTags::PreloadsNodeTag, "preload request");

            auto preloadRequestNode = preloadsNode->first_node(Audio::ATLXmlTags::ATLPreloadRequestTag);
            if (!preloadRequestNode)
            {
                return AZ::Failure(AZStd::string::format(NodeDoesNotExistMessage, Audio::ATLXmlTags::ATLPreloadRequestTag, "preload request"));
            }

            // Loop through the ATLPreloadRequest nodes...
            // Find any Wwise banks listed and add them to the banksReferenced vector.
            while (preloadRequestNode)
            {
                // Attempt to find the child node in the New Xml format...
                if (auto wwiseFileNode = preloadRequestNode->first_node(Audio::WwiseXmlTags::WwiseFileTag))
                {
                    while (wwiseFileNode)
                    {
                        const auto bankNameAttr = wwiseFileNode->first_attribute(Audio::WwiseXmlTags::WwiseNameAttribute);
                        if (bankNameAttr)
                        {
                            AZStd::string soundsPrefix = Audio::Wwise::DefaultBanksPath;
                            banksReferenced.emplace_back(soundsPrefix + bankNameAttr->value());
                        }
                        else
                        {
                            return AZ::Failure(AZStd::string::format(MalformedNodeMissingAttributeMessage,
                                Audio::WwiseXmlTags::WwiseFileTag, Audio::WwiseXmlTags::WwiseNameAttribute));
                        }

                        wwiseFileNode = wwiseFileNode->next_sibling(Audio::WwiseXmlTags::WwiseFileTag);
                    }
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Preloads Xml appears to be in an older format, trying Legacy parsing.\n"));
                }

                preloadRequestNode = preloadRequestNode->next_sibling(Audio::ATLXmlTags::ATLPreloadRequestTag);
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetEventsFromBankMetadata(const rapidjson::Value& rootObject, AZStd::set<AZStd::string>& eventNames)
        {
            if (!rootObject.IsObject())
            {
                return AZ::Failure(AZStd::string("The root of the metadata file is not an object. Please regenerate the metadata for this soundbank."));
            }

            // If the file doesn't define an events field, then there are no events in the bank
            if (!rootObject.HasMember(JsonEventsKey))
            {
                return AZ::Success();
            }

            const rapidjson::Value& eventsArray = rootObject[JsonEventsKey];
            if (!eventsArray.IsArray())
            {
                return AZ::Failure(AZStd::string("Events field is not an array. Please regenerate the metadata for this soundbank."));
            }

            for (rapidjson::SizeType eventIndex = 0; eventIndex < eventsArray.Size(); ++eventIndex)
            {
                eventNames.emplace(eventsArray[eventIndex].GetString());
            }

            return AZ::Success();
        }

        AZ::Outcome<void, AZStd::string> GetEventsFromBank(const AZStd::string& bankMetadataPath, AZStd::set<AZStd::string>& eventNames)
        {
            if (!AZ::IO::SystemFile::Exists(bankMetadataPath.c_str()))
            {
                return AZ::Failure(AZStd::string::format("Failed to find the soundbank metadata file %s. Full dependency information cannot be determined without the metadata file. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
            }

            AZ::u64 fileSize = AZ::IO::SystemFile::Length(bankMetadataPath.c_str());
            if (fileSize == 0)
            {
                return AZ::Failure(AZStd::string::format("Soundbank metadata file at path %s is an empty file. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
            }

            AZStd::vector<char> buffer(fileSize + 1);
            buffer[fileSize] = 0;
            if (!AZ::IO::SystemFile::Read(bankMetadataPath.c_str(), buffer.data()))
            {
                return AZ::Failure(AZStd::string::format("Failed to read the soundbank metadata file at path %s. Please make sure the file is not open or being edited by another program.", bankMetadataPath.c_str()));
            }

            rapidjson::Document bankMetadataDoc;
            bankMetadataDoc.Parse(buffer.data());
            if (bankMetadataDoc.GetParseError() != rapidjson::ParseErrorCode::kParseErrorNone)
            {
                return AZ::Failure(AZStd::string::format("Failed to parse soundbank metadata at path %s into JSON. Please regenerate the metadata for this soundbank.", bankMetadataPath.c_str()));
            }

            return GetEventsFromBankMetadata(bankMetadataDoc, eventNames);
        }

    } // namespace Internal



    AudioControlBuilderWorker::AudioControlBuilderWorker()
        : m_globalScopeControlsPath("libs/gameaudio/")
        , m_isShuttingDown(false)
    {
        AZ::StringFunc::Path::Normalize(m_globalScopeControlsPath);
    }

    void AudioControlBuilderWorker::ShutDown()
    {
        m_isShuttingDown = true;
    }

    void AudioControlBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            if (info.m_identifier == "server")
            {
                continue;
            }

            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Audio Control";
            descriptor.m_critical = true;
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            descriptor.m_priority = 0;
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    void AudioControlBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "AudioControlBuilderWorker Starting Job.\n");

        if (m_isShuttingDown)
        {
            AZ_TracePrintf(AssetBuilderSDK::WarningWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            return;
        }

        AZStd::string fileName;
        AZ::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);

        AssetBuilderSDK::JobProduct jobProduct(request.m_fullPath);

        if (!ParseProductDependencies(request, jobProduct.m_dependencies, jobProduct.m_pathDependencies))
        {
            AZ_Error(AssetBuilderSDK::ErrorWindow, false, "Error during parsing product dependencies for asset %s.\n", fileName.c_str());
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            return;
        }

        response.m_outputProducts.push_back(jobProduct);
        response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
    }

    bool AudioControlBuilderWorker::ParseProductDependencies(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        AZ::IO::FileIOStream fileStream;
        if (!fileStream.Open(request.m_fullPath.c_str(), AZ::IO::OpenMode::ModeRead))
        {
            return false;
        }

        AZ::IO::SizeType length = fileStream.GetLength();
        if (length == 0)
        {
            return false;
        }

        AZStd::vector<char> charBuffer;
        charBuffer.resize_no_construct(length + 1);
        fileStream.Read(length, charBuffer.data());
        charBuffer.back() = 0;

        // Get the XML root node
        AZ::rapidxml::xml_document<char> xmlDoc;
        if (!xmlDoc.parse<AZ::rapidxml::parse_no_data_nodes>(charBuffer.data()))
        {
            return false;
        }

        AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc.first_node();
        if (!xmlRootNode)
        {
            return false;
        }

        ParseProductDependenciesFromXmlFile(xmlRootNode,
            request.m_fullPath,
            request.m_sourceFile,
            request.m_platformInfo.m_identifier,
            productDependencies,
            pathDependencies);

        return true;
    }

    void AudioControlBuilderWorker::ParseProductDependenciesFromXmlFile(
        const AZ::rapidxml::xml_node<char>* node,
        const AZStd::string& fullPath,
        [[maybe_unused]] const AZStd::string& sourceFile,
        const AZStd::string& platformIdentifier,
        [[maybe_unused]] AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        AZ_Assert(node != nullptr, "AudioControlBuilderWorker::ParseProductDependenciesFromXmlFile - null xml root node!\n");

        const auto preloadsNode = node->first_node(Audio::ATLXmlTags::PreloadsNodeTag);
        if (!preloadsNode)
        {
            // No preloads were defined in this control file, so we can return. If triggers are defined in this preload file, we can't
            // validate that they'll be playable because we are unsure of what other control files for the given scope are defined.
            return;
        }

        // Collect any references to soundbanks, initially use the newer parsing format...
        AZStd::vector<AZStd::string> banksReferenced;
        AZ::Outcome<void, AZStd::string> gatherBankReferencesResult = Internal::GetBanksFromAtlPreloads(preloadsNode, banksReferenced);
        if (!gatherBankReferencesResult)
        {
            // Legacy...
            // Convert platform name to platform name that is used by wwise and ATL.
            AZStd::string atlPlatformName = AZStd::move(Internal::Legacy::GetAtlPlatformName(platformIdentifier));
            gatherBankReferencesResult = Internal::Legacy::GetBanksFromAtlPreloads(preloadsNode, atlPlatformName, banksReferenced);
        }

        if (!gatherBankReferencesResult)
        {
            AZ_Warning("Audio Control Builder", false, "Failed to gather product dependencies for Audio Control file %s.  %s\n",
                sourceFile.c_str(), gatherBankReferencesResult.GetError().c_str());
            return;
        }

        if (banksReferenced.size() == 0)
        {
            // If there are no banks referenced, then there are no dependencies to register, so return.
            return;
        }

        for (const AZStd::string& relativeBankPath : banksReferenced)
        {
            pathDependencies.emplace(relativeBankPath, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }

        // For each bank figure out what events are included in the bank, then run through every event referenced in the file and
        //  make sure it is in the list gathered from the banks.
        const auto triggersNode = node->first_node(Audio::ATLXmlTags::TriggersNodeTag);
        if (!triggersNode)
        {
            // No triggers were defined in this file, so we don't need to do any event validation.
            return;
        }

        AZStd::vector<AZStd::string> eventsReferenced;
        AZ::Outcome<void, AZStd::string> gatherEventReferencesResult = Internal::BuildAtlEventList(triggersNode, eventsReferenced);
        if (!gatherEventReferencesResult.IsSuccess())
        {
            AZ_Warning("Audio Control Builder", false, "Failed to gather list of events referenced by Audio Control file %s. %s",
                sourceFile.c_str(), gatherEventReferencesResult.GetError().c_str());
            return;
        }

        AZStd::string projectSourcePath = fullPath;
        AZ::u64 firstSubDirectoryIndex = AZ::StringFunc::Find(projectSourcePath, m_globalScopeControlsPath);
        AZ::StringFunc::LKeep(projectSourcePath, firstSubDirectoryIndex);

        AZStd::set<AZStd::string> wwiseEventsInReferencedBanks;

        // Load all bankdeps files for all banks referenced and aggregate the list of events in those files.
        for (const AZStd::string& relativeBankPath : banksReferenced)
        {
            // Create the full path to the bankdeps file from the bank file.
            AZStd::string bankMetadataPath;
            AZ::StringFunc::Path::Join(projectSourcePath.c_str(), relativeBankPath.c_str(), bankMetadataPath);
            AZ::StringFunc::Path::ReplaceExtension(bankMetadataPath, Internal::SoundbankDependencyFileExtension);

            AZ::Outcome<void, AZStd::string> getReferencedEventsResult = Internal::GetEventsFromBank(bankMetadataPath, wwiseEventsInReferencedBanks);
            if (!getReferencedEventsResult.IsSuccess())
            {
                // only warn if we couldn't get info from a bankdeps file. Won't impact registering dependencies, but used to help
                // customers potentially debug issues.
                AZ_Warning("Audio Control Builder", false, "Failed to gather list of events referenced by soundbank %s. %s", relativeBankPath.c_str(), getReferencedEventsResult.GetError().c_str());
            }
        }

        // Confirm that each event referenced by the file exists in the list of events available from the banks referenced.
        for (const AZStd::string& eventInControlFile : eventsReferenced)
        {
            if (wwiseEventsInReferencedBanks.find(eventInControlFile) == wwiseEventsInReferencedBanks.end())
            {
                AZ_Warning("Audio Control Builder", false, "Failed to find Wwise event %s in the list of events contained in banks referenced by %s. Event may fail to play properly.", eventInControlFile.c_str(), sourceFile.c_str());
            }
        }
    }

} // namespace AudioControlBuilder
