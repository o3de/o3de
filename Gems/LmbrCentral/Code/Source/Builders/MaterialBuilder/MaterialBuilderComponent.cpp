/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "MaterialBuilderComponent.h"
#include <AzCore/IO/Path/Path.h>
#include <AzCore/IO/SystemFile.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzFramework/IO/LocalFileIO.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/XML/rapidxml.h>
#include <cctype>

namespace MaterialBuilder
{
    [[maybe_unused]] const char s_materialBuilder[] = "MaterialBuilder";

    namespace Internal
    {
        const char g_nodeNameMaterial[] = "Material";
        const char g_nodeNameSubmaterial[] = "SubMaterials";
        const char g_nodeNameTexture[] = "Texture";
        const char g_nodeNameTextures[] = "Textures";
        const char g_attributeFileName[] = "File";

        const int g_numSourceImageFormats = 9;
        const char* g_sourceImageFormats[g_numSourceImageFormats] = { ".tif", ".tiff", ".bmp", ".gif", ".jpg", ".jpeg", ".tga", ".png", ".dds" };
        bool IsSupportedImageExtension(const AZStd::string& extension)
        {
            for (const char* format : g_sourceImageFormats)
            {
                if (extension == format)
                {
                    return true;
                }
            }
            return false;
        }

        // Cleans up legacy pathing from older materials
        const char* CleanLegacyPathingFromTexturePath(const char* texturePath)
        {
            // Copied from MaterialHelpers::SetTexturesFromXml, line 459
            // legacy.  Some textures used to be referenced using "engine\\" or "engine/" - this is no longer valid
            if (
                (strlen(texturePath) > 7) &&
                (azstrnicmp(texturePath, "engine", 6) == 0) &&
                ((texturePath[6] == '\\') || (texturePath[6] == '/'))
                )
            {
                texturePath = texturePath + 7;
            }

            // legacy:  Files were saved into a mtl with many leading forward or back slashes, we eat them all here.  We want it to start with a relative path.
            const char* actualFileName = texturePath;
            while ((actualFileName[0]) && ((actualFileName[0] == '\\') || (actualFileName[0] == '/')))
            {
                ++actualFileName;
            }
            return actualFileName;
        }

        // Parses the material XML for all texture paths
        AZ::Outcome<AZStd::string, AZStd::string> GetTexturePathsFromMaterial(AZ::rapidxml::xml_node<char>* materialNode, AZStd::vector<AZStd::string>& paths)
        {
            AZ::Outcome<AZStd::string, AZStd::string> resultOutcome = AZ::Failure(AZStd::string(""));
            AZStd::string success_with_warning_message;

            // check if this material has a set of textures defined, and if so, grab all the paths from the textures
            AZ::rapidxml::xml_node<char>* texturesNode = materialNode->first_node(g_nodeNameTextures);
            if (texturesNode)
            {
                AZ::rapidxml::xml_node<char>* textureNode = texturesNode->first_node(g_nodeNameTexture);
                // it is possible for an empty <Textures> node to exist for things like collision materials, so check
                //  to make sure that there is at least one child <Texture> node before starting to iterate.
                if (textureNode)
                {
                    do
                    {
                        AZ::rapidxml::xml_attribute<char>* fileAttribute = textureNode->first_attribute(g_attributeFileName);
                        if (!fileAttribute)
                        {
                            success_with_warning_message = "Texture node exists but does not have a file attribute defined";
                        }
                        else
                        {
                            const char* rawTexturePath = fileAttribute->value();
                            // do an initial clean-up of the path taken from the file, similar to MaterialHelpers::SetTexturesFromXml
                            AZStd::string texturePath = CleanLegacyPathingFromTexturePath(rawTexturePath);
                            paths.emplace_back(AZStd::move(texturePath));
                        }

                        textureNode = textureNode->next_sibling(g_nodeNameTexture);
                    } while (textureNode);
                }
            }

            // check to see if this material has sub materials defined. If so, recurse into this function for each sub material
            AZ::rapidxml::xml_node<char>* subMaterialsNode = materialNode->first_node(g_nodeNameSubmaterial);
            if (subMaterialsNode)
            {
                AZ::rapidxml::xml_node<char>* subMaterialNode = subMaterialsNode->first_node(g_nodeNameMaterial);
                if (subMaterialNode == nullptr)
                {
                    // this is a malformed material as there is no material node child in the SubMaterials node, so error out
                    return AZ::Failure(AZStd::string("SubMaterials node exists but does not have any child Material nodes."));
                }

                do
                {
                    // grab the texture paths from the submaterial, or error out if necessary
                    AZ::Outcome<AZStd::string, AZStd::string> subMaterialTexturePathsResult = GetTexturePathsFromMaterial(subMaterialNode, paths);
                    if (!subMaterialTexturePathsResult.IsSuccess())
                    {
                        return subMaterialTexturePathsResult;
                    }
                    else if (!subMaterialTexturePathsResult.GetValue().empty())
                    {
                        success_with_warning_message = subMaterialTexturePathsResult.GetValue();
                    }

                    subMaterialNode = subMaterialNode->next_sibling(g_nodeNameMaterial);
                } while (subMaterialNode);
            }

            if (texturesNode == nullptr && subMaterialsNode == nullptr)
            {
                return AZ::Failure(AZStd::string("Failed to find a Textures node or SubMaterials node in this material. At least one of these must exist to be able to gather texture dependencies."));
            }

            if (!success_with_warning_message.empty())
            {
                return AZ::Success(success_with_warning_message);
            }
            return AZ::Success(AZStd::string());
        }

