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

#include "CoreNodes.h"

#include <ScriptCanvas/Libraries/Libraries.h>
#include <Data/DataMacros.h>
#include <Data/DataTrait.h>
#include <ScriptCanvas/Core/Attributes.h>

#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>

namespace ContainerTypeReflection
{
    #define SCRIPT_CANVAS_CALL_REFLECT_ON_TRAITS(TYPE)\
        TraitsReflector<TYPE##Type>::Reflect(reflectContext);

    using namespace AZStd;
    using namespace ScriptCanvas;

    // use this to reflect on demand reflection targets in the appropriate place
    class ReflectOnDemandTargets
    {

    public:
        AZ_TYPE_INFO(ReflectOnDemandTargets, "{FE658DB8-8F68-4E05-971A-97F398453B92}");
        AZ_CLASS_ALLOCATOR(ReflectOnDemandTargets, AZ::SystemAllocator, 0);

        ReflectOnDemandTargets() = default;
        ~ReflectOnDemandTargets() = default;

        static void Reflect(AZ::ReflectContext* reflectContext)
        {
            using namespace ScriptCanvas::Data;

            // First Macro creates a list of all of the types, that is invoked using the second macro.
            SCRIPT_CANVAS_PER_DATA_TYPE(SCRIPT_CANVAS_CALL_REFLECT_ON_TRAITS);
        }
    };

#undef SCRIPT_CANVAS_CALL_REFLECT_ON_TRAITS
}

namespace ScriptCanvas
{
    namespace Library
    {
        void Core::Reflect(AZ::ReflectContext* reflection)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection))
            {
                serializeContext->Class<Core, LibraryDefinition>()
                    ->Version(1)
                    ;

                if (AZ::EditContext* editContext = serializeContext->GetEditContext())
                {
                    editContext->Class<Core>("Core", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/ScriptCanvas/Libraries/Core.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".time")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TimeNodeTitlePalette")
                        ;
                }
            }

            Nodes::Core::EBusEventEntry::Reflect(reflection);
            Nodes::Core::Internal::ScriptEventEntry::Reflect(reflection);
            Nodes::Core::Internal::ScriptEventBase::Reflect(reflection);
            Nodes::Core::Internal::Nodeling::Reflect(reflection);

            ContainerTypeReflection::ReflectOnDemandTargets::Reflect(reflection);

            ContainerTypeReflection::TraitsReflector<AzFramework::SliceInstantiationTicket>::Reflect(reflection);
        }

        void Core::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Core;
            AddNodeToRegistry<Core, Error>(nodeRegistry);
            AddNodeToRegistry<Core, ErrorHandler>(nodeRegistry);
            AddNodeToRegistry<Core, Method>(nodeRegistry);
            AddNodeToRegistry<Core, BehaviorContextObjectNode>(nodeRegistry);
            AddNodeToRegistry<Core, Start>(nodeRegistry);            
            AddNodeToRegistry<Core, ScriptCanvas::Nodes::Core::String>(nodeRegistry);
            AddNodeToRegistry<Core, EBusEventHandler>(nodeRegistry);
            AddNodeToRegistry<Core, ExtractProperty>(nodeRegistry);
            AddNodeToRegistry<Core, ForEach>(nodeRegistry);
            AddNodeToRegistry<Core, GetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, SetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, ReceiveScriptEvent>(nodeRegistry);
            AddNodeToRegistry<Core, SendScriptEvent>(nodeRegistry);
            AddNodeToRegistry<Core, Repeater>(nodeRegistry);
            AddNodeToRegistry<Core, FunctionNode>(nodeRegistry);
            AddNodeToRegistry<Core, ExecutionNodeling>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Core::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Core::Error::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ErrorHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Method::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::BehaviorContextObjectNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Start::CreateDescriptor(),                
                ScriptCanvas::Nodes::Core::String::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::EBusEventHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ExtractProperty::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ForEach::CreateDescriptor(),                
                ScriptCanvas::Nodes::Core::GetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ReceiveScriptEvent::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SendScriptEvent::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Repeater::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::FunctionNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ExecutionNodeling::CreateDescriptor(),
            });
        }
    }
}
