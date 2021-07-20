/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Plane.h>
#include <AzCore/Math/MathScriptHelpers.h>

namespace AZ
{
    namespace Internal
    {
        void PlaneDefaultConstructor(Plane* thisPtr)
        {
            new (thisPtr) Plane(Plane::CreateFromNormalAndPoint(Vector3(0, 0, 1), Vector3::CreateZero()));
        }


        void PlaneSetGeneric(Plane* thisPtr, ScriptDataContext& dc)
        {
            // Plane.SetGeneric has three overloads:
            // Set(Vector4)
            // Set(Vector3, float)
            // Set(float, float, float, float)
            bool valueWasSet = false;
            if (dc.GetNumArguments() == 1 && dc.IsClass<Vector4>(0))
            {
                Vector4 vector4 = Vector4::CreateZero();
                dc.ReadArg(0, vector4);
                thisPtr->Set(vector4);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 2 && dc.IsClass<Vector3>(0) && dc.IsNumber(1))
            {
                Vector3 normal = Vector3::CreateZero();
                float d = 0;
                dc.ReadArg(0, normal);
                dc.ReadArg(1, d);
                thisPtr->Set(normal, d);
                valueWasSet = true;
            }
            else if (dc.GetNumArguments() == 4 &&
                dc.IsNumber(0) &&
                dc.IsNumber(1) &&
                dc.IsNumber(2) &&
                dc.IsNumber(3))
            {
                float x = 0;
                float y = 0;
                float z = 0;
                float d = 0;
                dc.ReadArg(0, x);
                dc.ReadArg(1, y);
                dc.ReadArg(2, z);
                dc.ReadArg(3, d);
                thisPtr->Set(x, y, z, d);
                valueWasSet = true;
            }
            if (!valueWasSet)
            {
                AZ_Error("Script", false, "Plane.Set only supports Set(Vector4), Set(Vector3, number), Set(number, number, number, number)");
            }
        }


        void PlaneCastRayMultipleReturn(const Plane* thisPtr, ScriptDataContext& dc)
        {
            // CastRay as implemented for Planes has two overloads,
            // each with the same pair of inputs but different return values.
            // As one return value (hit point) is based on the other (hit time), for simplicity, the Lua implementation
            // just returns all three values: does the ray hit? When does it hit? Where does it hit?
            if (!dc.IsClass<Vector3>(0) || !dc.IsClass<Vector3>(1))
            {
                AZ_Error("Script", false, "ScriptPlane CastRay requires two ScriptVector3s as arguments.");
                return;
            }

            Vector3 start(Vector3::CreateZero()), dir(Vector3::CreateZero()), hitPoint;
            float hitTime = 0.0f;

            dc.ReadArg(0, start);
            dc.ReadArg(1, dir);

            bool result = thisPtr->CastRay(start, dir, hitTime);
            hitPoint = start + dir * hitTime;

            dc.PushResult(result);
            dc.PushResult(hitPoint);
            dc.PushResult(hitTime);
        }


        void PlaneIntersectSegmentMultipleReturn(const Plane* thisPtr, ScriptDataContext& dc)
        {
            // IntersectSegment as implemented for Planes has two overloads,
            // each with the same pair of inputs but different return values.
            // As one return value (hit point) is based on the other (hit time), for simplicity, the Lua implementation
            // just returns all three values: does the ray hit? When does it hit? Where does it hit?

            if (!dc.IsClass<Vector3>(0) || !dc.IsClass<Vector3>(1))
            {
                AZ_Error("Script", false, "ScriptPlane IntersectSegment requires two ScriptVector3s as arguments.");
                return;
            }

            Vector3 start(Vector3::CreateZero()), end(Vector3::CreateZero()), hitPoint;
            float hitTime = 0.0f;

            dc.ReadArg(0, start);
            dc.ReadArg(1, end);

            bool result = thisPtr->IntersectSegment(start, end, hitTime);
            Vector3 dir = end - start;
            hitPoint = start + dir * hitTime;

            dc.PushResult(result);
            dc.PushResult(hitPoint);
            dc.PushResult(hitTime);
        }


        AZStd::string PlaneToString(const Plane& p)
        {
            return AZStd::string::format("%s", Vector4ToString(p.GetPlaneEquationCoefficients()).c_str());
        }
    }


    void Plane::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Plane>()
                ->Field("plane", &Plane::m_plane);
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Plane>()->
                Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Attribute(AZ::Script::Attributes::Storage, AZ::Script::Attributes::StorageType::Value)->
                Attribute(AZ::Script::Attributes::GenericConstructorOverride, &Internal::PlaneDefaultConstructor)->
                Method("ToString", &Internal::PlaneToString)->
                    Attribute(AZ::Script::Attributes::Operator, AZ::Script::Attributes::OperatorType::ToString)->
                Method("Clone", [](const Plane& rhs) -> Plane { return rhs; })->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("CreateFromCoefficients", &Plane::CreateFromCoefficients)->                
                Method("CreateFromNormalAndPoint", &Plane::CreateFromNormalAndPoint)->
                Method("CreateFromNormalAndDistance", &Plane::CreateFromNormalAndDistance)->
                Method<void(Plane::*)(const Vector4&)>("Set", &Plane::Set)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::PlaneSetGeneric)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("SetNormal", &Plane::SetNormal)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("SetDistance", &Plane::SetDistance)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("ApplyTransform", &Plane::ApplyTransform)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("GetTransform", &Plane::GetTransform)->
                Method("GetPlaneEquationCoefficients", &Plane::GetPlaneEquationCoefficients)->
                Method("GetNormal", &Plane::GetNormal)->
                Method("GetDistance", &Plane::GetDistance)->
                Method("GetPointDist", &Plane::GetPointDist)->
                Method("GetProjected", &Plane::GetProjected)->
                Method<bool(Plane::*)(const Vector3&,const Vector3&,float&) const>("CastRay", &Plane::CastRay)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::PlaneCastRayMultipleReturn)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method<bool(Plane::*)(const Vector3&,const Vector3&, float&) const>("IntersectSegment", &Plane::IntersectSegment)->
                    Attribute(AZ::Script::Attributes::MethodOverride, &Internal::PlaneIntersectSegmentMultipleReturn)->
                    Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)->
                Method("IsFinite", &Plane::IsFinite)
                ;
        }
    }
}
