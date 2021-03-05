/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#include "GemDescription.h"
#include "GemRegistry.h"

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/JSON/error/en.h>
#include <AzFramework/StringFunc/StringFunc.h>

// For LinkTypeFromString
#include <unordered_map>
#include <string>

namespace Gems
{
    GemDescription::GemDescription()
        : m_id(AZ::Uuid::CreateNull())
        , m_name()
        , m_displayName()
        , m_version()
        , m_path()
        , m_absolutePath()
        , m_summary()
        , m_iconPath()
        , m_tags()
        , m_modules()
        , m_modulesByType()
        , m_engineModuleClass()
        , m_gemDependencies()
        , m_gameGem(false)
        , m_required(false)
        , m_engineDependency(nullptr)
    {
        m_modulesByType.emplace(ModuleDefinition::Type::GameModule);
        m_modulesByType.emplace(ModuleDefinition::Type::ServerModule);
        m_modulesByType.emplace(ModuleDefinition::Type::EditorModule);
        m_modulesByType.emplace(ModuleDefinition::Type::StaticLib);
        m_modulesByType.emplace(ModuleDefinition::Type::Builder);
        m_modulesByType.emplace(ModuleDefinition::Type::Standalone);
    }

    GemDescription::GemDescription(const GemDescription& rhs)
        : m_id(rhs.m_id)
        , m_name(rhs.m_name)
        , m_displayName(rhs.m_displayName)
        , m_version(rhs.m_version)
        , m_path(rhs.m_path)
        , m_absolutePath(rhs.m_absolutePath)
        , m_summary(rhs.m_summary)
        , m_iconPath(rhs.m_iconPath)
        , m_tags(rhs.m_tags)
        , m_modules(rhs.m_modules)
        , m_modulesByType(rhs.m_modulesByType)
        , m_engineModuleClass(rhs.m_engineModuleClass)
        , m_gemDependencies(rhs.m_gemDependencies)
        , m_gameGem(rhs.m_gameGem)
        , m_required(rhs.m_required)
        , m_engineDependency(rhs.m_engineDependency)
    {
    }


    GemDescription::GemDescription(GemDescription&& rhs)
        : m_id(rhs.m_id)
        , m_name(AZStd::move(rhs.m_name))
        , m_displayName(AZStd::move(rhs.m_displayName))
        , m_path(AZStd::move(rhs.m_path))
        , m_absolutePath(AZStd::move(rhs.m_absolutePath))
        , m_summary(AZStd::move(rhs.m_summary))
        , m_iconPath(AZStd::move(rhs.m_iconPath))
        , m_tags(AZStd::move(rhs.m_tags))
        , m_version(rhs.m_version)
        , m_modules(AZStd::move(rhs.m_modules))
        , m_modulesByType(AZStd::move(rhs.m_modulesByType))
        , m_engineModuleClass(AZStd::move(rhs.m_engineModuleClass))
        , m_gemDependencies(AZStd::move(rhs.m_gemDependencies))
        , m_gameGem(AZStd::move(rhs.m_gameGem))
        , m_required(AZStd::move(rhs.m_required))
        , m_engineDependency(AZStd::move(rhs.m_engineDependency))
    {
        rhs.m_id = AZ::Uuid::CreateNull();
        rhs.m_version = GemVersion { 0, 0, 0 };
    }

    // returns whether conversion was successful
    static bool LinkTypeFromString(const char* value, LinkType& linkTypeOut)
    {
        // static map for lookups
        static const std::unordered_map<std::string, LinkType> linkNameToType = {
            { GPF_TAG_LINK_TYPE_DYNAMIC,        LinkType::Dynamic },
            { GPF_TAG_LINK_TYPE_DYNAMIC_STATIC, LinkType::DynamicStatic },
            { GPF_TAG_LINK_TYPE_NO_CODE,        LinkType::NoCode },
        };

        auto found = linkNameToType.find(value);
        if (found != linkNameToType.end())
        {
            linkTypeOut = found->second;
            return true;
        }
        else
        {
            return false;
        }
    }
    
