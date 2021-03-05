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
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>


namespace EMotionFX
{
    // forward declarations
    class AnimGraphInstance;

    /**
     *
     *
     *
     */
    class EMFX_API AnimGraphTimeCondition
        : public AnimGraphTransitionCondition
    {
    public:
        AZ_RTTI(AnimGraphTimeCondition, "{9CFC3B92-0D9B-4EC8-9999-625EF21A9993}", AnimGraphTransitionCondition)
        AZ_CLASS_ALLOCATOR_DECL

        // the unique data
        class EMFX_API UniqueData
            : public AnimGraphObjectData
        {
            EMFX_ANIMGRAPHOBJECTDATA_IMPLEMENT_LOADSAVE

        public:
            AZ_CLASS_ALLOCATOR_DECL

            UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance);
            ~UniqueData() override;

        public:
            float   mElapsedTime;       /**< The elapsed time in seconds for the given anim graph instance. */
            float   mCountDownTime;     /**< The count down time in seconds for the given anim graph instance. */
        };

        AnimGraphTimeCondition();
        AnimGraphTimeCondition(AnimGraph* animGraph);
        ~AnimGraphTimeCondition();

        bool InitAfterLoading(AnimGraph* animGraph) override;

        void GetSummary(AZStd::string* outResult) const override;
        void GetTooltip(AZStd::string* outResult) const override;
        const char* GetPaletteName() const override;

        void Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds) override;
        bool TestCondition(AnimGraphInstance* animGraphInstance) const override;
        void Reset(AnimGraphInstance* animGraphInstance) override;
        AnimGraphObjectData* CreateUniqueData(AnimGraphInstance* animGraphInstance) override { return aznew UniqueData(this, animGraphInstance); }

        void SetCountDownTime(float countDownTime);
        float GetCountDownTime() const;

        void SetMinRandomTime(float minRandomTime);
        float GetMinRandomTime() const;

        void SetMaxRandomTime(float maxRandomTime);
        float GetMaxRandomTime() const;

        void SetUseRandomization(bool useRandomization);
        bool GetUseRandomization() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZ::Crc32 GetRandomTimeVisibility() const;

        float   m_countDownTime;
        float   m_minRandomTime;
        float   m_maxRandomTime;
        bool    m_useRandomization;
    };
}   // namespace EMotionFX
