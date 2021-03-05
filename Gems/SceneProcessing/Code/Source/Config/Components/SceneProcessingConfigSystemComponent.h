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
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <Config/SceneProcessingConfigBus.h>
#include <Config/SettingsObjects/SoftNameSetting.h>
#include <SceneAPI/SceneCore/Events/AssetImportRequest.h>

namespace AZ
{
    class DynamicModuleHandle;

    namespace SceneProcessingConfig
    {
        class SceneProcessingConfigSystemComponentSerializationEvents
            : public SerializeContext::IEventHandler
        {
        public:
            AZ_CLASS_ALLOCATOR(SceneProcessingConfigSystemComponentSerializationEvents, SystemAllocator, 0);

            void OnWriteBegin(void* classPtr) override;
        };

        class SceneProcessingConfigSystemComponent
            : public AZ::SceneAPI::SceneCore::SceneSystemComponent
            , protected SceneProcessingConfigRequestBus::Handler
            , public AZ::SceneAPI::Events::AssetImportRequestBus::Handler
        {
        public:
            AZ_COMPONENT(SceneProcessingConfigSystemComponent, "{80FE1130-91B4-44D4-869F-859BB996161A}", AZ::SceneAPI::SceneCore::SceneSystemComponent);

            SceneProcessingConfigSystemComponent();
            ~SceneProcessingConfigSystemComponent();

            void Activate() override;
            void Deactivate() override;

            void Clear();

            // SceneProcessingConfigRequestBus START
            const AZStd::vector<SoftNameSetting*>* GetSoftNames() override;
            bool AddNodeSoftName(const char* pattern,
                SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool includeChildren) override;
            bool AddFileSoftName(const char* pattern, SceneAPI::SceneCore::PatternMatcher::MatchApproach approach,
                const char* virtualType, bool inclusive, const AZStd::string& graphObjectTypeName) override;
            // SceneProcessingConfigRequestBus END

            void AreCustomNormalsUsed(bool &value) override;

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(ComponentDescriptor::DependencyArrayType& dependent);

        private:
            /// It is the responsibility of the caller to delete newSoftname if this method returns
            /// false.
            bool AddSoftName(SoftNameSetting* newSoftname);

            static void ReflectSceneModule(ReflectContext* context, const AZStd::unique_ptr<DynamicModuleHandle>& module);
            static void ActivateSceneModule(const AZStd::unique_ptr<DynamicModuleHandle>& module);
            static void DeactivateSceneModule(const AZStd::unique_ptr<DynamicModuleHandle>& module);

            AZStd::vector<SoftNameSetting*> m_softNames;
            bool m_UseCustomNormals;
        };
    } // namespace SceneProcessingConfig
} // namespace AZ
