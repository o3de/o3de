/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/IO/Path/Path.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/Json/JsonSerialization.h>
#include <AzCore/std/containers/unordered_set.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/optional.h>
#include <AzToolsFramework/Prefab/PrefabDomTypes.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class Template;
        using TemplateReference = AZStd::optional<AZStd::reference_wrapper<Template>>;

        // A prefab template is the primary product of loading a prefab file from disk. 
        class Template
        {
        public:
            AZ_CLASS_ALLOCATOR(Template, AZ::SystemAllocator);
            AZ_RTTI(Template, "{F6B7DC7B-386A-42DD-BA8B-919A4D024D7C}");

            using Links = AZStd::unordered_set<LinkId>;

            Template() = default;
            Template(const AZ::IO::Path& filePath, PrefabDom prefabDom);
            Template(const Template& other);
            Template& operator=(const Template& other);

            Template(Template&& other) noexcept;
            Template& operator=(Template&& other) noexcept;

            void GarbageCollect();

            virtual ~Template() noexcept = default;

            bool IsValid() const;

            bool IsLoadedWithErrors() const;
            void MarkAsLoadedWithErrors(bool loadedWithErrors);

            bool IsDirty() const;
            void MarkAsDirty(bool dirty);

            bool AddLink(LinkId newLinkId);
            bool RemoveLink(LinkId linkId);
            bool HasLink(LinkId linkId) const;
            const Links& GetLinks() const;

            PrefabDom& GetPrefabDom();
            const PrefabDom& GetPrefabDom() const;

            PrefabDomValueReference GetInstancesValue();
            PrefabDomValueConstReference GetInstancesValue() const;

            const AZ::IO::Path& GetFilePath() const;
            void SetFilePath(const AZ::IO::PathView& path);

            // To tell if this Template was created from an product asset
            bool IsProcedural() const;

        private:
            // Container for keeping links representing the Template's nested instances.
            Links m_links;

            // JSON Document contains parsed prefab file for users to access data in Prefab file easily.
            PrefabDom m_prefabDom;

            // File path of this Prefab Template.
            AZ::IO::Path m_filePath;

            // Flag to tell if this Template and all its nested instances loaded with any error.
            bool m_isLoadedWithErrors = false;

            // Flag to tell if this Template has changes that have yet to be saved to file.
            bool m_isDirty = false;

            // Flag to tell if this Template was generated outside the Editor
            mutable AZStd::optional<bool> m_isProcedural;
        };
    } // namespace Prefab
} // namespace AzToolsFramework
