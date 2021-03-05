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

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/JSON/document.h>

#include "GemRegistry.h"

namespace Gems
{
    class ProjectSettings
        : public IProjectSettings
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        ProjectSettings(GemRegistry* registry);
        ~ProjectSettings() override = default;

        // IProjectSettings
        AZ::Outcome<void, AZStd::string> Initialize(const AZStd::string& appRootFolder, const AZStd::string& projectSubFolder) override;

        bool EnableGem(const ProjectGemSpecifier& spec) override;
        bool DisableGem(const GemSpecifier& spec) override;
        bool IsGemEnabled(const GemSpecifier& spec) const override;
        bool IsGemEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const override;
        bool IsGemDependencyMet(const AZStd::shared_ptr<GemDependency> dep) const override;
        bool IsEngineDependencyMet(const AZStd::shared_ptr<EngineDependency> dep, const EngineVersion& againstVersion) const override;

        const ProjectGemSpecifierMap& GetGems() const override { return m_gems; }
        void SetGems(const ProjectGemSpecifierMap& newGemMap) override { m_gems = newGemMap; }
        AZ::Outcome<void, AZStd::string> ValidateDependencies(const EngineVersion& engineVersion) const override;

        AZ::Outcome<void, AZStd::string> Save() const override;
        const AZStd::string& GetProjectName() const override;
        const AZStd::string& GetProjectRootPath() const override;

        // ~IProjectSettings

        // Internal methods
        /// Loads settings from the path provided by m_settingsFilePath
        AZ::Outcome<void, AZStd::string> LoadSettings();
        /// Converts the ProjectGemSpecifierMap (m_gems) into it's Json representation for saving
        rapidjson::Document GetJsonRepresentation() const;
        /// Converts GEMS Json into the ProjectGemSpecifierMap (m_gems)
        AZ::Outcome<void, AZStd::string> ParseGemsJson(const rapidjson::Document& json);
        /// Reads from project.json to initialize project-specific values
        AZ::Outcome<void, AZStd::string> ParseProjectJson(const rapidjson::Document& json);

    private:
        ProjectGemSpecifierMap m_gems;
        GemRegistry* m_registry;
        AZStd::string m_gemsSettingsFilePath;
        AZStd::string m_projectSettingsFilePath;
        AZStd::string m_projectName;
        AZStd::string m_projectRootPath;
        bool m_initialized;
    };
} // namespace Gems
