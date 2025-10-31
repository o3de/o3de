/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Module/Module.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace VideoPlaybackFramework
{
    class VideoPlaybackFrameworkModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(VideoPlaybackFrameworkModule, "{5FCBDEE8-C3EC-45DD-9B71-B37D1F7A299A}", AZ::Module);
        AZ_CLASS_ALLOCATOR(VideoPlaybackFrameworkModule, AZ::SystemAllocator);

        VideoPlaybackFrameworkModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
