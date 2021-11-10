/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabSystemComponent.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/IO/Path/Path.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/Serialization/Json/RegistrationContext.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityIdMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceSerializer.h>
#include <AzToolsFramework/Prefab/Spawnable/EditorInfoRemover.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabCatchmentProcessor.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabConversionPipeline.h>
#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Entity/EditorEntityContextBus.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        void PrefabSystemComponent::Init()
        {
        }

        void PrefabSystemComponent::Activate()
        {
            AZ::Interface<PrefabSystemComponentInterface>::Register(this);
            m_prefabLoader.RegisterPrefabLoaderInterface();
            m_instanceUpdateExecutor.RegisterInstanceUpdateExecutorInterface();
            m_instanceToTemplatePropagator.RegisterInstanceToTemplateInterface();
            m_prefabPublicHandler.RegisterPrefabPublicHandlerInterface();
            m_prefabPublicRequestHandler.Connect();
            m_prefabSystemScriptingHandler.Connect(this);
            AZ::SystemTickBus::Handler::BusConnect();
        }

        void PrefabSystemComponent::Deactivate()
        {
            AZ::SystemTickBus::Handler::BusDisconnect();
            m_prefabSystemScriptingHandler.Disconnect();
            m_prefabPublicRequestHandler.Disconnect();
            m_prefabPublicHandler.UnregisterPrefabPublicHandlerInterface();
            m_instanceToTemplatePropagator.UnregisterInstanceToTemplateInterface();
            m_instanceUpdateExecutor.UnregisterInstanceUpdateExecutorInterface();
            m_prefabLoader.UnregisterPrefabLoaderInterface();
            AZ::Interface<PrefabSystemComponentInterface>::Unregister(this);
        }

        void PrefabSystemComponent::Reflect(AZ::ReflectContext* context)
        {
            Instance::Reflect(context);
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabConversionPipeline::Reflect(context);
            AzToolsFramework::Prefab::PrefabConversionUtils::PrefabCatchmentProcessor::Reflect(context);
            AzToolsFramework::Prefab::PrefabConversionUtils::EditorInfoRemover::Reflect(context);
            PrefabPublicRequestHandler::Reflect(context);
            PrefabLoader::Reflect(context);
            PrefabSystemScriptingHandler::Reflect(context);

            if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serialize->Class<PrefabSystemComponent, AZ::Component>()->Version(1);
            }

            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {

                behaviorContext->EBus<PrefabLoaderScriptingBus>("PrefabLoaderScriptingBus")
                    ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                    ->Attribute(AZ::Script::Attributes::Module, "prefab")
                    ->Attribute(AZ::Script::Attributes::Category, "Prefab")
                    ->Event("SaveTemplateToString", &PrefabLoaderScriptingBus::Events::SaveTemplateToString);
                ;
            }

            AZ::JsonRegistrationContext* jsonRegistration = azrtti_cast<AZ::JsonRegistrationContext*>(context);
            if (jsonRegistration)
            {
                jsonRegistration->Serializer<JsonInstanceSerializer>()->HandlesType<Instance>();
            }
        }

        void PrefabSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC_CE("PrefabSystem"));
        }

        void PrefabSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
        {
        }

        void PrefabSystemComponent::GetIncompatibleServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& incompatible)
        {
        }

        void PrefabSystemComponent::OnSystemTick()
        {
            m_instanceUpdateExecutor.UpdateTemplateInstancesInQueue();
        }

        AZStd::unique_ptr<Instance> PrefabSystemComponent::CreatePrefab(
            const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Instance>>&& instancesToConsume,
            AZ::IO::PathView filePath, AZStd::unique_ptr<AZ::Entity> containerEntity, InstanceOptionalReference parent,
            bool shouldCreateLinks)
        {
            AZStd::unique_ptr<Instance> newInstance = AZStd::make_unique<Instance>(AZStd::move(containerEntity), parent);
            CreatePrefab(entities, AZStd::move(instancesToConsume), filePath, newInstance, shouldCreateLinks);
            return newInstance;
        }

        void PrefabSystemComponent::CreatePrefab(
            const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Instance>>&& instancesToConsume,
            AZ::IO::PathView filePath, AZStd::unique_ptr<Instance>& newInstance, bool shouldCreateLinks)
        {
            AZ::IO::Path relativeFilePath = m_prefabLoader.GenerateRelativePath(filePath);
            if (GetTemplateIdFromFilePath(relativeFilePath) != InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "Filepath %s has already been registered with the Prefab System Component",
                    relativeFilePath.c_str());

                return;
            }

            for (AZ::Entity* entity : entities)
            {
                AZ_Assert(entity, "Prefab - Null entity passed in during Create Prefab");

                newInstance->AddEntity(*entity);
            }

            for (AZStd::unique_ptr<Instance>& instance : instancesToConsume)
            {
                AZ_Assert(instance, "Prefab - Null instance passed in during Create Prefab");

                newInstance->AddInstance(AZStd::move(instance));
            }

            newInstance->SetTemplateSourcePath(relativeFilePath);
            newInstance->SetContainerEntityName(relativeFilePath.Stem().Native());

            TemplateId newTemplateId = CreateTemplateFromInstance(*newInstance, shouldCreateLinks);
            if (newTemplateId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "Failed to create a Template associated with file path %s during CreatePrefab.",
                    relativeFilePath.c_str());

                newInstance = nullptr;
            }
            else
            {
                newInstance->SetTemplateId(newTemplateId);
            }
        }

        void PrefabSystemComponent::PropagateTemplateChanges(TemplateId templateId, InstanceOptionalReference instanceToExclude)
        {
            UpdatePrefabInstances(templateId, instanceToExclude);

            auto templateIdToLinkIdsIterator = m_templateToLinkIdsMap.find(templateId);
            if (templateIdToLinkIdsIterator != m_templateToLinkIdsMap.end())
            {
                // We need to initialize a queue here because once all linked instances of a template are updated,
                // we will find all the linkIds corresponding to the updated template and add them to this queue again.
                AZStd::queue<LinkIds> linkIdsToUpdateQueue;
                linkIdsToUpdateQueue.push(LinkIds(templateIdToLinkIdsIterator->second.begin(),
                    templateIdToLinkIdsIterator->second.end()));
                UpdateLinkedInstances(linkIdsToUpdateQueue);
            }
        }

        void PrefabSystemComponent::UpdatePrefabTemplate(TemplateId templateId, const PrefabDom& updatedDom)
        {
            auto templateToUpdate = FindTemplate(templateId);
            if (templateToUpdate)
            {
                PrefabDom& templateDomToUpdate = templateToUpdate->get().GetPrefabDom();
                if (AZ::JsonSerialization::Compare(templateDomToUpdate, updatedDom) != AZ::JsonSerializerCompareResult::Equal)
                {
                    templateDomToUpdate.CopyFrom(updatedDom, templateDomToUpdate.GetAllocator());
                    SetTemplateDirtyFlag(templateId, true);
                    PropagateTemplateChanges(templateId);
                }
            }
        }

        void PrefabSystemComponent::UpdatePrefabInstances(TemplateId templateId, InstanceOptionalReference instanceToExclude)
        {
            m_instanceUpdateExecutor.AddTemplateInstancesToQueue(templateId, instanceToExclude);
        }

        void PrefabSystemComponent::UpdateLinkedInstances(AZStd::queue<LinkIds>& linkIdsQueue)
        {
            while (!linkIdsQueue.empty())
            {
                // Fetch the list of linkIds at the head of the queue.
                LinkIds& LinkIdsToUpdate = linkIdsQueue.front();
                TargetTemplateIdToLinkIdMap targetTemplateIdToLinkIdMap;
                BucketLinkIdsByTargetTemplateId(LinkIdsToUpdate, targetTemplateIdToLinkIdMap);

                // Update all the linked instances corresponding to the LinkIds before fetching the next set of linkIds.
                // This will ensure that templates are updated with changes in the same order they are received.
                for (const LinkId& linkIdToUpdate : LinkIdsToUpdate)
                {
                    UpdateLinkedInstance(linkIdToUpdate, targetTemplateIdToLinkIdMap, linkIdsQueue);
                }
                linkIdsQueue.pop();
            }
        }

        void PrefabSystemComponent::BucketLinkIdsByTargetTemplateId(LinkIds& linkIdsToUpdate,
            TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap)
        {
            for (const LinkId& linkIdToUpdate : linkIdsToUpdate)
            {
                Link& linkToUpdate = m_linkIdMap[linkIdToUpdate];
                TemplateId targetTemplateId = linkToUpdate.GetTargetTemplateId();
                auto templateIdToLinkIdsIterator = targetTemplateIdToLinkIdMap.find(targetTemplateId);
                if (templateIdToLinkIdsIterator == targetTemplateIdToLinkIdMap.end())
                {
                    targetTemplateIdToLinkIdMap.emplace(targetTemplateId, AZStd::make_pair(LinkIdSet{linkIdToUpdate}, false));
                }
                else
                {
                    targetTemplateIdToLinkIdMap[targetTemplateId].first.insert(linkIdToUpdate);
                }
            }
        }

        void PrefabSystemComponent::UpdateLinkedInstance(const LinkId linkIdToUpdate,
            TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap, AZStd::queue<LinkIds>& linkIdsQueue)
        {
            Link& linkToUpdate = m_linkIdMap[linkIdToUpdate];
            TemplateId targetTemplateId = linkToUpdate.GetTargetTemplateId();
            PrefabDomValue& linkdedInstanceDom = linkToUpdate.GetLinkedInstanceDom();
            PrefabDomValue linkDomBeforeUpdate;
            linkDomBeforeUpdate.CopyFrom(linkdedInstanceDom, m_templateIdMap[targetTemplateId].GetPrefabDom().GetAllocator());
            linkToUpdate.UpdateTarget();

            // If any of the templates links are already updated, we don't need to check whether the linkedInstance DOM differs
            // in content because the template is already marked to be sent for change propagation.
            bool isTemplateUpdated = targetTemplateIdToLinkIdMap[targetTemplateId].second;
            if (isTemplateUpdated ||
                AZ::JsonSerialization::Compare(linkDomBeforeUpdate, linkdedInstanceDom) != AZ::JsonSerializerCompareResult::Equal)
            {
                targetTemplateIdToLinkIdMap[targetTemplateId].second = true;
            }

            if (targetTemplateIdToLinkIdMap.find(targetTemplateId) != targetTemplateIdToLinkIdMap.end())
            {
                targetTemplateIdToLinkIdMap[targetTemplateId].first.erase(linkIdToUpdate);
                UpdateTemplateChangePropagationQueue(targetTemplateIdToLinkIdMap, targetTemplateId, linkIdsQueue);
            }
        }

        void PrefabSystemComponent::UpdateTemplateChangePropagationQueue(
            TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap,
            const TemplateId targetTemplateId, AZStd::queue<LinkIds>& linkIdsQueue)
        {
            if (targetTemplateIdToLinkIdMap[targetTemplateId].first.empty() &&
                targetTemplateIdToLinkIdMap[targetTemplateId].second)
            {
                auto templateToLinkIter = m_templateToLinkIdsMap.find(targetTemplateId);
                if (templateToLinkIter != m_templateToLinkIdsMap.end())
                {
                    linkIdsQueue.push(LinkIds(templateToLinkIter->second.begin(),
                        templateToLinkIter->second.end()));
                }
            }
        }

        AZStd::unique_ptr<Instance> PrefabSystemComponent::InstantiatePrefab(
            AZ::IO::PathView filePath, InstanceOptionalReference parent)
        {
            // Retrieve the template id for the source prefab filepath
            Prefab::TemplateId templateId = GetTemplateIdFromFilePath(filePath);

            if (templateId == Prefab::InvalidTemplateId)
            {
                // Load the template from the file
                templateId = m_prefabLoader.LoadTemplateFromFile(filePath);
            }

            if (templateId == Prefab::InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "Could not load template from path %s during InstantiatePrefab. Unable to proceed",
                    filePath);

                return nullptr;
            }

            return InstantiatePrefab(templateId, parent);
        }

        AZStd::unique_ptr<Instance> PrefabSystemComponent::InstantiatePrefab(
            TemplateId templateId, InstanceOptionalReference parent)
        {
            TemplateReference instantiatingTemplate = FindTemplate(templateId);

            if (!instantiatingTemplate.has_value())
            {
                AZ_Error("Prefab", false,
                    "Could not find template using Id %llu during InstantiatePrefab. Unable to proceed",
                    templateId);

                return nullptr;
            }

            auto newInstance = AZStd::make_unique<Instance>(parent);
            Instance::EntityList newEntities;
            if (!PrefabDomUtils::LoadInstanceFromPrefabDom(*newInstance, newEntities, instantiatingTemplate->get().GetPrefabDom()))
            {
                AZ_Error("Prefab", false,
                    "Failed to Load Prefab Template associated with path %s. Instantiation Failed",
                    instantiatingTemplate->get().GetFilePath().c_str());
                return nullptr;
            }

            AzToolsFramework::EditorEntityContextRequestBus::Broadcast(
                &AzToolsFramework::EditorEntityContextRequests::HandleEntitiesAdded, newEntities);

            return newInstance;
        }

        TemplateId PrefabSystemComponent::CreateTemplateFromInstance(Instance& instance, bool shouldCreateLinks)
        {
            // We will register the template to match the path the instance has
            const AZ::IO::Path& templateSourcePath = instance.GetTemplateSourcePath();
            if (templateSourcePath.empty())
            {
                AZ_Assert(false,
                    "PrefabSystemComponent::CreateTemplateFromInstance - "
                    "Attempted to create a prefab template from an instance without a source file path. "
                    "Unable to proceed.");
                return InvalidTemplateId;
            }

            // Convert our instance into a serialized template dom
            PrefabDom serializedInstance;
            if (!PrefabDomUtils::StoreInstanceInPrefabDom(instance, serializedInstance))
            {
                return InvalidTemplateId;
            }

            // Generate a new template and store the dom data
            const AZ::IO::Path& instanceSourcePath = instance.GetTemplateSourcePath();
            TemplateId newTemplateId = AddTemplate(instanceSourcePath, AZStd::move(serializedInstance));
            if (newTemplateId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::CreateTemplateFromInstance - "
                    "Failed to create a template from instance with source file path '%s': "
                    "invalid template id returned.",
                    instanceSourcePath.c_str());

                return InvalidTemplateId;
            }

            if (shouldCreateLinks)
            {
                if (!GenerateLinksForNewTemplate(newTemplateId, instance))
                {
                    // Clear new template and any links associated with it
                    RemoveTemplate(newTemplateId);
                    return InvalidTemplateId;
                }
            }
            return newTemplateId;
        }

        TemplateReference PrefabSystemComponent::FindTemplate(TemplateId id)
        {
            auto found = m_templateIdMap.find(id);
            if (found != m_templateIdMap.end())
            {
                return found->second;
            }
            else
            {
                return AZStd::nullopt;
            }
        }

        PrefabDom& PrefabSystemComponent::FindTemplateDom(TemplateId templateId)
        {
            AZStd::optional<AZStd::reference_wrapper<Template>> findTemplateResult = FindTemplate(templateId);
            AZ_Assert(
                findTemplateResult.has_value(),
                "PrefabSystemComponent::FindTemplateDom - Unable to retrieve Prefab template with id: '%llu'. "
                "Template could not be found",
                templateId);

            AZ_Assert(findTemplateResult->get().IsValid(),
                "PrefabSystemComponent::FindTemplateDom - Unable to retrieve Prefab template with id: '%llu'. "
                "Template is invalid", templateId);

            return findTemplateResult->get().GetPrefabDom();
        }

        LinkReference PrefabSystemComponent::FindLink(const LinkId& id)
        {
            auto found = m_linkIdMap.find(id);
            if (found != m_linkIdMap.end())
            {
                return found->second;
            }
            else
            {
                return AZStd::nullopt;
            }
        }

        TemplateId PrefabSystemComponent::AddTemplate(const AZ::IO::Path& filePath, PrefabDom prefabDom)
        {
            TemplateId newTemplateId = CreateUniqueTemplateId();
            Template& newTemplate = m_templateIdMap.emplace(
                AZStd::make_pair(newTemplateId, AZStd::move(Template(filePath, AZStd::move(prefabDom))))).first->second;

            if (!newTemplate.IsValid())
            {
                AZ_Assert(false,
                    "Prefab - PrefabSystemComponent::AddTemplate - "
                    "Can't add this new Template on file path '%s' since it is invalid.",
                    filePath.c_str());

                m_templateIdMap.erase(newTemplateId);
                return InvalidTemplateId;
            }

            if (!m_templateInstanceMapper.RegisterTemplate(newTemplateId))
            {
                m_templateIdMap.erase(newTemplateId);
                return InvalidTemplateId;
            }

            m_templateFilePathToIdMap.emplace(AZStd::make_pair(filePath, newTemplateId));
            
            return newTemplateId;
        }

        void PrefabSystemComponent::UpdateTemplateFilePath(TemplateId templateId, const AZ::IO::PathView& filePath)
        {
            auto findTemplateResult = FindTemplate(templateId);
            if (!findTemplateResult.has_value())
            {
                AZ_Error(
                    "Prefab", false,
                    "Template associated by given Id '%llu' doesn't exist in PrefabSystemComponent.",
                    templateId);
                return;
            }

            if (!filePath.IsRelative())
            {
                AZ_Error("Prefab", false, "Provided filePath '%.*s' must be relative.", AZ_STRING_ARG(filePath.Native()));
                return;
            }

            Template& templateToChange = findTemplateResult->get();
            if (templateToChange.GetFilePath() == filePath)
            {
                return;
            }
            
            m_templateFilePathToIdMap.erase(templateToChange.GetFilePath());
            if (!m_templateFilePathToIdMap.try_emplace(filePath, templateId).second)
            {
                AZ_Error("Prefab", false, "Provided filePath '%.*s' already exists.", AZ_STRING_ARG(filePath.Native()));
                return;
            }

            PrefabDom& prefabDom = templateToChange.GetPrefabDom();
            PrefabDomValueReference pathReference = Prefab::PrefabDomUtils::FindPrefabDomValue(prefabDom, "Source");
            if (pathReference)
            {
                const AZStd::string_view pathStr = filePath.Native();
                pathReference->get().SetString(pathStr.data(), aznumeric_caster(pathStr.length()), prefabDom.GetAllocator());
            }

            templateToChange.SetFilePath(filePath);
        }

        void PrefabSystemComponent::RemoveTemplate(TemplateId templateId)
        {
            auto findTemplateResult = FindTemplate(templateId);
            if (!findTemplateResult.has_value())
            {
                AZ_Warning("Prefab", false,
                    "PrefabSystemComponent::RemoveTemplate - "
                    "Template associated by given Id '%llu' doesn't exist in PrefabSystemComponent.",
                    templateId);

                return;
            }
            
            //Remove all Links owned by the Template from TemplateToLinkIdsMap.
            Template& templateToDelete = findTemplateResult->get();
            const Template::Links& linkIdsToDelete = templateToDelete.GetLinks();
            bool result;
            for (auto linkId : linkIdsToDelete)
            {
                result = RemoveLinkIdFromTemplateToLinkIdsMap(linkId);
                AZ_Assert(result,
                    "Prefab - PrefabSystemComponent::RemoveTemplate - "
                    "Failed to remove Link with Id '%llu' owned by the Template with Id '%llu' on file path '%s' "
                    "from TemplateToLinkIdsMap.",
                    linkId, templateId, templateToDelete.GetFilePath().c_str());
            }

            //Remove all Links that depend on this source Template from other target Templates.
            //Also remove this Template from TemplateToLinkIdsMap.
            auto templateToLinkIterator = m_templateToLinkIdsMap.find(templateId);
            if (templateToLinkIterator != m_templateToLinkIdsMap.end())
            {
                for (auto linkId : templateToLinkIterator->second)
                {
                    result = RemoveLinkFromTargetTemplate(linkId);
                    AZ_Assert(result,
                        "Prefab - PrefabSystemComponent::RemoveTemplate - "
                        "Failed to remove Link with Id '%llu' that depend on the source Template with Id '%llu' on file path '%s'.",
                        linkId, templateId, templateToDelete.GetFilePath().c_str());
                }

                result = m_templateToLinkIdsMap.erase(templateToLinkIterator) != nullptr;
                AZ_Assert(result,
                    "Prefab - PrefabSystemComponent::RemoveTemplate - "
                    "Failed to remove Template with Id '%llu' on file path '%s' "
                    "from TemplateToLinkIdsMap.",
                    templateId, templateToDelete.GetFilePath().c_str());
            }

            //Remove this Template from the rest of the maps.
            result = m_templateFilePathToIdMap.erase(templateToDelete.GetFilePath()) != 0;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveTemplate - "
                "Failed to remove Template with Id '%llu' on file path '%s' "
                "from Template File Path To Id Map.",
                templateId, templateToDelete.GetFilePath().c_str());

            m_templateInstanceMapper.UnregisterTemplate(templateId);
            
            result = m_templateIdMap.erase(templateId) != 0;
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveTemplate - "
                "Failed to remove Template with Id '%llu' on file path '%s' "
                "from Template Id Map.",
                templateId, templateToDelete.GetFilePath().c_str());

            return;
        }

        void PrefabSystemComponent::RemoveAllTemplates()
        {
            AZStd::vector<TemplateId> templateIds;
            templateIds.reserve(m_templateIdMap.size());

            // Make a copy of the keys, we don't want to iterate over the map while we're removing items from it
            for (const auto& [id, templateObject] : m_templateIdMap)
            {
                templateIds.emplace_back(id);
            }

            for (auto id : templateIds)
            {
                RemoveTemplate(id);
            }
        }

        LinkId PrefabSystemComponent::AddLink(
            TemplateId sourceTemplateId,
            TemplateId targetTemplateId,
            PrefabDomValue::MemberIterator& instanceIterator,
            InstanceOptionalReference instance)
        {
            TemplateReference sourceTemplateReference = FindTemplate(sourceTemplateId);
            TemplateReference targetTemplateReference = FindTemplate(targetTemplateId);
            if (!sourceTemplateReference.has_value() || !targetTemplateReference.has_value())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::AddLink - "
                    "Can't find both source Template '%u' and target Template '%u' in Prefab System Component.",
                    sourceTemplateId, targetTemplateId);
                return false;
            }

            Template& targetTemplate = targetTemplateReference->get();

