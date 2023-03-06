/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Debug/Budget.h>
#include <AzCore/DOM/DomPrefixTree.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

AZ_DECLARE_BUDGET(PrefabSystem);

namespace AzToolsFramework
{
    namespace Prefab
    {
        // A prefab template is the primary product of loading a prefab file from disk.
        class Template;
        class Link;
        class PrefabSystemComponentInterface;

        using LinkReference = AZStd::optional<AZStd::reference_wrapper<Link>>;

        // A link is the primary point of communication between prefab templates.
        class Link
        {
        public:
            AZ_CLASS_ALLOCATOR(Link, AZ::SystemAllocator);
            AZ_RTTI(Link, "{49230756-7BAA-4456-8DFE-0E18CB887DB5}");

            // The structure to store metadata information about individual patches on a link.
            struct PrefabOverrideMetadata
            {
                AZ_RTTI(PrefabOverrideMetadata, "{03A2996F-8E93-4D78-B13B-A30AE5E778A4}");
                virtual ~PrefabOverrideMetadata() = default;

                PrefabOverrideMetadata(PrefabDom&& patch, AZ::u32 patchIndex) noexcept
                    : m_patch(AZStd::move(patch))
                    , m_patchIndex(patchIndex)
                {
                }

                PrefabOverrideMetadata(PrefabOverrideMetadata&& other) noexcept
                    : m_patch(AZStd::move(other.m_patch))
                    , m_patchIndex(other.m_patchIndex)
                {
                }

                PrefabOverrideMetadata& operator=(PrefabOverrideMetadata&& other) noexcept
                {
                    if (this != &other)
                    {
                        m_patch = AZStd::move(other.m_patch);
                        m_patchIndex = other.m_patchIndex;
                    }

                    return *this;
                }

                bool operator<(const PrefabOverrideMetadata& other) const
                {
                    return (m_patchIndex < other.m_patchIndex);
                }

                // An individual patch that can get applied to the target template of the link.
                PrefabDom m_patch;

                // The patch index to associate with each individual patch. This is needed to maintain the order of patches within a link.
                AZ::u32 m_patchIndex;
            };

            Link();
            Link(LinkId linkId);
            Link(const Link& other) = delete;
            Link& operator=(const Link& other) = delete;

            Link(Link&& other) noexcept;
            Link& operator=(Link&& other) noexcept;

            virtual ~Link() noexcept = default;

            void SetSourceTemplateId(TemplateId id);
            void SetTargetTemplateId(TemplateId id);
            void SetLinkDom(const PrefabDomValue& linkDom);
            void SetLinkPatches(const PrefabDom& patches);
            void SetInstanceName(const char* instanceName);

            bool IsValid() const;

            TemplateId GetSourceTemplateId() const;
            TemplateId GetTargetTemplateId() const;

            LinkId GetId() const;

            //! Populates the patches DOM provided with the patches fetched from 'm_linkPatchesTree'
            //! @param[out] patchesDom The DOM to populate with patches
            //! @param allocator The allocator to use for memory allocations of patches.
            void GetLinkPatches(PrefabDomValue& patchesDom, PrefabDomAllocator& allocator) const;

