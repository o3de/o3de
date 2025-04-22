/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/Entity/EntityTypes.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapper.h>
#include <AzToolsFramework/Prefab/Instance/InstanceUpdateExecutor.h>
#include <AzToolsFramework/Prefab/Instance/InstanceToTemplatePropagator.h>
#include <AzToolsFramework/Prefab/Instance/TemplateInstanceMapper.h>
#include <AzToolsFramework/Prefab/Link/Link.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>
#include <AzToolsFramework/Prefab/PrefabLoader.h>
#include <AzToolsFramework/Prefab/PrefabPublicHandler.h>
#include <AzToolsFramework/Prefab/PrefabPublicRequestHandler.h>
#include <AzToolsFramework/Prefab/PrefabSystemComponentInterface.h>
#include <AzToolsFramework/Prefab/Template/Template.h>
#include <Prefab/PrefabSystemScriptingHandler.h>

AZ_DECLARE_BUDGET(PrefabSystem);

namespace AZ
{
    class Entity;
    class SerializeContext;
} // namespace AZ

namespace AzToolsFramework
{
    namespace Prefab
    {
        using InstanceList = AZStd::vector<AzToolsFramework::Prefab::Instance*>;

        using LinkIds = AZStd::vector<LinkId>;
        using LinkIdSet = AZStd::unordered_set<LinkId>;

