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
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(AddAABB, k_categoryName, "{F0144525-655F-4597-B229-FC1993623704}", "returns the AABB that is the (min(min(A), min(B)), max(max(A), max(B)))", "A", "B");

        AZ_INLINE AABBType AddPoint(AABBType source, Vector3Type point)
        {
            source.AddPoint(point);
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(AddPoint, k_categoryName, "{7B9667C2-5466-4691-A6B5-E92FDF300BC1}", "returns the AABB that is the (min(min(Source), Point), max(max(Source), Point))", "Source", "Point");

        AZ_INLINE AABBType ApplyTransform(AABBType source, const TransformType& transformType)
        {
            source.ApplyTransform(transformType);
            return source;
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ApplyTransform, k_categoryName, "{94015C49-A59D-40C3-9B71-AD33E16F85E5}", "returns the AABB translated and possibly scaled by the Transform", "Source", "Transform");

        AZ_INLINE Vector3Type Center(const AABBType& source)
        {
            return source.GetCenter();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Center, k_categoryName, "{58154CBE-5720-45EA-847E-B19779E4B4CD}", "returns the center of Source", "Source");

        AZ_INLINE AABBType Clamp(const AABBType& source, const AABBType& clamp)
        {
            return source.GetClamped(clamp);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Clamp, k_categoryName, "{4F786E54-EA2F-4185-8B3A-37B11E66D1DD}", "returns the largest version of Source that can fit entirely within Clamp", "Source", "Clamp");

        AZ_INLINE BooleanType ContainsAABB(const AABBType& source, const AABBType& candidate)
        {
            return source.Contains(candidate);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ContainsAABB, k_categoryName, "{C58DD251-F894-444C-9DC6-6D586D4B4A7E}", "returns true if Source contains all of the bounds of Candidate, else false", "Source", "Candidate");

        AZ_INLINE BooleanType ContainsVector3(const AABBType& source, const Vector3Type& candidate)
        {
            return source.Contains(candidate);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ContainsVector3, k_categoryName, "{F2DA9405-E0CF-48D8-AB7F-E673249B502A}", "returns true if Source contains the Candidate, else false", "Source", "Candidate");

        AZ_INLINE NumberType Distance(const AABBType& source, const Vector3Type point)
        {
            return source.GetDistance(point);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Distance, k_categoryName, "{9E568CB1-B66D-4989-BCD9-4D0278FC1B80}", "returns the shortest distance from Point to Source, or zero of Point is contained in Source", "Source", "Point");

        AZ_INLINE AABBType Expand(const AABBType& source, const Vector3Type delta)
        {
            return source.GetExpanded(delta.GetAbs());
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Expand, k_categoryName, "{C3FC7ADC-B62C-4C3A-8FF7-FD819D68012D}", "returns the Source expanded in each axis by the absolute value of each axis in Delta", "Source", "Delta");

        AZ_INLINE Vector3Type Extents(const AABBType& source)
        {
            return source.GetExtents();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Extents, k_categoryName, "{9F7832E9-1693-48E2-A449-2DCDF5A8AF6D}", "returns the Vector3(Source.Width, Source.Height, Source.Depth)", "Source");

        AZ_INLINE AABBType FromCenterHalfExtents(const Vector3Type center, const Vector3Type halfExtents)
        {
            return AABBType::CreateCenterHalfExtents(center, halfExtents);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromCenterHalfExtents, k_categoryName, "{47F26393-1A16-4181-B107-F31294636DF7}", "returns the AABB with Min = Center - HalfExtents, Max = Center + HalfExtents", "Center", "HalfExtents");

        AZ_INLINE AABBType FromCenterRadius(const Vector3Type center, const NumberType radius)
        {
            return AABBType::CreateCenterRadius(center, static_cast<float>(radius));
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromCenterRadius, k_categoryName, "{5FEFD1BF-DC5B-4AFA-892F-082D92492548}", "returns the AABB with Min = Center - Vector3(radius, radius, radius), Max = Center + Vector3(radius, radius, radius)", "Center", "Radius");

        AZ_INLINE AABBType FromMinMax(const Vector3Type min, const Vector3Type max)
        {
            return min.IsLessEqualThan(max) ? AABBType::CreateFromMinMax(min, max) : AABBType::CreateFromPoint(max);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromMinMax, k_categoryName, "{9916F949-2654-439F-8E9E-492E3CF51791}", "returns the AABB from Min and Max if Min <= Max, else returns FromPoint(max)", "Min", "Max");

        AZ_INLINE AABBType FromOBB(const OBBType& source)
        {
            return AABBType::CreateFromObb(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromOBB, k_categoryName, "{5ED4C404-34E6-418B-9548-46EDBE7AC298}", "returns the AABB which contains Source", "Source");

        AZ_INLINE AABBType FromPoint(const Vector3Type& source)
        {
            return AABBType::CreateFromPoint(source);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(FromPoint, k_categoryName, "{2A851D46-E755-4838-B2E6-89743EA1A495}", "returns the AABB with min and max set to Source", "Source");

        AZ_INLINE Vector3Type GetMax(const AABBType& source)
        {
            return source.GetMax();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetMax, k_categoryName, "{963E5B53-B30D-43CF-B127-A056EEBF768D}", "returns the Vector3 that is the max value on each axis of Source", "Source");

        AZ_INLINE Vector3Type GetMin(const AABBType& source)
        {
            return source.GetMin();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(GetMin, k_categoryName, "{522BDB83-456D-4F63-BE73-D62D6805C0F9}", "returns the Vector3 that is the min value on each axis of Source", "Source");

        AZ_INLINE BooleanType IsFinite(const AABBType& source)
        {
            return source.IsFinite();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsFinite, k_categoryName, "{0AE946C3-08DB-44A4-BDF3-E80D4F1DF8B3}", "returns true if Source is finite, else false", "Source");

        AZ_INLINE BooleanType IsValid(const AABBType& source)
        {
            return source.IsValid();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(IsValid, k_categoryName, "{70E70747-4CAD-4D75-80DE-6E639DD672CC}", "returns ture if Source is valid, that is if Source.min <= Source.max, else false", "Source");

        AZ_INLINE AABBType Null()
        {
            return AABBType::CreateNull();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Null, k_categoryName, "{116A178D-7009-4053-8244-C30EB995DF00}", "returns an invalid AABB (min > max), adding any point to it will make it valid");

        AZ_INLINE BooleanType Overlaps(const AABBType& a, const AABBType& b)
        {
            return a.Overlaps(b);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Overlaps, k_categoryName, "{20040453-712E-49EB-9120-715CE9864527}", "returns true if A overlaps B, else false", "A", "B");

        AZ_INLINE NumberType SurfaceArea(const AABBType& source)
        {
            return source.GetSurfaceArea();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(SurfaceArea, k_categoryName, "{23CB209B-B2EF-4A9F-9725-5B4E8A68ED3C}", "returns the sum of the surface area of all six faces of Source", "Source");

        AZ_INLINE std::tuple<Vector3Type, NumberType> ToSphere(const AABBType& source)
        {
            Vector3Type center;
            float radius;
            source.GetAsSphere(center, radius);
            return std::make_tuple(center, radius);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_MULTI_RESULTS_NODE(ToSphere, k_categoryName, "{8210C06C-877E-483D-8A11-7FD2697304B8}", "returns the center and radius of smallest sphere that contains Source", "Source", "Center", "Radius");

        AZ_INLINE AABBType Translate(const AABBType& source, const Vector3Type translation)
        {
            return source.GetTranslated(translation);
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(Translate, k_categoryName, "{AF792AC7-C386-4937-8BB6-B785DF15D336}", "returns the Source with each point added with Translation", "Source", "Translation");
        
        AZ_INLINE NumberType XExtent(const AABBType& source)
        {
            return source.GetXExtent();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(XExtent, "Math/AABB", "{CAAE6CF8-4135-452D-97A9-D0D2535B68AD}", "returns the X extent (max X - min X) of Source", "Source");

        AZ_INLINE NumberType YExtent(const AABBType& source)
        {
            return source.GetYExtent();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(YExtent, "Math/AABB", "{0A0BF39A-AE50-4A7C-B68B-3163CD55E66B}", "returns the Y extent (max Y - min Y) of Source", "Source");

        AZ_INLINE NumberType ZExtent(const AABBType& source)
        {
            return source.GetZExtent();
        }
        SCRIPT_CANVAS_GENERIC_FUNCTION_NODE(ZExtent, "Math/AABB", "{34DC506E-5213-4063-BCC7-AEB6F1FA03DC}", "returns the Z extent (max Z - min Z) of Source", "Source");

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