    static bool ModuleTypeFromString(const char* value, ModuleDefinition::Type& moduleTypeOut)
    {
        static const std::unordered_map<const char*, ModuleDefinition::Type> moduleNameToType = {
            { GPF_TAG_MODULE_TYPE_GAME_MODULE,      ModuleDefinition::Type::GameModule },
            { GPF_TAG_MODULE_TYPE_SERVER_MODULE,    ModuleDefinition::Type::ServerModule },
            { GPF_TAG_MODULE_TYPE_EDITOR_MODULE,    ModuleDefinition::Type::EditorModule },
            { GPF_TAG_MODULE_TYPE_STATIC_LIB,       ModuleDefinition::Type::StaticLib },
            { GPF_TAG_MODULE_TYPE_BUILDER,          ModuleDefinition::Type::Builder },
            { GPF_TAG_MODULE_TYPE_STANDALONE,       ModuleDefinition::Type::Standalone },
        };

        auto found = AZStd::find_if(moduleNameToType.begin(), moduleNameToType.end(), [&value](decltype(moduleNameToType)::const_reference pair) {
            return strcmp(pair.first, value) == 0;
        });

        if (found != moduleNameToType.end())
        {
            moduleTypeOut = found->second;
            return true;
        }
        else
        {
            return false;
        }
    }

    // Bring contents of file up to current version.
    AZ::Outcome<void, AZStd::string> UpgradeGemDescriptionJson(rapidjson::Document& descNode)
    {
        // get format version
        if (!RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_FORMAT_VERSION, IsInt))
        {
            return AZ::Failure(AZStd::string(GPF_TAG_FORMAT_VERSION " int is required."));
        }

        int gemFormatVersion = descNode[GPF_TAG_FORMAT_VERSION].GetInt();

        // decline ancient and future versions
        if (gemFormatVersion < 2 || gemFormatVersion > GEM_DEF_FILE_VERSION)
        {
            return AZ::Failure(AZStd::string::format(GPF_TAG_FORMAT_VERSION " is version %d, but %d is expected.",
                gemFormatVersion, GEM_DEF_FILE_VERSION));
        }

        // upgrade v2 -> v3
        if (gemFormatVersion < 3)
        {
            // beginning in v3 Gems contain an AZ::Module, in the past they contained an IGem
            descNode.AddMember("IsLegacyIGem", true, descNode.GetAllocator());
        }

        // upgrade v3 -> v4
        if (gemFormatVersion < 4)
        {
            // read link type, if not NoCode, migrate to GameModule
            if (!RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_LINK_TYPE, IsString))
            {
                return AZ::Failure(AZStd::string(GPF_TAG_LINK_TYPE " string must not be empty."));
            }
            // Explicitly copy string so we can remove it from the object
            AZStd::string linkTypeString = descNode[GPF_TAG_LINK_TYPE].GetString();
            descNode.RemoveMember(GPF_TAG_LINK_TYPE);

            LinkType linkType;
            if (!LinkTypeFromString(linkTypeString.c_str(), linkType))
            {
                return AZ::Failure(AZStd::string(GPF_TAG_LINK_TYPE " string is invalid."));
            }

