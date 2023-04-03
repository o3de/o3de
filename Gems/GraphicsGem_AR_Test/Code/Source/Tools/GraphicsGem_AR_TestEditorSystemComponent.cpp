/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include "GraphicsGem_AR_TestEditorSystemComponent.h"

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>

namespace GraphicsGem_AR_Test
{
    AZ_COMPONENT_IMPL(GraphicsGem_AR_TestEditorSystemComponent, "GraphicsGem_AR_TestEditorSystemComponent",
        GraphicsGem_AR_TestEditorSystemComponentTypeId, BaseSystemComponent);

    void GraphicsGem_AR_TestEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<GraphicsGem_AR_TestEditorSystemComponent, GraphicsGem_AR_TestSystemComponent>()
                ->Version(0);
        }
    }

    GraphicsGem_AR_TestEditorSystemComponent::GraphicsGem_AR_TestEditorSystemComponent() = default;

    GraphicsGem_AR_TestEditorSystemComponent::~GraphicsGem_AR_TestEditorSystemComponent() = default;

    void GraphicsGem_AR_TestEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("GraphicsGem_AR_TestSystemEditorService"));
    }

    void GraphicsGem_AR_TestEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("GraphicsGem_AR_TestSystemEditorService"));
    }

    void GraphicsGem_AR_TestEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void GraphicsGem_AR_TestEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void GraphicsGem_AR_TestEditorSystemComponent::Activate()
    {
        GraphicsGem_AR_TestSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void GraphicsGem_AR_TestEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        GraphicsGem_AR_TestSystemComponent::Deactivate();
    }

} // namespace GraphicsGem_AR_Test
