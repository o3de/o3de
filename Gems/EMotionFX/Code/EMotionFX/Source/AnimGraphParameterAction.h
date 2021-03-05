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

#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTriggerAction.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>

namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;
    class ValueParameter;

    /**
     * AnimGraphParameterAction is a specific type of trigger action that modifies the parameter.
     */
    class EMFX_API AnimGraphParameterAction
        : public AnimGraphTriggerAction
        , public ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(AnimGraphParameterAction, "{57329F53-3E8F-47FA-997D-FEF390CB2E57}", AnimGraphTriggerAction, ObjectAffectedByParameterChanges)
        AZ_CLASS_ALLOCATOR_DECL

        AnimGraphParameterAction();
        AnimGraphParameterAction(AnimGraph* animGraph);
        ~AnimGraphParameterAction();

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void TriggerAction(AnimGraphInstance* animGraphInstance) const override;

        AZ::Outcome<size_t> GetParameterIndex() const;
        void SetParameterName(const AZStd::string& parameterName);
        const AZStd::string& GetParameterName() const;
        AZ::TypeId GetParameterType() const;

        void SetTriggerValue(float value) { m_triggerValue = value; }
        float GetTriggerValue() const { return m_triggerValue; }

        // ObjectAffectedByParameterChanges overrides
        void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName) override;
        void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange) override;
        void ParameterRemoved(const AZStd::string& oldParameterName) override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string                       m_parameterName;
        AZ::Outcome<size_t>                 m_parameterIndex;
        const ValueParameter*               m_valueParameter;
        float                               m_triggerValue;
    };
} // namespace EMotionFX
