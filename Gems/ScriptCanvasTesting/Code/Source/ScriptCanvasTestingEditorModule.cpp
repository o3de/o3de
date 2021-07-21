
/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <ScriptCanvasTestingSystemComponent.h>
#include <Editor/Framework/ScriptCanvasTraceUtilities.h>
#include <Source/Nodes/Nodeables/NodeableTestingLibrary.h>

namespace ScriptCanvasTesting
{
    using namespace ScriptCanvasEditor;

    class ScriptCanvasTestingModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(ScriptCanvasTestingModule, "{AF32BC51-C4E5-48C4-B5E4-D7877C303D43}", AZ::Module);
        AZ_CLASS_ALLOCATOR(ScriptCanvasTestingModule, AZ::SystemAllocator, 0);

        ScriptCanvasTestingModule()
        {
            m_descriptors.insert(m_descriptors.end(), {
                ScriptCanvasTestingSystemComponent::CreateDescriptor(),
                TraceMessageComponent::CreateDescriptor(),
            });

            AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(NodeableTestingLibrary::GetComponentDescriptors());
            m_descriptors.insert(m_descriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<ScriptCanvasTestingSystemComponent>(),
                azrtti_typeid<TraceMessageComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_ScriptCanvasTesting, ScriptCanvasTesting::ScriptCanvasTestingModule)