        // find a sequence of digits with a string starting from lastDigitIndex, and try to parse that sequence to and int
        //  and store it in outAnimIndex.
        bool ParseFilePathForCompleteNumber(const AZStd::string& filePath, int& lastDigitIndex, int& outAnimIndex)
        {
            int firstAnimIndexDigit = lastDigitIndex;
            while (isdigit(static_cast<unsigned char>(filePath[lastDigitIndex])))
            {
                ++lastDigitIndex;
            }
            if (!AzFramework::StringFunc::LooksLikeInt(filePath.substr(firstAnimIndexDigit, lastDigitIndex - firstAnimIndexDigit).c_str(), &outAnimIndex))
            {
                return false;
            }
            return true;
        }

        // Parse the texture path for a texture animation to determine the actual names of the textures to resolve that
        //  make up the entire sequence.
        AZ::Outcome<void, AZStd::string> GetAllTexturesInTextureSequence(const AZStd::string& path, AZStd::vector<AZStd::string>& texturesInSequence)
        {
            // Taken from CShaderMan::mfReadTexSequence
            // All comments next to variable declarations in this function are the original variable names in
            //  CShaderMan::mfReadTexSequence, to help keep track of how these variables relate to the original function
            AZStd::string prefix;
            AZStd::string postfix;

            AZStd::string filePath = path;  // name
            AZStd::string extension;        // ext
            AzFramework::StringFunc::Path::GetExtension(filePath.c_str(), extension);
            AzFramework::StringFunc::Path::StripExtension(filePath);

            // unsure if it is actually possible to enter here or the original version with '$' as the indicator
            //  for texture sequences, but they check for both just in case, so this will match the behavior.
            char separator = '#';           // chSep
            int firstSeparatorIndex = static_cast<int>(filePath.find(separator));
            if (firstSeparatorIndex == AZStd::string::npos)
            {
                firstSeparatorIndex = static_cast<int>(filePath.find('$'));
                if (firstSeparatorIndex == AZStd::string::npos)
                {
                    return AZ::Failure(AZStd::string("Failed to find separator '#' or '$' in texture path."));
                }
                separator = '$';
            }

            // we don't actually care about getting the speed of the animation, so just remove everything from the
            //  end of the string starting with the last open parenthesis
            size_t speedStartIndex = filePath.find_last_of('(');
            if (speedStartIndex != AZStd::string::npos)
            {
                AzFramework::StringFunc::LKeep(filePath, speedStartIndex);
                AzFramework::StringFunc::Append(filePath, '\0');
            }

            // try to find where the digits start after the separator (there can be any number of separators
            //  between the texture name prefix and where the digit range starts)
            int firstAnimIndexDigit = -1;   // m
            int numSeparators = 0;          // j
            for (int stringIndex = firstSeparatorIndex; stringIndex < filePath.length(); ++stringIndex)
            {
                if (filePath[stringIndex] == separator)
                {
                    ++numSeparators;
                    if (firstSeparatorIndex == -1)
                    {
                        firstSeparatorIndex = stringIndex;
                    }
                }
                else if (firstSeparatorIndex > 0 && firstAnimIndexDigit < 0)
                {
                    firstAnimIndexDigit = stringIndex;
                    break;
                }
            }
            if (numSeparators == 0)
            {
                return AZ::Failure(AZStd::string("Failed to find separator '#' or '$' in texture path."));
            }

            // store off everything before the separator
            prefix = AZStd::move(filePath.substr(0, firstSeparatorIndex));

            int startAnimIndex = 0; // startn
            int endAnimIndex = 0;   // endn
            // we only found the separator, but no indexes, so just assume its 0 - 999
            if (firstAnimIndexDigit < 0)
            {
                startAnimIndex = 0;
                endAnimIndex = 999;
            }
            else
            {
                // find the length of the first index, then parse that to an int
                int lastDigitIndex = firstAnimIndexDigit;
                if (!ParseFilePathForCompleteNumber(filePath, lastDigitIndex, startAnimIndex))
                {
                    return AZ::Failure(AZStd::string("Failed to determine first index of the sequence after the separators in texture path."));
                }

                // reset to the start of the next index
                ++lastDigitIndex;

                // find the length of the end index, then parse that to an int
                if (!ParseFilePathForCompleteNumber(filePath, lastDigitIndex, endAnimIndex))
                {
                    return AZ::Failure(AZStd::string("Failed to determine last index of the sequence after the first index of the sequence in texture path."));
                }

                // save off the rest of the string
                postfix = AZStd::move(filePath.substr(lastDigitIndex));
            }

            int numTextures = endAnimIndex - startAnimIndex + 1;
            const char* textureNameFormat = "%s%.*d%s%s"; // prefix, num separators (number of digits), sequence index, postfix, extension)
            for (int sequenceIndex = 0; sequenceIndex < numTextures; ++sequenceIndex)
            {
                texturesInSequence.emplace_back(AZStd::move(AZStd::string::format(textureNameFormat, prefix.c_str(), numSeparators, startAnimIndex + sequenceIndex, postfix.c_str(), extension.c_str())));
            }

            return AZ::Success();
        }

