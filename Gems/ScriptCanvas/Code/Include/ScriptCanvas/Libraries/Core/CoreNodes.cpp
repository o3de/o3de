/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CoreNodes.h"

#include <Data/DataMacros.h>
#include <Data/DataTrait.h>
#include <ScriptCanvas/Core/Attributes.h>
#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>
#include <ScriptCanvas/Libraries/Libraries.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>

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
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Core.png")->
                        Attribute(AZ::Edit::Attributes::CategoryStyle, ".time")->
                        Attribute(ScriptCanvas::Attributes::Node::TitlePaletteOverride, "TimeNodeTitlePalette")
                        ;
                }
            }

            Nodes::Core::EBusEventEntry::Reflect(reflection);
            Nodes::Core::AzEventEntry::Reflect(reflection);
            Nodes::Core::Internal::ScriptEventEntry::Reflect(reflection);
            Nodes::Core::Internal::ScriptEventBase::Reflect(reflection);
            Nodes::Core::Internal::Nodeling::Reflect(reflection);

            ContainerTypeReflection::ReflectOnDemandTargets::Reflect(reflection);
            
            // reflected to go over the network
            Grammar::Variable::Reflect(reflection);
            Grammar::FunctionPrototype::Reflect(reflection);

            // reflect to build nodes that are built from sub graph definitions
            Grammar::SubgraphInterface::Reflect(reflection);

            // used to speed up the broadcast of debug information from Lua
            Grammar::ReflectDebugSymbols(reflection);

            //ContainerTypeReflection::TraitsReflector<AzFramework::SliceInstantiationTicket>::Reflect(reflection);
            SlotExecution::Map::Reflect(reflection);
            EBusHandler::Reflect(reflection);
        }

        void Core::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            using namespace ScriptCanvas::Nodes::Core;
            AddNodeToRegistry<Core, Method>(nodeRegistry);
            AddNodeToRegistry<Core, MethodOverloaded>(nodeRegistry);
            AddNodeToRegistry<Core, Start>(nodeRegistry);            
            AddNodeToRegistry<Core, EBusEventHandler>(nodeRegistry);
            AddNodeToRegistry<Core, AzEventHandler>(nodeRegistry);
            AddNodeToRegistry<Core, ExtractProperty>(nodeRegistry);
            AddNodeToRegistry<Core, ForEach>(nodeRegistry);
            AddNodeToRegistry<Core, GetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, SetVariableNode>(nodeRegistry);
            AddNodeToRegistry<Core, ReceiveScriptEvent>(nodeRegistry);
            AddNodeToRegistry<Core, SendScriptEvent>(nodeRegistry);
            AddNodeToRegistry<Core, Repeater>(nodeRegistry);
            AddNodeToRegistry<Core, FunctionCallNode>(nodeRegistry);
            AddNodeToRegistry<Core, FunctionDefinitionNode>(nodeRegistry);
            // Nodeables
            AddNodeToRegistry<Core, Nodes::RepeaterNodeableNode>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Core::GetComponentDescriptors()
        {
            return AZStd::vector<AZ::ComponentDescriptor*>({
                ScriptCanvas::Nodes::Core::Method::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::MethodOverloaded::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Start::CreateDescriptor(),                
                ScriptCanvas::Nodes::Core::EBusEventHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::AzEventHandler::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ExtractProperty::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ForEach::CreateDescriptor(),                
                ScriptCanvas::Nodes::Core::GetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SetVariableNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::ReceiveScriptEvent::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::SendScriptEvent::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::Repeater::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::FunctionCallNode::CreateDescriptor(),
                ScriptCanvas::Nodes::Core::FunctionDefinitionNode::CreateDescriptor(),
                // Nodeables
                ScriptCanvas::Nodes::RepeaterNodeableNode::CreateDescriptor(),
            });
        }
    }
}
