/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "XmlBuilderWorker.h"

#include <AssetBuilderSDK/SerializationDependencies.h>
#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/std/string/wildcard.h>
#include <AzCore/Dependency/Dependency.h>
#include <AzFramework/FileFunc/FileFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzFramework/StringFunc/StringFunc.h>

#include "Builders/CopyDependencyBuilder/SchemaBuilderWorker/SchemaUtils.h"

namespace CopyDependencyBuilder
{
    namespace Internal
    {
        bool AddFileExtention(const AZStd::string expectedExtension, AZStd::string& fileName, bool isOptional)
        {
            if (!AzFramework::StringFunc::Path::HasExtension(fileName.c_str()))
            {
                // Open 3D Engine makes use of some files without extensions, only replace the extension if there is an expected extension.
                if (!expectedExtension.empty())
                {
                    AzFramework::StringFunc::Path::ReplaceExtension(fileName, expectedExtension.c_str());
                }
            }
            else if (!expectedExtension.empty())
            {
                AZStd::string existingExtension;
                AzFramework::StringFunc::Path::GetExtension(fileName.c_str(), existingExtension, false);
                if (expectedExtension.front() == '.')
                {
                    existingExtension = '.' + existingExtension;
                }

                if (existingExtension != expectedExtension)
                {
                    if (!isOptional)
                    {
                        AZ_Warning("XmlBuilderWorker", false, "Dependency %s already has an extension %s and the expected extension %s is different."
                            "The original extension is not replaced.", fileName.c_str(), existingExtension.c_str(), expectedExtension.c_str());
                    }
                    return false;
                }
            }
            return true;
        }

        AZ::Data::AssetId TextToAssetId(const char* text)
        {
            // Parse the asset id
            // Asset data could look like "id={00000000-0000-0000-0000-000000000000}:0,type={00000000-0000-0000-0000-000000000000},hint={asset_path}"
            // SubId is 16 based according to AssetSerializer
            AZ::Data::AssetId assetId;
            if (!text)
            {
                AZ_Error("XmlBuilderWorker", false, "Null string given to TextToAssetId");
                return assetId;
            }
            const char* idGuidStart = strchr(text, '{');
            if (!idGuidStart)
            {
                AZ_Error("XmlBuilderWorker", false, "Invalid asset guid data! %s", text);
                return assetId;
            }
            const char* idGuidEnd = strchr(idGuidStart, ':');
            AZ_Error("XmlBuilderWorker", idGuidEnd, "Invalid asset guid data! %s", idGuidStart);
            const char* idSubIdStart = idGuidEnd + 1;

            assetId.m_guid = AZ::Uuid::CreateString(idGuidStart, idGuidEnd - idGuidStart);
            assetId.m_subId = static_cast<AZ::u32>(strtoul(idSubIdStart, nullptr, 16));

            return assetId;
        }