            //! Populates the link DOM provided with 'Source' and 'Patches' fields. 'Patches' are fetched from 'm_linkPatchesTree'.
            //! @param[out] linkDom The DOM to populate with source and patches information.
            //! @param allocator The allocator to use for memory allocations of patches.
            void GetLinkDom(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const;

            //! Checks whether overrides are present by querying the patches tree with the provided path
            //! @param path The path to query the overrides tree with.
            //! @param prefixTreeTraversalFlags The traversal flags for the prefix tree. The default is to exclude parent paths because
            //!                                 we usually check for overrides on one or more components/properties within an entity.
            //! @return true if overrides are present at the provided path.
            bool AreOverridesPresent(
                AZ::Dom::Path path,
                AZ::Dom::PrefixTreeTraversalFlags prefixTreeTraversalFlags = AZ::Dom::PrefixTreeTraversalFlags::ExcludeParentPaths) const;

            //! Gets an override patch at the exact provided path
            //! @param path The path to get override for.
            //! @return an override patch if an override is present at the exact provided path.
            PrefabDomConstReference GetOverridePatchAtExactPath(AZ::Dom::Path path) const;
            
            //! Removes overrides at the provided path and all the nodes under it from the override tree
            //! @param path The path at which the overrides should be removed from
            //! @return The sub-tree representing the removed overrides.
            AZ::Dom::DomPrefixTree<PrefabOverrideMetadata> RemoveOverrides(AZ::Dom::Path path);

            //! Adds overrides at the provided path by attaching the provided subtree representing new overrides
            //! @param path The path at which new overrides should be added
            //! @param subTree The tree representing the new overrides to be added
            //! @return Whether the overrides are successfully added or not.
            bool AddOverrides(const AZ::Dom::Path& path, AZ::Dom::DomPrefixTree<PrefabOverrideMetadata>&& subTree);

            //! Adds overrides patches to the tree. It can be used to add patches to tree for an entity.
            //! @param patches The override patches that will be added to tree.
            void AddOverrides(const PrefabDomValue& patches);

            PrefabDomPath GetInstancePath() const;
            const AZStd::string& GetInstanceName() const;

            bool UpdateTarget();

            /**
             * Get the DOM of the instance that the link points to.
             * 
             * @return The DOM of the linked instance
             */
            PrefabDomValue& GetLinkedInstanceDom();

            /**
             * Adds a linkId name,value object to the DOM of an instance.
             * 
             * @param instanceDomValue The DOM value of the instance within the target template DOM.
             */
            void AddLinkIdToInstanceDom(PrefabDomValue& instanceDomValue);

        private:
            /**
             * Adds a linkId name,value object to the DOM of an instance.
             *
             * @param instanceDomValue The DOM value of the instance within the target template DOM.
             * @param allocator The allocator used while adding the linkId object to the instance DOM.
             */
            void AddLinkIdToInstanceDom(PrefabDomValue& instanceDomValue, PrefabDomAllocator& allocator);

            //! Populates the DOM provided with the patches fetched from 'm_linkPatchesTree'
            //! @param[out] linkDom The DOM to populate with patches
            //! @param allocator The allocator to use for memory allocations of patches.
            void ConstructLinkDomFromPatches(PrefabDomValue& linkDom, PrefabDomAllocator& allocator) const;

            //! Clears the existing tree and rebuilds it from the provided patches.
            //! @param patches The patches to build the tree with.
            void RebuildLinkPatchesTree(const PrefabDomValue& patches);

            //! Adds patches to the existing tree. It can be used to add patches to tree for an entity.
            //! @param patches The patches to be added to the tree.
            void AddLinkPatchesToTree(const PrefabDomValue& patches);

            // The prefix tree to store patches on a link. The tree is built with nodes. A node may or may not store a patch.
            // The path from the root to a node represents a path to a DOM value. Eg: 'Instances/Instance1/Entities/Entity1'.
            AZ::Dom::DomPrefixTree<PrefabOverrideMetadata> m_linkPatchesTree;

            // Target template id for propagation during updating templates.
            TemplateId m_targetTemplateId = InvalidTemplateId;

            // Source template id for unlink templates if needed.
            TemplateId m_sourceTemplateId = InvalidTemplateId;

            // Name of the nested instance of target Template.
            AZStd::string m_instanceName;

            // Identifier for the link.
            LinkId m_id = InvalidLinkId;

            // Index counter for generating patches.
            AZ::u32 m_patchIndexCounter = 0u;

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };

        using PrefabOverridePrefixTree = AZ::Dom::DomPrefixTree<Link::PrefabOverrideMetadata>;
    } // namespace Prefab
} // namespace AzToolsFramework
