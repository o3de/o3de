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

#include <AzCore/Module/ModuleManagerBus.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Module/DynamicModuleHandle.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/JSON/document.h>

#include <PythonCoverageEditorSystemComponent.h>

#pragma optimize("", off)

namespace PythonCoverage
{
    constexpr char* const Caller = "PythonCoverageEditorSystemComponent";

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

        // Attempt to discover the output directory for the test coverage files
        ParseCoverageOutputDirectory();

        if (m_coverageState == CoverageState::Disabled)
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

    void PythonCoverageEditorSystemComponent::ParseCoverageOutputDirectory()
    {
        m_coverageState = CoverageState::Disabled;
        const AZStd::string configFilePath = LY_TEST_IMPACT_DEFAULT_CONFIG_FILE;

        if (configFilePath.empty())
        {
            // Config file path will be empty if test impact analysis framework is disabled
            AZ_Warning(Caller, false, "No test impact analysis framework config found.");
            return;
        }

        const auto fileSize = AZ::IO::SystemFile::Length(configFilePath.c_str());
        if(!fileSize)
        {
            AZ_Error(Caller, false, "File %s does not exist", configFilePath.c_str());
            return;
        }

        AZStd::vector<char> buffer(fileSize + 1);
        buffer[fileSize] = '\0';
        if (!AZ::IO::SystemFile::Read(configFilePath.c_str(), buffer.data()))
        {
            AZ_Error(Caller, false, "Could not read contents of file %s", configFilePath.c_str());
            return;
        }
        
        const AZStd::string configurationData = AZStd::string(buffer.begin(), buffer.end());
        rapidjson::Document configurationFile;
        if (configurationFile.Parse(configurationData.c_str()).HasParseError())
        {
            AZ_Error(Caller, false, "Could not parse runtimeConfig data, JSON has errors");
            return;
        }

        const auto& tempConfig = configurationFile["workspace"]["temp"];
        const AZ::IO::Path tempWorkspaceRootDir = tempConfig["root"].GetString();
        const AZ::IO::Path artifactRelativeDir = tempConfig["relative_paths"]["artifact_dir"].GetString();
        m_coverageDir = tempWorkspaceRootDir / artifactRelativeDir;
        m_coverageState = CoverageState::Idle;
    }
    
    void PythonCoverageEditorSystemComponent::WriteCoverageFile()
    {
        AZStd::string contents;
        for (const auto& [testCase, entityComponents] : m_entityComponentMap)
        {
            const auto coveringModules = GetParentComponentModulesForAllActivatedEntities(entityComponents);
            if (coveringModules.empty())
            {
                return;
            }

            contents = testCase + "\n";
            for (const auto& coveringModule : coveringModules)
            {
                contents += AZStd::string::format(" %s\n", coveringModule.c_str());
            }
        }
    
        AZ::IO::SystemFile file;
        const AZStd::vector<char> bytes(contents.begin(), contents.end());
        if (!file.Open(
                m_coverageFile.c_str(),
                AZ::IO::SystemFile::SF_OPEN_CREATE | AZ::IO::SystemFile::SF_OPEN_CREATE_PATH | AZ::IO::SystemFile::SF_OPEN_WRITE_ONLY))
        {
            AZ_Error(
                Caller, false,
                "Couldn't open file %s for writing", m_coverageFile.c_str());
            return;
        }
    
        if (!file.Write(bytes.data(), bytes.size()))
        {
            AZ_Error(
                Caller, false,
                "Couldn't write contents for file %s", m_coverageFile.c_str());
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
            auto& entityComponents = m_entityComponentMap[m_testCase];
            for (const auto& entityComponent : entity->GetComponents())
            {
                const auto componentTypeId = entityComponent->GetUnderlyingComponentType();
                AZ::ComponentDescriptor* componentDescriptor = nullptr;
                AZ::ComponentDescriptorBus::EventResult(
                    componentDescriptor, componentTypeId, &AZ::ComponentDescriptorBus::Events::GetDescriptor);
                entityComponents[componentTypeId] = componentDescriptor;
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
    
    void PythonCoverageEditorSystemComponent::OnExecuteByFilenameAsTest(AZStd::string_view filename, AZStd::string_view testCase, [[maybe_unused]] const AZStd::vector<AZStd::string_view>& args)
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
            AZ_Error(Caller, false, "No test case specified, coverage data gathering will be disabled for this test");
            return;
        }

        const AZStd::string scriptName = AZ::IO::Path(filename).Stem().Native();
        const auto coverageFile = m_coverageDir / AZStd::string::format("%s.pycoverage", scriptName.c_str());

        // If this is a different python script we clear the existing entity components and start afresh
        if (m_coverageFile != coverageFile)
        {
            m_entityComponentMap.clear();
            m_coverageFile = coverageFile;
        }

        m_testCase = testCase;
        m_coverageState = CoverageState::Gathering;
    }
} // namespace PythonCoverage
