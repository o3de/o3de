/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/AliasingBarrierTracker.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;
        class Scope;
        class Semaphore;

        // Tracks aliased resource and adds the proper barriers and synchronization semaphores when two resources that
        // overlap each other, partially or totally. It doesn't add any type of synchronization between resources
        // that don't overlap. Resources must be added in order so the tracker knows which one is the source
        // and which one is the destination. It only supports images and buffers.
        class AliasingBarrierTracker
            : public RHI::AliasingBarrierTracker
        {
            using Base = RHI::AliasingBarrierTracker;
        public:
            AZ_CLASS_ALLOCATOR(AliasingBarrierTracker, AZ::SystemAllocator, 0);
            AZ_RTTI(AliasingBarrierTracker, "{6BF3509E-D38A-4F55-B539-E9207C714CA7}", Base);

            AliasingBarrierTracker(Device& device);

        private:
            //////////////////////////////////////////////////////////////////////////
            // RHI::AliasingBarrierTracker
            void AppendBarrierInternal(const RHI::AliasedResource& resourceBefore, const RHI::AliasedResource& resourceAfter) override;
            void EndInternal() override;
            //////////////////////////////////////////////////////////////////////////

            Device& m_device;
            AZStd::vector<Semaphore*> m_barrierSemaphores;
        };
    }
}
