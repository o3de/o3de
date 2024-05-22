/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <MCore/Source/RefCounted.h>
#include <EMotionFX/Source/EMotionFXConfig.h>

namespace EMotionFX
{
    class MotionInstance;

    class EMFX_API BlendSpaceParamEvaluator
        : public MCore::RefCounted
    {
    public:
        AZ_RTTI(BlendSpaceParamEvaluator, "{4736FF5A-8759-464C-9C7A-5D8D112AB1A0}")
        AZ_CLASS_ALLOCATOR_DECL

        virtual float ComputeParamValue(const MotionInstance& motionInstance) = 0;
        virtual const char* GetName() const = 0;
        virtual bool IsNullEvaluator() const { return false; }
    };

    class EMFX_API BlendSpaceParamEvaluatorNone
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceParamEvaluatorNone, "{17D8679E-5760-481C-9411-5A97D22BC745}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
        bool IsNullEvaluator() const override { return true; }
    };

    class EMFX_API BlendSpaceMoveSpeedParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceMoveSpeedParamEvaluator, "{9ADCE598-FA98-4C35-BD15-3461AE30AB42}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceTurnSpeedParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceTurnSpeedParamEvaluator, "{5DB83BA6-FF66-43B2-B242-BF7E7CE891D6}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };


    class EMFX_API BlendSpaceTravelDirectionParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceTravelDirectionParamEvaluator, "{BDA81A65-D952-49A3-8265-58D9F956D820}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };


    class EMFX_API BlendSpaceTravelSlopeParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceTravelSlopeParamEvaluator, "{BDDFC2B4-4D29-4D59-91B8-FC29AC25A5E5}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceTurnAngleParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceTurnAngleParamEvaluator, "{ACE7DC67-45D5-4AFF-8955-5CA4606FFEEF}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceTravelDistanceParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceTravelDistanceParamEvaluator, "{6B02BB26-8B29-416F-A141-BF700F60B4F4}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceLeftRightVelocityParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceLeftRightVelocityParamEvaluator, "{12034887-70D2-4946-A2FD-182D99BEC13E}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };

    class EMFX_API BlendSpaceFrontBackVelocityParamEvaluator
        : public BlendSpaceParamEvaluator
    {
    public:
        AZ_RTTI(BlendSpaceFrontBackVelocityParamEvaluator, "{0E769A8C-5106-4E73-9DAA-A5C37DFF6DDC}", BlendSpaceParamEvaluator)
        AZ_CLASS_ALLOCATOR_DECL

        float ComputeParamValue(const MotionInstance& motionInstance) override;
        const char* GetName() const override;
    };
} // namespace EMotionFX
