/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <Config/SceneProcessingConfigBus.h>
#include <Config/SettingsObjects/SoftNameSetting.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>
#include <SceneAPI/SceneCore/Events/ScriptConfigEventBus.h>

namespace AZ
{
    class DynamicModuleHandle;

    namespace SceneProcessingConfig
    {
        inline constexpr const char* AssetProcessorDefaultScriptsKey{ "/O3DE/AssetProcessor/SceneBuilder/defaultScripts" };

        class SceneProcessingConfigSystemComponentSerializationEvents
            : public SerializeContext::IEventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(SceneProcessingConfigSystemComponentSerializationEvents, SystemAllocator);

            void OnWriteBegin(void* classPtr) override;
        };

        class SceneProcessingConfigSystemComponent
            : public AZ::SceneAPI::SceneCore::SceneSystemComponent
            , protected SceneProcessingConfigRequestBus::Handler
            , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
            , public AZ::SceneAPI::Events::ScriptConfigEventBus::Handler
        {
        public:
            AZ_COMPONENT(SceneProcessingConfigSystemComponent, "{80FE1130-91B4-44D4-869F-859BB996161A}", AZ::SceneAPI::SceneCore::SceneSystemComponent);

            static constexpr const char AssetProcessorDefaultNodeSoftNameSettingsKey[] = "/O3DE/AssetProcessor/SceneBuilder/NodeSoftNameSettings";
            static constexpr const char AssetProcessorDefaultFileSoftNameSettingsKey[] = "/O3DE/AssetProcessor/SceneBuilder/FileSoftNameSettings";

            SceneProcessingConfigSystemComponent();
            ~SceneProcessingConfigSystemComponent();

            void Activate() override;
            void Deactivate() override;

            void Clear();

            // SceneProcessingConfigRequestBus START
            const AZStd::vector<AZStd::unique_ptr<SoftNameSetting>>* GetSoftNames() override;
            bool AddNodeSoftName(const char* pattern,
                SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool includeChildren) override;
            bool AddFileSoftName(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool inclusive, const AZStd::string& graphObjectTypeName) override;
            // SceneProcessingConfigRequestBus END

            // AssetImportRequestBus START
            void AreCustomNormalsUsed(bool &value) override;
            void GetPolicyName(AZStd::string& result) const override
            {
                result = "SceneProcessingConfigSystemComponent";
            }
            // AssetImportRequestBus END

            // AZ::SceneAPI::Events::ScriptConfigEventBus START
            void GetScriptConfigList(AZStd::vector<SceneAPI::Events::ScriptConfig>& scriptConfigList) const override;
            AZStd::optional<SceneAPI::Events::ScriptConfig> MatchesScriptConfig(const AZStd::string& sourceFile) const override;
            // AZ::SceneAPI::Events::ScriptConfigEventBus END

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);

        private:
            //unique_ptr cannot be copied -> vector of unique_ptrs cannot be copied -> class cannot be copied
            SceneProcessingConfigSystemComponent(const SceneProcessingConfigSystemComponent&) = delete;

            bool AddSoftName(AZStd::unique_ptr<SoftNameSetting> newSoftname);

            void PopulateSoftNameSettings();

            template<class SoftNameSettingsType>
            bool AddSoftNameSettingsFromSettingsRegistry(
                AZ::SettingsRegistryInterface* settingsRegistry, AZStd::string_view settingRegistryKey);

            void LoadScriptSettings();

            static void ReflectSceneModule(ReflectContext* context, const AZStd::unique_ptr<DynamicModuleHandle>& module);
            static void ActivateSceneModule(const AZStd::unique_ptr<DynamicModuleHandle>& module);
            static void DeactivateSceneModule(const AZStd::unique_ptr<DynamicModuleHandle>& module);

            AZStd::vector<AZ::SceneAPI::Events::ScriptConfig> m_scriptConfigList;
            AZStd::vector<AZStd::unique_ptr<SoftNameSetting>> m_softNames;
            bool m_UseCustomNormals;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
