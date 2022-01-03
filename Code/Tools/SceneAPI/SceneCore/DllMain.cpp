/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include <AzCore/Component/EntityBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/EBus/EBus.h>
#include <AzCore/Module/Environment.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <SceneAPI/SceneCore/Components/BehaviorComponent.h>
#include <SceneAPI/SceneCore/Components/LoadingComponent.h>
#include <SceneAPI/SceneCore/Components/GenerationComponent.h>
#include <SceneAPI/SceneCore/Components/ExportingComponent.h>
#include <SceneAPI/SceneCore/Components/Utilities/EntityConstructor.h>
#include <SceneAPI/SceneCore/Components/SceneSystemComponent.h>

#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/DataTypes/IManifestObject.h>
#include <SceneAPI/SceneCore/DataTypes/IGraphObject.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IBlendShapeRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ICommentRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMaterialRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IMeshAdvancedRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ILodRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ISkeletonProxyRule.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IScriptProcessorRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IAnimationData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBlendShapeData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMaterialData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexColorData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshVertexUVData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ISkinWeightData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>

#include <SceneAPI/SceneCore/DataTypes/ManifestBase/ISceneNodeSelectionList.h>
#include <SceneAPI/SceneCore/Export/MtlMaterialExporter.h>
#include <SceneAPI/SceneCore/Import/ManifestImportRequestHandler.h>
#include <SceneAPI/SceneCore/Utilities/PatternMatcher.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneCore
        {
            static class EntityMonitor* g_entityMonitor = nullptr;
            static AZ::Entity* g_behaviors = nullptr;
            static AZ::EntityId g_behaviorsId;
            static AZStd::vector<AZ::ComponentDescriptor*> g_componentDescriptors;
            static AZ::SceneAPI::Import::ManifestImportRequestHandler* g_manifestImporter = nullptr;

            class EntityMonitor
                : public AZ::EntityBus::Handler
            {
            public:
                AZ_CLASS_ALLOCATOR(EntityMonitor, AZ::SystemAllocator, 0);

                EntityMonitor()
                {
                    AZ::EntityBus::Handler::BusConnect(g_behaviorsId);
                }

                ~EntityMonitor()
                {
                    AZ::EntityBus::Handler::BusDisconnect(g_behaviorsId);
                }

                void OnEntityDestruction(const AZ::EntityId& entityId) override
                {
                    if (entityId == g_behaviorsId)
                    {
                        // Another part of the code has claimed and deleted this entity already.
                        g_behaviors = nullptr;
                        AZ::EntityBus::Handler::BusDisconnect(g_behaviorsId);
                        g_behaviorsId.SetInvalid();
                    }
                }
            };

            void Initialize()
            {
                // Explicitly creating this component early as this currently needs to be available to the
                //      RC before Gems are loaded in order to know the file extension.
                if (!g_manifestImporter)
                {
                    g_manifestImporter = aznew AZ::SceneAPI::Import::ManifestImportRequestHandler();
                    g_manifestImporter->Activate();
                }
            }

            bool IMeshGroupConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
            {
                if (classElement.GetVersion() == 1)
                {
                    // There have been 2 version of IMeshGroup, one that directly inherited from IGroup and one that
                    //      inherited as IMeshGroup : ISceneNodeGroup (was IMeshBaseGroup) : IGroup. To fix this, check
                    //      if {1D20FA11-B184-429E-8C86-745852234845} (ISceneNodeGroup) is present and if not add it.

                    AZ::SerializeContext::DataElementNode& baseClass = classElement.GetSubElement(0);
                    if (baseClass.GetId() != AZ::SceneAPI::DataTypes::ISceneNodeGroup::TYPEINFO_Uuid())
                    {
                        if (!baseClass.Convert<AZ::SceneAPI::DataTypes::ISceneNodeGroup>(context))
                        {
                            AZ_TracePrintf(AZ::SceneAPI::Utilities::ErrorWindow, "Failed to upgrade IMeshGroup from version 1.");
                            return false;
                        }
                    }
                }
                return true;
            }

            void ReflectTypes(AZ::SerializeContext* context)
            {
                if (!context)
                {
                    AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                }

                // Check if this library hasn't already been reflected. This can happen as the ResourceCompilerScene needs
                //      to explicitly load and reflect the SceneAPI libraries to discover the available extension, while
                //      Gems with system components need to do the same in the Project Manager.
                if (context && (context->IsRemovingReflection() || !context->FindClassData(AZ::SceneAPI::DataTypes::IGroup::TYPEINFO_Uuid())))
                {
                    AZ::SceneAPI::DataTypes::IManifestObject::Reflect(context);
                    AZ::SceneAPI::Events::CallProcessorBinder::Reflect(context);
                    // Register components
                    AZ::SceneAPI::SceneCore::BehaviorComponent::Reflect(context);
                    AZ::SceneAPI::SceneCore::LoadingComponent::Reflect(context);
                    AZ::SceneAPI::SceneCore::GenerationComponent::Reflect(context);
                    AZ::SceneAPI::SceneCore::ExportingComponent::Reflect(context);
                    AZ::SceneAPI::SceneCore::RCExportingComponent::Reflect(context);
                    AZ::SceneAPI::SceneCore::SceneSystemComponent::Reflect(context);
                    // Register group interfaces
                    context->Class<AZ::SceneAPI::DataTypes::IGroup, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ISceneNodeGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMeshGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(2, &IMeshGroupConverter);
                    context->Class<AZ::SceneAPI::DataTypes::ISkeletonGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ISkinGroup, AZ::SceneAPI::DataTypes::ISceneNodeGroup>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IAnimationGroup, AZ::SceneAPI::DataTypes::IGroup>()->Version(1);

                    // Register rule interfaces
                    context->Class<AZ::SceneAPI::DataTypes::IRule, AZ::SceneAPI::DataTypes::IManifestObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IBlendShapeRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ICommentRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMaterialRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMeshAdvancedRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ILodRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ISkeletonProxyRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IScriptProcessorRule, AZ::SceneAPI::DataTypes::IRule>()->Version(1);
                    // Register graph data interfaces
                    context->Class<AZ::SceneAPI::DataTypes::IAnimationData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IBlendShapeData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IBoneData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMaterialData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMeshData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMeshVertexColorData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::IMeshVertexUVData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ISkinWeightData, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);
                    context->Class<AZ::SceneAPI::DataTypes::ITransform, AZ::SceneAPI::DataTypes::IGraphObject>()->Version(1);

                    // Register base manifest types
                    context->Class<AZ::SceneAPI::DataTypes::ISceneNodeSelectionList>()->Version(1);

                    // Register containers
                    AZ::SceneAPI::Containers::RuleContainer::Reflect(context);
                    AZ::SceneAPI::Containers::SceneManifest::Reflect(context);

                    // Register utilities
                    AZ::SceneAPI::SceneCore::PatternMatcher::Reflect(context);
                    AZ::SceneAPI::Utilities::DebugSceneGraph::Reflect(context);
                }
            }

            void Reflect(AZ::SerializeContext* context)
            {
                ReflectTypes(context);

                // Descriptor registration is done in Reflect instead of Initialize because the ResourceCompilerScene initializes the libraries before
                // there's an application.
                if (g_componentDescriptors.empty())
                {
                    g_componentDescriptors.push_back(AZ::SceneAPI::Export::MaterialExporterComponent::CreateDescriptor());
                    g_componentDescriptors.push_back(AZ::SceneAPI::Export::RCMaterialExporterComponent::CreateDescriptor());
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        AZ::ComponentApplicationBus::Broadcast(&AZ::ComponentApplicationBus::Handler::RegisterComponentDescriptor, descriptor);
                    }
                }
            }

            void ReflectBehavior(AZ::BehaviorContext* context)
            {
                AZ::SceneAPI::Containers::Scene::Reflect(context);
                AZ::SceneAPI::Containers::SceneGraph::Reflect(context);
                AZ::SceneAPI::Containers::SceneManifest::Reflect(context);
                AZ::SceneAPI::Containers::RuleContainer::Reflect(context);
                AZ::SceneAPI::SceneCore::ExportingComponent::Reflect(context);
            }

            void Activate()
            {
                if (g_behaviors)
                {
                    return;
                }
                g_behaviors = AZ::SceneAPI::SceneCore::EntityConstructor::BuildEntityRaw("Scene Behaviors",
                    AZ::SceneAPI::SceneCore::BehaviorComponent::TYPEINFO_Uuid());
                g_behaviorsId = g_behaviors->GetId();

                AZ_Error("SceneCore", !g_entityMonitor, "The EntityMonitor has not been deactivated properly, cannot complete activation");
                if (!g_entityMonitor)
                {
                    g_entityMonitor = aznew EntityMonitor();
                }
            }

            void Deactivate()
            {
                if (g_entityMonitor)
                {
                    delete g_entityMonitor;
                    g_entityMonitor = nullptr;
                }

                if (g_behaviors)
                {
                    g_behaviors->Deactivate();
                    delete g_behaviors;
                    g_behaviors = nullptr;
                    g_behaviorsId.SetInvalid();
                }
            }

            void Uninitialize()
            {
                AZ::SerializeContext* context = nullptr;
                AZ::ComponentApplicationBus::BroadcastResult(context, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                if (context)
                {
                    context->EnableRemoveReflection();
                    Reflect(context);
                    context->DisableRemoveReflection();
                    context->CleanupModuleGenericClassInfo();
                }

                if (!g_componentDescriptors.empty())
                {
                    for (AZ::ComponentDescriptor* descriptor : g_componentDescriptors)
                    {
                        descriptor->ReleaseDescriptor();
                    }
                    g_componentDescriptors.clear();
                    g_componentDescriptors.shrink_to_fit();
                }

                if (g_manifestImporter)
                {
                    g_manifestImporter->Deactivate();
                    delete g_manifestImporter;
                    g_manifestImporter = nullptr;
                }
            }
        } // namespace SceneCore
    } // namespace SceneAPI
} // namespace AZ

