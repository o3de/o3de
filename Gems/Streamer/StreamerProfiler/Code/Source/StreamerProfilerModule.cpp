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
        AZ_RTTI(StreamerProfilerModule, "{9d4ba060-2f72-4c52-8c32-63c788517a2f}", StreamerProfilerModuleInterface);
        AZ_CLASS_ALLOCATOR(StreamerProfilerModule, AZ::SystemAllocator);
    };
}// namespace Streamer

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), Streamer::StreamerProfilerModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_StreamerProfiler, Streamer::StreamerProfilerModule)
#endif