        bool ParseAttributeNode(
            const AzFramework::XmlSchemaAttribute& schemaAttribute,
            const AZ::rapidxml::xml_node<char>* xmlFileNode,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
            const AZStd::string& sourceAssetFolder,
            const AZStd::string& nodeContent = "",
            const AZStd::string& watchFolder = "")
        {
            // Attribute nodes of the XML schema specify the attributes which are used to store product dependency info in the actual XML nodes
            AZStd::string schemaAttributeName = schemaAttribute.GetName();
            // The attribute name could be empty only if the XML element content specifies the product dependency
            // e.g. <entry>example.dds</entry>
            if (schemaAttributeName.empty() && nodeContent.empty())
            {
                return schemaAttribute.IsOptional();
            }

            AZ::rapidxml::xml_attribute<char>* xmlNodeNameAttr = xmlFileNode->first_attribute(schemaAttributeName.c_str(), 0, false);
            // Return early if the attribute specified in the schema doesn't exist
            if (!schemaAttributeName.empty() && !xmlNodeNameAttr)
            {
                return schemaAttribute.IsOptional();
            }

            AZStd::string dependencyValue = !schemaAttributeName.empty() ? xmlNodeNameAttr->value() : nodeContent;

            switch (schemaAttribute.GetType())
            {
                case AzFramework::XmlSchemaAttribute::AttributeType::RelativePath:
                {
                    AssetBuilderSDK::ProductPathDependencyType productPathDependencyType =
                        static_cast<AZ::u32>(schemaAttribute.GetPathDependencyType()) == static_cast<AZ::u32>(AssetBuilderSDK::ProductPathDependencyType::SourceFile)
                        ? AssetBuilderSDK::ProductPathDependencyType::SourceFile
                        : AssetBuilderSDK::ProductPathDependencyType::ProductFile;

                    if (dependencyValue.empty())
                    {
                        return true;
                    }

                    // Reject values that don't pass the match pattern
                    if(!schemaAttribute.GetMatchPattern().empty())
                    {
                        AZStd::regex match(schemaAttribute.GetMatchPattern(), AZStd::regex_constants::ECMAScript | AZStd::regex_constants::icase);

                        if(!AZStd::regex_search(dependencyValue, match))
                        {
                            return true;
                        }
                    }

                    if(!schemaAttribute.GetFindPattern().empty())
                    {
                        AZStd::regex find(schemaAttribute.GetFindPattern(), AZStd::regex_constants::ECMAScript | AZStd::regex_constants::icase);
                        dependencyValue = AZStd::regex_replace(dependencyValue, find, schemaAttribute.GetReplacePattern());
                    }

                    if (AddFileExtention(schemaAttribute.GetExpectedExtension(), dependencyValue, schemaAttribute.IsOptional()))
                    {
                        if (schemaAttribute.IsRelativeToSourceAssetFolder())
                        {
                            AzFramework::StringFunc::AssetDatabasePath::Join(sourceAssetFolder.c_str(), dependencyValue.c_str(), dependencyValue);
                        }
                        else if (schemaAttribute.CacheRelativePath())
                        {
                            AZStd::string_view depFolder{ sourceAssetFolder.c_str() };
                            if (watchFolder.length() && sourceAssetFolder.starts_with(watchFolder))
                            {
                                depFolder = &sourceAssetFolder[watchFolder.length()];
                                if (depFolder.front() == '/')
                                {
                                    depFolder = &sourceAssetFolder[watchFolder.length() + 1];
                                }

                            }
                            AzFramework::StringFunc::AssetDatabasePath::Join(depFolder.data(), dependencyValue.c_str(), dependencyValue);
                        }

                        pathDependencies.emplace(dependencyValue, productPathDependencyType);
                    }
                    break;
                }
                case AzFramework::XmlSchemaAttribute::AttributeType::Asset:
                {
                    productDependencies.emplace_back(AssetBuilderSDK::ProductDependency(TextToAssetId(dependencyValue.c_str()), {}));
                    break;
                }
                default:
                    AZ_Error("XmlBuilderWorker", false, "Unsupported schema attribute type. Choose from RelativePath and Asset.");
                    break;
            }

            return true;
        }

        bool ParseElementNode(
            const AzFramework::XmlSchemaElement& xmlSchemaElement,
            const AZ::rapidxml::xml_node<char>* xmlFileNode,
            AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
            AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
            const AZStd::string& sourceAssetFolder,
            const AZStd::string& watchFolderPath)
        {
            // Check whether the XML node matches the schema
            AZStd::string schemaElementName = xmlSchemaElement.GetName();
            if (strcmp(schemaElementName.c_str(), xmlFileNode->name()) != 0 && schemaElementName != "*")
            {
                return false;
            }

            AZStd::vector<AssetBuilderSDK::ProductDependency> localProductDependencies;
            AssetBuilderSDK::ProductPathDependencySet localPathDependencies;
            // Continue parsing the source XML using the child element and attribute nodes 
            for (const AzFramework::XmlSchemaElement& childSchemaElement : xmlSchemaElement.GetChildElements())
            {
                bool findMatchedChildNode = false;
                for (AZ::rapidxml::xml_node<char>* xmlFileChildNode = xmlFileNode->first_node(); xmlFileChildNode; xmlFileChildNode = xmlFileChildNode->next_sibling())
                {
                    findMatchedChildNode = ParseElementNode(childSchemaElement, xmlFileChildNode, localProductDependencies, localPathDependencies, sourceAssetFolder, watchFolderPath) || findMatchedChildNode;
                }

                if (!findMatchedChildNode && !childSchemaElement.IsOptional())
                {
                     return false;
                }
            }

            for (const AzFramework::XmlSchemaAttribute& schemaAttribute : xmlSchemaElement.GetAttributes())
            {
                AZStd::string xmlNodeValue = schemaAttribute.GetName().empty() ? xmlFileNode->value() : "";
                if (!ParseAttributeNode(schemaAttribute, xmlFileNode, localProductDependencies, localPathDependencies, sourceAssetFolder, xmlNodeValue, watchFolderPath))
                {
                    return false;
                }
            }

            // Only merge the dependencies if the attributes parsed cleanly. If a required dependency was missing, then don't add anything that was found.
            productDependencies.insert(productDependencies.end(), localProductDependencies.begin(), localProductDependencies.end());
            pathDependencies.insert(localPathDependencies.begin(), localPathDependencies.end());

            return true;
        }

