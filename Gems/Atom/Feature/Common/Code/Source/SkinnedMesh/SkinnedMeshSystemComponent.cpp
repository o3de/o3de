/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
            provided.push_back(AZ_CRC_CE("SkinnedMeshService"));
        }

        void SkinnedMeshSystemComponent::GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC_CE("SkinnedMeshService"));
        }

        void SkinnedMeshSystemComponent::GetRequiredServices(ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC_CE("RPISystem"));
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

