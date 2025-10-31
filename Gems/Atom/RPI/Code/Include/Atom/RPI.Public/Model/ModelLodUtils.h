/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Model/ModelLod.h>

#include <Atom/RPI.Reflect/Model/ModelLodIndex.h>

namespace AZ
{
    class Transform;

    namespace RPI
    {
        class View;
        class Model;

        namespace ModelLodUtils
        {
            ATOM_RPI_PUBLIC_API ModelLodIndex SelectLod(const View* view, const Transform& entityTransform, const Model& model, ModelLodIndex lodOverride = {});
            ATOM_RPI_PUBLIC_API ModelLodIndex SelectLod(const View* view, const Vector3& position, const Model& model, ModelLodIndex lodOverride = {});
            ATOM_RPI_PUBLIC_API uint8_t SelectLodFromBoundingSphere(const Vector3 center, const float radius, uint8_t numLods, const Matrix4x4& worldtoView, const Matrix4x4& viewToClip);

            //! Gets the approximate screen percentage coverage from a bounding sphere (used in LOD calculations).
            //! cameraPosition is typically viewToWorld.GetTranslation() or worldToView.GetInverse().GetTranslation()
            //! yScale is the [1][1] element from a projection matrix, i.e.: viewToClip.GetRow(1).GetY(),
            //!   which is generally cot(FovY/2) or 1.0/tan(FovY/2) or 2*nearPlaneDistance/nearPlaneHeight for perspective frustum and
            //!   2/(top-bottom) for orthogonal frustum.
            //!   We only use the vertical scale for two reasons: speed and for more consistent behavior with ultra-widescreen views
            ATOM_RPI_PUBLIC_API float ApproxScreenPercentage(const Vector3& center, float radius, const Vector3& cameraPosition, float yScale, bool isPerspective);
        } // namespace ModelLodUtils
    } // namespace RPI
} // namespace AZ
