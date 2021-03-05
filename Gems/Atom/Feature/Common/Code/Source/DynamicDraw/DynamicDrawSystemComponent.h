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

#include <Atom/RPI.Public/DynamicDraw/DynamicDrawSystemInterface.h>
#include <Atom/RPI.Public/Scene.h>

namespace AZ
{
    namespace Render
    {
        class DynamicDrawSystemComponent final
            : public AZ::Component
            , public RPI::DynamicDrawSystemInterface
        {
        public:
            AZ_COMPONENT(DynamicDrawSystemComponent, "{7FBBD80C-7711-4CF6-A507-E3CE3540873F}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

            // AZ::Component overrides...
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            
            // RPI::DynamicDrawSystemInterface overrides...
            RPI::DynamicDrawInterface* GetDynamicDrawInterface(RPI::Scene* scene) override;
            void RegisterDynamicDrawForScene(RPI::DynamicDrawInterface* dd, RPI::Scene* scene) override;
            void UnregisterDynamicDrawForScene(RPI::Scene* scene) override;

        private:
            AZStd::unordered_map<RPI::Scene*, RPI::DynamicDrawInterface*> m_sceneToDrawMap;
        };
    } // namespace RPI
} // namespace AZ
