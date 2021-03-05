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
#ifndef AZCORE_MEMORY_COMPONENT_H
#define AZCORE_MEMORY_COMPONENT_H

#include <AzCore/Component/Component.h>
#include <AzCore/Math/Crc.h>

namespace AZ
{
    /**
     * Memory manager component. It will manager all memory managers.
     * This is the only component that requires special care as memory managers
     * must be operational for any system to operate. In addition this component doesn't have a factory
     * as it's managed by the bootstrap component class.
     */
    class MemoryComponent
        : public Component
    {
    public:
        AZ_COMPONENT(AZ::MemoryComponent, "{6F450DDA-6F4D-40fd-A93B-E5CCCDBC72AB}")

        MemoryComponent();
        virtual ~MemoryComponent();

        //////////////////////////////////////////////////////////////////////////
        // Component base
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

    private:

        /// \ref ComponentDescriptor::GetProvidedServices
        static void GetProvidedServices(ComponentDescriptor::DependencyArrayType& provided);
        /// \ref ComponentDescriptor::GetIncompatibleServices
        static void GetIncompatibleServices(ComponentDescriptor::DependencyArrayType& incompatible);
        /// \ref ComponentDescriptor::Reflect
        static void Reflect(ReflectContext* reflection);

        // serialized data
        bool m_isPoolAllocator;
        bool m_isThreadPoolAllocator;

        // non-serialized data
        bool m_createdPoolAllocator;
        bool m_createdThreadPoolAllocator;
    };
}

#endif // AZCORE_MEMORY_COMPONENT_H
#pragma once
