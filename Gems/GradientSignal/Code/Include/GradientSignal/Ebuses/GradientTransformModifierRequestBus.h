/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    //! TransformType describes where the gradient's origin is mapped to.
    enum class TransformType : AZ::u8
    {
        //! The gradient's origin is the world position of this entity.
        World_ThisEntity = 0,
        //! The gradient's origin is the local position of this entity, but in world space.
        //! i.e. If the parent is at (2, 2), and the gradient is at (3,3) in local space, the gradient entity itself will be at (5,5) in
        //! world space but its origin will frozen at (3,3) in world space, no matter how much the parent moves around.
        Local_ThisEntity,
        //! The gradient's origin is the world position of the reference entity.
        World_ReferenceEntity,
        //! The gradient's origin is the local position of the reference entity, but in world space.
        Local_ReferenceEntity,
        //! The gradient's origin is at (0,0,0) in world space.
        World_Origin,
        //! The gradient's origin is in translated world space relative to the reference entity.
        Relative,
    };

    class GradientTransformModifierRequests
        : public AZ::ComponentBus
    {
    public:
        //! Overrides the default AZ::EBusTraits handler policy to allow only one listener.
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetAllowReference() const = 0;
        virtual void SetAllowReference(bool value) = 0;

        virtual AZ::EntityId GetShapeReference() const = 0;
        virtual void SetShapeReference(AZ::EntityId shapeReference) = 0;

        virtual bool GetOverrideBounds() const = 0;
        virtual void SetOverrideBounds(bool value) = 0;

        virtual AZ::Vector3 GetBounds() const = 0;
        virtual void SetBounds(const AZ::Vector3& bounds) = 0;

        virtual AZ::Vector3 GetCenter() const = 0;
        virtual void SetCenter(const AZ::Vector3& center) = 0;

        virtual TransformType GetTransformType() const = 0;
        virtual void SetTransformType(TransformType type) = 0;

        virtual bool GetOverrideTranslate() const = 0;
        virtual void SetOverrideTranslate(bool value) = 0;

        virtual AZ::Vector3 GetTranslate() const = 0;
        virtual void SetTranslate(const AZ::Vector3& translate) = 0;

        virtual bool GetOverrideRotate() const = 0;
        virtual void SetOverrideRotate(bool value) = 0;

        virtual AZ::Vector3 GetRotate() const = 0;
        virtual void SetRotate(const AZ::Vector3& rotate) = 0;

        virtual bool GetOverrideScale() const = 0;
        virtual void SetOverrideScale(bool value) = 0;

        virtual AZ::Vector3 GetScale() const = 0;
        virtual void SetScale(const AZ::Vector3& scale) = 0;

        virtual float GetFrequencyZoom() const = 0;
        virtual void SetFrequencyZoom(float frequencyZoom) = 0;

        virtual WrappingType GetWrappingType() const = 0;
        virtual void SetWrappingType(WrappingType type) = 0;

        virtual bool GetIs3D() const = 0;
        virtual void SetIs3D(bool value) = 0;

        virtual bool GetAdvancedMode() const = 0;
        virtual void SetAdvancedMode(bool value) = 0;
    };

    using GradientTransformModifierRequestBus = AZ::EBus<GradientTransformModifierRequests>;
}
