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
#pragma once

#include "GemRegistry/IGemRegistry.h"
#include "GemDescription.h"

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>

// Constants
#define UUID_STR_BUF_LEN 64
#define GEMS_ASSETS_FOLDER "Assets"
#define GEM_DEF_FILE "gem.json"
#define GEM_DEF_FILE_VERSION 4
#define GEMS_PROJECT_FILE "gems.json"
#define GEMS_PROJECT_FILE_VERSION 2
#define PROJECT_CONFIG_FILE "project.json"

// Gem project file JSON tags
#define GPF_TAG_FORMAT_VERSION              "GemFormatVersion"
#define GPF_TAG_LIST_FORMAT_VERSION         "GemListFormatVersion"
#define GPF_TAG_NAME                        "Name"
#define GPF_TAG_DISPLAY_NAME                "DisplayName"
#define GPF_TAG_GEM_ARRAY                   "Gems"
#define GPF_TAG_UUID                        "Uuid"
#define GPF_TAG_LY_VERSION                  "LumberyardVersion"
#define GPF_TAG_VERSION                     "Version"
#define GPF_TAG_DEPENDENCIES                "Dependencies"
#define GPF_TAG_VERSION_CONSTRAINTS         "VersionConstraints"
#define GPF_TAG_PATH                        "Path"
#define GPF_TAG_MODULE_CLASS                "EngineModuleClass"
#define GPF_TAG_EDITOR_MODULE               "EditorModule"
#define GPF_TAG_SUMMARY                     "Summary"
#define GPF_TAG_ICON_PATH                   "IconPath"
#define GPF_TAG_TAGS                        "Tags"
#define GPF_TAG_LINK_TYPE                   "LinkType"
#define GPF_TAG_LINK_TYPE_DYNAMIC           "Dynamic"
#define GPF_TAG_LINK_TYPE_DYNAMIC_STATIC    "DynamicStatic"
#define GPF_TAG_LINK_TYPE_NO_CODE           "NoCode"
#define GPF_TAG_MODULES                     "Modules"
#define GPF_TAG_MODULE_NAME                 "Name"
#define GPF_TAG_MODULE_TYPE                 "Type"
#define GPF_TAG_MODULE_TYPE_GAME_MODULE     "GameModule"
#define GPF_TAG_MODULE_TYPE_SERVER_MODULE   "ServerModule"
#define GPF_TAG_MODULE_TYPE_EDITOR_MODULE   "EditorModule"
#define GPF_TAG_MODULE_TYPE_STATIC_LIB      "StaticLib"
#define GPF_TAG_MODULE_TYPE_BUILDER         "Builder"
#define GPF_TAG_MODULE_TYPE_STANDALONE      "Standalone"
#define GPF_TAG_MODULE_EXTENDS              "Extends"
#define GPF_TAG_IS_GAME_GEM                 "IsGameGem"
#define GPF_TAG_IS_REQUIRED                 "IsRequired"
#define GPF_TAG_COMMENT                     "_comment"

namespace Gems
{
    class GemRegistry
        : public IGemRegistry
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL;

        //////////////////////////////////////////////////////////////////////////
        // IGemRegistry
        AZ::Outcome<void, AZStd::string> AddSearchPath(const SearchPath& searchPath, bool loadGemsNow) override;
        AZ::Outcome<void, AZStd::string> LoadAllGemsFromDisk() override;
        AZ::Outcome<void, AZStd::string> LoadProject(const IProjectSettings& settings, bool resetPreviousProjects) override;

        AZ::Outcome<IGemDescriptionConstPtr, AZStd::string> ParseToGemDescriptionPtr(const AZStd::string& gemFolderRelPath, const char* absoluteFilePath) override;
        IGemDescriptionConstPtr GetGemDescription(const GemSpecifier& spec) const override;
        IGemDescriptionConstPtr GetLatestGem(const AZ::Uuid& uuid) const override;
        AZStd::vector<IGemDescriptionConstPtr> GetAllGemDescriptions() const override;
        AZStd::vector<IGemDescriptionConstPtr> GetAllRequiredGemDescriptions() const override;
        IGemDescriptionConstPtr GetProjectGemDescription(const AZStd::string& projectName) const override;

        IProjectSettings* CreateProjectSettings() override;
        void DestroyProjectSettings(IProjectSettings* settings) override;

        ~GemRegistry() override = default;
        //////////////////////////////////////////////////////////////////////////

    private:
        using GemDescriptionPtr = AZStd::shared_ptr<GemDescription>;

        AZ::Outcome<void, AZStd::string> LoadGemsFromDir(const SearchPath& searchPath);
        // Pass nullptr for absoluteFolderPath to do a search for the Gem
        AZ::Outcome<IGemDescriptionConstPtr, AZStd::string> LoadGemDescription(const AZStd::string& gemFolderPath, const char* absoluteFilePath);
        AZ::Outcome<GemDescription, AZStd::string> ParseToGemDescription(const AZStd::string& gemFolderPath, const char* absoluteFilePath) const;

        AZStd::vector<SearchPath> m_searchPaths; // Explictly ordered so that AddSearchPath() order matters
        AZStd::unordered_map<AZ::Uuid, AZStd::unordered_map<GemVersion, GemDescriptionPtr> > m_gemDescs;
    };
} // namespace Gems

extern "C" AZ_DLL_EXPORT Gems::IGemRegistry * CreateGemRegistry();
extern "C" AZ_DLL_EXPORT void DestroyGemRegistry(Gems::IGemRegistry* reg);
