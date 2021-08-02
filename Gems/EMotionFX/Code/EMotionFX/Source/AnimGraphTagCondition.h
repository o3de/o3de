/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <EMotionFX/Source/ObjectAffectedByParameterChanges.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    class EMFX_API AnimGraphTagCondition
        : public AnimGraphTransitionCondition
        , public ObjectAffectedByParameterChanges
    {
    public:
        AZ_RTTI(AnimGraphTagCondition, "{2A786756-80F5-4A55-B00F-5AA876CC4D3A}", AnimGraphTransitionCondition, ObjectAffectedByParameterChanges)
        AZ_CLASS_ALLOCATOR_DECL

        enum EFunction : AZ::u8
        {
            FUNCTION_ALL        = 0,
            FUNCTION_NOTALL     = 1,
            FUNCTION_ONEORMORE  = 2,
            FUNCTION_NONE       = 3
        };

        AnimGraphTagCondition();
        AnimGraphTagCondition(AnimGraph* animGraph);
        ~AnimGraphTagCondition() override;

        void Reinit() override;
        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;

        const char* GetTestFunctionString() const;
        void CreateTagString(AZStd::string& outTagString) const;

        void SetFunction(EFunction function);
        void SetTags(const AZStd::vector<AZStd::string>& tags);
        const AZStd::vector<size_t>& GetTagParameterIndices() const;

        // ParameterDrivenPorts
        AZStd::vector<AZStd::string> GetParameters() const override;
        AnimGraph* GetParameterAnimGraph() const override;
        void ParameterMaskChanged(const AZStd::vector<AZStd::string>& newParameterMask) override;
        void AddRequiredParameters(AZStd::vector<AZStd::string>& parameterNames) const override;
        void ParameterAdded(const AZStd::string& newParameterName) override;
        void ParameterRenamed(const AZStd::string& oldParameterName, const AZStd::string& newParameterName) override;
        void ParameterOrderChanged(const ValueParameterVector& beforeChange, const ValueParameterVector& afterChange) override;
        void ParameterRemoved(const AZStd::string& oldParameterName) override;
        void BuildParameterRemovedCommands(MCore::CommandGroup& commandGroup, const AZStd::string& parameterNameToBeRemoved) override;

        static void Reflect(AZ::ReflectContext* context);

    private:
        static const char* s_functionAllTags;
        static const char* s_functionOneOrMoreInactive;
        static const char* s_functionOneOrMoreActive;
        static const char* s_functionNoTagActive;

        AZStd::vector<AZStd::string>    m_tags;
        AZStd::vector<size_t>           m_tagParameterIndices;
        EFunction                       m_function;
    };
} // namespace EMotionFX
