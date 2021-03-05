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

#include <SkinnedMesh/SkinnedMeshSystemComponent.h>
#include <SkinnedMesh/SkinnedMeshFeatureProcessor.h>
#include <SkinnedMesh/SkinnedMeshComputePass.h>
#include <SkinnedMesh/SkinnedMeshVertexStreamProperties.h>
#include <SkinnedMesh/SkinnedMeshOutputStreamManager.h>
#include <MorphTargets/MorphTargetComputePass.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <AzCore/Serialization/SerializeContext.h>


namespace AZ
{
    namespace Render
    {
        SkinnedMeshSystemComponent::SkinnedMeshSystemComponent() = default;
        SkinnedMeshSystemComponent::~SkinnedMeshSystemComponent() = default;

        void SkinnedMeshSystemComponent::Reflect(ReflectContext* context)
        {
            if (SerializeContext* serialize = azrtti_cast<SerializeContext*>(context))
            {
                serialize->Class<SkinnedMeshSystemComponent, Component>()
                    ->Version(0)
                    ;
            }
            
            SkinnedMeshFeatureProcessor::Reflect(context);
        }

        void SkinnedMeshSystemComponent::GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
        }

        void SkinnedMeshSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("SkinnedMeshService", 0xac7cea96));
        }

        void SkinnedMeshSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("RPISystem", 0xf2add773));
        }

        void SkinnedMeshSystemComponent::Init()
        {
        }

        void SkinnedMeshSystemComponent::Activate()
        {
            m_vertexStreamProperties = AZStd::make_unique<SkinnedMeshVertexStreamProperties>();
            m_outputStreamManager = AZStd::make_unique<SkinnedMeshOutputStreamManager>();

            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<SkinnedMeshFeatureProcessor>();

            auto* passSystem = RPI::PassSystemInterface::Get();
            AZ_Assert(passSystem, "Cannot get the pass system.");
            passSystem->AddPassCreator(Name("MorphTargetComputePass"), &MorphTargetComputePass::Create);
            passSystem->AddPassCreator(Name("SkinnedMeshComputePass"), &SkinnedMeshComputePass::Create);
        }

        void SkinnedMeshSystemComponent::Deactivate()
        {
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<SkinnedMeshFeatureProcessor>();
            m_outputStreamManager.reset();
            m_vertexStreamProperties.reset();
        }
    } // End Render namespace
} // End AZ namespace