            // If no-code, don't make module definitions
            if (linkType != LinkType::NoCode)
            {
                // Create modules list
                rapidjson::Value modulesList{ rapidjson::kArrayType };

                // Create module definition
                {
                    rapidjson::Value gameModule{ rapidjson::kObjectType };
                    gameModule.AddMember(GPF_TAG_MODULE_TYPE, GPF_TAG_MODULE_TYPE_GAME_MODULE, descNode.GetAllocator());
                    gameModule.AddMember(GPF_TAG_LINK_TYPE, rapidjson::Value(linkTypeString.c_str(), descNode.GetAllocator()), descNode.GetAllocator());

                    modulesList.PushBack(AZStd::move(gameModule), descNode.GetAllocator());
                }

                // Create server module definition
                if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_MODULE_TYPE_SERVER_MODULE, IsBool) && descNode[GPF_TAG_MODULE_TYPE_SERVER_MODULE].GetBool())
                {
                    rapidjson::Value serverModule{ rapidjson::kObjectType };
                    serverModule.AddMember(GPF_TAG_MODULE_TYPE, GPF_TAG_MODULE_TYPE_SERVER_MODULE, descNode.GetAllocator());
                    serverModule.AddMember(GPF_TAG_LINK_TYPE, rapidjson::Value(linkTypeString.c_str(), descNode.GetAllocator()), descNode.GetAllocator());
                    serverModule.AddMember(GPF_TAG_MODULE_NAME, "Server", descNode.GetAllocator());
                    modulesList.PushBack(AZStd::move(serverModule), descNode.GetAllocator());
                }

                // Create editor module definition
                if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_EDITOR_MODULE, IsBool) && descNode[GPF_TAG_EDITOR_MODULE].GetBool())
                {
                    rapidjson::Value editorModule{ rapidjson::kObjectType };
                    editorModule.AddMember(GPF_TAG_MODULE_TYPE, GPF_TAG_MODULE_TYPE_EDITOR_MODULE, descNode.GetAllocator());
                    editorModule.AddMember(GPF_TAG_MODULE_NAME, "Editor", descNode.GetAllocator());
                    editorModule.AddMember(GPF_TAG_MODULE_EXTENDS, "GameModule", descNode.GetAllocator());

                    modulesList.PushBack(AZStd::move(editorModule), descNode.GetAllocator());
                }
                descNode.RemoveMember(GPF_TAG_EDITOR_MODULE);

                // Add modules list to 
                descNode.AddMember(GPF_TAG_MODULES, modulesList, descNode.GetAllocator());
            }
        }

        // file is now up to date
        descNode[GPF_TAG_FORMAT_VERSION] = GEM_DEF_FILE_VERSION;
        return AZ::Success();
    }

    AZ::Outcome<GemDescription, AZStd::string> GemDescription::CreateFromJson(
        rapidjson::Document& descNode,
        const AZStd::string& gemFolderPath,
        const AZStd::string& absoluteFilePath)
    {
        // gem to build
        GemDescription gem;

        gem.m_path = gemFolderPath;
        gem.m_absolutePath = absoluteFilePath;
        AzFramework::StringFunc::Path::StripFullName(gem.m_absolutePath);
        AzFramework::StringFunc::RChop(gem.m_absolutePath, 1);

        if (!descNode.IsObject())
        {
            return AZ::Failure(AZStd::string("Json root element must be an object."));
        }

        // upgrade contents to current version
        auto upgradeOutcome = UpgradeGemDescriptionJson(descNode);
        if (!upgradeOutcome.IsSuccess())
        {
            return AZ::Failure(upgradeOutcome.TakeError());
        }

        // read name
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_NAME, IsString))
        {
            gem.m_name = descNode[GPF_TAG_NAME].GetString();
        }
        else
        {
            return AZ::Failure(AZStd::string(GPF_TAG_NAME " string must not be empty."));
        }

        // read display name
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_DISPLAY_NAME, IsString))
        {
            gem.m_displayName = descNode[GPF_TAG_DISPLAY_NAME].GetString();
        }

        // read id
        if (!RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_UUID, IsString))
        {
            return AZ::Failure(AZStd::string(GPF_TAG_UUID " string is required."));
        }
        gem.m_id = AZ::Uuid::CreateString(descNode[GPF_TAG_UUID].GetString());
        if (gem.m_id.IsNull())
        {
            return AZ::Failure(AZStd::string::format(GPF_TAG_UUID " string \"%s\" is invalid.", descNode[GPF_TAG_UUID].GetString()));
        }

        // read version
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_VERSION, IsString))
        {
            auto versionOutcome = GemVersion::ParseFromString(descNode[GPF_TAG_VERSION].GetString());

            if (versionOutcome)
            {
                gem.m_version = versionOutcome.GetValue();
            }
            else
            {
                return AZ::Failure(AZStd::string(versionOutcome.GetError()));
            }
        }
        else
        {
            return AZ::Failure(AZStd::string(GPF_TAG_VERSION " string is required."));
        }

        // To reduce the potential for user error, a Gem depending on the Lumberyard engine version
        // supports both arrays and strings.
        if (descNode.HasMember(GPF_TAG_LY_VERSION))
        {
            AZStd::vector<AZStd::string> versionConstraints;
            // read version constraints
            // Check if the version constraint is a string first.
            if(RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_LY_VERSION, IsString))
            {
                AZStd::string constraintString = descNode[GPF_TAG_LY_VERSION].GetString();
                versionConstraints.push_back(constraintString);
            }
            // If it wasn't a string, make sure it's an array. If not, error out.
            else if (!RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_LY_VERSION, IsArray))
            {
                return AZ::Failure(AZStd::string(GPF_TAG_LY_VERSION " array is required for engine version."));
            }
            // If it's an empty array, ignore it.
            // For ease of use editing the Gem.json files, we support empty arrays, so users
            // can leave the engine version key in without providing a value.
            else if (descNode[GPF_TAG_LY_VERSION].Size() > 0)
            {
                const auto& constraints = descNode[GPF_TAG_LY_VERSION];
                const auto& end = constraints.End();
                for (auto it = constraints.Begin(); it != end; ++it)
                {
                    const auto& constraint = *it;
                    if (!constraint.IsString())
                    {
                        return AZ::Failure(AZStd::string(GPF_TAG_LY_VERSION " array for engine version must contain strings."));
                    }
                    versionConstraints.push_back(constraint.GetString());
                }
            }
            // If constraints were actually provided, create the dependency.
            if(versionConstraints.size() > 0)
            {
                EngineDependency dep;
                dep.SetID(AZ::Uuid::CreateNull());
                AZ::Outcome<void, AZStd::string> outcome = dep.ParseVersions(versionConstraints);
                if (!outcome)
                {
                    return AZ::Failure(AZStd::string::format(GPF_TAG_LY_VERSION " for engine version is invalid. %s", outcome.GetError().c_str()));
                }
                gem.m_engineDependency = AZStd::make_shared<EngineDependency>(dep);
            }
        }

        // dependencies
        if (descNode.HasMember(GPF_TAG_DEPENDENCIES))
        {
            if (!descNode[GPF_TAG_DEPENDENCIES].IsArray())
            {
                return AZ::Failure(AZStd::string(GPF_TAG_DEPENDENCIES " must be an array."));
            }

            // List of descriptions of Gems we depend upon
            const rapidjson::Value& depsNode = descNode[GPF_TAG_DEPENDENCIES];
            const auto& end = depsNode.End();
            for (auto it = depsNode.Begin(); it != end; ++it)
            {
                const auto& depNode = *it;
                if (!depNode.IsObject())
                {
                    return AZ::Failure(AZStd::string(GPF_TAG_DEPENDENCIES " must contain objects."));
                }

                // read id
                if (!RAPIDJSON_IS_VALID_MEMBER(depNode, GPF_TAG_UUID, IsString))
                {
                    return AZ::Failure(AZStd::string(GPF_TAG_UUID " string is required for dependency."));
                }

                const char* idStr = depNode[GPF_TAG_UUID].GetString();
                AZ::Uuid id(idStr);
                if (id.IsNull())
                {
                    return AZ::Failure(AZStd::string::format(GPF_TAG_UUID " in dependency is invalid: %s.", idStr));
                }

                // read version constraints
                if (!RAPIDJSON_IS_VALID_MEMBER(depNode, GPF_TAG_VERSION_CONSTRAINTS, IsArray))
                {
                    return AZ::Failure(AZStd::string(GPF_TAG_VERSION_CONSTRAINTS " array is required for dependency."));
                }

                // Make sure versions are specified
                if (depNode[GPF_TAG_VERSION_CONSTRAINTS].Size() < 1)
                {
                    return AZ::Failure(AZStd::string(GPF_TAG_VERSION_CONSTRAINTS " must have at least 1 entry for dependency."));
                }

                AZStd::vector<AZStd::string> versionConstraints;
                const auto& constraints = depNode[GPF_TAG_VERSION_CONSTRAINTS];
                const auto& constraintsEnd(constraints.End());
                for (auto constraintIt = constraints.Begin(); constraintIt != constraintsEnd; ++constraintIt)
                {
                    const auto& constraint = *constraintIt;
                    if (!constraint.IsString())
                    {
                        return AZ::Failure(AZStd::string(GPF_TAG_VERSION_CONSTRAINTS " array for dependency must contain strings."));
                    }
                    versionConstraints.push_back(constraint.GetString());
                }

                // create Dependency
                GemDependency dep;
                dep.SetID(id);
                if (!dep.ParseVersions(versionConstraints))
                {
                    return AZ::Failure(AZStd::string(GPF_TAG_VERSION_CONSTRAINTS " for dependency is invalid"));
                }
                gem.m_gemDependencies.push_back(AZStd::make_shared<GemDependency>(dep));
            }
        }

        // Is Game Gem? flag
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_IS_GAME_GEM, IsBool))
        {
            gem.m_gameGem = descNode[GPF_TAG_IS_GAME_GEM].GetBool();
        }
        else
        {
            gem.m_gameGem = false;
        }

        // Is Required? flag
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_IS_REQUIRED, IsBool))
        {
            gem.m_required = descNode[GPF_TAG_IS_REQUIRED].GetBool();
        }
        else
        {
            gem.m_required = false;
        }

        // optional metadata
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_SUMMARY, IsString))
        {
            gem.m_summary = descNode[GPF_TAG_SUMMARY].GetString();
        }

        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_ICON_PATH, IsString))
        {
            gem.m_iconPath = descNode[GPF_TAG_ICON_PATH].GetString();
        }

        if (descNode.HasMember(GPF_TAG_TAGS))
        {
            const rapidjson::Value& tags = descNode[GPF_TAG_TAGS];
            if (tags.IsArray())
            {
                const auto& end = tags.End();
                for (auto it = tags.Begin(); it != end; ++it)
                {
                    const auto& tag = *it;
                    gem.m_tags.push_back(tag.GetString());
                }
            }
            else
            {
                return AZ::Failure(AZStd::string("Value for key " GPF_TAG_TAGS " must be an array."));
            }
        }

        // engine module class
        gem.m_engineModuleClass = RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_MODULE_CLASS, IsString)
            ? descNode[GPF_TAG_MODULE_CLASS].GetString()
            : gem.GetName() + AZStd::string("Gem");

        // Cache constants
        char idStr[UUID_STR_BUF_LEN];
        gem.GetID().ToString(idStr, UUID_STR_BUF_LEN, false, false);
        AZStd::to_lower(idStr, idStr + strlen(idStr));

        // Read the modules list
        if (RAPIDJSON_IS_VALID_MEMBER(descNode, GPF_TAG_MODULES, IsArray))
        {
            bool foundDefaultModule = false;
            AZStd::unordered_map<AZStd::string, AZStd::shared_ptr<ModuleDefinition>> modulesByName;
            AZStd::vector<AZStd::pair<AZStd::shared_ptr<ModuleDefinition>, AZStd::string>> dependencies;

            const rapidjson::Value& modulesNode = descNode[GPF_TAG_MODULES];
            for (auto moduleObjPtr = modulesNode.Begin(); moduleObjPtr != modulesNode.End(); ++moduleObjPtr)
            {
                const rapidjson::Value& moduleObj = *moduleObjPtr;
                if (!moduleObj.IsObject())
                {
                    return AZ::Failure(AZStd::string("Each object in " GPF_TAG_MODULES " must be an object!"));
                }

                auto modulePtr = AZStd::make_shared<ModuleDefinition>();
                gem.m_modules.emplace_back(modulePtr);

                // Get the module type
                if (!RAPIDJSON_IS_VALID_MEMBER(moduleObj, GPF_TAG_MODULE_TYPE, IsString))
                {
                    return AZ::Failure(AZStd::string("Each module requires a " GPF_TAG_MODULE_TYPE " field."));
                }
                const char* moduleTypeStr = moduleObj[GPF_TAG_MODULE_TYPE].GetString();
                if (!ModuleTypeFromString(moduleTypeStr, modulePtr->m_type))
                {
                    return AZ::Failure(AZStd::string::format("Module type %s is invalid!", moduleTypeStr));
                }

                // Get the module name (default to the type)
                if (RAPIDJSON_IS_VALID_MEMBER(moduleObj, GPF_TAG_MODULE_NAME, IsString))
                {
                    modulePtr->m_name = moduleObj[GPF_TAG_MODULE_NAME].GetString();
                }
                else if (modulePtr->m_type == ModuleDefinition::Type::GameModule || modulePtr->m_type == ModuleDefinition::Type::ServerModule)
                {
                    modulePtr->m_name = moduleTypeStr;
                }
                else
                {
                    return AZ::Failure(AZStd::string::format("Default \"" GPF_TAG_MODULE_NAME "\" field is only supported for modules of type \"GameModule\", not %s.", moduleTypeStr));
                }

                // Check for duplicate names
                if (modulesByName.find(modulePtr->m_name) != modulesByName.end())
                {
                    return AZ::Failure(AZStd::string::format("Module name \"%s\" is used more than once!", modulePtr->m_name.c_str()));
                }

                // If the type is GameModule, omit name from file name (maintains functionality of v3)
                if (modulePtr->m_type == ModuleDefinition::Type::GameModule || modulePtr->m_type == ModuleDefinition::Type::ServerModule)
                {
                    if (!foundDefaultModule)
                    {
                        foundDefaultModule = true;
                        // if the module name for 'GameModule' type is specified, such as 'Private' then it needs to be appended into the gem name
                        if (modulePtr->m_name != moduleTypeStr)
                        {
                            modulePtr->m_fileName = AZStd::string::format("Gem.%s.%s.%s.v%s", gem.GetName().c_str(), modulePtr->m_name.c_str(), idStr, gem.GetVersion().ToString().c_str());
                        }
                        else
                        {
                            modulePtr->m_fileName = AZStd::string::format("Gem.%s.%s.v%s", gem.GetName().c_str(), idStr, gem.GetVersion().ToString().c_str());
                        }
                    }

                    // If LinkType is specified, read and validate it.
                    if (RAPIDJSON_IS_VALID_MEMBER(moduleObj, GPF_TAG_LINK_TYPE, IsString))
                    {
                        const char* linkTypeStr = moduleObj[GPF_TAG_LINK_TYPE].GetString();
                        if (!LinkTypeFromString(linkTypeStr, modulePtr->m_linkType))
                        {
                            return AZ::Failure(AZStd::string::format(GPF_TAG_LINK_TYPE " specified (\"%s\") is invalid", linkTypeStr));
                        }
                    }
                }

                // If the module needs a file name, populate it.
                if (modulePtr->m_fileName.empty() && modulePtr->m_type != ModuleDefinition::Type::StaticLib)
                {
                    modulePtr->m_fileName = AZStd::string::format("Gem.%s.%s.%s.v%s", gem.GetName().c_str(), modulePtr->m_name.c_str(), idStr, gem.GetVersion().ToString().c_str());
                }

                modulesByName.emplace(modulePtr->m_name, modulePtr);

                // Populate extensions
                if (modulePtr->m_type != ModuleDefinition::Type::StaticLib &&
                    RAPIDJSON_IS_VALID_MEMBER(moduleObj, GPF_TAG_MODULE_EXTENDS, IsString))
                {
                    dependencies.emplace_back(modulePtr, AZStd::string(moduleObj[GPF_TAG_MODULE_EXTENDS].GetString()));
                }
            }

            // Populate dependencies
            for (const auto& dependencyPair : dependencies)
            {
                auto dependencyIterator = modulesByName.find(dependencyPair.second);
                if (dependencyIterator == modulesByName.end())
                {
                    return AZ::Failure(AZStd::string::format("Module \"%s\" depends on \"" GPF_TAG_MODULE_EXTENDS "\" invalid module \"%s\"", dependencyPair.first->m_name.c_str(), dependencyPair.second.c_str()));
                }
                
                if (dependencyIterator->second->m_type != ModuleDefinition::Type::GameModule && dependencyIterator->second->m_type != ModuleDefinition::Type::ServerModule)
                {
                    return AZ::Failure(AZStd::string::format("Modules may only \"" GPF_TAG_MODULE_EXTENDS "\" modules of type \"" GPF_TAG_MODULE_TYPE_GAME_MODULE "\", " GPF_TAG_MODULE_TYPE_SERVER_MODULE "\"."));
                }

                dependencyPair.first->m_parent = dependencyIterator->second;
                dependencyIterator->second->m_children.emplace_back(dependencyPair.first);
            }

            // Populate modulesByType
            for (const auto& modulePtr : gem.m_modules)
            {
                gem.m_modulesByType[modulePtr->m_type].emplace_back(modulePtr);

                // If this module is a GameModule, and there is no Editor override, apply it to Editor as well.
                if (modulePtr->m_type == ModuleDefinition::Type::GameModule)
                {
                    bool foundEditorModule = false;
                    // Check children for editor modules
                    for (const auto& childWeak : modulePtr->m_children)
                    {
                        auto child = childWeak.lock();
                        AZ_Assert(child, "Child somehow out of scope already!");
                        if (child->m_type == ModuleDefinition::Type::EditorModule)
                        {
                            foundEditorModule = true;
                            break;
                        }
                    }
                    if (!foundEditorModule)
                    {
                        // If no children are for editor, add module to editor list
                        gem.m_modulesByType[ModuleDefinition::Type::EditorModule].emplace_back(modulePtr);
                    }
                }
            }
        }

        return AZ::Success(AZStd::move(gem));
    }
} // namespace Gems
