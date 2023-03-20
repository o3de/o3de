/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Aabb.h>

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    //Script Wrappers for Axis Aligned Bounding Box
    void AabbDefaultConstructor(AZ::Aabb* thisPtr)
    {
        new (thisPtr) AZ::Aabb(AZ::Aabb::CreateNull());
    }


    void AabbSetGeneric(Aabb* thisPtr, ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsClass<Vector3>(1))
        {
            Vector3 min, max;
            dc.ReadArg<Vector3>(0, min);
            dc.ReadArg<Vector3>(1, max);
            thisPtr->Set(min, max);
        }
        else
        {
            AZ_Error("Script", false, "ScriptAabb Set only supports two arguments: Vector3 min, Vector3 max");
        }
    }


    void AabbGetAsSphereMultipleReturn(const Aabb* thisPtr, ScriptDataContext& dc)
    {
        Vector3 center;
        float radius;
        thisPtr->GetAsSphere(center, radius);
        dc.PushResult(center);
        dc.PushResult(radius);
    }


    void AabbContainsGeneric(const Aabb* thisPtr, ScriptDataContext& dc)
    {
        if (dc.GetNumArguments() == 1)
        {
            if (dc.IsClass<Vector3>(0))
            {
                Vector3 v = Vector3::CreateZero();
                dc.ReadArg<Vector3>(0, v);
                dc.PushResult<bool>(thisPtr->Contains(v));
            }
            else if (dc.IsClass<Aabb>(0))
            {
                Aabb aabb = Aabb::CreateNull();
                dc.ReadArg<Aabb>(0, aabb);
                dc.PushResult<bool>(thisPtr->Contains(aabb));
            }
        }

        if (dc.GetNumResults() == 0)
        {
            AZ_Error("Script", false, "ScriptAabb Contains expects one argument: Either Vector3 or Aabb");
        }
    }


    AZStd::string AabbToString(const Aabb& aabb)
    {
        return AZStd::string::format("Min %s Max %s", Vector3ToString(aabb.GetMin()).c_str(), Vector3ToString(aabb.GetMax()).c_str());
    }


    void Aabb::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Aabb>()
                ->Field("min", &Aabb::m_min)
                ->Field("max", &Aabb::m_max);
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Aabb>()
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Common)
                ->Attribute(AZ::Script::Attributes::Module, "math")
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)
                ->Attribute(AZ::Script::Attributes::GenericConstructorOverride, &AabbDefaultConstructor)
                ->Property("min", &Aabb::GetMin, &Aabb::SetMin)
                ->Property("max", &Aabb::GetMax, &Aabb::SetMax)
                ->Method("ToString", &AabbToString)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)
                ->Method("CreateNull", &Aabb::CreateNull)
                ->Method("IsValid", &Aabb::IsValid)
                ->Method("CreateFromPoint", &Aabb::CreateFromPoint)
                ->Method("CreateFromMinMax", &Aabb::CreateFromMinMax)
                ->Method("CreateFromMinMaxValues", &Aabb::CreateFromMinMaxValues)
                ->Method("CreateCenterHalfExtents", &Aabb::CreateCenterHalfExtents)
                ->Method("CreateCenterRadius", &Aabb::CreateCenterRadius)
                ->Method("GetExtents", &Aabb::GetExtents)
                ->Method("GetCenter", &Aabb::GetCenter)
                ->Method("Set", &Aabb::Set)
                ->Attribute(AZ::Script::Attributes::MethodOverride, &AabbSetGeneric)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("CreateFromObb", &Aabb::CreateFromObb)
                ->Method("GetXExtent", &Aabb::GetXExtent)
                ->Method("GetYExtent", &Aabb::GetYExtent)
                ->Method("GetZExtent", &Aabb::GetZExtent)
                ->Method("GetAsSphere", &Aabb::GetAsSphere, nullptr, "() -> Vector3(center) and float(radius)")
                ->Attribute(AZ::Script::Attributes::MethodOverride, &AabbGetAsSphereMultipleReturn)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method<bool (Aabb::*)(const Aabb&) const>("Contains", &Aabb::Contains, nullptr, "const Vector3& or const Aabb&")
                ->Attribute(AZ::Script::Attributes::MethodOverride, &AabbContainsGeneric)
                ->Method<bool (Aabb::*)(const Vector3&) const>("ContainsVector3", &Aabb::Contains, nullptr, "const Vector3&")
                ->Attribute(AZ::Script::Attributes::Ignore, 0) // ignore for script since we already got the generic contains above
                ->Method("Overlaps", &Aabb::Overlaps)
                ->Method("Expand", &Aabb::Expand)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("GetExpanded", &Aabb::GetExpanded)
                ->Method("AddPoint", &Aabb::AddPoint)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("AddAabb", &Aabb::AddAabb)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("GetDistance", &Aabb::GetDistance)
                ->Method("GetClamped", &Aabb::GetClamped)
                ->Method("Clamp", &Aabb::Clamp)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("SetNull", &Aabb::SetNull)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("Translate", &Aabb::Translate)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("GetTranslated", &Aabb::GetTranslated)
                ->Method("GetSurfaceArea", &Aabb::GetSurfaceArea)
                ->Method("GetTransformedObb", static_cast<Obb(Aabb::*)(const Transform&) const>(&Aabb::GetTransformedObb))
                ->Method("GetTransformedAabb", static_cast<Aabb(Aabb::*)(const Transform&) const>(&Aabb::GetTransformedAabb))
                ->Method("ApplyTransform", &Aabb::ApplyTransform)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("Clone", [](const Aabb& rhs) -> Aabb { return rhs; })
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly)
                ->Method("IsFinite", &Aabb::IsFinite)
                ->Method("Equal", &Aabb::operator==)
                ->Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::Equal)
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::ListOnly);
        }
    }


    Aabb Aabb::CreateFromObb(const Obb& obb)
    {
        Vector3 tmp[8];
        tmp[0] = obb.GetPosition() + obb.GetAxisX() * obb.GetHalfLengthX()
            + obb.GetAxisY() * obb.GetHalfLengthY()
            + obb.GetAxisZ() * obb.GetHalfLengthZ();
        tmp[1] = tmp[0] - obb.GetAxisZ() * (2.0f * obb.GetHalfLengthZ());
        tmp[2] = tmp[0] - obb.GetAxisX() * (2.0f * obb.GetHalfLengthX());
        tmp[3] = tmp[1] - obb.GetAxisX() * (2.0f * obb.GetHalfLengthX());
        tmp[4] = tmp[0] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
        tmp[5] = tmp[1] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
        tmp[6] = tmp[2] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());
        tmp[7] = tmp[3] - obb.GetAxisY() * (2.0f * obb.GetHalfLengthY());

        Vector3 min = tmp[0];
        Vector3 max = tmp[0];

        for (int i = 1; i < 8; ++i)
        {
            min = min.GetMin(tmp[i]);
            max = max.GetMax(tmp[i]);
        }

        return Aabb::CreateFromMinMax(min, max);
    }


    Obb Aabb::GetTransformedObb(const Transform& transform) const
    {
        /// \todo This can be optimized, by transforming directly from the Aabb without converting to a Obb first
        Obb temp = Obb::CreateFromAabb(*this);
        return transform * temp;
    }


    Obb Aabb::GetTransformedObb(const Matrix3x4& matrix3x4) const
    {
        Matrix3x4 matrixNoScale = matrix3x4;
        const AZ::Vector3 scale = matrixNoScale.ExtractScale();
        const AZ::Quaternion rotation = AZ::Quaternion::CreateFromMatrix3x4(matrixNoScale);

        return Obb::CreateFromPositionRotationAndHalfLengths(
            matrix3x4 * GetCenter(),
            rotation,
            0.5f * scale * GetExtents()
        );
    }


    void Aabb::ApplyTransform(const Transform& transform)
    {
        Vector3 newMin = transform.GetTranslation();
        Vector3 newMax = newMin;

        // Find extreme points for each axis.
        for (int axisIndex = 0; axisIndex < 3; axisIndex++)
        {
            Vector3 axis = Vector3::CreateZero();
            axis.SetElement(axisIndex, 1.0f);
            // The extreme values in each direction must be attained by transforming one of the 8 vertices of the original box.
            // Each co-ordinate of a transformed vertex is made up of three parts, corresponding to the components of the original
            // x, y and z co-ordinates which are mapped onto the new axis. Those three parts are independent, so we can take
            // the min and max of each part and sum them to get the min and max co-ordinate of the transformed box. For a given new axis,
            // the coefficients for what proportion of each original axis is rotated onto that new axis are the same as the components we
            // would get by performing the inverse rotation on the new axis, so we need to take the conjugate to get the inverse rotation.
            const Vector3 axisCoeffs = transform.GetUniformScale() * (transform.GetRotation().GetConjugate().TransformVector(axis));

            // These contain the existing min and max corners of the AABB that are rotated and scaled (but not translated)
            // and projected onto the current axis. The minimum and maximum contributions from both projected corners get
            // added into the new minimum and maximums independently for each axis.
            // Note that because of the rotation, a minimum contribution can come from the existing max corner, and a maximum
            // contribution can come from the existing min corner.
            const Vector3 projectedContributionsFromMin = axisCoeffs * m_min;
            const Vector3 projectedContributionsFromMax = axisCoeffs * m_max;

            newMin.SetElement(
                axisIndex,
                newMin.GetElement(axisIndex) +
                    projectedContributionsFromMin.GetMin(projectedContributionsFromMax).Dot(Vector3::CreateOne()));
            newMax.SetElement(
                axisIndex,
                newMax.GetElement(axisIndex) +
                    projectedContributionsFromMin.GetMax(projectedContributionsFromMax).Dot(Vector3::CreateOne()));
        }

        m_min = newMin;
        m_max = newMax;
    }


    void Aabb::ApplyMatrix3x4(const Matrix3x4& matrix3x4)
    {
        // See ApplyTransform for more details on how this works.

        Vector3 newMin = matrix3x4.GetTranslation();
        Vector3 newMax = newMin;

        // Find extreme points for each axis.
        for (int axisIndex = 0; axisIndex < 3; axisIndex++)
        {
            const Vector3 axisCoeffs = matrix3x4.GetRowAsVector3(axisIndex);
            const Vector3 projectedContributionsFromMin = axisCoeffs * m_min;
            const Vector3 projectedContributionsFromMax = axisCoeffs * m_max;

            newMin.SetElement(
                axisIndex,
                newMin.GetElement(axisIndex) +
                    projectedContributionsFromMin.GetMin(projectedContributionsFromMax).Dot(Vector3::CreateOne()));
            newMax.SetElement(
                axisIndex,
                newMax.GetElement(axisIndex) +
                    projectedContributionsFromMin.GetMax(projectedContributionsFromMax).Dot(Vector3::CreateOne()));
        }

        m_min = newMin;
        m_max = newMax;
    }
}
