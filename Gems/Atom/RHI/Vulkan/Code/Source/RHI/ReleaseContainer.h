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

#include <Atom/RHI/Object.h>
#include <AzCore/Memory/PoolAllocator.h>
#include <AzCore/std/functional.h>

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
            AZ_CLASS_ALLOCATOR(ReleaseContainer<T>, AZ::ThreadPoolAllocator, 0);
            using VkDestroyFunc = AZStd::function<void(VkDevice, T, const VkAllocationCallbacks*)>;

            ReleaseContainer(VkDevice vkDevice, T vkObject, VkDestroyFunc vkDestroyFunc)
                : m_vkDevice(vkDevice)
                , m_vkObject(vkObject)
                , m_vkDestroyFunc(vkDestroyFunc)
            {}

            ~ReleaseContainer() { m_vkDestroyFunc(m_vkDevice, m_vkObject, nullptr); }

        private:
            VkDevice m_vkDevice;;
            T m_vkObject;
            VkDestroyFunc m_vkDestroyFunc;
        };
    }
}
