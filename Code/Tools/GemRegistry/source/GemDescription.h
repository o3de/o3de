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

#include <AzCore/std/functional.h>
#include <AzCore/JSON/document.h>

namespace Gems
{
    class GemDescription
        : public IGemDescription
    {
    public:
        GemDescription(const GemDescription& rhs);
        GemDescription(GemDescription&& rhs);
        ~GemDescription() override = default;

        // IGemDescription
        const AZ::Uuid& GetID() const override { return m_id; }
        const AZStd::string& GetName() const override { return m_name; }
        const AZStd::string& GetDisplayName() const override { return m_displayName.empty() ? m_name : m_displayName; }
        const GemVersion& GetVersion() const override { return m_version; }
        const AZStd::string& GetPath() const override { return m_path; }
        const AZStd::string& GetAbsolutePath() const override { return m_absolutePath; }
        const AZStd::string& GetSummary() const override { return m_summary; }
        const AZStd::string& GetIconPath() const override { return m_iconPath; }
        const AZStd::vector<AZStd::string>& GetTags() const override { return m_tags; }
        const ModuleDefinitionVector& GetModules() const override { return m_modules; }
        const ModuleDefinitionVector& GetModulesOfType(ModuleDefinition::Type type) const override { return m_modulesByType.find(type)->second; }
        const AZStd::string& GetEngineModuleClass() const override { return m_engineModuleClass; }
        const AZStd::vector<AZStd::shared_ptr<GemDependency> >& GetGemDependencies() const override { return m_gemDependencies; }
        const bool IsGameGem() const override { return m_gameGem;  }
        const bool IsRequired() const override { return m_required; }
        const AZStd::shared_ptr<EngineDependency> GetEngineDependency() const override { return m_engineDependency; }

        // ~IGemDescription

        // Internal methods

        /// Create GemDescription from Json.
        ///
        /// \param[in]  json    Json object to parse. json may be modified during parse.
        /// \param[in]  gemFolderPath Relative path from engine root to Gem folder.
        ///
        /// \returns    If successful, the GemDescription.
        ///             If unsuccessful, an explanation why parsing failed.
        static AZ::Outcome<GemDescription, AZStd::string> CreateFromJson(
            rapidjson::Document& json,
            const AZStd::string& gemFolderPath,
            const AZStd::string& absoluteFilePath);

    private:
        // Outsiders may not create an empty GemDescription
        GemDescription();

        /// The ID of the Gem
        AZ::Uuid m_id;
        /// The name of the Gem
        AZStd::string m_name;
        /// The UI-friendly name of the Gem
        AZStd::string m_displayName;
        /// The version of the Gem
        GemVersion m_version;
        /// Relative path to Gem folder
        AZStd::string m_path;
        /// Absolute path to Gem folder
        AZStd::string m_absolutePath;
        /// Summary description of the Gem
        AZStd::string m_summary;
        /// Icon path of the gem
        AZStd::string m_iconPath;
        /// Tags to associate with the Gem
        AZStd::vector<AZStd::string> m_tags;
        /// List of modules produced by the Gem
        ModuleDefinitionVector m_modules;
        /// All modules to be loaded for a given function
        AZStd::unordered_map<ModuleDefinition::Type, ModuleDefinitionVector> m_modulesByType;
        /// The name of the engine module class to initialize
        AZStd::string m_engineModuleClass;
        /// A Gem's dependencies
        AZStd::vector<AZStd::shared_ptr<GemDependency> > m_gemDependencies;
        /// Flag to indicate if this is a Game GEM
        bool m_gameGem;
        /// Flag to indicate that this is a required GEM
        bool m_required;
        /// A Gem's engine dependency
        AZStd::shared_ptr<EngineDependency> m_engineDependency = nullptr;
    };
} // namespace Gems
