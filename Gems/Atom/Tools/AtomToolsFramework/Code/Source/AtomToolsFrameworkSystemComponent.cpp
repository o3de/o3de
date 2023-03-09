/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsAnyDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocument.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystem.h>
#include <AtomToolsFramework/DynamicProperty/DynamicPropertyGroup.h>
#include <AtomToolsFramework/EntityPreviewViewport/EntityPreviewViewportSettingsSystem.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNode.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodeManager.h>
#include <AtomToolsFramework/Graph/DynamicNode/DynamicNodePaletteItem.h>
#include <AtomToolsFramework/Graph/GraphCompiler.h>
#include <AtomToolsFramework/Graph/GraphDocument.h>
#include <AtomToolsFramework/Graph/GraphViewConstructPresets.h>
#include <AtomToolsFramework/Graph/GraphViewSettings.h>
#include <AtomToolsFramework/Inspector/InspectorWidget.h>
#include <AtomToolsFrameworkSystemComponent.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <Inspector/PropertyWidgets/PropertyStringBrowseEditCtrl.h>

namespace AtomToolsFramework
{
    void AtomToolsFrameworkSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        AtomToolsDocument::Reflect(context);
        AtomToolsAnyDocument::Reflect(context);
        AtomToolsDocumentSystem::Reflect(context);
        CreateDynamicNodeMimeEvent::Reflect(context);
        DynamicNode::Reflect(context);
        DynamicProperty::Reflect(context);
        DynamicPropertyGroup::Reflect(context);
        EntityPreviewViewportSettingsSystem::Reflect(context);
        GraphCompiler::Reflect(context);
        GraphDocument::Reflect(context);
        GraphViewSettings::Reflect(context);
        GraphViewConstructPresets::Reflect(context);
        InspectorWidget::Reflect(context);

        if (auto serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->RegisterGenericType<AZStd::unordered_map<AZStd::string, bool>>();
            serialize->RegisterGenericType<AZStd::map<AZStd::string, AZStd::vector<AZStd::string>>>();

            serialize->Class<AtomToolsFrameworkSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (auto editContext = serialize->GetEditContext())
            {
                editContext->Class<AtomToolsFrameworkSystemComponent>("AtomToolsFrameworkSystemComponent", "")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomToolsFrameworkSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("AtomToolsFrameworkSystemService"));
    }

    void AtomToolsFrameworkSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("AtomToolsFrameworkSystemService"));
    }

    void AtomToolsFrameworkSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        AZ_UNUSED(required);
    }

    void AtomToolsFrameworkSystemComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        AZ_UNUSED(dependent);
    }

    void AtomToolsFrameworkSystemComponent::Init()
    {
    }

    void AtomToolsFrameworkSystemComponent::Activate()
    {
        RegisterStringBrowseEditHandler();
    }

    void AtomToolsFrameworkSystemComponent::Deactivate()
    {
    }
}
