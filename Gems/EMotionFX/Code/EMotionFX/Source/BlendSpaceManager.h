/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/BlendSpaceParamEvaluator.h>


namespace EMotionFX
{
    class EMFX_API BlendSpaceManager
        : public MCore::RefCounted
    {
    public:
        AZ_RTTI(BlendSpaceManager, "{FCCE8C41-01A5-43F2-A9AD-04E8ECE3304F}")
        AZ_CLASS_ALLOCATOR_DECL

        BlendSpaceManager();
        ~BlendSpaceManager();

        void RegisterEvaluator(BlendSpaceParamEvaluator* evaluator);
        void ClearEvaluators();

        size_t GetEvaluatorCount() const;
        BlendSpaceParamEvaluator* GetEvaluator(size_t index) const;
        BlendSpaceParamEvaluator* FindEvaluatorByType(const AZ::TypeId& type) const;

    private:
        AZStd::vector<BlendSpaceParamEvaluator*> m_evaluators;
    };
} // namespace EMotionFX