        // Determine which product path to use based on the path stored in the texture, and make it relative to
        //  the cache.
        bool ResolveMaterialTexturePath(const AZStd::string& path, AZStd::string& outPath)
        {
            AZStd::string aliasedPath = path;

            //if its a source image format try to load the dds
            AZStd::string extension;
            bool hasExtension = AzFramework::StringFunc::Path::GetExtension(path.c_str(), extension);

            // Replace all supported extensions with DDS if it has an extension. If the extension exists but is not supported, fail out.
            if (hasExtension && IsSupportedImageExtension(extension))
            {
                AzFramework::StringFunc::Path::ReplaceExtension(aliasedPath, ".dds");
            }
            else if (hasExtension)
            {
                AZ_Warning(s_materialBuilder, false, "Failed to resolve texture path %s as the path is not to a supported texture format. Please make sure that textures in materials are formats supported by Open 3D Engine.", aliasedPath.c_str());
                return false;
            }

            AZStd::to_lower(aliasedPath.begin(), aliasedPath.end());
            AzFramework::StringFunc::Path::Normalize(aliasedPath);

            AZStd::string currentFolderSpecifier = AZStd::string::format(".%c", AZ_CORRECT_FILESYSTEM_SEPARATOR);
            if (AzFramework::StringFunc::StartsWith(aliasedPath, currentFolderSpecifier))
            {
                AzFramework::StringFunc::Strip(aliasedPath, currentFolderSpecifier.c_str(), false, true);
            }

            AZStd::string resolvedPath;
            char fullPathBuffer[AZ_MAX_PATH_LEN] = {};
            // if there is an alias already at the front of the path, resolve it, and try to make it relative to the
            //  cache (@products@). If it can't, then error out.
            // This case handles the possibility of aliases existing in texture paths in materials that is still supported
            //  by the legacy loading code, however it is not currently used, so the else path is always taken.
            if (aliasedPath[0] == '@')
            {
                if (!AZ::IO::FileIOBase::GetDirectInstance()->ResolvePath(aliasedPath.c_str(), fullPathBuffer, AZ_MAX_PATH_LEN))
                {
                    AZ_Warning(s_materialBuilder, false, "Failed to resolve the alias in texture path %s. Please make sure all aliases are registered with the engine.", aliasedPath.c_str());
                    return false;
                }
                resolvedPath = fullPathBuffer;
                AzFramework::StringFunc::Path::Normalize(resolvedPath);
                if (!AzFramework::StringFunc::Replace(resolvedPath, AZ::IO::FileIOBase::GetDirectInstance()->GetAlias("@products@"), ""))
                {
                    AZ_Warning(s_materialBuilder, false, "Failed to resolve aliased texture path %s to be relative to the asset cache. Please make sure this alias resolves to a path within the asset cache.", aliasedPath.c_str());
                    return false;
                }
            }
            else
            {
                resolvedPath = AZStd::move(aliasedPath);
            }

            // AP deferred path resolution requires UNIX separators and no leading separators, so clean up and convert here
            if (AzFramework::StringFunc::StartsWith(resolvedPath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING))
            {
                AzFramework::StringFunc::Strip(resolvedPath, AZ_CORRECT_FILESYSTEM_SEPARATOR, false, true);
            }
            AzFramework::StringFunc::Replace(resolvedPath, AZ_CORRECT_FILESYSTEM_SEPARATOR_STRING, "/");

            outPath = AZStd::move(resolvedPath);
            return true;
        }

    }

