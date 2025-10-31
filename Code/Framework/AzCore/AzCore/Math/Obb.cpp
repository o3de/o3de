/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Math/Obb.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathScriptHelpers.h>
#include <AzCore/Serialization/Locale.h>

namespace AZ
{
    namespace Internal
    {
        Obb ConstructObbFromValues
            ( float posX, float posY, float posZ
            , float axisXX, float axisXY, float axisXZ, float halfX
            , float axisYX, float axisYY, float axisYZ, float halfY
            , float axisZX, float axisZY, float axisZZ, float halfZ)
        {
            const Vector3 position(posX, posY, posZ);
            const Quaternion rotation = Quaternion::CreateFromMatrix3x3(Matrix3x3::CreateFromColumns(
                Vector3(axisXX, axisXY, axisXZ), Vector3(axisYX, axisYY, axisYZ), Vector3(axisZX, axisZY, axisZZ)));
            const Vector3 halfLengths(halfX, halfY, halfZ);

            return Obb::CreateFromPositionRotationAndHalfLengths(position, rotation, halfLengths);
        }


        void ObbDefaultConstructor(Obb* thisPtr)
        {
            new (thisPtr) Obb(Obb::CreateFromPositionRotationAndHalfLengths(
                Vector3::CreateZero(),
                Quaternion::CreateIdentity(),
                Vector3(0.5f)));
        }


        AZStd::string ObbToString(const Obb& obb)
        {
            AZ::Locale::ScopedSerializationLocale locale;

            return AZStd::string::format("Position %s AxisX %s AxisY %s AxisZ %s halfLengthX=%.7f halfLengthY=%.7f halfLengthZ=%.7f",
                Vector3ToString(obb.GetPosition()).c_str(),
                Vector3ToString(obb.GetAxisX()).c_str(),
                Vector3ToString(obb.GetAxisY()).c_str(),
                Vector3ToString(obb.GetAxisZ()).c_str(),
                obb.GetHalfLengthX(),
                obb.GetHalfLengthY(),
                obb.GetHalfLengthZ());
        }
    }


    void Obb::Reflect(ReflectContext* context)
    {
        auto serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<Obb>()
                ->Field("position", &Obb::m_position)
                ->Field("rotation", &Obb::m_rotation)
                ->Field("halfLengths", &Obb::m_halfLengths)
                ;
        }

        auto behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<Obb>()->
                Attribute(Script::Attributes::ExcludeFrom, Script::Attributes::ExcludeFlags::ListOnly)->
                Attribute(Script::Attributes::Storage, Script::Attributes::StorageType::Value)->
                Attribute(Script::Attributes::GenericConstructorOverride, &Internal::ObbDefaultConstructor)->
                Property("position", &Obb::GetPosition, &Obb::SetPosition)->
                Property("rotation", &Obb::GetRotation, &Obb::SetRotation)->
                Property("halfLengths", &Obb::GetHalfLengths, &Obb::SetHalfLengths)->
                Method("ToString", &Internal::ObbToString)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::ToString)->
                Method("CreateFromPositionRotationAndHalfLengths", &Obb::CreateFromPositionRotationAndHalfLengths)->
                Method("CreateFromAabb", &Obb::CreateFromAabb)->
                Method("ConstructObbFromValues", &Internal::ConstructObbFromValues)->
                Method("GetAxis", &Obb::GetAxis)->
                Method("GetAxisX", &Obb::GetAxisX)->
                Method("GetAxisY", &Obb::GetAxisY)->
                Method("GetAxisZ", &Obb::GetAxisZ)->
                Method("GetHalfLength", &Obb::GetHalfLength)->
                Method("GetHalfLengthX", &Obb::GetHalfLengthX)->
                Method("GetHalfLengthY", &Obb::GetHalfLengthY)->
                Method("GetHalfLengthZ", &Obb::GetHalfLengthZ)->
                Method("SetHalfLength", &Obb::SetHalfLength)->
                Method("SetHalfLengthX", &Obb::SetHalfLengthX)->
                Method("SetHalfLengthY", &Obb::SetHalfLengthY)->
                Method("SetHalfLengthZ", &Obb::SetHalfLengthZ)->
                Method("Clone", [](const Obb& rhs) -> Obb { return rhs; })->
                Method("IsFinite", &Obb::IsFinite)->
                Method("Equal", &Obb::operator==)->
                    Attribute(Script::Attributes::Operator, Script::Attributes::OperatorType::Equal);
        }
    }


    Obb Obb::CreateFromPositionRotationAndHalfLengths(
        const Vector3& position, const Quaternion& rotation, const Vector3& halfLengths)
    {
        Obb result;
        result.m_position = position;
        result.m_rotation = rotation;
        result.m_halfLengths = halfLengths;
        return result;
    }


    Obb Obb::CreateFromAabb(const Aabb& aabb)
    {
        return CreateFromPositionRotationAndHalfLengths(
            aabb.GetCenter(),
            Quaternion::CreateIdentity(),
            0.5f * aabb.GetExtents()
        );
    }


    bool Obb::IsFinite() const
    {
        return
            m_position.IsFinite() &&
            m_rotation.IsFinite() &&
            m_halfLengths.IsFinite();
    }


    bool Obb::operator==(const Obb& rhs) const
    {
        return
            m_position.IsClose(rhs.m_position) &&
            m_rotation.IsClose(rhs.m_rotation) &&
            m_halfLengths.IsClose(rhs.m_halfLengths);
    }


    bool Obb::operator!=(const Obb& rhs) const
    {
        return !(*this == rhs);
    }


    Obb operator*(const Transform& transform, const Obb& obb)
    {
        return Obb::CreateFromPositionRotationAndHalfLengths(
            transform.TransformPoint(obb.GetPosition()),
            transform.GetRotation() * obb.GetRotation(),
            transform.GetUniformScale() * obb.GetHalfLengths()
        );
    }
}