#if defined(AZ_ENABLE_TRACING)
            Template& sourceTemplate = sourceTemplateReference->get();
            AZStd::string_view instanceName(instanceIterator->name.GetString(), instanceIterator->name.GetStringLength());

            const AZStd::string& targetTemplateFilePath = targetTemplate.GetFilePath().Native();
            const AZStd::string& sourceTemplateFilePath = sourceTemplate.GetFilePath().Native();
#endif

            LinkId newLinkId = CreateUniqueLinkId();
            Link newLink(newLinkId);
            if (!ConnectTemplates(newLink, sourceTemplateId, targetTemplateId, instanceIterator))
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::AddLink - "
                    "New Link to Nested Template Instance '%.*s' connecting source Template '%s' and target Template '%s' failed.",
                    aznumeric_cast<int>(instanceName.size()), instanceName.data(),
                    sourceTemplateFilePath.c_str(),
                    targetTemplateFilePath.c_str());

                return InvalidLinkId;
            }

            if (!newLink.IsValid())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::AddLink - "
                    "New Link to Nested Template Instance '%.*s' connecting source Template '%s' and target Template '%s' is invalid.",
                    aznumeric_cast<int>(instanceName.size()), instanceName.data(),
                    sourceTemplateFilePath.c_str(),
                    targetTemplateFilePath.c_str());

                return InvalidLinkId;
            }

            if (instance != AZStd::nullopt)
            {
                instance->get().SetLinkId(newLinkId);
            }

            m_linkIdMap.emplace(AZStd::make_pair(newLinkId, AZStd::move(newLink)));

            targetTemplate.AddLink(newLinkId);
            m_templateToLinkIdsMap[sourceTemplateId].emplace(newLinkId);

            return newLinkId;
        }

        LinkId PrefabSystemComponent::CreateLink(
            TemplateId linkTargetId,
            TemplateId linkSourceId,
            const InstanceAlias& instanceAlias,
            const PrefabDomConstReference linkPatches,
            const LinkId& linkId)
        {
            if (linkTargetId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false, "Invalid Link Target Template Id");
            }

            TemplateReference targetTemplateRef = FindTemplate(linkTargetId);

            if (targetTemplateRef == AZStd::nullopt)
            {
                AZ_Error("Prefab", false, "Link Target Template not found");
            }

            if (linkSourceId == InvalidTemplateId)
            {
                AZ_Error("Prefab", false, "Invalid Link Source Template Id");
            }

            TemplateReference sourceTemplateRef = FindTemplate(linkSourceId);

            if (sourceTemplateRef == AZStd::nullopt)
            {
                AZ_Error("Prefab", false, "Link Source Template not found");
            }

            //use an existing link id if provided
            LinkId newLinkId = linkId;
            if (newLinkId == InvalidLinkId)
            {
                newLinkId = CreateUniqueLinkId();
            }

            //get owner template and add the link
            Template& targetTemplate = targetTemplateRef->get();

            if (!targetTemplate.AddLink(newLinkId))
            {
                AZ_Error("Prefab", false, "Failed to add link id '%llu' to '%s'", newLinkId, targetTemplate.GetFilePath().c_str());
            }

            //insert nested instance alias into the template owner dom
            PrefabDom& targetTemplateDom = targetTemplate.GetPrefabDom();
            auto memberFound = targetTemplateDom.FindMember(PrefabDomUtils::InstancesName);

            PrefabDomValueReference instancesValue;
            if (memberFound == targetTemplateDom.MemberEnd())
            {
                //add the instance alias to the template dom
                instancesValue = targetTemplateDom.AddMember(rapidjson::StringRef(PrefabDomUtils::InstancesName),
                    PrefabDomValue(),
                    targetTemplateDom.GetAllocator());

                //when AddMember returns, it returns the object that the member was added to, not the added
                //member itself, so we need to move instancesValue to the correct position for the next insert
                memberFound = instancesValue->get().FindMember(PrefabDomUtils::InstancesName);
                instancesValue = memberFound->value;
            }
            else
            {
                instancesValue = memberFound->value;
            }

            if (!instancesValue->get().IsObject())
            {
                instancesValue->get().SetObject();
            }
            // Only add the instance if it's not there already
            if (instancesValue->get().FindMember(rapidjson::StringRef(instanceAlias.c_str())) == instancesValue->get().MemberEnd())
            {
                instancesValue->get().AddMember(
                    rapidjson::Value(instanceAlias.c_str(), targetTemplateDom.GetAllocator()), PrefabDomValue(),
                    targetTemplateDom.GetAllocator());
            }

            Template& sourceTemplate = sourceTemplateRef->get();

            //setup initial link values and link dom
            Link newLink(newLinkId);
            newLink.SetTargetTemplateId(linkTargetId);
            newLink.SetSourceTemplateId(linkSourceId);
            newLink.SetInstanceName(instanceAlias.c_str());
            newLink.GetLinkDom().SetObject();
            newLink.GetLinkDom().AddMember(
                rapidjson::StringRef(PrefabDomUtils::SourceName), rapidjson::StringRef(sourceTemplate.GetFilePath().c_str()),
                newLink.GetLinkDom().GetAllocator());

            if (linkPatches && linkPatches->get().IsArray() && !(linkPatches->get().Empty()))
            {
                m_instanceToTemplatePropagator.AddPatchesToLink(linkPatches.value(), newLink);
            }

            //update the target template dom to have the proper values for the source template dom
            if (!newLink.UpdateTarget())
            {
                AZ_Error("Prefab", false, "Failed to update link with template information");
            }

            //add the link to the link maps
            m_linkIdMap.emplace(AZStd::make_pair(newLinkId, AZStd::move(newLink)));
            m_templateToLinkIdsMap[linkSourceId].emplace(newLinkId);

            return newLinkId;
        }

        void PrefabSystemComponent::RemoveLink(const LinkId& linkId)
        {
            auto findLinkResult = FindLink(linkId);
            if (!findLinkResult.has_value())
            {
                AZ_Warning("Prefab", false,
                    "PrefabSystemComponent::RemoveLink - "
                    "Link associated by given Id '%llu' doesn't exist in PrefabSystemComponent.",
                    linkId);

                return;
            }

            Link& link = findLinkResult->get();
            bool result;
            result = RemoveLinkIdFromTemplateToLinkIdsMap(linkId, link);
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveLink - "
                "Failed to remove Link with Id '%llu' for Instance '%s' of source Template with Id '%llu' "
                "from TemplateToLinkIdsMap.",
                linkId, link.GetInstanceName().c_str(), link.GetSourceTemplateId());

            result = RemoveLinkFromTargetTemplate(linkId, link);
            AZ_Assert(result,
                "Prefab - PrefabSystemComponent::RemoveLink - "
                "Failed to remove Link with Id '%llu' for Instance '%s' of source Template with Id '%llu' "
                "from target Template with Id '%llu'.",
                linkId, link.GetInstanceName().c_str(), link.GetSourceTemplateId(), link.GetTargetTemplateId());

            m_linkIdMap.erase(linkId);

            return;
        }

        TemplateId PrefabSystemComponent::GetTemplateIdFromFilePath(AZ::IO::PathView filePath) const
        {
            AZ_Assert(!filePath.IsAbsolute(), "Prefab - GetTemplateIdFromFilePath was passed an absolute path. Prefabs use paths relative to the project folder.");

            auto found = m_templateFilePathToIdMap.find(filePath);
            if (found != m_templateFilePathToIdMap.end())
            {
                return found->second;
            }
            else
            {
                return InvalidTemplateId;
            }
        }

        bool PrefabSystemComponent::IsTemplateDirty(TemplateId templateId)
        {
            auto templateRef = FindTemplate(templateId);

            if (templateRef.has_value())
            {
                return templateRef->get().IsDirty();
            }

            return false;
        }

        void PrefabSystemComponent::SetTemplateDirtyFlag(TemplateId templateId, bool dirty)
        {
            if (auto templateReference = FindTemplate(templateId); templateReference.has_value())
            {
                templateReference->get().MarkAsDirty(dirty);

                PrefabPublicNotificationBus::Broadcast(
                    &PrefabPublicNotificationBus::Events::OnPrefabTemplateDirtyFlagUpdated, templateId, dirty);
            }
        }

        bool PrefabSystemComponent::AreDirtyTemplatesPresent(TemplateId rootTemplateId)
        {
            TemplateReference prefabTemplate = FindTemplate(rootTemplateId);

            if (!prefabTemplate.has_value())
            {
                AZ_Assert(false, "Template with id %llu is not found", rootTemplateId);
                return false;
            }

            if (IsTemplateDirty(rootTemplateId))
            {
                return true;
            }

            const Template::Links& linkIds = prefabTemplate->get().GetLinks();

            for (LinkId linkId : linkIds)
            {
                auto linkIterator = m_linkIdMap.find(linkId);
                if (linkIterator != m_linkIdMap.end())
                {
                    if (AreDirtyTemplatesPresent(linkIterator->second.GetSourceTemplateId()))
                    {
                        return true;
                    }
                    else
                    {
                        continue;
                    }
                }
            }
            return false;
        }

        void PrefabSystemComponent::SaveAllDirtyTemplates(TemplateId rootTemplateId)
        {
            AZStd::set<AZ::IO::PathView> dirtyTemplatePaths = GetDirtyTemplatePaths(rootTemplateId);  

            for (AZ::IO::PathView dirtyTemplatePath : dirtyTemplatePaths)
            {
                auto dirtyTemplateIterator = m_templateFilePathToIdMap.find(dirtyTemplatePath);
                if (dirtyTemplateIterator == m_templateFilePathToIdMap.end())
                {
                    AZ_Assert(false, "Template id for template with path '%s' is not found.", dirtyTemplatePath);
                }
                else
                {
                    m_prefabLoader.SaveTemplate(dirtyTemplateIterator->second);
                }
            }
        }

        AZStd::set<AZ::IO::PathView> PrefabSystemComponent::GetDirtyTemplatePaths(TemplateId rootTemplateId)
        {
            AZStd::vector<AZ::IO::PathView> dirtyTemplatePathVector;
            GetDirtyTemplatePathsHelper(rootTemplateId, dirtyTemplatePathVector);
            AZStd::set<AZ::IO::PathView> dirtyTemplatePaths;
            dirtyTemplatePaths.insert(dirtyTemplatePathVector.begin(), dirtyTemplatePathVector.end());
            return AZStd::move(dirtyTemplatePaths);
        }

        void PrefabSystemComponent::GetDirtyTemplatePathsHelper(
            TemplateId rootTemplateId, AZStd::vector<AZ::IO::PathView>& dirtyTemplatePaths)
        {
            TemplateReference prefabTemplate = FindTemplate(rootTemplateId);

            if (!prefabTemplate.has_value())
            {
                AZ_Assert(false, "Template with id %llu is not found", rootTemplateId);
                return;
            }

            if (IsTemplateDirty(rootTemplateId))
            {
                dirtyTemplatePaths.emplace_back(prefabTemplate->get().GetFilePath());
            }

            const Template::Links& linkIds = prefabTemplate->get().GetLinks();

            for (LinkId linkId : linkIds)
            {
                auto linkIterator = m_linkIdMap.find(linkId);
                if (linkIterator != m_linkIdMap.end())
                {
                    GetDirtyTemplatePathsHelper(linkIterator->second.GetSourceTemplateId(), dirtyTemplatePaths);
                }
            }
        }

        bool PrefabSystemComponent::ConnectTemplates(
            Link& link,
            TemplateId sourceTemplateId,
            TemplateId targetTemplateId,
            PrefabDomValue::MemberIterator& instanceIterator)
        {
            TemplateReference sourceTemplateReference = FindTemplate(sourceTemplateId);
            TemplateReference targetTemplateReference = FindTemplate(targetTemplateId);
            if (!sourceTemplateReference.has_value() || !targetTemplateReference.has_value())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::ConnectTemplates - "
                    "Can't find both source Template '%u' and target Template '%u' in Prefab System Component.",
                    sourceTemplateId, targetTemplateId);
                return false;
            }

