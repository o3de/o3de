/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "GraphicsGem_AR_TestSystemComponent.h"

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>

#include <AzCore/Serialization/SerializeContext.h>

#include <Atom/RPI.Public/FeatureProcessorFactory.h>

#include <Render/GraphicsGem_AR_TestFeatureProcessor.h>

namespace GraphicsGem_AR_Test
{
    AZ_COMPONENT_IMPL(GraphicsGem_AR_TestSystemComponent, "GraphicsGem_AR_TestSystemComponent",
        GraphicsGem_AR_TestSystemComponentTypeId);

    void GraphicsGem_AR_TestSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsGem_AR_TestSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }

        GraphicsGem_AR_TestFeatureProcessor::Reflect(context);
    }

    void GraphicsGem_AR_TestSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("GraphicsGem_AR_TestSystemService"));
    }

    void GraphicsGem_AR_TestSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("GraphicsGem_AR_TestSystemService"));
    }

    void GraphicsGem_AR_TestSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void GraphicsGem_AR_TestSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    GraphicsGem_AR_TestSystemComponent::GraphicsGem_AR_TestSystemComponent()
    {
        if (GraphicsGem_AR_TestInterface::Get() == nullptr)
        {
            GraphicsGem_AR_TestInterface::Register(this);
        }
    }

    GraphicsGem_AR_TestSystemComponent::~GraphicsGem_AR_TestSystemComponent()
    {
        if (GraphicsGem_AR_TestInterface::Get() == this)
        {
            GraphicsGem_AR_TestInterface::Unregister(this);
        }
    }

    void GraphicsGem_AR_TestSystemComponent::Init()
    {
    }

    void GraphicsGem_AR_TestSystemComponent::Activate()
    {
        GraphicsGem_AR_TestRequestBus::Handler::BusConnect();

        AZ::RPI::FeatureProcessorFactory::Get()->RegisterFeatureProcessor<GraphicsGem_AR_TestFeatureProcessor>();
    }

    void GraphicsGem_AR_TestSystemComponent::Deactivate()
    {
        AZ::RPI::FeatureProcessorFactory::Get()->UnregisterFeatureProcessor<GraphicsGem_AR_TestFeatureProcessor>();

        GraphicsGem_AR_TestRequestBus::Handler::BusDisconnect();
    }

} // namespace GraphicsGem_AR_Test
