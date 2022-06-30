/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Aabb.h>
#include <ScriptCanvas/Core/NodeFunctionGeneric.h>
#include <ScriptCanvas/Data/NumericData.h>
#include <ScriptCanvas/Libraries/Math/MathNodeUtilities.h>

namespace ScriptCanvas
{
    //! AABBNodes is deprecated
    //! This file is deprecated and will be removed. Keep it to allow for seamless migration from node generic framework
    //! to new AutoGen function.
    namespace AABBNodes
    {
        using namespace Data;
        using namespace MathNodeUtilities;
        static constexpr const char* k_categoryName = "Math/AABB";

        AZ_INLINE AABBType AddAABB(AABBType a, const AABBType& b)
        {
            a.AddAabb(b);
            return a;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(AddAABB, k_categoryName, "{F0144525-655F-4597-B229-FC1993623704}", "ScriptCanvas_AABBFunctions_AddAABB");

        AZ_INLINE AABBType AddPoint(AABBType source, Vector3Type point)
        {
            source.AddPoint(point);
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(AddPoint, k_categoryName, "{7B9667C2-5466-4691-A6B5-E92FDF300BC1}", "ScriptCanvas_AABBFunctions_AddPoint");

        AZ_INLINE AABBType ApplyTransform(AABBType source, const TransformType& transformType)
        {
            source.ApplyTransform(transformType);
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ApplyTransform, k_categoryName, "{94015C49-A59D-40C3-9B71-AD33E16F85E5}", "ScriptCanvas_AABBFunctions_ApplyTransform");

        AZ_INLINE Vector3Type Center(const AABBType& source)
        {
            return source.GetCenter();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Center, k_categoryName, "{58154CBE-5720-45EA-847E-B19779E4B4CD}", "ScriptCanvas_AABBFunctions_Center");

        AZ_INLINE AABBType Clamp(const AABBType& source, const AABBType& clamp)
        {
            return source.GetClamped(clamp);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Clamp, k_categoryName, "{4F786E54-EA2F-4185-8B3A-37B11E66D1DD}", "ScriptCanvas_AABBFunctions_Clamp");

        AZ_INLINE BooleanType ContainsAABB(const AABBType& source, const AABBType& candidate)
        {
            return source.Contains(candidate);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ContainsAABB, k_categoryName, "{C58DD251-F894-444C-9DC6-6D586D4B4A7E}", "ScriptCanvas_AABBFunctions_ContainsAABB");

        AZ_INLINE BooleanType ContainsVector3(const AABBType& source, const Vector3Type& candidate)
        {
            return source.Contains(candidate);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ContainsVector3, k_categoryName, "{F2DA9405-E0CF-48D8-AB7F-E673249B502A}", "ScriptCanvas_AABBFunctions_ContainsVector3");

        AZ_INLINE NumberType Distance(const AABBType& source, const Vector3Type point)
        {
            return source.GetDistance(point);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Distance, k_categoryName, "{9E568CB1-B66D-4989-BCD9-4D0278FC1B80}", "ScriptCanvas_AABBFunctions_Distance");

        AZ_INLINE AABBType Expand(const AABBType& source, const Vector3Type delta)
        {
            return source.GetExpanded(delta.GetAbs());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Expand, k_categoryName, "{C3FC7ADC-B62C-4C3A-8FF7-FD819D68012D}", "ScriptCanvas_AABBFunctions_Expand");

        AZ_INLINE Vector3Type Extents(const AABBType& source)
        {
            return source.GetExtents();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Extents, k_categoryName, "{9F7832E9-1693-48E2-A449-2DCDF5A8AF6D}", "ScriptCanvas_AABBFunctions_Extents");

        AZ_INLINE AABBType FromCenterHalfExtents(const Vector3Type center, const Vector3Type halfExtents)
        {
            return AABBType::CreateCenterHalfExtents(center, halfExtents);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromCenterHalfExtents, k_categoryName, "{47F26393-1A16-4181-B107-F31294636DF7}", "ScriptCanvas_AABBFunctions_FromCenterHalfExtents");

        AZ_INLINE AABBType FromCenterRadius(const Vector3Type center, const NumberType radius)
        {
            return AABBType::CreateCenterRadius(center, static_cast<float>(radius));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromCenterRadius, k_categoryName, "{5FEFD1BF-DC5B-4AFA-892F-082D92492548}", "ScriptCanvas_AABBFunctions_FromCenterRadius");

        AZ_INLINE AABBType FromMinMax(const Vector3Type min, const Vector3Type max)
        {
            return min.IsLessEqualThan(max) ? AABBType::CreateFromMinMax(min, max) : AABBType::CreateFromPoint(max);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromMinMax, k_categoryName, "{9916F949-2654-439F-8E9E-492E3CF51791}", "ScriptCanvas_AABBFunctions_FromMinMax");

        AZ_INLINE AABBType FromOBB(const OBBType& source)
        {
            return AABBType::CreateFromObb(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromOBB, k_categoryName, "{5ED4C404-34E6-418B-9548-46EDBE7AC298}", "ScriptCanvas_AABBFunctions_FromOBB");

        AZ_INLINE AABBType FromPoint(const Vector3Type& source)
        {
            return AABBType::CreateFromPoint(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(FromPoint, k_categoryName, "{2A851D46-E755-4838-B2E6-89743EA1A495}", "ScriptCanvas_AABBFunctions_FromPoint");

        AZ_INLINE Vector3Type GetMax(const AABBType& source)
        {
            return source.GetMax();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(GetMax, k_categoryName, "{963E5B53-B30D-43CF-B127-A056EEBF768D}", "ScriptCanvas_AABBFunctions_GetMax");

        AZ_INLINE Vector3Type GetMin(const AABBType& source)
        {
            return source.GetMin();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(GetMin, k_categoryName, "{522BDB83-456D-4F63-BE73-D62D6805C0F9}", "ScriptCanvas_AABBFunctions_GetMin");

        AZ_INLINE BooleanType IsFinite(const AABBType& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(IsFinite, k_categoryName, "{0AE946C3-08DB-44A4-BDF3-E80D4F1DF8B3}", "ScriptCanvas_AABBFunctions_IsFinite");

        AZ_INLINE BooleanType IsValid(const AABBType& source)
        {
            return source.IsValid();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(IsValid, k_categoryName, "{70E70747-4CAD-4D75-80DE-6E639DD672CC}", "ScriptCanvas_AABBFunctions_IsValid");

        AZ_INLINE AABBType Null()
        {
            return AABBType::CreateNull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Null, k_categoryName, "{116A178D-7009-4053-8244-C30EB995DF00}", "ScriptCanvas_AABBFunctions_Null");

        AZ_INLINE BooleanType Overlaps(const AABBType& a, const AABBType& b)
        {
            return a.Overlaps(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Overlaps, k_categoryName, "{20040453-712E-49EB-9120-715CE9864527}", "ScriptCanvas_AABBFunctions_Overlaps");

        AZ_INLINE NumberType SurfaceArea(const AABBType& source)
        {
            return source.GetSurfaceArea();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(SurfaceArea, k_categoryName, "{23CB209B-B2EF-4A9F-9725-5B4E8A68ED3C}", "ScriptCanvas_AABBFunctions_SurfaceArea");

        AZ_INLINE std::tuple<Vector3Type, NumberType> ToSphere(const AABBType& source)
        {
            Vector3Type center;
            float radius;
            source.GetAsSphere(center, radius);
            return std::make_tuple(center, radius);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ToSphere, k_categoryName, "{8210C06C-877E-483D-8A11-7FD2697304B8}", "ScriptCanvas_AABBFunctions_ToSphere");

        AZ_INLINE AABBType Translate(const AABBType& source, const Vector3Type translation)
        {
            return source.GetTranslated(translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(Translate, k_categoryName, "{AF792AC7-C386-4937-8BB6-B785DF15D336}", "ScriptCanvas_AABBFunctions_Translate");
        
        AZ_INLINE NumberType XExtent(const AABBType& source)
        {
            return source.GetXExtent();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(XExtent, "Math/AABB", "{CAAE6CF8-4135-452D-97A9-D0D2535B68AD}", "ScriptCanvas_AABBFunctions_XExtent");

        AZ_INLINE NumberType YExtent(const AABBType& source)
        {
            return source.GetYExtent();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(YExtent, "Math/AABB", "{0A0BF39A-AE50-4A7C-B68B-3163CD55E66B}", "ScriptCanvas_AABBFunctions_YExtent");

        AZ_INLINE NumberType ZExtent(const AABBType& source)
        {
            return source.GetZExtent();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_REPLACEMENT(ZExtent, "Math/AABB", "{34DC506E-5213-4063-BCC7-AEB6F1FA03DC}", "ScriptCanvas_AABBFunctions_ZExtent");

        using Registrar = RegistrarGeneric
            < AddAABBNode
            , AddPointNode
            , ApplyTransformNode
            , CenterNode
            , ClampNode
            , ContainsAABBNode
            , ContainsVector3Node
            , DistanceNode
            , ExpandNode
            , ExtentsNode
            , FromCenterHalfExtentsNode
            , FromCenterRadiusNode
            , FromMinMaxNode
            , FromOBBNode
            , FromPointNode
            , IsFiniteNode
            , IsValidNode
            , NullNode
            , OverlapsNode
            , SurfaceAreaNode
            , GetMaxNode
            , GetMinNode
            , ToSphereNode
            , TranslateNode
            , XExtentNode
            , YExtentNode
            , ZExtentNode
            > ;

    }
} 

