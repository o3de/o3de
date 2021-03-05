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

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/weak_ptr.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/containers/unordered_map.h>

#include "GemRegistry/Dependency.h"
#include "GemRegistry/Version.h"

namespace Gems
{
    /// Describes how other Gems (and the final executable) will link against this Gem (valid only for Type::GameModule and Type::ServerModule).
    enum class LinkType
    {
        /// Do not link against this Gem, it is loaded as a Dynamic Library at runtime.
        Dynamic,
        /// Link against this Gem, it is also loaded as a Dynamic Library at runtime.
        DynamicStatic,
        /// Gem has no code, there is nothing to link against.
        NoCode,
    };

    /**
     * Defines a module produced by a Gem.
     */
    struct ModuleDefinition
    {
        enum class Type
        {
            GameModule,
            EditorModule,
            StaticLib,
            Builder,
            Standalone,
            ServerModule,
        };

        /// The type of module this represents
        Type m_type;
        /// The name of the module (for dll naming)
        AZStd::string m_name;
        /// If this module is type GameModule, how is it linked?
        LinkType m_linkType = LinkType::NoCode;
        /// The complete name of the file produced (for all Types but StaticLib)
        AZStd::string m_fileName;
        /// If the module extends another module, this points to that
        AZStd::weak_ptr<const ModuleDefinition> m_parent;
        /// All of the modules that extend this module
        AZStd::vector<AZStd::weak_ptr<const ModuleDefinition>> m_children;

        // Construction and Destruction is trivial
        ModuleDefinition() = default;
        ~ModuleDefinition() = default;

        // Disallow copying and moving
        ModuleDefinition(const ModuleDefinition&) = delete;
        ModuleDefinition& operator=(const ModuleDefinition&) = delete;
        ModuleDefinition(ModuleDefinition&&) = delete;
        ModuleDefinition& operator=(ModuleDefinition&& rhs) = delete;
    };
    using ModuleDefinitionConstPtr = AZStd::shared_ptr<const ModuleDefinition>;
    using ModuleDefinitionVector = AZStd::vector<ModuleDefinitionConstPtr>;

    /**
     * Describes an instance of a Gem.
     */
    class IGemDescription
    {
    public:
        /// The ID of the Gem
        virtual const AZ::Uuid& GetID() const = 0;
        /// The name of the Gem
        virtual const AZStd::string& GetName() const = 0;
        /// The UI-friendly name of the Gem
        virtual const AZStd::string& GetDisplayName() const = 0;
        /// The version of the Gem
        virtual const GemVersion& GetVersion() const = 0;
        /// Relative path to the folder of this Gem
        virtual const AZStd::string& GetPath() const = 0;
        /// Absolute path to the folder of this Gem
        virtual const AZStd::string& GetAbsolutePath() const = 0;
        /// Summary description of the Gem
        virtual const AZStd::string& GetSummary() const = 0;
        /// Icon path of the gem
        virtual const AZStd::string& GetIconPath() const = 0;
        /// Tags to associated with the Gem
        virtual const AZStd::vector<AZStd::string>& GetTags() const = 0;
        /// Get the list of modules produced by the Gem
        virtual const ModuleDefinitionVector& GetModules() const = 0;
        /// Get all modules to be loaded for a given function
        /// This method traverses children to find the most derived module of each type per tree
        virtual const ModuleDefinitionVector& GetModulesOfType(ModuleDefinition::Type type) const = 0;
        /// The name of the engine module class to initialize
        virtual const AZStd::string& GetEngineModuleClass() const = 0;
        /// Get the Gem's other gems dependencies
        virtual const AZStd::vector<AZStd::shared_ptr<GemDependency> >& GetGemDependencies() const = 0;
        /// Get the Gem's engine dependency
        virtual const AZStd::shared_ptr<EngineDependency> GetEngineDependency() const = 0;
        /// Determine if this is a Game Gem
        virtual const bool IsGameGem() const = 0;
        /// Determine if this is a required Gem
        virtual const bool IsRequired() const = 0;

        virtual ~IGemDescription() = default;
    };
    using IGemDescriptionConstPtr = AZStd::shared_ptr<const IGemDescription>;

    /// A specific Gem known to a Project.
    /// The Gem is not used unless it is enabled.
    struct ProjectGemSpecifier
        : public GemSpecifier
    {
        /// Folder in which this specific Gem can be found.
        AZStd::string m_path;

        ProjectGemSpecifier(const AZ::Uuid& id, const GemVersion& version, const AZStd::string& path)
            : GemSpecifier(id, version)
            , m_path(path)
        {}
        ~ProjectGemSpecifier() override = default;
    };
    using ProjectGemSpecifierMap = AZStd::unordered_map<AZ::Uuid, ProjectGemSpecifier>;

