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

#include <AzCore/Module/Module.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace VideoPlaybackFramework
{
    class VideoPlaybackFrameworkModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(VideoPlaybackFrameworkModule, "{5FCBDEE8-C3EC-45DD-9B71-B37D1F7A299A}", AZ::Module);
        AZ_CLASS_ALLOCATOR(VideoPlaybackFrameworkModule, AZ::SystemAllocator, 0);

        VideoPlaybackFrameworkModule();

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override;
    };
}
