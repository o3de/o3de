/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>


namespace ProjectSettingsTool
{
    class LastPathTraits
        : public AZ::EBusTraits
    {
    public:
        using Bus = AZ::EBus<LastPathTraits>;

        // Bus Configuration
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual QString GetLastImagePath() = 0;
        virtual void SetLastImagePath(const QString& path) = 0;
    };

    typedef AZ::EBus<LastPathTraits> LastPathBus;
} // namespace ProjectSettingsTool
