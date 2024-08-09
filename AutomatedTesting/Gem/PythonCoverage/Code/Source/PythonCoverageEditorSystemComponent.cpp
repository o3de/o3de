/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PythonCoverageEditorSystemComponent.h>

#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>
#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/regex.h>
#include <AzCore/StringFunc/StringFunc.h>

namespace PythonCoverage
{
    static constexpr const char* const LogCallSite = "PythonCoverageEditorSystemComponent";

    void PythonCoverageEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<PythonCoverageEditorSystemComponent, AZ::Component>()->Version(1);
        }
    }

    void PythonCoverageEditorSystemComponent::Activate()
    {
        AzToolsFramework::EditorPythonScriptNotificationsBus::Handler::BusConnect();
        AZ::EntitySystemBus::Handler::BusConnect();

        // If no output directory discovered, coverage gathering will be disabled
        if (ParseCoverageOutputDirectory() == CoverageState::Disabled)
        {
            return;
        }

        EnumerateAllModuleComponents();
    }

    void PythonCoverageEditorSystemComponent::Deactivate()
    {
        AZ::EntitySystemBus::Handler::BusDisconnect();
        AzToolsFramework::EditorPythonScriptNotificationsBus::Handler::BusDisconnect();
    }

    void PythonCoverageEditorSystemComponent::OnEntityActivated(const AZ::EntityId& entityId)
    {
        if (m_coverageState == CoverageState::Disabled)
        {
            return;
        }

        EnumerateComponentsForEntity(entityId);

        // There is currently no way to receive a graceful exit signal in order to properly handle the coverage end of life so
        // instead we have to serialize the data on-the-fly with blocking disk writes on the main thread... if this adversely
        // affects performance in a measurable way then this could potentially be put on a worker thread, although it remains to
        // be seen whether the asynchronous nature of such a thread results in queued up coverage being lost due to the hard exit
        if (m_coverageState == CoverageState::Gathering)
        {
            WriteCoverageFile();
        }
    }

    PythonCoverageEditorSystemComponent::CoverageState PythonCoverageEditorSystemComponent::ParseCoverageOutputDirectory()
    {
        m_coverageState = CoverageState::Disabled;
        const AZStd::string configFilePath = LY_TEST_IMPACT_DEFAULT_CONFIG_FILE;

        if (configFilePath.empty())
        {
            AZ_Warning(LogCallSite, false, "No test impact analysis framework config file specified.");
            return m_coverageState;
        }

        const auto fileSize = AZ::IO::SystemFile::Length(configFilePath.c_str());
        if(!fileSize)
        {
            AZ_Error(LogCallSite, false, "Test impact analysis framework config file '%s' does not exist", configFilePath.c_str());
            return m_coverageState;
        }

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = '\0';
        if (!AZ::IO::SystemFile::Read(configFilePath.c_str(), buffer.data()))
        {
            AZ_Error(LogCallSite, false, "Could not read contents of test impact analysis framework config file '%s'", configFilePath.c_str());
            return m_coverageState;
        }
        
        const AZStd::string configurationData = AZStd::string(buffer.begin(), buffer.end());
        rapidjson::Document configurationFile;
        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            AZ_Error(LogCallSite, false, "Could not parse test impact analysis framework config file data, JSON has errors");
            return m_coverageState;
        }

        m_coverageDir = configurationFile["python"]["workspace"]["temp"]["coverage_artifact_dir"].GetString();

        // Everything is good to go, await the first python test case
        m_coverageState = CoverageState::Idle;
        return m_coverageState;
    }
    
    void PythonCoverageEditorSystemComponent::WriteCoverageFile()
    {
        AZStd::string contents;

        // Compile the coverage for this test case
        const auto coveringModules = GetParentComponentModulesForAllActivatedEntities(m_entityComponents);
        if (coveringModules.empty())
        {
            return;
        }

        contents = AZStd::string::format(
            "%s\n%s\n%s\n%s\n", m_parentScriptPath.c_str(), m_scriptPath.c_str(), m_testFixture.c_str(), m_testCase.c_str());

        for (const auto& coveringModule : coveringModules)
        {
            contents += AZStd::string::format("%s\n", coveringModule.c_str());
        }
    
        AZ::IO::SystemFile file;
        const AZStd::vector<char> bytes(contents.begin(), contents.end());
        if (!file.Open(
                m_coverageFile.c_str(),
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error(LogCallSite, false, "Couldn't open file '%s' for writing", m_coverageFile.c_str());
            return;
        }
    
        if (!file.Write(bytes.data(), bytes.size()))
        {
            AZ_Error(LogCallSite, false, "Couldn't write contents for file '%s'", m_coverageFile.c_str());
            return;
        }
    }
    
    void PythonCoverageEditorSystemComponent::EnumerateAllModuleComponents()
    {
        AZ::ModuleManagerRequestBus::Broadcast(
            &AZ::ModuleManagerRequestBus::Events::EnumerateModules,
            [this](const AZ::ModuleData& moduleData)
            {
                // We can only enumerate shared libs, static libs are invisible to us
                if (moduleData.GetDynamicModuleHandle())
                {
                    for (const auto* moduleComponentDescriptor : moduleData.GetModule()->GetComponentDescriptors())
                    {
                        m_moduleComponents[moduleComponentDescriptor->GetUuid()] = moduleData.GetDebugName();
                    }
                }
    
                return true;
            });
    }
    
    void PythonCoverageEditorSystemComponent::EnumerateComponentsForEntity(const AZ::EntityId& entityId)
    {
        AZ::Entity* entity = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(entity, &AZ::ComponentApplicationBus::Events::FindEntity, AZ::EntityId(entityId));
    
        if (entity)
        {
            for (const auto& entityComponent : entity->GetComponents())
            {
                const auto componentTypeId = entityComponent->GetUnderlyingComponentType();
                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                m_entityComponents[componentTypeId] = componentDescriptor;
            }
        }
    }
    
    AZStd::unordered_set<AZStd::string> PythonCoverageEditorSystemComponent::GetParentComponentModulesForAllActivatedEntities(
        const AZStd::unordered_map<AZ::Uuid, AZ::ComponentDescriptor*>& entityComponents) const
    {
        AZStd::unordered_set<AZStd::string> coveringModuleOutputNames;
        for (const auto& [uuid, componentDescriptor] : entityComponents)
        {
            if (const auto moduleComponent = m_moduleComponents.find(uuid); moduleComponent != m_moduleComponents.end())
            {
                coveringModuleOutputNames.insert(moduleComponent->second);
            }
        }
    
        return coveringModuleOutputNames;
    }

    AZStd::string CompileParentFolderName(const AZStd::string& parentScriptPath)
    {
        // Compile a unique folder name based on the parent script path
        auto parentfolder = parentScriptPath;
        AZ::StringFunc::Replace(parentfolder, '/', '_');
        AZ::StringFunc::Replace(parentfolder, '\\', '_');
        AZ::StringFunc::Replace(parentfolder, '.', '_');
        return parentfolder;
    }
    
    void PythonCoverageEditorSystemComponent::OnStartExecuteByFilenameAsTest(AZStd::string_view filename, AZStd::string_view testCase, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args)
    {
        if (m_coverageState == CoverageState::Disabled)
        {
            return;
        }

        if (m_coverageState == CoverageState::Gathering)
        {
            // Dump any existing coverage data to disk
            WriteCoverageFile();
            m_coverageState = CoverageState::Idle;
        }

        if (testCase.empty())
        {
            // We need to be able to pinpoint the coverage data to the specific test case names otherwise we will not be able
            // to specify which specific tests should be run in the future (filename does not necessarily equate to test case name)
            AZ_Error(LogCallSite, false, "No test case specified, coverage data gathering will be disabled for this test");
            return;
        }

        const auto matcherPattern = AZStd::regex("(.*)::(.*)::(.*)");
        const auto strTestCase = AZStd::string(testCase);
        AZStd::smatch testCaseMatches;
        if (!AZStd::regex_search(strTestCase, testCaseMatches, matcherPattern))
        {
            AZ_Error(
                LogCallSite,
                false,
                "The test case name '%s' did not comply to the format expected by the coverage gem "
                "'parent_script_path::fixture_name::test_case_name', coverage data gathering will be disabled for this test",
                strTestCase.c_str());
            return;
        }

        m_parentScriptPath = testCaseMatches[1];
        m_testFixture = testCaseMatches[2];
        m_testCase = testCaseMatches[3];
        m_entityComponents.clear();
        m_scriptPath = filename;
        const auto coverageFile = m_coverageDir / CompileParentFolderName(m_parentScriptPath) / AZStd::string::format("%s.pycoverage", m_testCase.c_str());

#ifdef MAX_PATH
        if (strlen(coverageFile.c_str()) >= (MAX_PATH - 1))
        {
            AZ_Error(
                LogCallSite,
                false,
                "The generated python coverage file path '%s' is too long for the current file system to write. "
                "Use a shorter folder name or shorten the class name.",
                coverageFile.c_str());
            return;
        }
#endif

        m_coverageFile = coverageFile;
        m_coverageState = CoverageState::Gathering;
    }
} // namespace PythonCoverage

