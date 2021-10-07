/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/RTTI/TypeInfo.h>

struct IMaterial;

namespace AZ
{
    class BehaviorContext;
}
namespace LmbrCentral
{
    //! Wraps a IMaterial pointer in a way that BehaviorContext can use it
    class MaterialHandle
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialHandle, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(MaterialHandle, "{BF659DC6-ACDD-4062-A52E-4EC053286F4F}");

        MaterialHandle()
        {
        }

        MaterialHandle(const MaterialHandle& handle)
            : m_material(handle.m_material)
        {
        }

        ~MaterialHandle()
        {
        }

        MaterialHandle& operator=(const MaterialHandle& rhs)
        {
            m_material = rhs.m_material;
            return *this;
        }

        IMaterial* m_material;

        static void Reflect(AZ::BehaviorContext* behaviorContext);
        static void Reflect(AZ::SerializeContext* serializeContext);
    };

}
