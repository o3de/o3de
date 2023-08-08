/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    //! VisibleGeometry describes details about visible geometry surfaces stored as generic indexed triangle lists
    struct VisibleGeometry
    {
        AZ_TYPE_INFO(VisibleGeometry, "{4B011208-B105-4BC1-A4F3-FD5C44785D71}");
        AZ_CLASS_ALLOCATOR(VisibleGeometry, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        AZ::Matrix4x4 m_transform;
        AZStd::vector<float> m_vertices;
        AZStd::vector<uint32_t> m_indices;
    };

    using VisibleGeometryContainer = AZStd::vector<VisibleGeometry>;

    //! Interface for components to provide generic geometric data, potentially for occlusion culling and other systems
    class VisibleGeometryRequests : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        //! Returns a container of visible geometry
        virtual void GetVisibleGeometry(VisibleGeometryContainer& geometryContainer) const = 0;

    protected:
        ~VisibleGeometryRequests() = default;
    };

    using VisibleGeometryRequestBus = AZ::EBus<VisibleGeometryRequests>;
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::VisibleGeometryRequests);
