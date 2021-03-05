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

#include <DynamicDraw/DynamicDrawSystemComponent.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/Feature/DynamicDraw/DynamicDrawFeatureProcessor.h>
#include <Atom/RPI.Public/FeatureProcessorFactory.h>
#include <Atom/RPI.Public/RPISystemInterface.h>

namespace AZ
{
    namespace Render
    {
        void DynamicDrawSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<DynamicDrawSystemComponent, AZ::Component>()
                    ->Version(0)
                    ;
            }
            DynamicDrawFeatureProcessor::Reflect(context);
        }

        void DynamicDrawSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("DynamicDrawService", 0x023c1673)); 
        }

        void DynamicDrawSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
            incompatible.push_back(AZ_CRC("DynamicDrawService", 0x023c1673));
        }

        void DynamicDrawSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
        {
            required.push_back(AZ_CRC("RPISystem", 0xf2add773));
        }

        void DynamicDrawSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
        {
            AZ_UNUSED(dependent);
        }

        void DynamicDrawSystemComponent::Init()
        {
        }

        void DynamicDrawSystemComponent::Activate()
        {
            AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<DynamicDrawFeatureProcessor>();
            Interface<DynamicDrawSystemInterface>::Register(this);
        }

        void DynamicDrawSystemComponent::Deactivate()
        {
            AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<DynamicDrawFeatureProcessor>();
            Interface<DynamicDrawSystemInterface>::Unregister(this);
        }

        RPI::DynamicDrawInterface* DynamicDrawSystemComponent::GetDynamicDrawInterface(RPI::Scene* scene)
        {
            if (!scene)
            {
                scene = RPI::RPISystemInterface::Get()->GetDefaultScene().get();
            }
            RPI::DynamicDrawInterface* dd = scene ? m_sceneToDrawMap[scene] : nullptr;
            return dd;
        }

        void DynamicDrawSystemComponent::RegisterDynamicDrawForScene(RPI::DynamicDrawInterface* dd, RPI::Scene* scene)
        {
            AZ_Assert(m_sceneToDrawMap[scene] == nullptr, "DynamicDraw object already registered for this scene!");
            m_sceneToDrawMap[scene] = dd;
        }

        void DynamicDrawSystemComponent::UnregisterDynamicDrawForScene(RPI::Scene* scene)
        {
            AZ_Assert(m_sceneToDrawMap[scene] != nullptr, "DynamicDraw object was never registered for this scene!");
            m_sceneToDrawMap.erase(scene);
        }

    } // namespace RPI
} // namespace AZ
