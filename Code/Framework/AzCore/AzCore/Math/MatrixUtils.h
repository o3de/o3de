/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Matrix4x4.h>

namespace AZ
{
    //! Builds a right-handed perspective projection matrix with fov and near/far distances.
    //! The generated projection matrix transforms vectors to clip space which 
    //!     x is in [-1, 1] range and positive x points to right
    //!     y is in [-1, 1] range and positive y points to top
    //!     z is in [ 0, 1] range and negative near and negative far map to [0, 1] or [1, 0] if reverseDepth is set to 1 
    //! @param out Matrix which stores output result
    //! @param fovY Field of view in the y direction, in radians. Must be greater than zero.
    //! @param aspectRatio Aspect ratio, defined as view space width divided by height. Must be greater than zero.
    //! @param near Distance to the near view-plane. Must be greater than zero.
    //! @param far Distance to the far view-plane. Must be greater than zero.
    //! @param reverseDepth Set to true to reverse depth which means near distance maps to 1 and far distance maps to 0.
    //! @return Pointer of the output matrix
    Matrix4x4* MakePerspectiveFovMatrixRH(Matrix4x4& out, float fovY, float aspectRatio, float nearDist, float farDist, bool reverseDepth = false);

    //! Builds a right-handed perspective projection matrix from frustum data. 
    //! The generated projection matrix transforms vectors to clip space which 
    //!     x is in [-1, 1] range and positive x points to right
    //!     y is in [-1, 1] range and positive y points to top
    //!     z is in [ 0, 1] range and negative near and negative far map to [0, 1] or [1, 0] if reverseDepth is set to 1 
    //! @param out Matrix which stores output result
    //! @param left The x coordinate of left side of frustum at near view-plane
    //! @param right The x coordinate of right side of frustum at near view-plane
    //! @param bottom The y coordinate of bottom side of frustum at near view-plane
    //! @param top The y coordinate of top side of frustum at near view-plane
    //! @param near Distance to the near view-plane. Must be greater than zero.
    //! @param far Distance to the far view-plane. Must be greater than zero.
    //! @param reverseDepth Set to true to reverse depth which means near distance maps to 1 and far distance maps to 0.
    //! @return Pointer of the output matrix
    Matrix4x4* MakeFrustumMatrixRH(Matrix4x4& out, float left, float right, float bottom, float top, float nearDist, float farDist, bool reverseDepth = false);
    
    //! Builds a right-handed orthographic projection matrix from frustum data. 
    //! Expected use-cases are view-to-clip matrix of directional light,
    //! so the frustum may contain the view point (light origin).
    //! The generated projection matrix transforms vectors to clip space which 
    //!     x is in [-1, 1] range and positive x points to right
    //!     y is in [-1, 1] range and positive y points to top
    //!     z is in [ 0, 1] range and negative z in original space will be transformed to forward in clip space
    //! @param out Matrix which stores output result
    //! @param left The x coordinate of left view-plane
    //! @param right The x coordinate of right view-plane
    //! @param bottom The y coordinate of bottom view-plane
    //! @param top The y coordinate of top view-plane
    //! @param near Distance to the near view-plane. Must be no less than zero.
    //! @param far Distance to the far view-plane. Must be greater than zero.
    //! @return Pointer of the output matrix
    Matrix4x4* MakeOrthographicMatrixRH(Matrix4x4& out, float left, float right, float bottom, float top, float nearDist, float farDist);

    //! Transforms a position by a matrix. This function can be used with any generic cases which include projection matrices.
    Vector3 MatrixTransformPosition(const Matrix4x4& matrix, const Vector3& inPosition);

} // namespace AZ
