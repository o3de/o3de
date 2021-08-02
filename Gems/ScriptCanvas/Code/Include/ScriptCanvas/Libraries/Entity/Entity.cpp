/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Libraries.h>

#include "Entity.h"

namespace ScriptCanvas
{
    static bool OldEntityIdIsValidNodeVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& rootNodeElement)
    {
        int nodeElementIndex = rootNodeElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
        if (nodeElementIndex == -1)
        {
            AZ_Error("Script Canvas", false, "Unable to find base class node element for old EntityId::IsValid function node");
            return false;
        }

        // The DataElementNode is being copied purposefully in this statement to clone the data
        AZ::SerializeContext::DataElementNode baseNodeElement = rootNodeElement.GetSubElement(nodeElementIndex);
        if (!rootNodeElement.Convert(context, azrtti_typeid<EntityNodes::IsValidNode>()))
        {
            AZ_Error("Script Canvas", false, "Unable to convert old Entity::IsValid function node(%s) to new EntityId::IsValid function node(%s)",
                rootNodeElement.GetId().ToString<AZStd::string>().data(), azrtti_typeid<EntityNodes::IsValidNode>().ToString<AZStd::string>().data());
            return false;
        }

        if (rootNodeElement.AddElement(baseNodeElement) == -1)
        {
            AZ_Error("Script Canvas", false, "Unable to add base class node element to new EntityId::IsValid function node");
            return false;
        }

        return true;
    }

    namespace Library
    {
        void Entity::Reflect(AZ::ReflectContext* reflection)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(reflection);
            if (serializeContext)
            {
                serializeContext->Class<Entity, LibraryDefinition>()
                    ->Version(1)
                    ;

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<Entity>("Entity", "")->
                        ClassElement(AZ::Edit::ClassElements::EditorData, "")->
                        Attribute(AZ::Edit::Attributes::Icon, "Icons/ScriptCanvas/Libraries/Entity.png")
                        ;
                }

                // Reflects the old EntityId IsValid function node
                const AZ::TypeId nodeFunctionGenericMultiReturnTemplateTypeId("{DC5B1799-6C5B-4190-8D90-EF0C2D1BCE4E}");
                const AZ::TypeId oldEntityIdIsValidFuncSignatureTypeId = azrtti_typeid<bool(*)(AZ::EntityId)>();
                const AZ::TypeId oldEntityIdIsValidTypeId("{7CEC53AE-E12B-4738-B542-4587B8B95DC2}");

                // Aggregate NodeFunctionGenericMultiReturn<bool(*)(AZ::EntityId), OldIsValidTraits> typeid
                // NOTE: Aggregation order addition is not commutative and must be in the added last to first: (FirstUuid + ( SecondUuid + (ThirdUuid + ... + (NthUuid))
                const AZ::TypeId oldIsValidNodeAggregateTypeId = nodeFunctionGenericMultiReturnTemplateTypeId + (oldEntityIdIsValidFuncSignatureTypeId + oldEntityIdIsValidTypeId);
                serializeContext->ClassDeprecate("EntityId::IsValidNode", oldIsValidNodeAggregateTypeId, &OldEntityIdIsValidNodeVersionConverter);
            }
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(reflection))
            {
                SCRIPT_CANVAS_GENERICS_TO_VM(EntityNodes::Registrar, Entity, behaviorContext, EntityNodes::k_categoryName);
            }

            ScriptCanvas::Entity::RotateMethod::Reflect(reflection);
        }

        void Entity::InitNodeRegistry(NodeRegistry& nodeRegistry)
        {
            EntityNodes::Registrar::AddToRegistry<Entity>(nodeRegistry);
        }

        AZStd::vector<AZ::ComponentDescriptor*> Entity::GetComponentDescriptors()
        {
            AZStd::vector<AZ::ComponentDescriptor*> descriptors;
            EntityNodes::Registrar::AddDescriptors(descriptors);
            return descriptors;
        }
    }
}
