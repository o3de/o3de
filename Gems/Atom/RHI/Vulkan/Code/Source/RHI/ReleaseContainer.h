/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI/Object.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/functional.h>
#include <Atom/RHI.Reflect/VkAllocator.h>

namespace AZ
{
    namespace Vulkan
    {
        class Device;

        //! Utility class used for containing a Vulkan object that will
        //! be destroyed on subsequent frames.
        template<typename T>
        class ReleaseContainer
            : public RHI::Object
        {
        public:
            AZ_CLASS_ALLOCATOR(ReleaseContainer<T>, AZ::ThreadPoolAllocator);
            using VkDestroyFunc = AZStd::function<void(VkDevice, T, const VkAllocationCallbacks*)>;

            ReleaseContainer(VkDevice vkDevice, T vkObject, VkDestroyFunc vkDestroyFunc)
                : m_vkDevice(vkDevice)
                , m_vkObject(vkObject)
                , m_vkDestroyFunc(vkDestroyFunc)
            {}

            ~ReleaseContainer()
            {
                m_vkDestroyFunc(m_vkDevice, m_vkObject, VkSystemAllocator::Get());
            }

        private:
            VkDevice m_vkDevice;;
            T m_vkObject;
            VkDestroyFunc m_vkDestroyFunc;
        };
    }
}
