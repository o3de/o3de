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
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Matrix4x4.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/std/containers/vector.h>

namespace AZ
{
    class ReflectContext;
}

namespace AzFramework
{
    //! VisibleGeometry describes visible geometry surfaces stored as generic indexed triangle lists
    struct VisibleGeometry
    {
        AZ_TYPE_INFO(VisibleGeometry, "{4B011208-B105-4BC1-A4F3-FD5C44785D71}");
        AZ_CLASS_ALLOCATOR(VisibleGeometry, AZ::SystemAllocator);

        static void Reflect(AZ::ReflectContext* context);

        //! Local to world matrix transforming vertices into world space. 
        AZ::Matrix4x4 m_transform;

        //! Vertex list must contain 3 floats, XYZ, per vertex.
        AZStd::vector<float> m_vertices;

        //! Index list must contain 3 uint32_t per triangle face. Each index references a position in the vertex list.
        AZStd::vector<uint32_t> m_indices;

        //! Flag signifying if the geometry has transparent elements.
        bool m_transparent = false;
    };

    using VisibleGeometryContainer = AZStd::vector<VisibleGeometry>;

    //! Interface for components to provide generic geometric data, potentially for occlusion culling and other systems
    class VisibleGeometryRequests : public AZ::ComponentBus
    {
    public:
        static void Reflect(AZ::ReflectContext* context);

        //! This function should be implemented by components to add VisibleGeometry items to the VisibleGeometryContainer.
        //! @param bounds is a bounding volume in world space used for sampling geometric data. If bounds is invalid it should be ignored.
        //! @param geometryContainer is a container of all of the geometry added by the connected entity. It is passed by reference to give
        //! multiple components on an entity an opportunity to add their geometry. It is the caller's responsibility to manage and clear the
        //! container between calls. The container should not be cleared within the function, which might destroy a prior components
        //! contribution.
        virtual void BuildVisibleGeometry(const AZ::Aabb& bounds, VisibleGeometryContainer& geometryContainer) const = 0;

        //! GetVisibleGeometry is being deprecated in favor of BuildVisibleGeometry.
        virtual void GetVisibleGeometry(const AZ::Aabb& bounds, VisibleGeometryContainer& geometryContainer) const
        {
            BuildVisibleGeometry(bounds, geometryContainer);
        }

    protected:
        ~VisibleGeometryRequests() = default;
    };

    using VisibleGeometryRequestBus = AZ::EBus<VisibleGeometryRequests>;
} // namespace AzFramework

DECLARE_EBUS_EXTERN(AzFramework::VisibleGeometryRequests);
