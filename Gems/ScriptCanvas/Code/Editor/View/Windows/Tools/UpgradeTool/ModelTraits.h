/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class ModelRequestsTraits
            : public AZ::EBusTraits
        {
        public:
            virtual const AZStd::vector<AZStd::string>* GetLogs();
        };
        using ModelRequestsBus = AZ::EBus<ModelRequestsTraits>;

        class ModelNotificationsTraits
            : public AZ::EBusTraits
        {
        public:
        };
        using ModelNotificationsBus = AZ::EBus<ModelNotificationsTraits>;
    }
}
