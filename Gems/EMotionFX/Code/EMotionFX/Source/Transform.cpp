/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// include required headers
#include "Transform.h"
#include "Actor.h"
#include "PlayBackInfo.h"
#include <MCore/Source/Compare.h>
#include <MCore/Source/LogManager.h>


namespace EMotionFX
{
    Transform::Transform(const AZ::Vector3& pos, const AZ::Quaternion& rotation)
    {
        Set(pos, rotation);
    }


    Transform::Transform(const AZ::Vector3& pos, const AZ::Quaternion& rotation, const AZ::Vector3& scale)
    {
        Set(pos, rotation, scale);
    }


    // init from a matrix
    Transform::Transform(const AZ::Transform& transform)
    {
        InitFromAZTransform(transform);
    }

    Transform Transform::CreateIdentity()
    {
        return Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity());
    }

    Transform Transform::CreateIdentityWithZeroScale()
    {
        return Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateIdentity(), AZ::Vector3::CreateZero());
    }

    Transform Transform::CreateZero()
    {
        return Transform(AZ::Vector3::CreateZero(), AZ::Quaternion::CreateZero(), AZ::Vector3::CreateZero());
    }

    // set
    void Transform::Set(const AZ::Vector3& position, const AZ::Quaternion& rotation)
    {
        m_rotation       = rotation;
        m_position       = position;

        EMFX_SCALECODE
        (
            m_scale = AZ::Vector3::CreateOne();
        )
    }


    // set
    void Transform::Set(const AZ::Vector3& position, const AZ::Quaternion& rotation, const AZ::Vector3& scale)
    {
    #ifdef EMFX_SCALE_DISABLED
        MCORE_UNUSED(scale);
    #endif

        m_rotation       = rotation;
        m_position       = position;

        EMFX_SCALECODE
        (
            m_scale          = scale;
        )
    }


    // check if this transform is equal to another
    bool Transform::operator == (const Transform& right) const
    {
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(m_position, right.m_position, MCore::Math::epsilon) == false)
        {
            return false;
        }

        if (MCore::Compare<AZ::Quaternion>::CheckIfIsClose(m_rotation, right.m_rotation, MCore::Math::epsilon) == false)
        {
            return false;
        }

        EMFX_SCALECODE
        (
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(m_scale, right.m_scale, MCore::Math::epsilon) == false)
            {
                return false;
            }
        )

        return true;
    }


    // check if this transform is not equal to another
    bool Transform::operator != (const Transform& right) const
    {
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(m_position, right.m_position, MCore::Math::epsilon))
        {
            return true;
        }

        if (MCore::Compare<AZ::Quaternion>::CheckIfIsClose(m_rotation, right.m_rotation, MCore::Math::epsilon))
        {
            return true;
        }

        EMFX_SCALECODE
        (
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(m_scale, right.m_scale, MCore::Math::epsilon))
            {
                return true;
            }
        )

        return false;
    }


    // add a given transform
    Transform Transform::operator +  (const Transform& right) const
    {
        Transform result(*this);
        return result.Add(right);
    }


    // subtract a given transform
    Transform Transform::operator - (const Transform& right) const
    {
        Transform result(*this);
        return result.Subtract(right);
    }


    // multiply a given transform
    Transform Transform::operator * (const Transform& right) const
    {
        Transform result(*this);
        return result.Multiply(right);
    }


    // add
    Transform& Transform::operator += (const Transform& right)
    {
        return Add(right);
    }


    // subtract
    Transform& Transform::operator -= (const Transform& right)
    {
        return Subtract(right);
    }


    // multiply
    Transform& Transform::operator *= (const Transform& right)
    {
        return Multiply(right);
    }


    // init from a matrix
    void Transform::InitFromAZTransform(const AZ::Transform& transform)
    {
    #ifndef EMFX_SCALE_DISABLED
        m_position = transform.GetTranslation();
        m_scale = AZ::Vector3(transform.GetUniformScale());
        m_rotation = transform.GetRotation();
    #else
        m_position = transform.GetTranslation();
        m_rotation = transform.GetRotation();
    #endif

        m_rotation.Normalize();
    }


    // convert to a matrix
    AZ::Transform Transform::ToAZTransform() const
    {
        AZ::Transform result;

    #ifndef EMFX_SCALE_DISABLED
        result = MCore::CreateFromQuaternionAndTranslationAndScale(m_rotation, m_position, m_scale);

    #else
        result = AZ::Transform::CreateFromQuaternionAndTranslation(m_rotation, m_position);
    #endif

        return result;
    }


    // identity the transform
    void Transform::Identity()
    {
        m_position = AZ::Vector3::CreateZero();
        m_rotation = AZ::Quaternion::CreateIdentity();

        EMFX_SCALECODE
        (
            m_scale = AZ::Vector3::CreateOne();
        )
    }

    // Zero out the position, scale, and rotation.
    void Transform::Zero()
    {
        m_position = AZ::Vector3::CreateZero();
        m_rotation = AZ::Quaternion::CreateZero();
        EMFX_SCALECODE
        (
            m_scale = AZ::Vector3::CreateZero();
        )
    }

    // Zero out the position and scale, but set quaternion to identity.
    void Transform::IdentityWithZeroScale()
    {
        m_position = AZ::Vector3::CreateZero();
        m_rotation = AZ::Quaternion::CreateIdentity();

        EMFX_SCALECODE
        (
            m_scale = AZ::Vector3::CreateZero();
        );
    }

    // pre multiply with another
    Transform& Transform::PreMultiply(const Transform& other)
    {
    #ifdef EMFX_SCALE_DISABLED
        m_position   += m_rotation * other.m_position;
    #else
        m_position   += m_rotation.TransformVector((other.m_position * m_scale));
    #endif

        m_rotation = m_rotation * other.m_rotation;
        m_rotation.Normalize();

        EMFX_SCALECODE
        (
            m_scale = m_scale * other.m_scale;
        )

        return *this;
    }


    // return a multiplied copy
    Transform Transform::PreMultiplied(const Transform& other) const
    {
        Transform result(*this);
        result.PreMultiply(other);
        return result;
    }


    AZ::Vector3 Transform::TransformPoint(const AZ::Vector3& point) const
    {
        #ifdef EMFX_SCALE_DISABLED
        return m_position + m_rotation * point;
        #else
        return m_position + m_rotation.TransformVector((point * m_scale));
        #endif
    }


    AZ::Vector3 Transform::RotateVector(const AZ::Vector3& v) const
    {
        return m_rotation.TransformVector(v);
    }


    AZ::Vector3 Transform::TransformVector(const AZ::Vector3& v) const
    {
        #ifdef EMFX_SCALE_DISABLED
        return m_rotation.TransformVector(v);
        #else
        return m_rotation.TransformVector((v * m_scale));
        #endif
    }


    // multiply
    Transform& Transform::Multiply(const Transform& other)
    {
    #ifdef EMFX_SCALE_DISABLED
        m_position   = other.m_rotation.TransformVector(m_position) + other.m_position;
    #else
        m_position   = other.m_rotation.TransformVector((m_position * other.m_scale)) + other.m_position;
    #endif

        m_rotation = other.m_rotation * m_rotation;
        m_rotation.Normalize();

        EMFX_SCALECODE
        (
            m_scale = other.m_scale * m_scale;
        )
        return *this;
    }


    // return a multiplied copy
    Transform Transform::Multiplied(const Transform& other) const
    {
        Transform result(*this);
        result.Multiply(other);
        return result;
    }


    // normalize the quaternions
    Transform& Transform::Normalize()
    {
        m_rotation.Normalize();
        return *this;
    }


    // return a normalized copy
    Transform Transform::Normalized() const
    {
        Transform result(*this);
        result.Normalize();
        return result;
    }


    // inverse the transform
    Transform& Transform::Inverse()
    {
        EMFX_SCALECODE
        (
            m_scale = m_scale.GetReciprocal();
        )

        m_rotation = m_rotation.GetConjugate();

    #ifdef EMFX_SCALE_DISABLED
        m_position = m_rotation.TransformVector(-m_position);
    #else
        m_position = m_rotation.TransformVector(-m_position) * m_scale;
    #endif

        return *this;
    }


    // return an inversed copy
    Transform Transform::Inversed() const
    {
        Transform result(*this);
        result.Inverse();
        return result;
    }


    // mirror the transform over the given normal
    Transform& Transform::Mirror(const AZ::Vector3& planeNormal)
    {
        // mirror the position over the plane with the specified normal
        m_position = MCore::Mirror(m_position, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(m_rotation.GetX(), m_rotation.GetY(), m_rotation.GetZ()), planeNormal);

        // update the rotation quaternion with inverted angle
        m_rotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -m_rotation.GetW());
        m_rotation.Normalize();

        return *this;
    }


    // mirror the transform over the given normal, with given mirror flags
    Transform& Transform::MirrorWithFlags(const AZ::Vector3& planeNormal, uint8 mirrorFlags)
    {
        // apply the mirror flags
        ApplyMirrorFlags(this, mirrorFlags);

        // mirror the position over the plane with the specified normal
        m_position = MCore::Mirror(m_position, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(m_rotation.GetX(), m_rotation.GetY(), m_rotation.GetZ()), planeNormal);

        // update the rotation quaternion with inverted angle
        m_rotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -m_rotation.GetW());
        m_rotation.Normalize();

        return *this;
    }


    // return an mirrored copy
    Transform Transform::Mirrored(const AZ::Vector3& planeNormal) const
    {
        Transform result(*this);
        result.Mirror(planeNormal);
        return result;
    }


    // multiply but output into another
    void Transform::PreMultiply(const Transform& other, Transform* outResult) const
    {
    #ifdef EMFX_SCALE_DISABLED
        outResult->m_position    = m_position + m_rotation.TransformVector(other.m_position);
    #else
        outResult->m_position    = m_position + (m_rotation.TransformVector(other.m_position) * m_scale);
    #endif

        outResult->m_rotation = m_rotation * other.m_rotation;
        outResult->m_rotation.Normalize();

        EMFX_SCALECODE
        (
            outResult->m_scale       = m_scale * other.m_scale;
        )
    }


    // multiply but output into another
    void Transform::Multiply(const Transform& other, Transform* outResult) const
    {
    #ifdef EMFX_SCALE_DISABLED
        outResult->m_position    = other.m_position + other.m_rotation.TransformVector(m_position);
    #else
        outResult->m_position    = other.m_position + (other.m_rotation.TransformVector(m_position) * other.m_scale);
    #endif

        outResult->m_rotation = other.m_rotation * m_rotation;
        outResult->m_rotation.Normalize();

        EMFX_SCALECODE
        (
            outResult->m_scale       = other.m_scale * m_scale;
        )
    }



    // inverse but store the result into another
    void Transform::Inverse(Transform* outResult) const
    {
        *outResult = *this;
        outResult->Inverse();
    }


    // calculate the transform relative to another
    void Transform::CalcRelativeTo(const Transform& relativeTo, Transform* outTransform) const
    {
    #ifndef EMFX_SCALE_DISABLED
        const AZ::Vector3 invScale = relativeTo.m_scale.GetReciprocal();
        const AZ::Quaternion invRot = relativeTo.m_rotation.GetConjugate();

        outTransform->m_position = (invRot.TransformVector(m_position - relativeTo.m_position)) * invScale;
        outTransform->m_rotation = invRot * m_rotation;
        outTransform->m_scale    = m_scale * invScale;
    #else
        const AZ::Quaternion invRot = relativeTo.m_rotation.GetConjugate();
        outTransform->m_position = invRot.TransformVector(m_position - relativeTo.m_position);
        outTransform->m_rotation = invRot * m_rotation;
    #endif
        outTransform->m_rotation.Normalize();
    }


    // return this transformation relative to another
    Transform Transform::CalcRelativeTo(const Transform& relativeTo) const
    {
        Transform result;
        CalcRelativeTo(relativeTo, &result);
        return result;
    }


    // mirror but output into another transform
    void Transform::Mirror(const AZ::Vector3& planeNormal, Transform* outResult) const
    {
        // mirror the position over the normal
        outResult->m_position = MCore::Mirror(m_position, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(m_rotation.GetX(), m_rotation.GetY(), m_rotation.GetZ()), planeNormal);
        outResult->m_rotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -m_rotation.GetW()); // store the mirrored quat
        outResult->m_rotation.Normalize();

        EMFX_SCALECODE
        (
            outResult->m_scale = m_scale;
        )
    }


    // check if there is scale present
    bool Transform::CheckIfHasScale() const
    {
    #ifdef EMFX_SCALE_DISABLED
        return false;
    #else
        return (
            !MCore::Compare<float>::CheckIfIsClose(m_scale.GetX(), 1.0f, MCore::Math::epsilon) ||
            !MCore::Compare<float>::CheckIfIsClose(m_scale.GetY(), 1.0f, MCore::Math::epsilon) ||
            !MCore::Compare<float>::CheckIfIsClose(m_scale.GetZ(), 1.0f, MCore::Math::epsilon));
    #endif
    }


    // blend into another transform
    Transform& Transform::Blend(const Transform& dest, float weight)
    {
        m_position = MCore::LinearInterpolate<AZ::Vector3>(m_position, dest.m_position, weight);
        m_rotation = MCore::NLerp(m_rotation, dest.m_rotation, weight);

        EMFX_SCALECODE
        (
            m_scale          = MCore::LinearInterpolate<AZ::Vector3>(m_scale, dest.m_scale, weight);
        )

        return *this;
    }


    // additive blend
    Transform& Transform::BlendAdditive(const Transform& dest, const Transform& orgTransform, float weight)
    {
        const AZ::Vector3    relPos      = dest.m_position - orgTransform.m_position;
        const AZ::Quaternion& orgRot     = orgTransform.m_rotation;
        const AZ::Quaternion rot         = MCore::NLerp(orgRot, dest.m_rotation, weight);

        // apply the relative changes
        m_rotation = m_rotation * (orgRot.GetConjugate() * rot);
        m_rotation.Normalize();
        m_position += (relPos * weight);

        EMFX_SCALECODE
        (
            m_scale += (dest.m_scale - orgTransform.m_scale) * weight;
        )

        return *this;
    }


    Transform& Transform::ApplyAdditive(const Transform& additive)
    {
        m_position += additive.m_position;
        m_rotation = m_rotation * additive.m_rotation;
        m_rotation.Normalize();

        EMFX_SCALECODE
        (
            m_scale *= additive.m_scale;
        )
        return *this;
    }


    Transform& Transform::ApplyAdditive(const Transform& additive, float weight)
    {
        m_position += additive.m_position * weight;
        m_rotation = MCore::NLerp(m_rotation, m_rotation * additive.m_rotation, weight);
        EMFX_SCALECODE
        (
            m_scale *= AZ::Vector3::CreateOne().Lerp(additive.m_scale, weight);
        )
        return *this;
    }


    // sum the transforms
    Transform& Transform::Add(const Transform& other, float weight)
    {
        m_position += other.m_position    * weight;

        // make sure we use the correct hemisphere
        if (m_rotation.Dot(other.m_rotation) < 0.0f)
        {
            m_rotation += -(other.m_rotation) * weight;
        }
        else
        {
            m_rotation += other.m_rotation * weight;
        }

        EMFX_SCALECODE
        (
            m_scale += other.m_scale * weight;
        )

        return *this;
    }


    // add a transform
    Transform& Transform::Add(const Transform& other)
    {
        m_position += other.m_position;
        m_rotation += other.m_rotation;
        EMFX_SCALECODE
        (
            m_scale += other.m_scale;
        )
        return *this;
    }


    // subtract a transform
    Transform& Transform::Subtract(const Transform& other)
    {
        m_position -= other.m_position;
        m_rotation -= other.m_rotation;
        EMFX_SCALECODE
        (
            m_scale -= other.m_scale;
        )
        return *this;
    }


    // log the transform
    void Transform::Log(const char* name) const
    {
        if (name)
        {
            MCore::LogInfo("Transform(%s):", name);
        }

        MCore::LogInfo("m_position = %.6f, %.6f, %.6f",
            static_cast<float>(m_position.GetX()),
            static_cast<float>(m_position.GetY()),
            static_cast<float>(m_position.GetZ()));
        MCore::LogInfo("m_rotation = %.6f, %.6f, %.6f, %.6f", 
            static_cast<float>(m_rotation.GetX()),
            static_cast<float>(m_rotation.GetY()),
            static_cast<float>(m_rotation.GetZ()),
            static_cast<float>(m_rotation.GetW()));

        EMFX_SCALECODE
        (
            MCore::LogInfo("m_scale    = %.6f, %.6f, %.6f",
                static_cast<float>(m_scale.GetX()),
                static_cast<float>(m_scale.GetY()),
                static_cast<float>(m_scale.GetZ()));
        )
    }


    // apply mirror flags to this transformation
    void Transform::ApplyMirrorFlags(Transform* inOutTransform, uint8 mirrorFlags)
    {
        if (mirrorFlags == 0)
        {
            return;
        }

        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_X)
        {
            inOutTransform->m_rotation.SetW(inOutTransform->m_rotation.GetW() * -1.0f);
            inOutTransform->m_rotation.SetX(inOutTransform->m_rotation.GetX() * -1.0f);
            inOutTransform->m_position.SetY(inOutTransform->m_position.GetY() * -1.0f);
            inOutTransform->m_position.SetZ(inOutTransform->m_position.GetZ() * -1.0f);
            return;
        }
        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_Y)
        {
            inOutTransform->m_rotation.SetW(inOutTransform->m_rotation.GetW() * -1.0f);
            inOutTransform->m_rotation.SetY(inOutTransform->m_position.GetY() * -1.0f);
            inOutTransform->m_position.SetX(inOutTransform->m_position.GetX() * -1.0f);
            inOutTransform->m_position.SetZ(inOutTransform->m_position.GetZ() * -1.0f);
            return;
        }
        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_Z)
        {
            inOutTransform->m_rotation.SetW(inOutTransform->m_rotation.GetW() * -1.0f);
            inOutTransform->m_rotation.SetZ(inOutTransform->m_position.GetZ() * -1.0f);
            inOutTransform->m_position.SetX(inOutTransform->m_position.GetX() * -1.0f);
            inOutTransform->m_position.SetY(inOutTransform->m_position.GetY() * -1.0f);
            return;
        }
    }


    // apply the mirrored version of the delta to this transformation
    void Transform::ApplyDeltaMirrored(const Transform& sourceTransform, const Transform& targetTransform, const AZ::Vector3& mirrorPlaneNormal, uint8 mirrorFlags)
    {
        // calculate the delta from source towards target transform
        const Transform invSourceTransform = sourceTransform.Inversed();

        Transform delta = targetTransform.Multiplied(invSourceTransform);

        Transform::ApplyMirrorFlags(&delta, mirrorFlags);

        // mirror the delta over the plane which the specified normal
        delta.Mirror(mirrorPlaneNormal);

        // apply the mirrored delta to the current transform
        PreMultiply(delta);
    }


    // apply the delta from source towards target transform to this transformation
    void Transform::ApplyDelta(const Transform& sourceTransform, const Transform& targetTransform)
    {
        // calculate the delta from source towards target transform
        const Transform invSourceTransform = sourceTransform.Inversed();

        const Transform delta = targetTransform.Multiplied(invSourceTransform);

        PreMultiply(delta);
    }


    // apply the delta from source towards target transform to this transformation
    void Transform::ApplyDeltaWithWeight(const Transform& sourceTransform, const Transform& targetTransform, float weight)
    {
        // calculate the delta from source towards target transform
        const Transform invSourceTransform = sourceTransform.Inversed();
        const Transform targetDelta = targetTransform.Multiplied(invSourceTransform);
        const Transform finalDelta = Transform::CreateIdentity().Blend(targetDelta, weight); // inits at identity

        // apply the delta to the current transform
        PreMultiply(finalDelta);
    }


    void Transform::ApplyMotionExtractionFlags(EMotionExtractionFlags flags)
    {
        // Only keep translation over the XY plane and assume a height of 0.
        if (!(flags & MOTIONEXTRACT_CAPTURE_Z))
        {
            m_position.SetZ(0.0f);
        }

        // Only keep the rotation on the Z axis.
        m_rotation.SetX(0.0f);
        m_rotation.SetY(0.0f);
        m_rotation.Normalize();
    }


    // Return a version of this transform projected to the ground plane.
    Transform Transform::ProjectedToGroundPlane() const
    {
        Transform result(*this);

        // Only keep translation over the XY plane and assume a height of 0.
        result.m_position.SetZ(0.0f);

        // Only keep the rotation on the Z axis.
        result.m_rotation.SetX(0.0f);
        result.m_rotation.SetY(0.0f);
        result.m_rotation.Normalize();

        return result;
    }
}   // namespace EMotionFX