    /**
     * Stores project-specific settings, such as which Gems are enabled and which versions are required.
     */
    class IProjectSettings
    {
    public:
        /**
         * Initializes the ProjectSettings with a project name to load the settings from.
         *
         * \param[in] appRootFolder     The application root folder where the project sub folder resides
         * \param[in] projectSubFolder  The folder in which the project's assets reside (and the configuration file)
         *
         * \returns                 True on success, false on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> Initialize(const AZStd::string& appRootFolder, const AZStd::string& projectSubFolder) = 0;

        /**
         * Enables the specified instance of a Gem.
         *
         * \param[in] spec      The specific Gem to enable.
         *
         * \returns             True on success, False on failure.
         */
        virtual bool EnableGem(const ProjectGemSpecifier& spec) = 0;

        /**
         * Disables the specified instance of a Gem.
         *
         * \param[in] spec      The specific Gem to disable.
         *
         * \returns             True on success, False on failure.
         */
        virtual bool DisableGem(const GemSpecifier& spec) = 0;

        /**
         * Checks if a Gem of the specified description is enabled.
         *
         * \param[in] spec      The specific Gem to check.
         *
         * \returns             True if the Gem is enabled, False if it is disabled.
         */
        virtual bool IsGemEnabled(const GemSpecifier& spec) const = 0;

        /**
         * Checks if a Gem of the specified ID and version constraints is enabled.
         *
         * \param[in] id                     The ID of the Gem to check.
         * \param[in] versionConstraints     An array of strings, each of which is a condition using the gem dependency syntax
         *
         * \returns             True if the Gem is enabled and passes every version constraint condition, False if it is disabled or does not match the version constraints.
         */
        virtual bool IsGemEnabled(const AZ::Uuid& id, const AZStd::vector<AZStd::string>& versionConstraints) const = 0;

        /**
         * Checks if a Gem of the specified dependency is enabled.
         *
         * \param[in] dep       The dependency to validate.
         *
         * \returns             True if the Gem is enabled, False if it is disabled.
         */
        virtual bool IsGemDependencyMet(const AZStd::shared_ptr<GemDependency> dep) const = 0;

        /**
        * Checks if the engine dependency is met.
        *
        * \param[in] dep       The dependency to validate.
        * \param[in] againstVersion 
        *                      The version of the engine to validate against
        *
        * \returns             True if the Gem is enabled, False if it is disabled.
        */
        virtual bool IsEngineDependencyMet(const AZStd::shared_ptr<EngineDependency> dep, const EngineVersion& againstVersion) const = 0;

        /**
         * Gets the Gems known to this project.
         * Only enabled Gems are actually used at runtime.
         * A project can only reference one version of a Gem.
         *
         * \returns             The vector of enabled Gems.
         */
        virtual const ProjectGemSpecifierMap& GetGems() const = 0;

        /**
        * Sets the Gem map to the passed in list. Used when resetting after a failed save.
        */
        virtual void SetGems(const ProjectGemSpecifierMap& newGemMap) = 0;

        /**
         * Checks that all installed Gems have their dependencies met.
         * Any unmet dependencies can be found via IGemRegistry::GetErrorMessage();
         *
         * \param[in] engineVersion
         *                      The version of the engine to validate against
         *
         * \returns             Void on success, error message on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> ValidateDependencies(const EngineVersion& engineVersion = EngineVersion()) const = 0;

        /**
         * Saves the current state of the project settings to it's project configuration file.
         *
         * \returns             Void on success, error message on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> Save() const = 0;

        /**
         * Get the project name that this project settings represents

         * \returns             The project name.
         */
        virtual const AZStd::string& GetProjectName() const = 0;

        /**
        * Get the app root folder for the project

        * \returns             The project root path.
        */
        virtual const AZStd::string& GetProjectRootPath() const = 0;

        virtual ~IProjectSettings() = default;
    };

    /**
     * Defines how to search for Gems.
     */
    struct SearchPath
    {
        /// The root path to search
        AZStd::string m_path;
        /// The filter to apply to TOP LEVEL searching
        AZStd::string m_filter;

        explicit SearchPath(const AZStd::string& path)
            : m_path(path)
            , m_filter("*")
        { }
        SearchPath(const AZStd::string& path, const AZStd::string& filter)
            : m_path(path)
            , m_filter(filter)
        { }
    };

