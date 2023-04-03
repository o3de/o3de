/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>
#include <GraphicsGem_AR_TestModuleInterface.h>
#include "GraphicsGem_AR_TestEditorSystemComponent.h"
#include "Components/EditorGraphicsGem_AR_TestComponent.h"

namespace GraphicsGem_AR_Test
{
    class GraphicsGem_AR_TestEditorModule
        : public GraphicsGem_AR_TestModuleInterface
    {
    public:
        AZ_RTTI(GraphicsGem_AR_TestEditorModule, GraphicsGem_AR_TestEditorModuleTypeId, GraphicsGem_AR_TestModuleInterface);
        AZ_CLASS_ALLOCATOR(GraphicsGem_AR_TestEditorModule, AZ::SystemAllocator);

        GraphicsGem_AR_TestEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                GraphicsGem_AR_TestEditorSystemComponent::CreateDescriptor(),
                GraphicsGem_AR_TestComponent::CreateDescriptor(),
                EditorGraphicsGem_AR_TestComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<GraphicsGem_AR_TestEditorSystemComponent>(),
            };
        }
    };
}// namespace GraphicsGem_AR_Test

AZ_DECLARE_MODULE_CLASS(Gem_GraphicsGem_AR_Test, GraphicsGem_AR_Test::GraphicsGem_AR_TestEditorModule)
