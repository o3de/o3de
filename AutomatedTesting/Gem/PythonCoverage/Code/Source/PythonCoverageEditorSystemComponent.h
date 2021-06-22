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

#include <AzCore/Component/Component.h>
#include <AzCore/Component/EntityBus.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>
#include <AzToolsFramework/API/EditorPythonScriptNotificationsBus.h>

namespace AZ
{
    class ComponentDescriptor;
}

namespace PythonCoverage
{
    //! System component for PythonCoverage editor.
    class PythonCoverageEditorSystemComponent
        : public AZ::Component
        , private AZ::EntitySystemBus::Handler
        , private AzToolsFramework::EditorPythonScriptNotificationsBus::Handler
    {
    public:
        AZ_COMPONENT(PythonCoverageEditorSystemComponent, "{33370075-3aea-49c4-823d-476f8ac95b6f}");
        static void Reflect(AZ::ReflectContext* context);

        PythonCoverageEditorSystemComponent() = default;

    private:
        // AZ::Component overrides...
        void Activate() override;
        void Deactivate() override;

        // AZ::EntitySystemBus overrides...
        void OnEntityActivated(const AZ::EntityId& entityId) override;

        // AZ::EditorPythonScriptNotificationsBus ...
        void OnExecuteByFilenameAsTest(AZStd::string_view filename, AZStd::string_view testCase, const AZStd::vector<AZStd::string_view>& args) override;

        //! Attempts to parse the test impact analysis framework configuration file.
        //! If either the test impact analysis framework is disabled or the configuration file cannot be parsed, python coverage
        //! is disabled.
        void ParseCoverageOutputDirectory();

        //! Enumerates all of the loaded shared library modules and the component descriptors that belong to them.
        void EnumerateAllModuleComponents();

        //! Enumerates all of the component descriptors for the specified entity.
        void EnumerateComponentsForEntity(const AZ::EntityId& entityId);

        //! Returns all of the shared library modules that parent the component descriptors of the specified set of activated entities.
        //! @note Entity component descriptors are still retrieved even if the entity in question has since been deactivated.
        //! @param entityComponents The set of activated entities and their component descriptors to get the parent modules for.
        AZStd::unordered_set<AZStd::string> GetParentComponentModulesForAllActivatedEntities(
            const AZStd::unordered_map<AZ::Uuid, AZ::ComponentDescriptor*>& entityComponents) const;

        //! Writes the current coverage data snapshot to disk.
        void WriteCoverageFile();

        enum class CoverageState : AZ::u8
        {
            Disabled, //!< Python coverage is disabled.
            Idle, //!< Python coverage is enabled but not actively gathering coverage data.
            Gathering //!< Python coverage is enabled and actively gathering coverage data.
        };

        CoverageState m_coverageState = CoverageState::Disabled; //!< Current coverage state.
        AZStd::unordered_map<AZStd::string, AZStd::unordered_map<AZ::Uuid, AZ::ComponentDescriptor*>> m_entityComponentMap; //!< Map of
        //!< component IDs to component descriptors for all activated entities, organized by test cases.
        AZStd::unordered_map<AZ::Uuid, AZStd::string> m_moduleComponents; //!< Map of component IDs to module names for all modules.
        AZ::IO::Path m_coverageDir; //!< Directory to write coverage data to.
        AZ::IO::Path m_coverageFile; //!< Full file path to write coverage data to.
        AZStd::string m_testCase; //!< Name of current test case that coverage data is being gathered for.
    };
} // namespace PythonCoverage
