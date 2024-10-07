/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Plane.h"

#include <Include/ScriptCanvas/Libraries/Math/Plane.generated.h>

namespace ScriptCanvas
{
    namespace PlaneFunctions
    {
        using namespace Data;

        NumberType DistanceToPoint(PlaneType source, Vector3Type point)
        {
            return source.GetPointDist(point);
        }

        PlaneType FromCoefficients(NumberType a, NumberType b, NumberType c, NumberType d)
        {
            return PlaneType::CreateFromCoefficients(
                aznumeric_cast<float>(a), aznumeric_cast<float>(b), aznumeric_cast<float>(c), aznumeric_cast<float>(d));
        }

        PlaneType FromNormalAndDistance(Vector3Type normal, NumberType distance)
        {
            return PlaneType::CreateFromNormalAndDistance(normal, aznumeric_cast<float>(distance));
        }

        PlaneType FromNormalAndPoint(Vector3Type normal, Vector3Type point)
        {
            return PlaneType::CreateFromNormalAndPoint(normal, point);
        }

        NumberType GetDistance(PlaneType source)
        {
            return source.GetDistance();
        }

        Vector3Type GetNormal(PlaneType source)
        {
            return source.GetNormal();
        }

        AZStd::tuple<NumberType, NumberType, NumberType, NumberType> GetPlaneEquationCoefficients(PlaneType source)
        {
            const AZ::Vector4& coefficients = source.GetPlaneEquationCoefficients();
            return AZStd::make_tuple(
                coefficients.GetElement(0), coefficients.GetElement(1), coefficients.GetElement(2), coefficients.GetElement(3));
        }

        BooleanType IsFinite(PlaneType source)
        {
            return source.IsFinite();
        }

        Vector3Type Project(PlaneType source, Vector3Type point)
        {
            return source.GetProjected(point);
        }

        PlaneType Transform(PlaneType source, const TransformType& transform)
        {
            return source.GetTransform(transform);
        }
    } // namespace PlaneFunctions
} // namespace ScriptCanvas
