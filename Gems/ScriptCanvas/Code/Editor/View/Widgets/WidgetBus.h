/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <AzCore/Math/Uuid.h>
#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    class WidgetNotification : public AZ::EBusTraits
    {
    public:
        virtual void OnLibrarySelected(const AZ::Uuid& library) = 0;
    };

    using WidgetNotificationBus = AZ::EBus<WidgetNotification>;

}
