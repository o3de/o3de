/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplication.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Module/ModuleManager.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/ObjectStream.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            namespace EntityConstructor
            {
                EntityPointer BuildEntity(const char* entityName, const AZ::Uuid& baseComponentType)
                {
                    return EntityPointer(BuildEntityRaw(entityName, baseComponentType), [](AZ::Entity* entity)
                    {
                        entity->Deactivate();
                        delete entity;
                    });
                }

                Entity* BuildEntityRaw(const char* entityName, const AZ::Uuid& baseComponentType)
                {
                    SerializeContext* context = nullptr;
                    ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);

                    Entity* entity = aznew AZ::Entity(entityName);
                    if (context)
                    {
                        context->EnumerateDerived(
                            [entity](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& /*typeId*/) -> bool
                            {
                                entity->CreateComponent(data->m_typeId);
                                return true;
                            }, baseComponentType, baseComponentType);
                    }
                    entity->Init();
                    entity->Activate();
                    
                    return entity;
                }

                Entity* BuildSceneSystemEntity()
                {
                    SerializeContext* context = nullptr;
                    ComponentApplicationBus::BroadcastResult(context, &ComponentApplicationBus::Events::GetSerializeContext);
                    if (!context)
                    {
                        AZ_TracePrintf(SceneAPI::Utilities::ErrorWindow, "Unable to retrieve serialize context.");
                        return nullptr;
                    }

                    // Starting all system components would be too expensive for a builder/ResourceCompiler, so only the system components needed
                    // for the SceneAPI will be created.
                    AZStd::unique_ptr<Entity> entity(aznew AZ::Entity("Scene System"));

                    const Uuid sceneSystemComponentType = azrtti_typeid<AZ::SceneAPI::SceneCore::SceneSystemComponent>();
                    context->EnumerateDerived(
                        [&entity](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& typeId) -> bool
                    {
                        AZ_UNUSED(typeId);
                        // Before adding a new instance of a SceneSystemComponent, first check if the entity already has
                        // a component of the same type. Just like regular system components, there should only ever be
                        // a single instance of a SceneSystemComponent.
                        bool alreadyAdded = false;
                        for (const Component* component : entity->GetComponents())
                        {
                            if (component->RTTI_GetType() == data->m_typeId)
                            {
                                alreadyAdded = true;
                                break;
                            }
                        }
                        if (!alreadyAdded)
                        {
                            entity->CreateComponent(data->m_typeId);
                        }
                        return true;
                    }, sceneSystemComponentType, sceneSystemComponentType);
                    return entity.release();

                }
            } // namespace EntityConstructor
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ
