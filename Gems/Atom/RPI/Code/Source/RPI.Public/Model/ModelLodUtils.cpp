/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Atom/RPI.Public/Model/ModelLodUtils.h>

#include <Atom/RPI.Public/Model/Model.h>
#include <Atom/RPI.Public/View.h>
#include <Atom/RPI.Reflect/Model/ModelAsset.h>

#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector2.h>

namespace AZ
{
    namespace RPI
    {
        namespace ModelLodUtils
        {
            ModelLodIndex SelectLod(const View* view, const Transform& entityTransform, const Model& model, ModelLodIndex lodOverride)
            {
                return SelectLod(view, entityTransform.GetTranslation(), model, lodOverride);
            }

            ModelLodIndex SelectLod(const View* view, const Vector3& position, const Model& model, ModelLodIndex lodOverride)
            {
                AZ_PROFILE_SCOPE(RPI, "ModelLodUtils: SelectLod");
                ModelLodIndex lodIndex;
                if (model.GetLodCount() == 1)
                {
                    lodIndex = ModelLodIndex(0);
                }
                else if (lodOverride.IsNull())
                {
                    /*
                        Simple screen-space Lod determination algorithm.

                        The idea here is simple. We take the bounding sphere of the model
                        and project it into clip space. From there we measure the area of the
                        ellipse in screen space and determine what percent of the total
                        screen it takes up.

                        With that percentage we can determine which Lod we want to use.
                    */
                    Aabb modelAabb = model.GetModelAsset()->GetAabb();
                    modelAabb.Translate(position);

                    Vector3 center; 
                    float radius;
                    modelAabb.GetAsSphere(center, radius);

                    // Projection of a sphere to screen space 
                    // Derived from https://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm
                    const Matrix4x4 worldToViewMatrix = view->GetWorldToViewMatrix();
                    const Matrix4x4 viewToClipMatrix = view->GetViewToClipMatrix();

                    lodIndex = ModelLodIndex(SelectLodFromBoundingSphere(center, static_cast<float>(radius), aznumeric_cast<uint8_t>(model.GetLodCount()), worldToViewMatrix, viewToClipMatrix));
                }
                else
                {
                    lodIndex = lodOverride;
                }

                return lodIndex;
            }

            uint8_t SelectLodFromBoundingSphere(const Vector3 center, float radius, uint8_t numLods, const Matrix4x4& worldtoView, const Matrix4x4& viewToClip)
            {
                // Projection of a sphere to screen space 
                // Derived from https://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm

                const Vector3 cameraPosition = worldtoView.GetTranslation();
                const Vector3 cameraToCenter = cameraPosition - center;

                const float m00 = viewToClip.GetRow(0).GetX();
                const float m11 = viewToClip.GetRow(1).GetY();

                const float viewScale = AZStd::max(0.5f * m00, 0.5f * m11);
                const float cameraToCenterLength = cameraToCenter.GetLength();
                const float screenPercentage = (viewScale * radius) / AZStd::max(1.0f, cameraToCenterLength);

                uint8_t lodIndex;
                if (screenPercentage > 0.25f)
                {
                    lodIndex = 0;
                }
                else if (screenPercentage > 0.075)
                {
                    lodIndex = 1;
                }
                else
                {
                    lodIndex = 2;
                }
                return AZStd::min<uint8_t>(lodIndex, numLods - (uint8_t)1);
            }

            float ApproxScreenPercentage(const Vector3& center, float radius, const Vector3& cameraPosition, float yScale, bool isPerspective)
            {
                if (isPerspective)
                { // view to clip matrix is perspective
                    //Derivation:
                    //let x = approxScreenPercentage (unknown)
                    //let H = nearPlaneHeight
                    //let N = nearPlaneDistance
                    //yScale = cot(FovY/2) = 2*N/H  (by the geometry)
                    //therefore  H = 2*N/yScale
                    //let S = diameter projected onto near plane = x*H
                    //let R = radius
                    //let D = cameraToCenter
                    //2*R/D = S/N           (by like triangles)
                    //2*R/D = (x*H)/N       (substitute for S)
                    //R/D = x/yScale        (substitute for H, cancel the N's and the 2's)
                    //x = yScale*R/D
                    const Vector3 cameraToCenter = cameraPosition - center;
                    const float cameraToCenterLength = cameraToCenter.GetLength();
                    const float approxScreenPercentage = AZStd::GetMin(((yScale * radius) / cameraToCenterLength), 1.0f);
                    return approxScreenPercentage;
                }
                else
                { // view to clip matrix is orthogonal.
                    //Derivation:
                    //let x = approxScreenPercentage (unknown)
                    //let H = frustum height (top - bottom)
                    //yScale = 2/(top - bottom) = 2/H
                    //threfore H = 2/yScale
                    //let R = radius
                    //x = 2*R/H = yScale*R
                    const float approxScreenPercentage = AZStd::GetMin((yScale * radius), 1.0f);
                    return approxScreenPercentage;
                }
            }
        } // namespace ModelLodUtils
    } // namespace RPI
} // namespace AZ
