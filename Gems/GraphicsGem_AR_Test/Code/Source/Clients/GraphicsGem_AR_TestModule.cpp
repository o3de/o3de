/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <GraphicsGem_AR_Test/GraphicsGem_AR_TestTypeIds.h>
#include <GraphicsGem_AR_TestModuleInterface.h>
#include "GraphicsGem_AR_TestSystemComponent.h"

#include <AzCore/RTTI/RTTI.h>

#include <Components/GraphicsGem_AR_TestComponent.h>

namespace GraphicsGem_AR_Test
{
    class GraphicsGem_AR_TestModule
        : public GraphicsGem_AR_TestModuleInterface
    {
    public:
        AZ_RTTI(GraphicsGem_AR_TestModule, GraphicsGem_AR_TestModuleTypeId, GraphicsGem_AR_TestModuleInterface);
        AZ_CLASS_ALLOCATOR(GraphicsGem_AR_TestModule, AZ::SystemAllocator);

        GraphicsGem_AR_TestModule()
        {
            m_descriptors.insert(m_descriptors.end(),
                {
                    GraphicsGem_AR_TestSystemComponent::CreateDescriptor(),
                    GraphicsGem_AR_TestComponent::CreateDescriptor(),
                });
        }

        AZ::ComponentTypeList GetRequiredSystemComponents() const
        {
            return AZ::ComponentTypeList{ azrtti_typeid<GraphicsGem_AR_TestSystemComponent>() };
        }
    };
}// namespace GraphicsGem_AR_Test

AZ_DECLARE_MODULE_CLASS(Gem_GraphicsGem_AR_Test, GraphicsGem_AR_Test::GraphicsGem_AR_TestModule)