    inline bool operator==(const SearchPath& left, const SearchPath& right)
    {
        return left.m_path == right.m_path
            && left.m_filter == right.m_filter;
    }

    /**
     * Manages installed Gems.
     */
    class IGemRegistry
    {
    public:
        /**
         * Add to the list of paths to search for Gems when calling LoadAllGemsFromDisk
         *
         * \param[in] searchPath    The path to add
         * \param[in] loadGemsNow   Load Gems from searchPath now
         *
         * \returns                 Void on success, error message on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> AddSearchPath(const SearchPath& searchPath, bool loadGemsNow) = 0;

        /**
         * Scans the Gems/ folder for all installed Gems.
         *
         * In the event of an error loading a Gem, the error will be recorded,
         * the Gem will not be loaded, and false will be returned.
         *
         * Loaded Gems can be accessed via GetGemDescription() or GetAllGemDescriptions().
         *
         * \returns                 AZ::Success if the search succeeded
         *                          AZ::Failure with error message if any errors occurred.
         */
        virtual AZ::Outcome<void, AZStd::string> LoadAllGemsFromDisk() = 0;

        /**
        * Looks for a gems.json file in the given folder and returns a IGemDescriptionConstPtr if found
        *
        * \param[in] gemFolderRelPath   A valid path on disk to a directory that contains a gem.json file relative to the engine root
        *
        * \returns                 IGemDescriptionConstPtr if a gem.json file could be parsed out of the gemFolderPath,
        *                          AZ::Failure with error message if any errors occurred.
        */
        virtual AZ::Outcome<IGemDescriptionConstPtr, AZStd::string> ParseToGemDescriptionPtr(const AZStd::string& gemFolderRelPath, const char* absoluteFilePath) = 0;

        /**
         * Loads Gems for the specified project.
         * May be called for multiple projects.
         *
         * \param[in] settings              The project to load Gems for.
         * \param[in] resetPreviousProjects If true, reset any setting/gem descriptors that any previous calls to LoadProject may have added
         *
         * \returns                 Void on success, error message on failure.
         */
        virtual AZ::Outcome<void, AZStd::string> LoadProject(const IProjectSettings& settings, bool resetPreviousProjects) = 0;

        /**
         * Gets the description for a Gem.
         *
         * \param[in] spec      The specific Gem to search for.
         *
         * \returns             A pointer to the Gem's description if it exists, otherwise nullptr.
         */
        virtual IGemDescriptionConstPtr GetGemDescription(const GemSpecifier& spec) const = 0;

        /**
         * Gets the description for the latest version of a Gem.
         *
         * \param[in] uuid      The specific uuid of the Gem to search for.
         *
         * \returns             A pointer to the Gem's description highest version if it exists, otherwise nullptr.
         */
        virtual IGemDescriptionConstPtr GetLatestGem(const AZ::Uuid& uuid) const = 0;

        /**
         * Gets a list of all loaded Gem descriptions.
         */
        virtual AZStd::vector<IGemDescriptionConstPtr> GetAllGemDescriptions() const = 0;

        /**
        * Gets a list of all loaded required Gem descriptions.
        */
        virtual AZStd::vector<IGemDescriptionConstPtr> GetAllRequiredGemDescriptions() const = 0;


        /**
        * Get the project-specific gem description if any
        */
        virtual IGemDescriptionConstPtr GetProjectGemDescription(const AZStd::string& projectName) const = 0;


        /**
         * Creates a new instance of IProjectSettings.
         *
         * \returns                 A new project settings object.
         */
        virtual IProjectSettings* CreateProjectSettings() = 0;

        /**
         * Destroys an instance of IProjectSettings.
         *
         * \params[in] settings     The settings instance to destroy.
         */
        virtual void DestroyProjectSettings(IProjectSettings* settings) = 0;

        virtual ~IGemRegistry() = default;
    };

    /**
     * The type of function exported for creating a new GemRegistry.
     */
    using RegistryCreatorFunction = IGemRegistry * (*)();
    #define GEMS_REGISTRY_CREATOR_FUNCTION_NAME "CreateGemRegistry"

    /**
     * The type of function exported for destroying a GemRegistry.
     */
    using RegistryDestroyerFunction = void(*)(IGemRegistry*);
    #define GEMS_REGISTRY_DESTROYER_FUNCTION_NAME "DestroyGemRegistry"
} // namespace Gems
