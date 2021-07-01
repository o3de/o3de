/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    class RecorderNotifications
        : public AZ::EBusTraits
    {
    public:
        virtual ~RecorderNotifications() = default;

        virtual void OnRecordingFailed(const AZStd::string& /*why*/) {}
    };
    using RecorderNotificationBus = AZ::EBus<RecorderNotifications>;

} // end namespace EMotionFX