    BuilderPluginComponent::BuilderPluginComponent()
    {
    }

    BuilderPluginComponent::~BuilderPluginComponent()
    {
    }

    void BuilderPluginComponent::Init()
    {
    }

    void BuilderPluginComponent::Activate()
    {
        // Register material builder
        AssetBuilderSDK::AssetBuilderDesc builderDescriptor;
        builderDescriptor.m_name = "MaterialBuilderWorker";
        builderDescriptor.m_patterns.push_back(AssetBuilderSDK::AssetBuilderPattern("*.mtl", AssetBuilderSDK::AssetBuilderPattern::PatternType::Wildcard));
        builderDescriptor.m_busId = MaterialBuilderWorker::GetUUID();
        builderDescriptor.m_version = 5;
        builderDescriptor.m_createJobFunction = AZStd::bind(&MaterialBuilderWorker::CreateJobs, &m_materialBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);
        builderDescriptor.m_processJobFunction = AZStd::bind(&MaterialBuilderWorker::ProcessJob, &m_materialBuilder, AZStd::placeholders::_1, AZStd::placeholders::_2);

        // (optimization) this builder does not emit source dependencies:
        builderDescriptor.m_flags |= AssetBuilderSDK::AssetBuilderDesc::BF_EmitsNoDependencies;

        m_materialBuilder.BusConnect(builderDescriptor.m_busId);

        EBUS_EVENT(AssetBuilderSDK::AssetBuilderBus, RegisterBuilderInformation, builderDescriptor);
    }

    void BuilderPluginComponent::Deactivate()
    {
        m_materialBuilder.BusDisconnect();
    }

