/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

namespace AZ
{
     //! Notifications broadcast by the data patching system.
    class DataPatchNotifications
        : public AZ::EBusTraits
    {
    public:
        //! Broadcast when a legacy data patch has been successfully loaded
        virtual void OnLegacyDataPatchLoaded() {}

        //! Broadcast when a legacy data patch fails to load
        virtual void OnLegacyDataPatchLoadFailed() {}
    };

    using DataPatchNotificationBus = AZ::EBus<DataPatchNotifications>;
} // namespace AZ
