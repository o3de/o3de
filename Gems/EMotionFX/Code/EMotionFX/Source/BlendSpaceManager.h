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

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <EMotionFX/Source/BlendSpaceParamEvaluator.h>


namespace EMotionFX
{
    class EMFX_API BlendSpaceManager
        : public BaseObject
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