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
        };

        using PrefabPublicNotificationBus = AZ::EBus<PrefabPublicNotifications>;

    } // namespace Prefab
} // namespace AzToolsFramework
