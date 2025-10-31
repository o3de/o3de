/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/tuple.h>
#include <ScriptCanvas/Data/NumericData.h>

namespace ScriptCanvas
{
    namespace PlaneFunctions
    {
        using namespace Data;

        NumberType DistanceToPoint(PlaneType source, Vector3Type point);

        PlaneType FromCoefficients(NumberType a, NumberType b, NumberType c, NumberType d);

        PlaneType FromNormalAndDistance(Vector3Type normal, NumberType distance);

        PlaneType FromNormalAndPoint(Vector3Type normal, Vector3Type point);

        NumberType GetDistance(PlaneType source);

        Vector3Type GetNormal(PlaneType source);

        AZStd::tuple<NumberType, NumberType, NumberType, NumberType> GetPlaneEquationCoefficients(PlaneType source);

        BooleanType IsFinite(PlaneType source);

        Vector3Type Project(PlaneType source, Vector3Type point);

        PlaneType Transform(PlaneType source, const TransformType& transform);
    } // namespace PlaneFunctions
} // namespace ScriptCanvas

#include <Include/ScriptCanvas/Libraries/Math/Plane.generated.h>
