/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzFramework/Logging/MissingAssetNotificationBus.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzToolsFramework
{
    class MissingAssetNotificationHandler final
        : public AzFramework::MissingAssetNotificationBus::Handler
        , public AZ::BehaviorEBusHandler
    {
    public:
        AZ_EBUS_BEHAVIOR_BINDER(
            MissingAssetNotificationHandler, "{74DCFB1F-74E7-4A83-8482-6D27B320666E}", AZ::SystemAllocator, FileMissing);

        void FileMissing(const char* filePath) override;

        static void Reflect(AZ::ReflectContext* context);
    };
} // namespace AzToolsFramework
