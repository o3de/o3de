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
