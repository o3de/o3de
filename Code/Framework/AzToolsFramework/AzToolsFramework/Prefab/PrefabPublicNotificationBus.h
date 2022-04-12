/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzToolsFramework/Prefab/PrefabIdTypes.h>

namespace AzToolsFramework
{
    namespace Prefab
    {
        class PrefabPublicNotifications
            : public AZ::EBusTraits
        {
        public:
            virtual ~PrefabPublicNotifications() = default;

            virtual void OnPrefabInstancePropagationBegin() {}
            virtual void OnPrefabInstancePropagationEnd() {}

            virtual void OnPrefabTemplateDirtyFlagUpdated(
                [[maybe_unused]] TemplateId templateId, [[maybe_unused]] bool status) {}

            // Sent after a single template has been removed
            // Does not get sent when all templates are being removed at once.  Be sure to handle OnAllTemplatesRemoved as well.
            virtual void OnTemplateRemoved([[maybe_unused]] TemplateId templateId) {}

            // Sent after all templates have been removed
            // Does not trigger individual OnTemplateRemoved events
            virtual void OnAllTemplatesRemoved() {}
        };
        using PrefabPublicNotificationBus = AZ::EBus<PrefabPublicNotifications>;

        //! Used to notify changes to a prefab template.
        class PrefabTemplateNotifications
            : public AZ::EBusTraits
        {
        public:
            //////////////////////////////////////////////////////////////////////////
            // EBusTraits overrides
            static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Multiple;
            static const AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::ById;
            using BusIdType = TemplateId;
            //////////////////////////////////////////////////////////////////////////

            //! Triggered when the prefab template is saved to disk.
            virtual void OnPrefabTemplateSaved() = 0;

        protected:
            ~PrefabTemplateNotifications() = default;
        };
        using PrefabTemplateNotificationBus = AZ::EBus<PrefabTemplateNotifications>;

    } // namespace Prefab
} // namespace AzToolsFramework
