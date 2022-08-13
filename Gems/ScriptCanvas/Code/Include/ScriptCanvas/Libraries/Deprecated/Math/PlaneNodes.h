/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Color.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! PlaneNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace PlaneNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/Plane";
                
        AZ_INLINE NumberType DistanceToPoint(PlaneType source, Vector3Type point)
        {
            return source.GetPointDist(point);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            DistanceToPoint,
            k_categoryName, "{19A7625B-4EC2-4DBC-BE72-F2203F96E79F}", "ScriptCanvas_PlaneFunctions_DistanceToPoint");
        
        AZ_INLINE PlaneType FromCoefficients(NumberType a, NumberType b, NumberType c, NumberType d)
        {
            return PlaneType::CreateFromCoefficients(aznumeric_cast<float>(a), aznumeric_cast<float>(b), aznumeric_cast<float>(c), aznumeric_cast<float>(d));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromCoefficients,
            k_categoryName, "{1020CBAE-BE78-4F7A-9DDB-AFAC9E628A0F}", "ScriptCanvas_PlaneFunctions_FromCoefficients");

        AZ_INLINE PlaneType FromNormalAndDistance(Vector3Type normal, NumberType distance)
        {
            return PlaneType::CreateFromNormalAndDistance(normal, aznumeric_cast<float>(distance));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromNormalAndDistance,
            k_categoryName, "{C29C366D-17FD-4C20-8E6F-CCCBA86C9F8A}", "ScriptCanvas_PlaneFunctions_FromNormalAndDistance");

        AZ_INLINE PlaneType FromNormalAndPoint(Vector3Type normal, Vector3Type point)
        {
            return PlaneType::CreateFromNormalAndPoint(normal, point);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            FromNormalAndPoint,
            k_categoryName, "{19547801-06AF-4095-B76D-8227E6EC7AC1}", "ScriptCanvas_PlaneFunctions_FromNormalAndPoint");
        
        AZ_INLINE NumberType GetDistance(PlaneType source)
        {
            return source.GetDistance();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetDistance,
            k_categoryName, "{AB875E5D-6457-494B-A597-104EA0571433}", "ScriptCanvas_PlaneFunctions_GetDistance");

        AZ_INLINE Vector3Type GetNormal(PlaneType source)
        {
            return source.GetNormal();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetNormal, k_categoryName, "{0E24785A-6610-4DE7-9497-DAB3C0031B26}", "ScriptCanvas_PlaneFunctions_GetNormal");

        AZ_INLINE std::tuple<NumberType, NumberType, NumberType, NumberType> GetPlaneEquationCoefficients(PlaneType source)
        {
            const AZ::Vector4& coefficients = source.GetPlaneEquationCoefficients();
            return std::make_tuple(coefficients.GetElement(0), coefficients.GetElement(1), coefficients.GetElement(2), coefficients.GetElement(3));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            GetPlaneEquationCoefficients,
            k_categoryName,
            "{28F3E290-589B-419B-B2A6-9BDFEC9F81A0}",
            "ScriptCanvas_PlaneFunctions_GetPlaneEquationCoefficients");

        AZ_INLINE BooleanType IsFinite(PlaneType source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            IsFinite, k_categoryName, "{2F0A267B-C532-4943-904B-AF13391A77CC}", "ScriptCanvas_PlaneFunctions_IsFinite");

        AZ_INLINE Vector3Type Project(PlaneType source, Vector3Type point)
        {
            return source.GetProjected(point);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Project,
            k_categoryName, "{91BF0A95-3108-49E6-B34D-3E7C93769593}", "ScriptCanvas_PlaneFunctions_Project");

        AZ_INLINE PlaneType Transform(PlaneType source, const TransformType& transform)
        {
            return source.GetTransform(transform);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(
            Transform,
            k_categoryName, "{436082CE-2B1D-44D6-894B-E235844C07CE}", "ScriptCanvas_PlaneFunctions_Transform");
        
        using Registrar = RegistrarGeneric<
            DistanceToPointNode
            , FromCoefficientsNode
            , FromNormalAndDistanceNode
            , FromNormalAndPointNode
            , GetDistanceNode
            , GetNormalNode
            , GetPlaneEquationCoefficientsNode
            , IsFiniteNode
            , ProjectNode
            , TransformNode
        >;

    }
} 

