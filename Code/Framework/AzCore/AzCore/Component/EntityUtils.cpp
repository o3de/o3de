/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/EntityUtils.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/std/containers/fixed_vector.h>

namespace AZ::EntityUtils
{
    //=========================================================================
    // Reflect
    //=========================================================================
    void Reflect(ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<SerializeContext*>(context))
        {
            serializeContext->Class<SerializableEntityContainer>()->
                Version(1)->
                Field("Entities", &SerializableEntityContainer::m_entities);
        }
    }

    struct StackDataType
    {
        const SerializeContext::ClassData* m_classData;
        const SerializeContext::ClassElement* m_elementData;
        void* m_dataPtr;
        bool m_isModifiedContainer;
    };

    //=========================================================================
    // EnumerateEntityIds
    //=========================================================================
    void EnumerateEntityIds(const void* classPtr, const Uuid& classUuid, const EntityIdVisitor& visitor, SerializeContext* context)
    {
        AZ_PROFILE_FUNCTION(AzCore);

        if (!context)
        {
            context = GetApplicationSerializeContext();
            if (!context)
            {
                AZ_Error("Serialization", false, "No serialize context provided! Failed to get component application default serialize context! ComponentApp is not started or input serialize context should not be null!");
                return;
            }
        }
        AZStd::vector<const SerializeContext::ClassData*> parentStack;
        parentStack.reserve(30);
        auto beginCB = [ &](void* ptr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* elementData) -> bool
            {
                (void)elementData;

                if (classData->m_typeId == SerializeTypeInfo<EntityId>::GetUuid())
                {
                    // determine if this is entity ref or just entityId (please refer to the function documentation for more info)
                    bool isEntityId = false;
                    if (!parentStack.empty() && parentStack.back()->m_typeId == SerializeTypeInfo<Entity>::GetUuid())
                    {
                        // our parent in the entity (currently entity has only one EntityId member, but we can check the offset for future proof
                        AZ_Assert(elementData && strcmp(elementData->m_name, "Id") == 0, "class Entity, should have only ONE EntityId member, the actual entity id!");
                        isEntityId = true;
                    }

                    EntityId* entityIdPtr = (elementData->m_flags & SerializeContext::ClassElement::FLG_POINTER) ?
                        *reinterpret_cast<EntityId**>(ptr) : reinterpret_cast<EntityId*>(ptr);
                    visitor(*entityIdPtr, isEntityId, elementData);
                }

                parentStack.push_back(classData);
                return true;
            };

        auto endCB = [ &]() -> bool
            {
                parentStack.pop_back();
                return true;
            };

        SerializeContext::EnumerateInstanceCallContext callContext(
            beginCB,
            endCB,
            context,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            nullptr
        );

        context->EnumerateInstanceConst(
            &callContext,
            classPtr,
            classUuid,
            nullptr,
            nullptr
            );
    }

    //=========================================================================
    // GetApplicationSerializeContext
    //=========================================================================
    SerializeContext* GetApplicationSerializeContext()
    {
        SerializeContext* context = nullptr;
        ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
        return context;
    }

    //=========================================================================
    // FindFirstDerivedComponent
    //=========================================================================
    Component* FindFirstDerivedComponent(const Entity* entity, const Uuid& typeId)
    {
        for (AZ::Component* component : entity->GetComponents())
        {
            if (azrtti_istypeof(typeId, component))
            {
                return component;
            }
        }
        return nullptr;
    }

    Component* FindFirstDerivedComponent(EntityId entityId, const Uuid& typeId)
    {
        Entity* entity{};
        ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);
        return entity ? FindFirstDerivedComponent(entity, typeId) : nullptr;
    }

    //=========================================================================
    // FindDerivedComponents
    //=========================================================================
    Entity::ComponentArrayType FindDerivedComponents(const Entity* entity, const Uuid& typeId)
    {
        Entity::ComponentArrayType result;
        for (AZ::Component* component : entity->GetComponents())
        {
            if (azrtti_istypeof(typeId, component))
            {
                result.push_back(component);
            }
        }
        return result;
    }

    Entity::ComponentArrayType FindDerivedComponents(EntityId entityId, const Uuid& typeId)
    {
        Entity* entity{};
        ComponentApplicationBus::BroadcastResult(entity, &ComponentApplicationRequests::FindEntity, entityId);
        return entity ? FindDerivedComponents(entity, typeId) : Entity::ComponentArrayType();
    }

    bool EnumerateBaseRecursive(SerializeContext* context, const EnumerateBaseRecursiveVisitor& baseClassVisitor, const TypeId& typeToExamine)
    {
        AZ_Assert(context, "CheckDeclaresSerializeBaseClass called with no serialize context.");
        if (!context)
        {
            return false;
        }

        AZStd::fixed_vector<TypeId, 64> knownBaseClasses = { typeToExamine };  // avoid allocating heap here if possible.  64 types are 64*sizeof(Uuid) which is only 1k.
        bool foundBaseClass = false;
        auto enumerateBaseVisitor = [&baseClassVisitor, &knownBaseClasses](const AZ::SerializeContext::ClassData* classData, const TypeId& examineTypeId)
        {
            if (!classData)
            {
                return false;
            }

            if (AZStd::find(knownBaseClasses.begin(), knownBaseClasses.end(), classData->m_typeId) == knownBaseClasses.end())
            {
                if (knownBaseClasses.size() == 64)
                {
                    // this should be pretty unlikely since a single class would have to have many other classes in its heirarchy
                    // and it'd all have to be basically in one layer, as we are popping as we explore.
                    AZ_WarningOnce("EntityUtils", false, "While trying to find a base class, all available slots were consumed.  consider increasing the size of knownBaseClasses.\n");
                    // we cannot continue any further, assume we did not find it.
                    return false;
                }
                knownBaseClasses.push_back(classData->m_typeId);
            }

            return baseClassVisitor(classData, examineTypeId);
        };

        while (!knownBaseClasses.empty() && !foundBaseClass)
        {
            TypeId toExamine = knownBaseClasses.back();
            knownBaseClasses.pop_back();

            context->EnumerateBase(enumerateBaseVisitor, toExamine);
        }

        return foundBaseClass;
    }

    bool CheckIfClassIsDeprecated(SerializeContext* context, const TypeId& typeToExamine)
    {
        bool isDeprecated = false;
        auto classVisitorFn = [&isDeprecated](const AZ::SerializeContext::ClassData* classData, const TypeId& /*rttiBase*/)
        {
            // Stop iterating once we stop receiving SerializeContext::ClassData*.
            if (!classData)
            {
                return false;
            }

            // Stop iterating if we've found that the class is deprecated
            if (classData->IsDeprecated())
            {
                isDeprecated = true;
                return false;
            }

            return true; // keep iterating
        };

        // Check if the type is deprecated
        const AZ::SerializeContext::ClassData* classData = context->FindClassData(typeToExamine);
        if (classData->IsDeprecated())
        {
            return true;
        }

        // Check if any of its bases are deprecated
        EnumerateBaseRecursive(context, classVisitorFn, typeToExamine);

        return isDeprecated;
    }

    bool CheckDeclaresSerializeBaseClass(SerializeContext* context, const TypeId& typeToFind, const TypeId& typeToExamine)
    {
        AZ_Assert(context, "CheckDeclaresSerializeBaseClass called with no serialize context.");
        if (!context)
        {
            return false;
        }

        bool foundBaseClass = false;
        auto baseClassVisitorFn = [&typeToFind, &foundBaseClass](const AZ::SerializeContext::ClassData* reflectedBase, const TypeId& /*rttiBase*/)
        {
            if (!reflectedBase)
            {
                foundBaseClass = false;
                return false; // stop iterating
            }

            foundBaseClass = (reflectedBase->m_typeId == typeToFind);
            if (foundBaseClass)
            {
                return false; // we have a base, stop iterating
            }

            return true; // keep iterating
        };

        EnumerateBaseRecursive(context, baseClassVisitorFn, typeToExamine);

        return foundBaseClass;
    }

    bool RemoveDuplicateServicesOfAndAfterIterator(
        const ComponentDescriptor::DependencyArrayType::iterator& iterator,
        ComponentDescriptor::DependencyArrayType& providedServiceArray,
        const Entity* entity)
    {
        // Build types that strip out AZ_Warnings will complain that entity is unused without this.
        (void)entity;
        if (iterator == providedServiceArray.end())
        {
            return false;
        }

        bool duplicateFound = false;

        for (ComponentDescriptor::DependencyArrayType::iterator duplicateCheckIter = AZStd::next(iterator);
            duplicateCheckIter != providedServiceArray.end();)
        {
            if (*iterator == *duplicateCheckIter)
            {
                AZ_Warning("Entity", false, "Duplicate service %d found on entity %s [%s]",
                    *duplicateCheckIter,
                    entity ? entity->GetName().c_str() : "Entity not provided",
                    entity ? entity->GetId().ToString().c_str() : "");
                duplicateCheckIter = providedServiceArray.erase(duplicateCheckIter);
                duplicateFound = true;
            }
            else
            {
                ++duplicateCheckIter;
            }
        }
        return duplicateFound;
    }

    void ConvertComponentVectorToMap(
        AZStd::span<AZ::Component* const> components, AZStd::unordered_map<AZStd::string, AZ::Component*>& componentMapOut)
    {
        for (AZ::Component* component : components)
        {
            if (component)
            {
                AZStd::string componentAlias = component->GetSerializedIdentifier();
                if (componentAlias.empty())
                {
                    // Component alias can be empty for non-editor components. Fallback to use the id as the component alias.
                    componentAlias = AZStd::string::format("Component_[%llu]", component->GetId());
                }
                componentMapOut.emplace(AZStd::move(componentAlias), component);
            }
        }
    }

} // namespace AZ::EntityUtils
