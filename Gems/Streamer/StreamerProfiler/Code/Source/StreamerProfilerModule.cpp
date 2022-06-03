/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <StreamerProfilerModuleInterface.h>
#include <StreamerProfilerSystemComponent.h>

namespace Streamer
{
    class StreamerProfilerModule
        : public StreamerProfilerModuleInterface
    {
    public:
        AZ_RTTI(StreamerProvfilerModule, "{9d4ba060-2f72-4c52-8c32-63c788517a2f}", StreamerProfilerModuleInterface);
        AZ_CLASS_ALLOCATOR(StreamerProfilerModule, AZ::SystemAllocator, 0);
    };
}// namespace Streamer

AZ_DECLARE_MODULE_CLASS(Gem_StreamerProfiler, Streamer::StreamerProfilerModule)
