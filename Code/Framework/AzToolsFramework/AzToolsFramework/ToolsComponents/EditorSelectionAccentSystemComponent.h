/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/Component/Component.h>
#include <AzToolsFramework/API/ToolsApplicationAPI.h>
#include "EditorSelectionAccentingBus.h"

namespace AzToolsFramework
{
    namespace Components
    {
        class EditorSelectionAccentSystemComponent
            : public AZ::Component
            , public AzToolsFramework::ToolsApplicationEvents::Bus::Handler
            , public AzToolsFramework::Components::EditorSelectionAccentingRequestBus::Handler
        {
        public:

            enum class ComponentEntityAccentType : AZ::u8
            {
                None,
                Hover,
                Selected,
                ParentSelected,
                SliceSelected
            };

            AZ_COMPONENT(EditorSelectionAccentSystemComponent, "{6E0F0E2C-1FE5-4AFB-9672-DC92B3D2D844}");

            ~EditorSelectionAccentSystemComponent() override = default;

            static void Reflect(AZ::ReflectContext* context);

            ////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Activate() override;
            void Deactivate() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            void BeforeEntitySelectionChanged() override {};
            void AfterEntitySelectionChanged(const AzToolsFramework::EntityIdList&, const AzToolsFramework::EntityIdList&) override;

            void BeforeEntityHighlightingChanged() override {};
            void AfterEntityHighlightingChanged() override;
            ////////////////////////////////////////////////////////////////////////

            ////////////////////////////////////////////////////////////////////////
            void ForceSelectionAccentRefresh() override;
            ////////////////////////////////////////////////////////////////////////

        protected:

            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC("EditorSelectionAccentingSelectionService", 0x8b5253cf));
            }

            /**
            * \brief Queues the invalidation, recalculation and application of accents
            */
            void QueueAccentRefresh();

            /**
            * \brief Invalidates all currently applied accents
            */
            void InvalidateAccents();

            /**
            * \brief Recalculates and applies accenting on the currently selected set of entities
            */
            void RecalculateAndApplyAccents();

            // Stores a list of entities that are currently accented
            AZStd::unordered_set<AZ::EntityId> m_currentlyAccentedEntities;

            // Indicates if a refresh of accenting is already queued
            bool m_isAccentRefreshQueued = false;
        };
    }

    using EntityAccentType = Components::EditorSelectionAccentSystemComponent::ComponentEntityAccentType;
} // namespace LmbrCentral