        /**
        * The prefab system component provides a central point for owning manipulating prefabs.
        */
        class PrefabSystemComponent
            : public AZ::Component
            , private PrefabSystemComponentInterface
            , private AZ::SystemTickBus::Handler
            , private AzToolsFramework::AssetBrowser::AssetBrowserFileActionNotificationBus::Handler
        {
        public:

            using TargetTemplateIdToLinkIdMap = AZStd::unordered_map<TemplateId, AZStd::pair<AZStd::unordered_set<LinkId>, bool>>;

            AZ_COMPONENT(PrefabSystemComponent, "{27203AE6-A398-4614-881B-4EEB5E9B34E9}");

            PrefabSystemComponent() = default;

            virtual ~PrefabSystemComponent() = default;

            //////////////////////////////////////////////////////////////////////////
            // AZ::Component overrides
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            static void Reflect(AZ::ReflectContext* context);
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

            // SystemTickBus...
            void OnSystemTick() override;

            // AssetBrowserSourceActionNotificationBus...
            void OnSourceFilePathNameChanged(const AZStd::string_view fromPathName, const AZStd::string_view toPathName) override;
            void OnSourceFolderPathNameChanged(const AZStd::string_view fromPathName, const AZStd::string_view toPathName) override;

            //////////////////////////////////////////////////////////////////////////
            // PrefabSystemComponentInterface interface implementation
            /**
             * Find Template with given Template id from Prefab System Component.
             * @param id A unique id of a Template.
             * @return Reference of Template if the Template exists.
             */
            TemplateReference FindTemplate(TemplateId id) override;

            /**
             * Find Link with given Link id from Prefab System Component.
             * @param id A unique id of a Link.
             * @return Reference of Link if the Link exists.
             */
            LinkReference FindLink(const LinkId& id) override;

            /**
            * Add a new Template into Prefab System Component and create a unique id for it.
            * @param filePath A file path where the Template file is located.
            * @param prefabDom A Prefab DOM presenting this Template.
            * @return A unique id for the new Template.
            */
            TemplateId AddTemplate(const AZ::IO::Path& filePath, PrefabDom prefabDom) override;

            /**
             * Updates relative filepath location of a prefab (in case of SaveAs operation).
             * @param templateId An id of a Template to change filepath of.
             * @param filePath new relative path of the Template.
             */
            void UpdateTemplateFilePath(TemplateId templateId, const AZ::IO::PathView& filePath) override;

            /**
            * Remove the Template associated with the given id from Prefab System Component.
            * @param templateId A unique id of a Template.
            */
            void RemoveTemplate(TemplateId templateId) override;

            /**
             * Remove all Templates from the Prefab System Component.
             */
            void RemoveAllTemplates() override;

            /**
            * Generates a new Prefab Instance based on the Template whose source is stored in filepath.
            * @param filePath The path to the prefab source file containing the template being instantiated.
            * @param parent Reference of the target instance the instantiated instance will be placed under.
            * @param instantiatedEntitiesCallback An optional callback that can be used to modify the instantiated entities.
            * @return A unique_ptr to the newly instantiated instance. Null if operation failed.
            */
            AZStd::unique_ptr<Instance> InstantiatePrefab(
                AZ::IO::PathView filePath,
                InstanceOptionalReference parent = AZStd::nullopt,
                const InstantiatedEntitiesCallback& instantiatedEntitiesCallback = {}) override;

            /**
            * Generates a new Prefab Instance based on the Template referenced by templateId.
            * @param templateId The id of the template being instantiated.
            * @param parent Reference of the target instance the instantiated instance will be placed under.
            * @param instantiatedEntitiesCallback An optional callback that can be used to modify the instantiated entities.
            * @return A unique_ptr to the newly instantiated instance. Null if operation failed.
            */
            AZStd::unique_ptr<Instance> InstantiatePrefab(
                TemplateId templateId,
                InstanceOptionalReference parent = AZStd::nullopt,
                const InstantiatedEntitiesCallback& instantiatedEntitiesCallback = {}) override;

            /**
            * Add a new Link into Prefab System Component and create a unique id for it.
            * @param sourceTemplateId The Id of Template whose instance is referred by targetTemplate.
            * @param targetTemplateId The Id of Template which owns this Link.
            * @param instanceIterator The Prefab DOM value iterator that points to the instance associated by this Link.
            * @param instance The instance that this link may point to. This is optional and can also take the value of nullopt.
            * @return A unique id for the new Link.
            */
            LinkId AddLink(
                TemplateId sourceTemplateId,
                TemplateId targetTemplateId,
                PrefabDomValue::MemberIterator& instanceIterator,
                InstanceOptionalReference instance) override;

            /**
            * Create a new Link with Prefab System Component and create a unique id for it.
            * @param linkTargetId The Id of Template which owns this Link.
            * @param linkSourceId The Id of Template whose instance is referred by targetTemplate.
            * @param instanceAlias The alias of the instance that should be included in link.
            * @param linkId The id of the link.  If invalid, create a new link id.  If valid, use to recreate
            * a prior link.
            * @return A unique id for the new Link.
            */
            LinkId CreateLink(
                TemplateId linkTargetId,
                TemplateId linkSourceId,
                const InstanceAlias& instanceAlias,
                const PrefabDomConstReference linkPatches,
                const LinkId& linkId = InvalidLinkId) override;

            /**
            * Remove the Link associated with the given id from Prefab System Component.
            * @param linkId A unique id of a Link.
            * @return whether link was successfully removed or not.
            */
            bool RemoveLink(const LinkId& linkId) override;

            /**
             * Get id of Template on given file path if it has already been loaded into Prefab System Component.
             * @param filePath A path to a Prefab Template file.
             * @return A unique id of Template on filePath. Return InvalidTemplateId if Template on filePath doesn't exist.
             */
            TemplateId GetTemplateIdFromFilePath(AZ::IO::PathView filePath) const override;

            /**
             * Gets the value of the dirty flag of the template with the id provided.
             * @param templateId The id of the template to query.
             * @return The value of the dirty flag on the template.
             */
            bool IsTemplateDirty(TemplateId templateId) override;

            /**
             * Sets the dirty flag of the template to the value provided.
             * @param templateId The id of the template to flag.
             * @param dirty The new value of the dirty flag.
             */
            void SetTemplateDirtyFlag(TemplateId templateId, bool dirty) override;

            bool AreDirtyTemplatesPresent(TemplateId rootTemplateId) override;

            void SaveAllDirtyTemplates(TemplateId rootTemplateId) override;

            AZStd::set<AZ::IO::PathView> GetDirtyTemplatePaths(TemplateId rootTemplateId) override;

            //////////////////////////////////////////////////////////////////////////

            /**
            * Builds a new Prefab Template out of entities and instances and returns the first instance comprised of
            * these entities and instances.
            * @param entities A vector of entities that will be used in the new instance. May be empty.
            * @param nestedInstances A vector of Prefab Instances that will be nested in the new instance, will be consumed and moved.
            *                  May be empty.
            * @param filePath The path to associate the template of the new instance to.
            * @param containerEntity The container entity for the prefab to be created. It will be created if a nullptr is provided.
            * @param parent Reference of an instance the created instance will be placed under, if given.
            * @param shouldCreateLinks The flag indicating if links should be created between the templates of the instance
            *        and its nested instances.
            * @return A pointer to the newly created instance. nullptr on failure.
            */
            AZStd::unique_ptr<Instance> CreatePrefab(
                const AZStd::vector<AZ::Entity*>& entities, AZStd::vector<AZStd::unique_ptr<Instance>>&& nestedInstances,
                AZ::IO::PathView filePath, AZStd::unique_ptr<AZ::Entity> containerEntity = nullptr,
                InstanceOptionalReference parent = AZStd::nullopt, bool shouldCreateLinks = true) override;

            AZStd::unique_ptr<Instance> CreatePrefabWithCustomEntityAliases(
                const AZStd::map<EntityAlias, AZ::Entity*>& entities,
                AZStd::vector<AZStd::unique_ptr<Instance>>&& nestedInstances,
                AZ::IO::PathView filePath,
                AZStd::unique_ptr<AZ::Entity> containerEntity = nullptr,
                InstanceOptionalReference parent = AZStd::nullopt,
                bool shouldCreateLinks = true) override;

            PrefabDom& FindTemplateDom(TemplateId templateId) override;

            /**
             * Updates a template with the given updated DOM.
             *
             * @param templateId The id of the template to update.
             * @param updatedDom The DOM to update the template with.
             */
            void UpdatePrefabTemplate(TemplateId templateId, const PrefabDom& updatedDom) override;

            void PropagateTemplateChanges(TemplateId templateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt) override;

            /**
             * Updates all Instances owned by a Template.
             *
             * @param templateId The id of the Template owning Instances to update.
             * @param instanceToExclude An optional reference to an instance of the template being updated that should not be refreshed
             *        as part of propagation.Defaults to nullopt, which means that all instances will be refreshed.
             */
            void UpdatePrefabInstances(TemplateId templateId, InstanceOptionalConstReference instanceToExclude = AZStd::nullopt);

        private:
            AZ_DISABLE_COPY_MOVE(PrefabSystemComponent);

            /**
            * Builds a new Prefab Template out of entities and instances and returns the first instance comprised of
            * these entities and instances.
            * @param entities A vector of entities that will be used in the new instance. May be empty.
            * @param nestedInstances A vector of Prefab Instances that will be nested in the new instance, will be consumed and moved.
            *                  May be empty.
            * @param filePath The path to associate the template of the new instance to.
            * @param instance Reference of a pointer to the newly created instance which needs initiation.
            * @param shouldCreateLinks The flag indicating if links should be created between the templates of the instance
            *        and its nested instances.
            */
            void CreatePrefab(
                const AZStd::map<EntityAlias, AZ::Entity*>& entities,
                AZStd::vector<AZStd::unique_ptr<Instance>>&& nestedInstances, AZ::IO::PathView filePath,
                AZStd::unique_ptr<Instance>& instance, bool shouldCreateLinks);

            /**
             * Updates all the linked Instances corresponding to the linkIds in the provided queue.
             * Queue gets populated with more linkId lists as linked instances are updated. Updating stops when the queue is empty.
             *
             * @param linkIdsQueue A queue of vector of link-Ids to update.
             */
            void UpdateLinkedInstances(AZStd::queue<LinkIds>& linkIdsQueue);

            /**
             * Given a vector of link ids to update, splits them into smaller lists based on the target template id of the links.
             *
             * @param linkIdsToUpdate The list of link ids to update.
             * @param targetTemplateIdToLinkIdMap The map of target templateIds to a pair of lists of linkIds and a bool flag indicating
             *                                    whether any of the instances of the target template were updated.
             */
            void BucketLinkIdsByTargetTemplateId(LinkIds& linkIdsToUpdate,
                TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap);

            /**
             * Updates a single linked instance corresponding to the given link Id and adds more linkIds to the
             * template change propagation queue(linkIdsQueue) when necessary.
             *
             * @param linkIdToUpdate The id of the linked instance to update
             * @param targetTemplateIdToLinkIdMap The map of target templateIds to a pair of lists of linkIds and a bool flag indicating
             *                                    whether any of the instances of the target template were updated.
             * @param linkIdsQueue A queue of vector of link-Ids to update.
             */
            void UpdateLinkedInstance(const LinkId linkIdToUpdate, TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap,
                AZStd::queue<LinkIds>& linkIdsQueue);

            /**
             * If all linked instances of a target template are updated and if the content of any of the linked instances changed,
             * this method fetches all the linked instances sourced by it and adds their corresponding ids to the LinkIdsQueue.
             *
             * @param targetTemplateIdToLinkIdMap The map of target templateIds to a pair of lists of linkIds and a bool flag indicating
             *                                    whether any of the instances of the target template were updated.
             * @param targetTemplateId The id of the template, whose linked instances we need to find if the template was updated.
             * @param linkIdsQueue A queue of vector of link-Ids to update.
             */
            void UpdateTemplateChangePropagationQueue(TargetTemplateIdToLinkIdMap& targetTemplateIdToLinkIdMap,
                const TemplateId targetTemplateId, AZStd::queue<LinkIds>& linkIdsQueue);

            /**
            * Takes a prefab instance and generates a new Prefab Template
            * along with any new Prefab Links representing any of the nested instances present
            * @param instance The instance used to generate the new Template.
            * @param shouldCreateLinks The flag indicating if links should be created between the templates of the instance
            *        and its nested instances.
            */
            TemplateId CreateTemplateFromInstance(Instance& instance, bool shouldCreateLinks);

            /**
             * Connect two templates with given link, and a nested instance value iterator
             * in target template's Prefab DOM.
             * @param newLink A Link used to fill in data for connecting the link with Templates.
             * @param sourceTemplateId A unique id of source Template.
             * @param targetTemplateId A unique id of target Template.
             * @param instanceIterator A nested instance value iterator from targetTemplate.
             * @return If newLink connects sourceTemplate and targetTemplate successfully.
             */
            bool ConnectTemplates(
                Link& link,
                TemplateId sourceTemplateId,
                TemplateId targetTemplateId,
                PrefabDomValue::MemberIterator& instanceIterator);

            /**
            * Given a template this will traverse any nested instances found in its Prefab Dom
            * and generate empty Links connecting the source template of each instance to the template passed in.
            * @param newTemplateId The unique Id of the Template to receive Links
            * @param instance The instance that the template was created from. This needs to be editable for inserting linkId into it.
            * @return bool on whether the operation succeeded
            */
            bool GenerateLinksForNewTemplate(TemplateId newTemplateId, Instance& instance);

            /**
             * Create a unique Template id for newly created Template.
             * @return A new Template id.
             */
            TemplateId CreateUniqueTemplateId();

            /**
             * Create a unique Link id for newly created Link.
             * @return A new Link id.
             */
            LinkId CreateUniqueLinkId();

            /**
             * Remove given Link Id from source Template to Link Ids mapping.
             * @return bool on whether the operation succeeded.
             */
            bool RemoveLinkIdFromTemplateToLinkIdsMap(const LinkId& linkId);

            /**
             * Given Link and its Id, remove the Id from source Template to Link Ids mapping.
             * @return bool on whether the operation succeeded.
             */
            bool RemoveLinkIdFromTemplateToLinkIdsMap(const LinkId& linkId, const Link& link);

            /**
             * Remove given Link Id from the Link's target Template.
             * @return bool on whether the operation succeeded.
             */
            bool RemoveLinkFromTargetTemplate(const LinkId& linkId);

            /**
             * Given Link and its Id, remove the link from the target Template..
             * @return bool on whether the operation succeeded.
             */
            bool RemoveLinkFromTargetTemplate(const LinkId& linkId, const Link& link);

            // Helper function for GetDirtyTemplatePaths(). It uses vector to speed up iteration times.
            void GetDirtyTemplatePathsHelper(TemplateId rootTemplateId, AZStd::vector<AZ::IO::PathView>& dirtyTemplatePaths);

            //! Free memory if memory is available to free.  This is only necessary if the underlying system
            //! retains memory (ie, rapidjson, not AZ::DOM).  Doing so will invalidate all existing PrefabDom&
            //! and PrefabDomValue& references that might be pointing at the garbage collected memory.
            //! @param doAll if doAll is true, it will garbage collect all templates in the set of those needing
            //! to be garbage collected - otherwise, it will only garbage collect one (meant for continuous calling).
            void GarbageCollectTemplates(bool doAll);

            // A container for mapping Templates to the Links they may propagate changes to.
            AZStd::unordered_map<TemplateId, AZStd::unordered_set<LinkId>> m_templateToLinkIdsMap;

            // A container of Prefab Templates mapped by their Template ids.
            AZStd::unordered_map<TemplateId, Template> m_templateIdMap;

            // A container for mapping Templates' file paths to their Template ids.
            AZStd::unordered_map<AZ::IO::Path, TemplateId> m_templateFilePathToIdMap;

            // A map of entity id to hashed path used for generation of entity id during deserialization.
            // This map is needed when there is a many-to-one relationship between entity ids and hashed paths.
            AZStd::unordered_map<AZ::EntityId, AZ::IO::Path> m_entityIdToHashedPathMap;

            // A container of Prefab Links mapped by their Link ids.
            AZStd::unordered_map<LinkId, Link> m_linkIdMap;

            // A counter for generating unique Template Ids.
            AZStd::atomic<TemplateId> m_templateIdCounter = 0u;

            // A counter for generating unique Link Ids.
            AZStd::atomic<LinkId> m_linkIdCounter = 0u;

            // Used for finding the owning instance of an arbitrary entity.
            InstanceEntityMapper m_instanceEntityMapper;

            // Used for finding the Instances owned by an arbitrary Template.
            TemplateInstanceMapper m_templateInstanceMapper;

            // Used for loading/saving Prefab Template files.
            PrefabLoader m_prefabLoader;

            // Used for updating Instances of Prefab Template.
            InstanceUpdateExecutor m_instanceUpdateExecutor;

            // Used for updating Templates when Instances are modified.
            InstanceToTemplatePropagator m_instanceToTemplatePropagator;

            // Handles the public Prefab API used by UI and scripting.
            PrefabPublicHandler m_prefabPublicHandler;

            // Handler of the public Prefab requests.
            PrefabPublicRequestHandler m_prefabPublicRequestHandler;

            PrefabSystemScriptingHandler m_prefabSystemScriptingHandler;

            // If true, individual template-remove messages will be suppressed.
            // It also prevents unnecessary link instance updates during template removal.
            bool m_removingAllTemplates = false;

            AZStd::unordered_set<TemplateId> m_templatesWhichNeedGarbageCollection;
        };
    } // namespace Prefab
} // namespace AzToolsFramework

