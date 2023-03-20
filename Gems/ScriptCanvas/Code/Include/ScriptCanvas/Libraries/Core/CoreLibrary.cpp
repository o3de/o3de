/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CoreLibrary.h"
#include "EBusEventHandler.h"
#include "Method.h"
#include "MethodOverloaded.h"
#include "ScriptEventBase.h"

#include <Data/DataMacros.h>
#include <Data/DataTrait.h>
#include <ScriptCanvas/Core/EBusHandler.h>
#include <ScriptCanvas/Core/SubgraphInterface.h>
#include <ScriptCanvas/Grammar/DebugMap.h>
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>
#include <ScriptCanvas/Libraries/ScriptCanvasNodeRegistry.h>

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
        AZ_CLASS_ALLOCATOR(ReflectOnDemandTargets, AZ::SystemAllocator);

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

namespace ScriptCanvas::CoreLibrary
{
    void Reflect(AZ::ReflectContext* reflection)
    {
        Nodes::Core::EBusEventEntry::Reflect(reflection);
        Nodes::Core::AzEventEntry::Reflect(reflection);
        Nodes::Core::Internal::ScriptEventEntry::Reflect(reflection);

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

    void InitNodeRegistry(NodeRegistry* nodeRegistry)
    {
        using namespace ScriptCanvas::Nodes::Core;
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<Method>::Uuid());
        nodeRegistry->m_nodes.push_back(AZ::AzTypeInfo<MethodOverloaded>::Uuid());
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetComponentDescriptors()
    {
        return AZStd::vector<AZ::ComponentDescriptor*>({
            ScriptCanvas::Nodes::Core::Method::CreateDescriptor(),
            ScriptCanvas::Nodes::Core::MethodOverloaded::CreateDescriptor(),
        });
    }
}
