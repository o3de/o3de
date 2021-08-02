/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/VertexContainer.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>

namespace AZ
{
    class Aabb;
    class ReflectContext;
    void PolygonPrismReflect(ReflectContext* context);

    //! Formal Definition: A (right) polygonal prism is a 3-dimensional prism made from two translated polygons connected by rectangles. Parallelogram sides are not allowed.
    //! Here the representation is defined by one polygon (internally represented as a vertex container - list of vertices) and a height (extrusion) property.
    //! All vertices lie on the local plane Z = 0.
    class PolygonPrism
    {
    public:
        AZ_RTTI(PolygonPrism, "{F01C8BDD-6F24-4344-8945-521A8750B30B}")
        AZ_CLASS_ALLOCATOR_DECL

        PolygonPrism() = default;
        virtual ~PolygonPrism() = default;

        //! Set the height of the polygon prism.
        void SetHeight(float height);

        //! Return the height of the polygon prism.
        float GetHeight() const { return m_height; }

        //! Set the non-uniform scale applied to the polygon prism.
        void SetNonUniformScale(const AZ::Vector3& nonUniformScale);

        //! Return the non-uniform scale applied to the polygon prism.
        AZ::Vector3 GetNonUniformScale() const;

        //! Override callbacks to be used when polygon prism changes/is modified (general).
        void SetCallbacks(
            const VoidFunction& onChangeElement,
            const VoidFunction& onChangeContainer,
            const VoidFunction& onChangeHeight,
            const VoidFunction& onChangeNonUniformScale);

        //! Override callbacks to be used when spline changes/is modified (specific).
        //! (use if you need more fine grained control over modifications to the container)
        void SetCallbacks(
            const IndexFunction& onAddVertex, const IndexFunction& onRemoveVertex,
            const IndexFunction& onUpdateVertex, const VoidFunction& onSetVertices,
            const VoidFunction& onClearVertices, const VoidFunction& onChangeHeight,
            const VoidFunction& onChangeNonUniformScale);

        static void Reflect(ReflectContext* context);

        VertexContainer<Vector2> m_vertexContainer; ///< Reference to underlying vertex data.

    private:
        VoidFunction m_onChangeHeightCallback = nullptr; //!< Callback for when height is changed.
        VoidFunction m_onChangeNonUniformScaleCallback = nullptr; //!< Callback for when non-uniform scale is changed.
        float m_height = 1.0f; //!< Height of polygon prism (about local Z) - default to 1m.
        AZ::Vector3 m_nonUniformScale = AZ::Vector3::CreateOne(); //!< Allows non-uniform scale to be applied to the prism.

        //! Internally used to call OnChangeCallback when height values are modified in the property grid.
        void OnChangeHeight();

        //! Internally used to call OnChangeCallback when non-uniform scale values are modified.
        void OnChangeNonUniformScale();
    };

    using PolygonPrismPtr = AZStd::shared_ptr<PolygonPrism>;
    using ConstPolygonPrismPtr = AZStd::shared_ptr<const PolygonPrism>;
}