        AZ::Outcome <AZ::rapidxml::xml_node<char>*, AZStd::string> GetSourceFileRootNode(
            const AZStd::string& filePath,
            AZStd::vector<char>& charBuffer,
            AZ::rapidxml::xml_document<char>& xmlDoc)
        {
            AZ::IO::FileIOStream fileStream;
            if (!fileStream.Open(filePath.c_str(), AZ::IO::OpenMode::ModeRead | AZ::IO::OpenMode::ModeBinary))
            {
                return AZ::Failure(AZStd::string::format("Failed to open source file %s.", filePath.c_str()));
            }

            AZ::IO::SizeType length = fileStream.GetLength();
            if (length == 0)
            {
                return AZ::Failure(AZStd::string("Failed to get the file stream length."));
            }

            charBuffer.resize_no_construct(length + 1);
            fileStream.Read(length, charBuffer.data());
            charBuffer.back() = 0;

            if (!xmlDoc.parse<AZ::rapidxml::parse_default>(charBuffer.data()))
            {
                return AZ::Failure(AZStd::string::format("Failed to parse the source file %s.", filePath.c_str()));
            }

            AZ::rapidxml::xml_node<char>* xmlRootNode = xmlDoc.first_node();
            if (!xmlRootNode)
            {
                return AZ::Failure(AZStd::string::format("Failed to get the root node of the source file %s.", filePath.c_str()));
            }

            return AZ::Success(AZStd::move(xmlRootNode));
        }

        AZStd::string GetSourceFileVersion(
            const AZ::rapidxml::xml_node<char>* xmlFileRootNode,
            const AZStd::string& rootNodeAttributeName)
        {
            if (!rootNodeAttributeName.empty())
            {
                AZ::rapidxml::xml_attribute<char>* xmlNodeNameAttr = xmlFileRootNode->first_attribute(rootNodeAttributeName.c_str(), 0, false);
                if (xmlNodeNameAttr)
                {
                    return xmlNodeNameAttr->value();
                }
            }

            AZStd::string result = "0";
            for (size_t count = 1; count < MaxVersionPartsCount; ++count)
            {
                result += ".0";
            }
            return result;
        }

        bool MatchesVersionConstraints(const AZ::Version<MaxVersionPartsCount>& version, const AZStd::vector<AZStd::string>& versionConstraints)
        {
            if (versionConstraints.size() == 0)
            {
                return true;
            }

            AZ::Dependency<MaxVersionPartsCount> dependency;
            AZ::Outcome<void, AZStd::string> parseOutcome = dependency.ParseVersions(versionConstraints);
            if (!parseOutcome.IsSuccess())
            {
                AZ_Error("XmlBuilderWorker", false, parseOutcome.TakeError().c_str());
                return false;
            }

            return dependency.IsFullfilledBy(AZ::Specifier<MaxVersionPartsCount>(AZ::Uuid::CreateNull(), version));
        }

