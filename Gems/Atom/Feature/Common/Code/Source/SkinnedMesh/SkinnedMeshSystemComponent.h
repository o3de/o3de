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

namespace AZ
{
    namespace Render
    {
        class SkinnedMeshVertexStreamProperties;
        class SkinnedMeshOutputStreamManager;

        class SkinnedMeshSystemComponent
            : public Component
        {
        public:
            AZ_COMPONENT(SkinnedMeshSystemComponent, "{5B36DCDC-9120-4C12-8594-8D0F8E9A7197}");

            SkinnedMeshSystemComponent();
            ~SkinnedMeshSystemComponent();

            static void Reflect(ReflectContext* context);

            static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
            static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
            static void GetRequiredServices(ComponentDescriptor::DependencyArrayType& required);

        protected:
        
            ////////////////////////////////////////////////////////////////////////
            // Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            AZStd::unique_ptr<SkinnedMeshVertexStreamProperties> m_vertexStreamProperties;
            AZStd::unique_ptr<SkinnedMeshOutputStreamManager> m_outputStreamManager;
        };
    } // End Render namespace
} // End AZ namespace
