/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
        mRotation       = rotation;
        mPosition       = position;

        EMFX_SCALECODE
        (
            mScale = AZ::Vector3::CreateOne();
        )
    }


    // set
    void Transform::Set(const AZ::Vector3& position, const AZ::Quaternion& rotation, const AZ::Vector3& scale)
    {
    #ifdef EMFX_SCALE_DISABLED
        MCORE_UNUSED(scale);
    #endif

        mRotation       = rotation;
        mPosition       = position;

        EMFX_SCALECODE
        (
            mScale          = scale;
        )
    }


    // check if this transform is equal to another
    bool Transform::operator == (const Transform& right) const
    {
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mPosition, right.mPosition, MCore::Math::epsilon) == false)
        {
            return false;
        }

        if (MCore::Compare<AZ::Quaternion>::CheckIfIsClose(mRotation, right.mRotation, MCore::Math::epsilon) == false)
        {
            return false;
        }

        EMFX_SCALECODE
        (
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mScale, right.mScale, MCore::Math::epsilon) == false)
            {
                return false;
            }
        )

        return true;
    }


    // check if this transform is not equal to another
    bool Transform::operator != (const Transform& right) const
    {
        if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mPosition, right.mPosition, MCore::Math::epsilon))
        {
            return true;
        }

        if (MCore::Compare<AZ::Quaternion>::CheckIfIsClose(mRotation, right.mRotation, MCore::Math::epsilon))
        {
            return true;
        }

        EMFX_SCALECODE
        (
            if (MCore::Compare<AZ::Vector3>::CheckIfIsClose(mScale, right.mScale, MCore::Math::epsilon))
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
        mPosition = transform.GetTranslation();
        mScale = AZ::Vector3(transform.GetUniformScale());
        mRotation = transform.GetRotation();
    #else
        mPosition = transform.GetTranslation();
        mRotation = transform.GetRotation();
    #endif

        mRotation.Normalize();
    }


    // convert to a matrix
    AZ::Transform Transform::ToAZTransform() const
    {
        AZ::Transform result;

    #ifndef EMFX_SCALE_DISABLED
        result = MCore::CreateFromQuaternionAndTranslationAndScale(mRotation, mPosition, mScale);

    #else
        result = AZ::Transform::CreateFromQuaternionAndTranslation(mRotation, mPosition);
    #endif

        return result;
    }


    // identity the transform
    void Transform::Identity()
    {
        mPosition = AZ::Vector3::CreateZero();
        mRotation = AZ::Quaternion::CreateIdentity();

        EMFX_SCALECODE
        (
            mScale = AZ::Vector3::CreateOne();
        )
    }

    // Zero out the position, scale, and rotation.
    void Transform::Zero()
    {
        mPosition = AZ::Vector3::CreateZero();
        mRotation = AZ::Quaternion::CreateZero();
        EMFX_SCALECODE
        (
            mScale = AZ::Vector3::CreateZero();
        )
    }

    // Zero out the position and scale, but set quaternion to identity.
    void Transform::IdentityWithZeroScale()
    {
        mPosition = AZ::Vector3::CreateZero();
        mRotation = AZ::Quaternion::CreateIdentity();

        EMFX_SCALECODE
        (
            mScale = AZ::Vector3::CreateZero();
        );
    }

    // pre multiply with another
    Transform& Transform::PreMultiply(const Transform& other)
    {
    #ifdef EMFX_SCALE_DISABLED
        mPosition   += mRotation * other.mPosition;
    #else
        mPosition   += mRotation.TransformVector((other.mPosition * mScale));
    #endif

        mRotation = mRotation * other.mRotation;
        mRotation.Normalize();

        EMFX_SCALECODE
        (
            mScale = mScale * other.mScale;
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
        return mPosition + mRotation * point;
        #else
        return mPosition + mRotation.TransformVector((point * mScale));
        #endif
    }


    AZ::Vector3 Transform::RotateVector(const AZ::Vector3& v) const
    {
        return mRotation.TransformVector(v);
    }


    AZ::Vector3 Transform::TransformVector(const AZ::Vector3& v) const
    {
        #ifdef EMFX_SCALE_DISABLED
        return mRotation.TransformVector(v);
        #else
        return mRotation.TransformVector((v * mScale));
        #endif
    }


    // multiply
    Transform& Transform::Multiply(const Transform& other)
    {
    #ifdef EMFX_SCALE_DISABLED
        mPosition   = other.mRotation.TransformVector(mPosition) + other.mPosition;
    #else
        mPosition   = other.mRotation.TransformVector((mPosition * other.mScale)) + other.mPosition;
    #endif

        mRotation = other.mRotation * mRotation;
        mRotation.Normalize();

        EMFX_SCALECODE
        (
            mScale = other.mScale * mScale;
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
        mRotation.Normalize();
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
            mScale = mScale.GetReciprocal();
        )

        mRotation = mRotation.GetConjugate();

    #ifdef EMFX_SCALE_DISABLED
        mPosition = mRotation.TransformVector(-mPosition);
    #else
        mPosition = mRotation.TransformVector(-mPosition) * mScale;
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
        mPosition = MCore::Mirror(mPosition, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(mRotation.GetX(), mRotation.GetY(), mRotation.GetZ()), planeNormal);

        // update the rotation quaternion with inverted angle
        mRotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -mRotation.GetW());
        mRotation.Normalize();

        return *this;
    }


    // mirror the transform over the given normal, with given mirror flags
    Transform& Transform::MirrorWithFlags(const AZ::Vector3& planeNormal, uint8 mirrorFlags)
    {
        // apply the mirror flags
        ApplyMirrorFlags(this, mirrorFlags);

        // mirror the position over the plane with the specified normal
        mPosition = MCore::Mirror(mPosition, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(mRotation.GetX(), mRotation.GetY(), mRotation.GetZ()), planeNormal);

        // update the rotation quaternion with inverted angle
        mRotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -mRotation.GetW());
        mRotation.Normalize();

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
        outResult->mPosition    = mPosition + mRotation.TransformVector(other.mPosition);
    #else
        outResult->mPosition    = mPosition + (mRotation.TransformVector(other.mPosition) * mScale);
    #endif

        outResult->mRotation = mRotation * other.mRotation;
        outResult->mRotation.Normalize();

        EMFX_SCALECODE
        (
            outResult->mScale       = mScale * other.mScale;
        )
    }


    // multiply but output into another
    void Transform::Multiply(const Transform& other, Transform* outResult) const
    {
    #ifdef EMFX_SCALE_DISABLED
        outResult->mPosition    = other.mPosition + other.mRotation.TransformVector(mPosition);
    #else
        outResult->mPosition    = other.mPosition + (other.mRotation.TransformVector(mPosition) * other.mScale);
    #endif

        outResult->mRotation = other.mRotation * mRotation;
        outResult->mRotation.Normalize();

        EMFX_SCALECODE
        (
            outResult->mScale       = other.mScale * mScale;
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
        const AZ::Vector3 invScale = relativeTo.mScale.GetReciprocal();
        const AZ::Quaternion invRot = relativeTo.mRotation.GetConjugate();

        outTransform->mPosition = (invRot.TransformVector(mPosition - relativeTo.mPosition)) * invScale;
        outTransform->mRotation = invRot * mRotation;
        outTransform->mScale    = mScale * invScale;
    #else
        const AZ::Quaternion invRot = relativeTo.mRotation.GetConjugate();
        outTransform->mPosition = invRot.TransformVector(mPosition - relativeTo.mPosition);
        outTransform->mRotation = invRot * mRotation;
    #endif
        outTransform->mRotation.Normalize();
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
        outResult->mPosition = MCore::Mirror(mPosition, planeNormal);

        // mirror the quaternion axis component
        AZ::Vector3 mirrored = MCore::Mirror(AZ::Vector3(mRotation.GetX(), mRotation.GetY(), mRotation.GetZ()), planeNormal);
        outResult->mRotation.Set(mirrored.GetX(), mirrored.GetY(), mirrored.GetZ(), -mRotation.GetW()); // store the mirrored quat
        outResult->mRotation.Normalize();

        EMFX_SCALECODE
        (
            outResult->mScale = mScale;
        )
    }


    // check if there is scale present
    bool Transform::CheckIfHasScale() const
    {
    #ifdef EMFX_SCALE_DISABLED
        return false;
    #else
        return (
            !MCore::Compare<float>::CheckIfIsClose(mScale.GetX(), 1.0f, MCore::Math::epsilon) ||
            !MCore::Compare<float>::CheckIfIsClose(mScale.GetY(), 1.0f, MCore::Math::epsilon) ||
            !MCore::Compare<float>::CheckIfIsClose(mScale.GetZ(), 1.0f, MCore::Math::epsilon));
    #endif
    }


    // blend into another transform
    Transform& Transform::Blend(const Transform& dest, float weight)
    {
        mPosition = MCore::LinearInterpolate<AZ::Vector3>(mPosition, dest.mPosition, weight);
        mRotation = MCore::NLerp(mRotation, dest.mRotation, weight);

        EMFX_SCALECODE
        (
            mScale          = MCore::LinearInterpolate<AZ::Vector3>(mScale, dest.mScale, weight);
        )

        return *this;
    }


    // additive blend
    Transform& Transform::BlendAdditive(const Transform& dest, const Transform& orgTransform, float weight)
    {
        const AZ::Vector3    relPos      = dest.mPosition - orgTransform.mPosition;
        const AZ::Quaternion& orgRot     = orgTransform.mRotation;
        const AZ::Quaternion rot         = MCore::NLerp(orgRot, dest.mRotation, weight);

        // apply the relative changes
        mRotation = mRotation * (orgRot.GetConjugate() * rot);
        mRotation.Normalize();
        mPosition += (relPos * weight);

        EMFX_SCALECODE
        (
            mScale += (dest.mScale - orgTransform.mScale) * weight;
        )

        return *this;
    }


    Transform& Transform::ApplyAdditive(const Transform& additive)
    {
        mPosition += additive.mPosition;
        mRotation = mRotation * additive.mRotation;
        mRotation.Normalize();

        EMFX_SCALECODE
        (
            mScale *= additive.mScale;
        )
        return *this;
    }


    Transform& Transform::ApplyAdditive(const Transform& additive, float weight)
    {
        mPosition += additive.mPosition * weight;
        mRotation = MCore::NLerp(mRotation, mRotation * additive.mRotation, weight);
        EMFX_SCALECODE
        (
            mScale *= AZ::Vector3::CreateOne().Lerp(additive.mScale, weight);
        )
        return *this;
    }


    // sum the transforms
    Transform& Transform::Add(const Transform& other, float weight)
    {
        mPosition += other.mPosition    * weight;

        // make sure we use the correct hemisphere
        if (mRotation.Dot(other.mRotation) < 0.0f)
        {
            mRotation += -(other.mRotation) * weight;
        }
        else
        {
            mRotation += other.mRotation * weight;
        }

        EMFX_SCALECODE
        (
            mScale += other.mScale * weight;
        )

        return *this;
    }


    // add a transform
    Transform& Transform::Add(const Transform& other)
    {
        mPosition += other.mPosition;
        mRotation += other.mRotation;
        EMFX_SCALECODE
        (
            mScale += other.mScale;
        )
        return *this;
    }


    // subtract a transform
    Transform& Transform::Subtract(const Transform& other)
    {
        mPosition -= other.mPosition;
        mRotation -= other.mRotation;
        EMFX_SCALECODE
        (
            mScale -= other.mScale;
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

        MCore::LogInfo("mPosition = %.6f, %.6f, %.6f",
            static_cast<float>(mPosition.GetX()),
            static_cast<float>(mPosition.GetY()),
            static_cast<float>(mPosition.GetZ()));
        MCore::LogInfo("mRotation = %.6f, %.6f, %.6f, %.6f", 
            static_cast<float>(mRotation.GetX()),
            static_cast<float>(mRotation.GetY()),
            static_cast<float>(mRotation.GetZ()),
            static_cast<float>(mRotation.GetW()));

        EMFX_SCALECODE
        (
            MCore::LogInfo("mScale    = %.6f, %.6f, %.6f",
                static_cast<float>(mScale.GetX()),
                static_cast<float>(mScale.GetY()),
                static_cast<float>(mScale.GetZ()));
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
            inOutTransform->mRotation.SetW(inOutTransform->mRotation.GetW() * -1.0f);
            inOutTransform->mRotation.SetX(inOutTransform->mRotation.GetX() * -1.0f);
            inOutTransform->mPosition.SetY(inOutTransform->mPosition.GetY() * -1.0f);
            inOutTransform->mPosition.SetZ(inOutTransform->mPosition.GetZ() * -1.0f);
            return;
        }
        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_Y)
        {
            inOutTransform->mRotation.SetW(inOutTransform->mRotation.GetW() * -1.0f);
            inOutTransform->mRotation.SetY(inOutTransform->mPosition.GetY() * -1.0f);
            inOutTransform->mPosition.SetX(inOutTransform->mPosition.GetX() * -1.0f);
            inOutTransform->mPosition.SetZ(inOutTransform->mPosition.GetZ() * -1.0f);
            return;
        }
        if (mirrorFlags & Actor::MIRRORFLAG_INVERT_Z)
        {
            inOutTransform->mRotation.SetW(inOutTransform->mRotation.GetW() * -1.0f);
            inOutTransform->mRotation.SetZ(inOutTransform->mPosition.GetZ() * -1.0f);
            inOutTransform->mPosition.SetX(inOutTransform->mPosition.GetX() * -1.0f);
            inOutTransform->mPosition.SetY(inOutTransform->mPosition.GetY() * -1.0f);
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
            mPosition.SetZ(0.0f);
        }

        // Only keep the rotation on the Z axis.
        mRotation.SetX(0.0f);
        mRotation.SetY(0.0f);
        mRotation.Normalize();
    }


    // Return a version of this transform projected to the ground plane.
    Transform Transform::ProjectedToGroundPlane() const
    {
        Transform result(*this);

        // Only keep translation over the XY plane and assume a height of 0.
        result.mPosition.SetZ(0.0f);

        // Only keep the rotation on the Z axis.
        result.mRotation.SetX(0.0f);
        result.mRotation.SetY(0.0f);
        result.mRotation.Normalize();

        return result;
    }
}   // namespace EMotionFX