        AZ::Outcome<AZ::Version<MaxVersionPartsCount>, AZStd::string> ParseFromString(const AZStd::string& versionStr)
        {
            AZ::Version<MaxVersionPartsCount> result;
            AZStd::vector<AZStd::string> versionParts;
            AzFramework::StringFunc::Tokenize(versionStr.c_str(), versionParts, VERSION_SEPARATOR_CHAR);

            size_t versionPartsCount = versionParts.size();
            if (versionPartsCount > MaxVersionPartsCount)
            {
                return AZ::Failure(AZStd::string::format(
                    "Failed to parse invalid version string \"%s\". "
                    "Only version number with at most %zu parts is supported. "
                    , versionStr.c_str(), MaxVersionPartsCount));
            }

            for (int index = 0; index < MaxVersionPartsCount; ++index)
            {
                if (index >= versionPartsCount)
                {
                    result.m_parts[index] = 0;
                }
                else
                {
                    AZStd::string versionPart = versionParts[index];
                    if (!AZStd::all_of(versionPart.begin(), versionPart.end(), isdigit))
                    {
                        return AZ::Failure(AZStd::string::format(
                            "Failed to parse invalid version string \"%s\". "
                            "Unexpected separator character encountered. "
                            "Expected: \"%d\""
                            , versionStr.c_str(), VERSION_SEPARATOR_CHAR));
                    }

                    result.m_parts[index] = AZStd::stoi(versionParts[index]);
                }
            }

            return AZ::Success(result);
        }
    }

    // m_skipServer (3rd Param) should be false - we want to process xml files on the server as it's a generic data format which could
    // have meaningful data for a server
    XmlBuilderWorker::XmlBuilderWorker()
        : CopyDependencyBuilderWorker("xml", true, false)
    {
    }

    void XmlBuilderWorker::RegisterBuilderWorker()
    {
        AssetBuilderSDK::AssetBuilderDesc xmlSchemaBuilderDescriptor;
        xmlSchemaBuilderDescriptor.m_name = "XmlBuilderWorker";
        xmlSchemaBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("(?!.*libs\\/gameaudio\\/).*\\.xml", AssetBuilderSDK::AssetBuilderPattern::PatternType::Regex));
        xmlSchemaBuilderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.vegdescriptorlist", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        xmlSchemaBuilderDescriptor.m_busId = azrtti_typeid<XmlBuilderWorker>();
        xmlSchemaBuilderDescriptor.m_version = 10;
        xmlSchemaBuilderDescriptor.m_createJobFunction =
            AZStd::bind(&XmlBuilderWorker::CreateJobs, this, AZStd::placeholders::_1, AZStd::placeholders::_2);
        xmlSchemaBuilderDescriptor.m_processJobFunction =
            AZStd::bind(&XmlBuilderWorker::ProcessJob, this, AZStd::placeholders::_1, AZStd::placeholders::_2);

        BusConnect(xmlSchemaBuilderDescriptor.m_busId);
        AssetBuilderSDK::AssetBuilderBus::Broadcast(&AssetBuilderSDK::AssetBuilderBusTraits::RegisterBuilderInformation, xmlSchemaBuilderDescriptor);

        bool success;
        AZStd::vector<AZStd::string> assetSafeFolders;
        AzToolsFramework::AssetSystemRequestBus::BroadcastResult(success, &AzToolsFramework::AssetSystemRequestBus::Events::GetAssetSafeFolders, assetSafeFolders);

        for (const AZStd::string& assetSafeFolder : assetSafeFolders)
        {
            AZStd::string schemaFolder = "Schema";
            AzFramework::StringFunc::AssetDatabasePath::Join(assetSafeFolder.c_str(), schemaFolder.c_str(), schemaFolder);
            AddSchemaFileDirectory(schemaFolder);
        }
    }

    void XmlBuilderWorker::AddSchemaFileDirectory(const AZStd::string& schemaFileDirectory)
    {
        if (!AZ::IO::FileIOBase::GetInstance()->Exists(schemaFileDirectory.c_str()))
        {
             return;
        }

        char resolvedPath[AZ_MAX_PATH_LEN];
        AZ::IO::FileIOBase::GetInstance()->ResolvePath(schemaFileDirectory.c_str(), resolvedPath, AZ_MAX_PATH_LEN);
        m_schemaFileDirectories.emplace_back(resolvedPath);
    }

    void XmlBuilderWorker::UnregisterBuilderWorker()
    {
        BusDisconnect();
    }

    AZ::Outcome<AZStd::vector<AssetBuilderSDK::SourceFileDependency>, AZStd::string> XmlBuilderWorker::GetSourceDependencies(
        const AssetBuilderSDK::CreateJobsRequest& request) const
    {
        AZStd::vector<AssetBuilderSDK::SourceFileDependency> sourceDependencies;
        AZStd::vector<AZStd::string> matchedSchemas;

        AZStd::string fullPath;
        AzFramework::StringFunc::AssetDatabasePath::Join(request.m_watchFolder.c_str(), request.m_sourceFile.c_str(), fullPath);

        // Iterate through each schema file and check whether the source XML matches its file path pattern
        for (const AZStd::string& schemaFileDirectory : m_schemaFileDirectories)
        {
            AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> findFilesResult = AzFramework::FileFunc::FindFilesInPath(schemaFileDirectory, SchemaNamePattern, true);
            if (!findFilesResult.IsSuccess())
            {
                return AZ::Failure(AZStd::string::format("Failed to find schema files in directory %s.", schemaFileDirectory.c_str()));
            }

            for (const AZStd::string& schemaPath : findFilesResult.GetValue())
            {
                AzFramework::XmlSchemaAsset schemaAsset;
                AZ::ObjectStream::FilterDescriptor loadFilter = AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
                if (!AZ::Utils::LoadObjectFromFileInPlace(schemaPath, schemaAsset, nullptr, loadFilter))
                {
                    return AZ::Failure(AZStd::string::format("Failed to load schema file: %s.", schemaPath.c_str()));
                }
                if (SourceFileDependsOnSchema(schemaAsset, fullPath.c_str()))
                {
                    matchedSchemas.emplace_back(schemaPath);
                }
            }
        }

        // If we have matched any schemas, then add both the schemas as well as the path dependencies as source dependencies.
        if (matchedSchemas.size() > 0)
        {
            for (const AZStd::string& schemaPath : matchedSchemas)
            {
                AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                sourceFileDependency.m_sourceFileDependencyPath = schemaPath;
                sourceDependencies.emplace_back(sourceFileDependency);
            }

            AZStd::vector<AssetBuilderSDK::ProductDependency> productDependencies;
            AssetBuilderSDK::ProductPathDependencySet pathDependencies;
            if (MatchExistingSchema(fullPath, matchedSchemas, productDependencies, pathDependencies, request.m_watchFolder) != SchemaMatchResult::Error)
            {
                // Product dependencies with wildcards are treated as source dependencies
                for (const auto& pathDependency : pathDependencies)
                {
                    if (!pathDependency.m_dependencyPath.contains('*') && !pathDependency.m_dependencyPath.contains('?'))
                        continue;

                    AssetBuilderSDK::SourceFileDependency sourceFileDependency;
                    sourceFileDependency.m_sourceFileDependencyPath = pathDependency.m_dependencyPath;
                    sourceFileDependency.m_sourceDependencyType = AssetBuilderSDK::SourceFileDependency::SourceFileDependencyType::Wildcards;

                    sourceDependencies.emplace_back(sourceFileDependency);
                }
            }
        }

        return AZ::Success(sourceDependencies);
    }

    bool XmlBuilderWorker::ParseProductDependencies(
        const AssetBuilderSDK::ProcessJobRequest& request,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies)
    {
        AZStd::vector<AZStd::string> matchedSchemas;
        // We've already iterate through all the schemas and found source dependencies in CreateJobs
        // Retrieve the matched schemas from the job parameters in ProcessJob to avoid redundant work
        const auto& paramMap = request.m_jobDescription.m_jobParameters;
        auto startIter = paramMap.find(AZ_CRC_CE("sourceDependencyStartPoint"));
        auto sourceNumIter = paramMap.find(AZ_CRC_CE("sourceDependenciesNum"));
        if (startIter != paramMap.end() && sourceNumIter != paramMap.end())
        {
            int startPoint = AzFramework::StringFunc::ToInt(startIter->second.c_str());
            int sourceDependenciesNum = AzFramework::StringFunc::ToInt(sourceNumIter->second.c_str());
            for (int index = 0; index < sourceDependenciesNum; ++index)
            {
                auto thisSchemaIter = paramMap.find(startPoint + index);
                if (thisSchemaIter != paramMap.end())
                {
                    matchedSchemas.emplace_back(thisSchemaIter->second);
                }
            }
        }
        // If a schema is found or not found, the result is valid. Return false if there was an error.
        return MatchExistingSchema(request.m_fullPath, matchedSchemas, productDependencies, pathDependencies, request.m_watchFolder) != SchemaMatchResult::Error;
    }

    XmlBuilderWorker::SchemaMatchResult XmlBuilderWorker::MatchLastUsedSchema(
        [[maybe_unused]] const AZStd::string& sourceFilePath,
        [[maybe_unused]] AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        [[maybe_unused]] AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
        [[maybe_unused]] const AZStd::string& watchFolderPath) const
    {
        // Check the existing schema info stored in the asset database
        // LY-99056
        return SchemaMatchResult::NoMatchFound;
    }

    XmlBuilderWorker::SchemaMatchResult XmlBuilderWorker::MatchExistingSchema(
        const AZStd::string& sourceFilePath,
        AZStd::vector<AZStd::string>& sourceDependencyPaths,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
        const AZStd::string& watchFolderPath) const
    {
        if (m_printDebug)
        {
            printf("Searching %zu source dependency paths\n", sourceDependencyPaths.size());
        }
        if (sourceDependencyPaths.empty())
        {
            // Iterate through all the schema files if no source dependencies are detected in CreateJobs
            for (const AZStd::string& schemaFileDirectory : m_schemaFileDirectories)
            {
                if (m_printDebug)
                {
                    printf("Finding files in %s\n", schemaFileDirectory.c_str());
                }
                AZ::Outcome<AZStd::list<AZStd::string>, AZStd::string> searchResult = AzFramework::FileFunc::FindFilesInPath(schemaFileDirectory, SchemaNamePattern, true);
                if (searchResult.IsSuccess())
                {
                    AZStd::list<AZStd::string> newSchemaFiles = searchResult.GetValue();
                    if (m_printDebug)
                    {
                        printf("Found %zu files\n", newSchemaFiles.size());
                    }
                    for (const AZStd::string& newSchemaFile : newSchemaFiles)
                    {
                        if (m_printDebug)
                        {
                            printf("Adding %s\n", newSchemaFile.c_str());
                        }
                        sourceDependencyPaths.emplace_back(newSchemaFile);
                    }
                }
            }
        }

        for (const AZStd::string& schemaFilePath : sourceDependencyPaths)
        {
            SchemaMatchResult matchResult = ParseXmlFile(schemaFilePath, sourceFilePath, productDependencies, pathDependencies, watchFolderPath);
            if (m_printDebug)
            {
                printf("Match on %s returns %d\n", schemaFilePath.c_str(), (int)matchResult);
            }
            switch (matchResult)
            {
            case SchemaMatchResult::MatchFound:
                // Update the LastUsedSchema info stored in the asset database
                // LY-99056
                AZ_Printf("XmlBuilderWorker", "Schema file %s found for source %s.", schemaFilePath.c_str(), sourceFilePath.c_str());
                return matchResult;
            case SchemaMatchResult::NoMatchFound:
                // Continue searching through schemas if this one didn't match.
                break;
            case SchemaMatchResult::Error:
            default:
                return matchResult;
            }
        }

        return SchemaMatchResult::NoMatchFound;
    }

    XmlBuilderWorker::SchemaMatchResult XmlBuilderWorker::ParseXmlFile(
        const AZStd::string& schemaFilePath, 
        const AZStd::string& sourceFilePath,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
        const AZStd::string& watchFolderPath) const
    {
        if (schemaFilePath.empty())
        {
            return SchemaMatchResult::NoMatchFound;
        }

        AzFramework::XmlSchemaAsset schemaAsset;
        AZ::ObjectStream::FilterDescriptor loadFilter = AZ::ObjectStream::FilterDescriptor(&AZ::Data::AssetFilterNoAssetLoading, AZ::ObjectStream::FILTERFLAG_IGNORE_UNKNOWN_CLASSES);
        if (!AZ::Utils::LoadObjectFromFileInPlace(schemaFilePath, schemaAsset, nullptr, loadFilter))
        {
            AZ_Error("XmlBuilderWorker", false, "Failed to load schema file: %s.", schemaFilePath.c_str());
            // This isn't a blocking error, the error was on this schema, so try checking the next schema for a match.
            return SchemaMatchResult::NoMatchFound;
        }

        // Get the source file root node and version info
        AZStd::vector<char> xmlFileBuffer;
        AZ::rapidxml::xml_document<char> xmlFileDoc;
        AZ::Outcome <AZ::rapidxml::xml_node<char>*, AZStd::string> rootNodeOutcome = Internal::GetSourceFileRootNode(sourceFilePath, xmlFileBuffer, xmlFileDoc);
        if (!rootNodeOutcome.IsSuccess())
        {
            AZ_Error("XmlBuilderWorker", false, rootNodeOutcome.TakeError().c_str());
            // The XML file couldn't be loaded.
            // We can't know whether this is intentionally an empty file any more than if it were an empty xml with a root node that that were incorrect
            // So we leave it as "nothing will match this" and emit the above error
            return SchemaMatchResult::NoMatchFound;
        }
        AZ::rapidxml::xml_node<char>* xmlFileRootNode = rootNodeOutcome.GetValue();

        AZStd::string sourceFileVersionStr = Internal::GetSourceFileVersion(xmlFileRootNode, schemaAsset.GetVersionSearchRule().GetRootNodeAttributeName());
        AZ::Outcome <AZ::Version<MaxVersionPartsCount>, AZStd::string> SourceFileVersionOutcome = Internal::ParseFromString(sourceFileVersionStr);
        if (!SourceFileVersionOutcome.IsSuccess())
        {
            AZ_Warning("XmlBuilderWorker", false, SourceFileVersionOutcome.TakeError().c_str());
            // This isn't a blocking error, the error was on this schema, so try checking the next schema for a match.
            return SchemaMatchResult::NoMatchFound;
        }
        AZ::Version<MaxVersionPartsCount> version = SourceFileVersionOutcome.GetValue();

        AZ::Outcome <void, bool> matchingRuleOutcome = SearchForMatchingRule(sourceFilePath, schemaFilePath, version, schemaAsset.GetMatchingRules(), watchFolderPath);
        if (!matchingRuleOutcome.IsSuccess())
        {
            // This isn't a blocking error, the error was on this schema, so try checking the next schema for a match.
            return SchemaMatchResult::NoMatchFound;
        }

        bool dependencySearchRuleResult = false;
        if (schemaAsset.UseAZSerialization())
        {
            AZ::SerializeContext* context = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);

            dependencySearchRuleResult = AssetBuilderSDK::GatherProductDependenciesForFile(
                *context,
                sourceFilePath,
                productDependencies,
                pathDependencies);
        }
        else
        {
            AZStd::string sourceAssetFolder = sourceFilePath;
            AzFramework::StringFunc::Path::GetFullPath(sourceAssetFolder.c_str(), sourceAssetFolder);

            dependencySearchRuleResult = SearchForDependencySearchRule(xmlFileRootNode, schemaFilePath, version,
                schemaAsset.GetDependencySearchRules(), productDependencies, pathDependencies, sourceAssetFolder, watchFolderPath);
            if (!dependencySearchRuleResult)
            {
                AZ_Warning("XmlBuilderWorker", false, "File %s matches schema %s's maching rules defined for version %s,"
                    "but has no matching dependency search rules. No dependencies will be emitted for this file."
                    "To resolve this warning, add a new dependency search rule that matches this version and leave it empty if no dependencies need to be emitted.",
                    sourceFilePath.c_str(), schemaFilePath.c_str(), sourceFileVersionStr.c_str());
            }
        }

        // The schema matched, so return either a match was found or there was an error.
        return dependencySearchRuleResult ? SchemaMatchResult::MatchFound : SchemaMatchResult::Error;
    }

    AZ::Outcome <void, bool> XmlBuilderWorker::SearchForMatchingRule(
        const AZStd::string& sourceFilePath, 
        [[maybe_unused]] const AZStd::string& schemaFilePath,
        const AZ::Version<MaxVersionPartsCount>& version,
        const AZStd::vector<AzFramework::MatchingRule>& matchingRules,
        [[maybe_unused]] const AZStd::string& watchFolderPath) const
    {
        if (matchingRules.size() == 0)
        {
            AZ_Error("XmlBuilderWorker", false, "Matching rules are missing.");
            return AZ::Failure(false);
        }

        // Check each matching rule
        for (const AzFramework::MatchingRule& matchingRule : matchingRules)
        {
            if (!matchingRule.Valid())
            {
                 AZ_Error("XmlBuilderWorker", false, "Matching rules defined in schema file %s are invalid.", schemaFilePath.c_str());
                return AZ::Failure(false);
            }

            AZStd::string filePathPattern = matchingRule.GetFilePathPattern();
            AZStd::string excludedFilePathPattern = matchingRule.GetExcludedFilePathPattern();

            if (!Internal::MatchesVersionConstraints(version, matchingRule.GetVersionConstraints()) ||
                !AZStd::wildcard_match(filePathPattern.c_str(), sourceFilePath.c_str()) ||
                (!excludedFilePathPattern.empty() && AZStd::wildcard_match(excludedFilePathPattern.c_str(), sourceFilePath.c_str())))
            {
                continue;
            }
            else
            {
                return AZ::Success();
            }
        }

        return AZ::Failure(true);
    }

    bool XmlBuilderWorker::SearchForDependencySearchRule(
        AZ::rapidxml::xml_node<char>* xmlFileRootNode,
        [[maybe_unused]] const AZStd::string& schemaFilePath,
        const AZ::Version<MaxVersionPartsCount>& version,
        const AZStd::vector<AzFramework::DependencySearchRule>& dependencySearchRules,
        AZStd::vector<AssetBuilderSDK::ProductDependency>& productDependencies,
        AssetBuilderSDK::ProductPathDependencySet& pathDependencies,
        const AZStd::string& sourceAssetFolder,
        const AZStd::string& watchFolderPath) const
    {
        if (dependencySearchRules.size() == 0)
        {
            AZ_Error("XmlBuilderWorker", false, "Dependency search rules are missing.");
            return false;
        }

        for (const AzFramework::DependencySearchRule& dependencySearchRule : dependencySearchRules)
        {
            if (!Internal::MatchesVersionConstraints(version, dependencySearchRule.GetVersionConstraints()))
            {
                continue;
            }

            // Pre-calculate the list of all the XML nodes and mappings from node names to the corresponding nodes
            // This could help to reduce the number of traversals when we need to find a match which could appear multiple times in the source file
            AZStd::unordered_map<AZStd::string, AZStd::vector<const AZ::rapidxml::xml_node<char>*>> xmlNodeMappings;
            AZStd::vector<const AZ::rapidxml::xml_node<char>*> xmlNodeList;
            TraverseSourceFile(xmlFileRootNode, xmlNodeMappings, xmlNodeList);

            AZStd::vector<AzFramework::SearchRuleDefinition> searchRuleDefinitions = dependencySearchRule.GetSearchRules();

            for (const AzFramework::SearchRuleDefinition& searchRuleDefinition : searchRuleDefinitions)
            {
                AzFramework::XmlSchemaElement searchRuleRootNode = searchRuleDefinition.GetSearchRuleStructure();

                AZStd::vector<const AZ::rapidxml::xml_node<char>*> validNodes;
                if (searchRuleRootNode.GetName() == "*")
                {
                    // If the schema element node name is "*", it could match any node in the source XML file
                    // We can use this to specify an attribute which contains product dependency info and could exist in any XML node
                    validNodes = xmlNodeList;
                }
                else
                {
                    if (searchRuleDefinition.IsRelativeToXmlRoot())
                    {
                        // If the dependency search rule is relative to the root, we will only care about the match at the root level
                        validNodes = { xmlFileRootNode };
                    }
                    else
                    {
                        // Otherwise we need to check for any match that appears in the XML file structure
                        validNodes = xmlNodeMappings[searchRuleRootNode.GetName()];
                    }
                }

                for (const AZ::rapidxml::xml_node<char>* validNode : validNodes)
                {
                    Internal::ParseElementNode(searchRuleRootNode, validNode, productDependencies, pathDependencies, sourceAssetFolder, watchFolderPath);
                }
            }

            return true;
        }

        return false;
    }

    void XmlBuilderWorker::TraverseSourceFile(
        const AZ::rapidxml::xml_node<char>* currentNode,
        AZStd::unordered_map<AZStd::string, AZStd::vector<const AZ::rapidxml::xml_node<char>*>>& xmlNodeMappings,
        AZStd::vector<const AZ::rapidxml::xml_node<char>*>& xmlNodeList) const
    {
        if (!currentNode)
        {
            return;
        }

        xmlNodeMappings[currentNode->name()].emplace_back(currentNode);
        xmlNodeList.emplace_back(currentNode);

        for (AZ::rapidxml::xml_node<char>* childNode = currentNode->first_node(); childNode; childNode = childNode->next_sibling())
        {
            TraverseSourceFile(childNode, xmlNodeMappings, xmlNodeList);
        }
    }
}
