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
#include <AzCore/Component/TransformBus.h>

#include <AtomLyIntegration/CommonFeatures/PostProcess/LookModification/LookModificationBus.h>
#include <AtomLyIntegration/CommonFeatures/PostProcess/LookModification/LookModificationComponentConfig.h>

#include <ACES/Aces.h>
#include <Atom/Feature/PostProcess/LookModification/LookModificationSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessSettingsInterface.h>
#include <Atom/Feature/PostProcess/PostProcessFeatureProcessorInterface.h>
#include <AzCore/Asset/AssetCommon.h>
#include <Atom/RPI.Reflect/System/AnyAsset.h>

namespace AZ
{
    namespace Render
    {
        class LookModificationComponentController final
            : public LookModificationRequestBus::Handler
        {
        public:
            friend class EditorLookModificationComponent;

            AZ_TYPE_INFO(AZ::Render::LookModificationComponentController, "{66912D19-CAB2-457C-A4EF-88FE4AF592B1}");
            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

            LookModificationComponentController() = default;
            LookModificationComponentController(const LookModificationComponentConfig& config);

            void Activate(EntityId entityId);
            void Deactivate();
            void SetConfiguration(const LookModificationComponentConfig& config);
            const LookModificationComponentConfig& GetConfiguration() const;

            // Auto-gen function override declarations (functions definitions in .cpp)...
#include <Atom/Feature/ParamMacros/StartParamFunctionsOverride.inl>
#include <Atom/Feature/PostProcess/LookModification/LookModificationParams.inl>
#include <Atom/Feature/ParamMacros/EndParams.inl>

        private:
            AZ_DISABLE_COPY(LookModificationComponentController);

            void OnConfigChanged();

            PostProcessSettingsInterface* m_postProcessInterface = nullptr;
            LookModificationSettingsInterface* m_settingsInterface = nullptr;
            LookModificationComponentConfig m_configuration;
            EntityId m_entityId;
        };
    }
}