#if defined(AZ_ENABLE_TRACING)
            Template& sourceTemplate = sourceTemplateReference->get();
            Template& targetTemplate = targetTemplateReference->get();
#endif

            AZStd::string_view instanceName(instanceIterator->name.GetString(), instanceIterator->name.GetStringLength());

            link.SetSourceTemplateId(sourceTemplateId);
            link.SetTargetTemplateId(targetTemplateId);
            link.SetInstanceName(instanceName.data());

            PrefabDomValue& instance = instanceIterator->value;
            AZ_Assert(instance.IsObject(), "Nested instance DOM provided is not a valid JSON object.");
            [[maybe_unused]] PrefabDomValueReference sourceTemplateName = PrefabDomUtils::FindPrefabDomValue(instance, PrefabDomUtils::SourceName);
            AZ_Assert(sourceTemplateName, "Couldn't find source template name in the DOM of the nested instance while creating a link.");
            AZ_Assert(sourceTemplateName->get() == sourceTemplate.GetFilePath().c_str(),
                "The name of the source template in the nested instance DOM does not match the name of the source template already loaded");

            PrefabDomValueReference patchesReference = PrefabDomUtils::FindPrefabDomValue(instance, PrefabDomUtils::PatchesName);
            if (patchesReference.has_value())
            {
                AZ_Assert(patchesReference->get().IsArray(), "Patches in the nested instance DOM are not represented as an array.");
            }

            link.SetLinkDom(instance);

            if (!link.UpdateTarget())
            {
                AZ_Error("Prefab", false,
                    "PrefabSystemComponent::ConnectTemplates - "
                    "Failed to update the linked instance with source Prefab file '%s' and target Prefab file '%s'.",
                    sourceTemplate.GetFilePath().c_str(), targetTemplate.GetFilePath().c_str());
                return false;
            }

            link.AddLinkIdToInstanceDom(instance);
            return true;
        }

        bool PrefabSystemComponent::GenerateLinksForNewTemplate(TemplateId newTemplateId, Instance& instance)
        {
            TemplateReference newTemplateReference = FindTemplate(newTemplateId);
            if (!newTemplateReference.has_value())
            {
                return false;
            }

            Template& newTemplate = newTemplateReference->get();

            // Gather the Instances member from the template DOM
            PrefabDomValueReference instancesReference = newTemplate.GetInstancesValue();

            // No nested instances to perform link operations on
            if (instancesReference == AZStd::nullopt)
            {
                return true;
            }

            PrefabDomValue& instances = instancesReference->get();

            for (PrefabDomValue::MemberIterator instanceIterator = instances.MemberBegin(); instanceIterator != instances.MemberEnd(); ++instanceIterator)
            {
                // Acquire the source member of the nested template so we can get its template id
                // and join it with our new template via a link
                PrefabDomValueReference instanceSourceReference =
                    PrefabDomUtils::FindPrefabDomValue(instanceIterator->value, PrefabDomUtils::SourceName);

                if (!instanceSourceReference.has_value() || !instanceSourceReference->get().IsString())
                {
                    AZ_Error("Prefab", false,
                        "PrefabSystemComponent::GenerateLinksForNewTemplate - "
                        "Failed to acquire the source path value of a nested instance during creation of a new template associated with prefab %s"
                        "Unable to proceed",
                        newTemplate.GetFilePath().c_str());

                    return false;
                }

                const PrefabDomValue& source = instanceSourceReference->get();
                TemplateId nestedTemplateId = GetTemplateIdFromFilePath(source.GetString());
                if (nestedTemplateId == InvalidTemplateId)
                {
                    AZ_Error("Prefab", false,
                        "PrefabSystemComponent::GenerateLinksForNewTemplate - "
                        "Nested Template Instance with prefab path %s is not registered with the prefab system component"
                        "Unable to acquire its template id"
                        "Unable to complete creation of Template with prefab path %s",
                        newTemplate.GetFilePath().c_str());

                    return false;
                }

                // Construct and store the new link in our mapping.
                AZStd::string_view nestedTemplatePath(source.GetString(), source.GetStringLength());

                InstanceAlias instanceAlias = instanceIterator->name.GetString();
                LinkId newLinkId = AddLink(nestedTemplateId, newTemplateId, instanceIterator, instance.FindNestedInstance(instanceAlias));
                if (newLinkId == InvalidLinkId)
                {
                    AZ_Error("Prefab", false,
                        "PrefabSystemComponent::GenerateLinksForNewTemplate - "
                        "Failed to add a new Link to Nested Template Instance '%s' which connects source Template '%.*s' and target Template '%s'.",
                        instanceIterator->name.GetString(),
                        aznumeric_cast<int>(nestedTemplatePath.size()), nestedTemplatePath.data(),
                        newTemplate.GetFilePath().c_str());

                    return false;
                }
            }

            return true;
        }

        TemplateId PrefabSystemComponent::CreateUniqueTemplateId()
        {
            return m_templateIdCounter++;
        }

        LinkId PrefabSystemComponent::CreateUniqueLinkId()
        {
            return m_linkIdCounter++;
        }

        bool PrefabSystemComponent::RemoveLinkIdFromTemplateToLinkIdsMap(const LinkId& linkId)
        {
            auto findLinkResult = FindLink(linkId);
            if (!findLinkResult.has_value())
            {
                return false;
            }

            Link& link = findLinkResult->get();
            return RemoveLinkIdFromTemplateToLinkIdsMap(linkId, link);
        }

        bool PrefabSystemComponent::RemoveLinkIdFromTemplateToLinkIdsMap(const LinkId& linkId, const Link& link)
        {
            TemplateId sourceTemplateId = link.GetSourceTemplateId();
            auto templateToLinkIterator = m_templateToLinkIdsMap.find(sourceTemplateId);
            bool removed = false;
            if (templateToLinkIterator != m_templateToLinkIdsMap.end())
            {
                removed = templateToLinkIterator->second.erase(linkId) != 0;
                if (templateToLinkIterator->second.empty())
                {
                    m_templateToLinkIdsMap.erase(templateToLinkIterator);
                }
            }

            return removed;
        }

        bool PrefabSystemComponent::RemoveLinkFromTargetTemplate(const LinkId& linkId)
        {
            auto findLinkResult = FindLink(linkId);
            if (!findLinkResult.has_value())
            {
                return false;
            }

            Link& link = findLinkResult->get();
            return RemoveLinkFromTargetTemplate(linkId, link);
        }

        bool PrefabSystemComponent::RemoveLinkFromTargetTemplate(const LinkId& linkId, const Link& link)
        {
            TemplateId targetTemplateId = link.GetTargetTemplateId();

            auto templateIterator = m_templateIdMap.find(targetTemplateId);
            bool removed = false;
            if (templateIterator != m_templateIdMap.end())
            {
                removed = templateIterator->second.RemoveLink(linkId);

                //remove link
                PrefabDomValueReference templateInstancesRef = templateIterator->second.GetInstancesValue();
                if (templateInstancesRef == AZStd::nullopt)
                {
                    AZ_Error("Prefab", false, "Failed to get template reference");
                    return false;
                }

                removed = templateInstancesRef->get().RemoveMember(link.GetInstanceName().c_str())
                    ? removed : false;
            }

            return removed;
        }

    } // namespace Prefab
} // namespace AzToolsFramework
