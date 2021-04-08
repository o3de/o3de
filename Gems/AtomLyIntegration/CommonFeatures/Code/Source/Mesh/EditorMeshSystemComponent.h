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
#include <AzFramework/Application/Application.h>

namespace AZ
{
    namespace Render
    {
        //! System component that sets up necessary logic related to EditorMeshComponent.
        class EditorMeshSystemComponent
            : public AZ::Component
            , private AzFramework::ApplicationLifecycleEvents::Bus::Handler
        {
        public:
            AZ_COMPONENT(EditorMeshSystemComponent, "{4D332E3D-C4FC-410B-A915-8E234CBDD4EC}");

            static void Reflect(AZ::ReflectContext* context);

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        protected:
            // AZ::Component interface overrides...
            void Activate() override;
            void Deactivate() override;

        private:
            // AzFramework::ApplicationLifecycleEvents overrides...
            void OnApplicationAboutToStop() override;

            void SetupThumbnails();
            void TeardownThumbnails();
        };
    } // namespace Render
} // namespace AZ
