/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/BlendSpaceManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(BlendSpaceManager, BlendSpaceManagerAllocator)

    BlendSpaceManager::BlendSpaceManager()
        : MCore::RefCounted()
    {
        m_evaluators.reserve(9);

        // Register standard parameter evaluators.
        RegisterEvaluator(aznew BlendSpaceParamEvaluatorNone());
        RegisterEvaluator(aznew BlendSpaceFrontBackVelocityParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceLeftRightVelocityParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceMoveSpeedParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceTravelDirectionParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceTravelDistanceParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceTravelSlopeParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceTurnAngleParamEvaluator());
        RegisterEvaluator(aznew BlendSpaceTurnSpeedParamEvaluator());
    }


    BlendSpaceManager::~BlendSpaceManager()
    {
        ClearEvaluators();
    }


    void BlendSpaceManager::RegisterEvaluator(BlendSpaceParamEvaluator* evaluator)
    {
        m_evaluators.emplace_back(evaluator);
    }


    void BlendSpaceManager::ClearEvaluators()
    {
        for (BlendSpaceParamEvaluator* evaluator : m_evaluators)
        {
            evaluator->Destroy();
        }

        m_evaluators.clear();
    }


    size_t BlendSpaceManager::GetEvaluatorCount() const
    {
        return m_evaluators.size();
    }


    BlendSpaceParamEvaluator* BlendSpaceManager::GetEvaluator(size_t index) const
    {
        if (index >= m_evaluators.size())
        {
            return nullptr;
        }

        return m_evaluators[index];
    }


    BlendSpaceParamEvaluator* BlendSpaceManager::FindEvaluatorByType(const AZ::TypeId& type) const
    {
        for (BlendSpaceParamEvaluator* evaluator : m_evaluators)
        {
            if (azrtti_typeid<>(evaluator) == type)
            {
                return evaluator;
            }
        }

        return nullptr;
    }
} // namespace EMotionFX