extern "C" AZ_DLL_EXPORT void InitializeDynamicModule(void* env)
{
    if (AZ::Environment::IsReady())
    {
        return;
    }

    AZ::Environment::Attach(static_cast<AZ::EnvironmentInstance>(env));

    AZ::SceneAPI::SceneCore::Initialize();
}

extern "C" AZ_DLL_EXPORT void Reflect(AZ::SerializeContext* context)
{
    AZ::SceneAPI::SceneCore::Reflect(context);
}

extern "C" AZ_DLL_EXPORT void ReflectBehavior(AZ::BehaviorContext * context)
{
    AZ::SceneAPI::SceneCore::ReflectBehavior(context);
}

extern "C" AZ_DLL_EXPORT void ReflectTypes(AZ::SerializeContext * context)
{
    AZ::SceneAPI::SceneCore::ReflectTypes(context);
}

extern "C" AZ_DLL_EXPORT void Activate()
{
    AZ::SceneAPI::SceneCore::Activate();
}

extern "C" AZ_DLL_EXPORT void Deactivate()
{
    AZ::SceneAPI::SceneCore::Deactivate();
}

extern "C" AZ_DLL_EXPORT void UninitializeDynamicModule()
{
    if (!AZ::Environment::IsReady())
    {
        return;
    }
    AZ::SceneAPI::SceneCore::Uninitialize();

    // This module does not own these allocators, but must clear its cached EnvironmentVariables
    // because it is linked into other modules, and thus does not get unloaded from memory always
    if (AZ::AllocatorInstance<AZ::SystemAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::SystemAllocator>::Destroy();
    }
    if (AZ::AllocatorInstance<AZ::OSAllocator>::IsReady())
    {
        AZ::AllocatorInstance<AZ::OSAllocator>::Destroy();
    }

    AZ::Environment::Detach();
}

#endif // !defined(AZ_MONOLITHIC_BUILD)