    void BuilderPluginComponent::Reflect([[maybe_unused]] AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<BuilderPluginComponent, AZ::Component>()
                ->Version(1)
                ->Attribute(AZ::Edit::Attributes::SystemComponentTags, AZStd::vector<AZ::Crc32>({ AssetBuilderSDK::ComponentTags::AssetBuilder }));
        }
    }

    MaterialBuilderWorker::MaterialBuilderWorker()
    {
    }
    MaterialBuilderWorker::~MaterialBuilderWorker()
    {
    }

    void MaterialBuilderWorker::ShutDown()
    {
        // This will be called on a different thread than the process job thread
        m_isShuttingDown = true;
    }

    // This happens early on in the file scanning pass.
    // This function should always create the same jobs and not do any checking whether the job is up to date.
    void MaterialBuilderWorker::CreateJobs(const AssetBuilderSDK::CreateJobsRequest& request, AssetBuilderSDK::CreateJobsResponse& response)
    {
        if (m_isShuttingDown)
        {
            response.m_result = AssetBuilderSDK::CreateJobsResultCode::ShuttingDown;
            return;
        }

        for (const AssetBuilderSDK::PlatformInfo& info : request.m_enabledPlatforms)
        {
            AssetBuilderSDK::JobDescriptor descriptor;
            descriptor.m_jobKey = "Material Builder Job";
            descriptor.SetPlatformIdentifier(info.m_identifier.c_str());
            descriptor.m_priority = 8; // meshes are more important (at 10) but mats are still pretty important.
            response.m_createJobOutputs.push_back(descriptor);
        }

        response.m_result = AssetBuilderSDK::CreateJobsResultCode::Success;
    }

    // The request will contain the CreateJobResponse you constructed earlier, including any keys and
    // values you placed into the hash table
    void MaterialBuilderWorker::ProcessJob(const AssetBuilderSDK::ProcessJobRequest& request, AssetBuilderSDK::ProcessJobResponse& response)
    {
        AZ_TracePrintf(AssetBuilderSDK::InfoWindow, "Starting Job.\n");
        AZStd::string fileName;
        AzFramework::StringFunc::Path::GetFullFileName(request.m_fullPath.c_str(), fileName);
        AZStd::string destPath;

        // Do all work inside the tempDirPath.
        AzFramework::StringFunc::Path::ConstructFull(request.m_tempDirPath.c_str(), fileName.c_str(), destPath, true);

        AZ::IO::LocalFileIO fileIO;
        if (!m_isShuttingDown && fileIO.Copy(request.m_fullPath.c_str(), destPath.c_str()) == AZ::IO::ResultCode::Success)
        {
            // Push assets back into the response's product list
            // Assets you created in your temp path can be specified using paths relative to the temp path
            // since that is assumed where you're writing stuff.
            AZStd::string relPath = destPath;
            AssetBuilderSDK::ProductPathDependencySet dependencyPaths;
            response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Success;
            AssetBuilderSDK::JobProduct jobProduct(fileName);

            bool dependencyResult = GatherProductDependencies(request.m_fullPath, dependencyPaths);
            if (dependencyResult)
            {
                jobProduct.m_pathDependencies = AZStd::move(dependencyPaths);
                jobProduct.m_dependenciesHandled = true; // We've output the dependencies immediately above so it's OK to tell the AP we've handled dependencies
            }
            else
            {
                AZ_Error(s_materialBuilder, false, "Dependency gathering for %s failed.", request.m_fullPath.c_str());
            }
            response.m_outputProducts.push_back(jobProduct);
        }
        else
        {
            if (m_isShuttingDown)
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Cancelled job %s because shutdown was requested.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Cancelled;
            }
            else
            {
                AZ_TracePrintf(AssetBuilderSDK::ErrorWindow, "Error during processing job %s.\n", request.m_fullPath.c_str());
                response.m_resultCode = AssetBuilderSDK::ProcessJobResult_Failed;
            }
        }
    }

    bool MaterialBuilderWorker::GetResolvedTexturePathsFromMaterial(const AZStd::string& path, AZStd::vector<AZStd::string>& resolvedPaths)
    {
        if (!AZ::IO::SystemFile::Exists(path.c_str()))
        {
            AZ_Error(s_materialBuilder, false, "Failed to find material at path %s. Please make sure this material exists on disk.", path.c_str());
            return false;
        }

        uint64_t fileSize = AZ::IO::SystemFile::Length(path.c_str());
        if (fileSize == 0)
        {
            AZ_Error(s_materialBuilder, false, "Material at path %s is an empty file. Please make sure this material was properly saved to disk.", path.c_str());
            return false;
        }

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = 0;
        if (!AZ::IO::SystemFile::Read(path.c_str(), buffer.data()))
        {
            AZ_Error(s_materialBuilder, false, "Failed to read material at path %s. Please make sure the file is not open or being edited by another program.", path.c_str());
            return false;
        }

        AZ::rapidxml::xml_document<char>* xmlDoc = azcreate(AZ::rapidxml::xml_document<char>, (), AZ::SystemAllocator, "Mtl builder temp XML Reader");
        if (!xmlDoc->parse<AZ::rapidxml::parse_no_data_nodes>(buffer.data()))
        {
            azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
            AZ_Error(s_materialBuilder, false, "Failed to parse material at path %s into XML. Please make sure that the material was properly saved to disk.", path.c_str());
            return false;
        }

        // if the first node in this file isn't a material, this must not actually be a material so it can't have deps
        AZ::rapidxml::xml_node<char>* rootNode = xmlDoc->first_node(Internal::g_nodeNameMaterial);
        if (!rootNode)
        {
            azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
            AZ_Error(s_materialBuilder, false, "Failed to find root material node for material at path %s. Please make sure that the material was properly saved to disk.", path.c_str());
            return false;
        }

        AZStd::vector<AZStd::string> texturePaths;
        // gather all textures in the material file
        AZ::Outcome<AZStd::string, AZStd::string> texturePathsResult = Internal::GetTexturePathsFromMaterial(rootNode, texturePaths);
        if (!texturePathsResult.IsSuccess())
        {
            azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);
            AZ_Error(s_materialBuilder, false, "Failed to gather dependencies for %s as the material file is malformed. %s", path.c_str(), texturePathsResult.GetError().c_str());
            return false;
        }
        else if (!texturePathsResult.GetValue().empty())
        {
            AZ_Warning(s_materialBuilder, false, "Some nodes in material %s could not be read as the material is malformed. %s. Some dependencies might not be reported correctly. Please make sure that the material was properly saved to disk.", path.c_str(), texturePathsResult.GetValue().c_str());
        }
        azdestroy(xmlDoc, AZ::SystemAllocator, AZ::rapidxml::xml_document<char>);

        // fail this if there are absolute paths.
        for (const AZStd::string& texPath : texturePaths)
        {
            if (AZ::IO::PathView(texPath).IsAbsolute())
            {
                AZ_Warning(s_materialBuilder, false, "Skipping resolving of texture path %s in material %s as the texture path is an absolute path. Please update the texture path to be relative to the asset cache.", texPath.c_str(), path.c_str());
                texturePaths.erase(AZStd::find(texturePaths.begin(), texturePaths.end(), texPath));
            }
        }

        // for each path in the array, split any texture animation entry up into the individual files and add each to the list.
        for (const AZStd::string& texPath : texturePaths)
        {
            if (texPath.find('#') != AZStd::string::npos)
            {
                AZStd::vector<AZStd::string> actualTexturePaths;
                AZ::Outcome<void, AZStd::string> parseTextureSequenceResult = Internal::GetAllTexturesInTextureSequence(texPath, actualTexturePaths);
                if (parseTextureSequenceResult.IsSuccess())
                {
                    texturePaths.erase(AZStd::find(texturePaths.begin(), texturePaths.end(), texPath));
                    texturePaths.insert(texturePaths.end(), actualTexturePaths.begin(), actualTexturePaths.end());
                }
                else
                {
                    texturePaths.erase(AZStd::find(texturePaths.begin(), texturePaths.end(), texPath));
                    AZ_Warning(s_materialBuilder, false, "Failed to parse texture sequence %s when trying to gather dependencies for %s. %s Please make sure the texture sequence path is formatted correctly. Registering dependencies for the texture sequence will be skipped.", texPath.c_str(), path.c_str(), parseTextureSequenceResult.GetError().c_str());
                }
            }
        }

        // for each texture in the file
        for (const AZStd::string& texPath : texturePaths)
        {
            // if the texture path starts with a '$' then it is a special runtime defined texture, so it it doesn't have
            //  an actual asset on disk to depend on. If the texture path doesn't have an extension, then it is a texture
            //  that is determined at runtime (such as 'nearest_cubemap'), so also ignore those, as other things pull in
            //  those dependencies.
            if (AzFramework::StringFunc::StartsWith(texPath, "$") || !AzFramework::StringFunc::Path::HasExtension(texPath.c_str()))
            {
                continue;
            }

            // resolve the path in the file.
            AZStd::string resolvedPath;
            if (!Internal::ResolveMaterialTexturePath(texPath, resolvedPath))
            {
                AZ_Warning(s_materialBuilder, false, "Failed to resolve texture path %s to a product path when gathering dependencies for %s. Registering dependencies on this texture path will be skipped.", texPath.c_str(), path.c_str());
                continue;
            }

            resolvedPaths.emplace_back(AZStd::move(resolvedPath));
        }

        return true;
    }

    bool MaterialBuilderWorker::PopulateProductDependencyList(AZStd::vector<AZStd::string>& resolvedPaths, AssetBuilderSDK::ProductPathDependencySet& dependencies)
    {
        for (const AZStd::string& texturePath : resolvedPaths)
        {
            if (texturePath.empty())
            {
                AZ_Warning(s_materialBuilder, false, "Resolved path is empty.\n");
                return false;
            }

            dependencies.emplace(texturePath, AssetBuilderSDK::ProductPathDependencyType::ProductFile);
        }
        return true;
    }

    bool MaterialBuilderWorker::GatherProductDependencies(const AZStd::string& path, AssetBuilderSDK::ProductPathDependencySet& dependencies)
    {
        AZStd::vector<AZStd::string> resolvedTexturePaths;
        if (!GetResolvedTexturePathsFromMaterial(path, resolvedTexturePaths))
        {
            return false;
        }

        if (!PopulateProductDependencyList(resolvedTexturePaths, dependencies))
        {
            AZ_Warning(s_materialBuilder, false, "Failed to populate dependency list for material %s with possible variants for textures.", path.c_str());
        }

        return true;
    }

    AZ::Uuid MaterialBuilderWorker::GetUUID()
    {
        return AZ::Uuid::CreateString("{258D34AC-12F8-4196-B535-3206D8E7287B}");
    }
}
