/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <GradientSignal/Util.h>

namespace GradientSignal
{
    class GradientTransformModifierRequests
        : public AZ::ComponentBus
    {
    public:
        /**
         * Overrides the default AZ::EBusTraits handler policy to allow one
         * listener only.
         */
        static const AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;

        virtual bool GetAllowReference() const = 0;
        virtual void SetAllowReference(bool value) = 0;

        virtual AZ::EntityId GetShapeReference() const = 0;
        virtual void SetShapeReference(AZ::EntityId shapeReference) = 0;

        virtual bool GetOverrideBounds() const = 0;
        virtual void SetOverrideBounds(bool value) = 0;

        virtual AZ::Vector3 GetBounds() const = 0;
        virtual void SetBounds(AZ::Vector3 bounds) = 0;

        virtual TransformType GetTransformType() const = 0;
        virtual void SetTransformType(TransformType type) = 0;

        virtual bool GetOverrideTranslate() const = 0;
        virtual void SetOverrideTranslate(bool value) = 0;

        virtual AZ::Vector3 GetTranslate() const = 0;
        virtual void SetTranslate(AZ::Vector3 translate) = 0;

        virtual bool GetOverrideRotate() const = 0;
        virtual void SetOverrideRotate(bool value) = 0;

        virtual AZ::Vector3 GetRotate() const = 0;
        virtual void SetRotate(AZ::Vector3 rotate) = 0;

        virtual bool GetOverrideScale() const = 0;
        virtual void SetOverrideScale(bool value) = 0;

        virtual AZ::Vector3 GetScale() const = 0;
        virtual void SetScale(AZ::Vector3 scale) = 0;

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