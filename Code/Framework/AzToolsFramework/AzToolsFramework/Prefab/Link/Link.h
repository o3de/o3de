/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

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
            AZ_CLASS_ALLOCATOR(Link, AZ::SystemAllocator, 0);
            AZ_RTTI(Link, "{49230756-7BAA-4456-8DFE-0E18CB887DB5}");

            Link();
            Link(LinkId linkId);
            Link(const Link& other);
            Link& operator=(const Link& other);

            Link(Link&& other) noexcept;
            Link& operator=(Link&& other) noexcept;

            virtual ~Link() noexcept = default;

            void SetSourceTemplateId(TemplateId id);
            void SetTargetTemplateId(TemplateId id);
            void SetLinkDom(const PrefabDomValue& linkDom);
            void SetInstanceName(const char* instanceName);

            bool IsValid() const;

            TemplateId GetSourceTemplateId() const;
            TemplateId GetTargetTemplateId() const;

            LinkId GetId() const;

            PrefabDom& GetLinkDom();
            const PrefabDom& GetLinkDom() const;

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

            PrefabDomValueReference GetLinkPatches();

        private:

            /**
             * Adds a linkId name,value object to the DOM of an instance.
             *
             * @param instanceDomValue The DOM value of the instance within the target template DOM.
             * @param allocator The allocator used while adding the linkId object to the instance DOM.
             */
            void AddLinkIdToInstanceDom(PrefabDomValue& instanceDomValue, PrefabDom::AllocatorType& allocator);

            // Target template id for propagation during updating templates.
            TemplateId m_targetTemplateId = InvalidTemplateId;

            // Source template id for unlink templates if needed.
            TemplateId m_sourceTemplateId = InvalidTemplateId;

            // JSON patches for overrides in Template.
            PrefabDom m_linkDom;

            // Name of the nested instance of target Template.
            AZStd::string m_instanceName;

            LinkId m_id = InvalidLinkId;

            PrefabSystemComponentInterface* m_prefabSystemComponentInterface = nullptr;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
