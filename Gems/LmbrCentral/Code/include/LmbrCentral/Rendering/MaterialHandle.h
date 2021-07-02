/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <RenderBus.h>
#include <IMaterial.h>
#include <AzCore/RTTI/TypeInfo.h>

namespace AZ
{
    class BehaviorContext;
}

namespace LmbrCentral
{
    //! Wraps a IMaterial pointer in a way that BehaviorContext can use it
    class MaterialHandle
        : public AZ::RenderNotificationsBus::Handler
    {
    public:
        AZ_CLASS_ALLOCATOR(MaterialHandle, AZ::SystemAllocator, 0);
        AZ_TYPE_INFO(MaterialHandle, "{BF659DC6-ACDD-4062-A52E-4EC053286F4F}");

        MaterialHandle()
        {
            AZ::RenderNotificationsBus::Handler::BusConnect();
        }

        MaterialHandle(const MaterialHandle& handle)
            : m_material(handle.m_material)
        {
            AZ::RenderNotificationsBus::Handler::BusConnect();
        }

        MaterialHandle& operator=(const MaterialHandle& rhs)
        {
            m_material = rhs.m_material;
            return *this;
        }

        ~MaterialHandle() override
        {
            AZ::RenderNotificationsBus::Handler::BusDisconnect();
        }

        //! Handle the renderer's free resources event by nullifying m_material.
        //! This is used to prevent material handles that may have been queued for release in the next frame
        //! from having dangling pointers after the renderer has already shut down.
        void OnRendererFreeResources([[maybe_unused]] int flags) override
        {
            m_material = nullptr;
        }

        _smart_ptr<IMaterial> m_material;

        static void Reflect(AZ::BehaviorContext* behaviorContext);
        static void Reflect(AZ::SerializeContext* serializeContext);
    };

}
