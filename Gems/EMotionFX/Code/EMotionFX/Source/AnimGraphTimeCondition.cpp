/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <MCore/Source/Compare.h>
#include <MCore/Source/Random.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <EMotionFX/Source/AnimGraphTimeCondition.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphInstance.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>


namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTimeCondition, AnimGraphConditionAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(AnimGraphTimeCondition::UniqueData, AnimGraphObjectUniqueDataAllocator)

    AnimGraphTimeCondition::AnimGraphTimeCondition()
        : AnimGraphTransitionCondition()
        , m_countDownTime(1.0f)
        , m_minRandomTime(0.0f)
        , m_maxRandomTime(1.0f)
        , m_useRandomization(false)
    {
    }


    AnimGraphTimeCondition::AnimGraphTimeCondition(AnimGraph* animGraph)
        : AnimGraphTimeCondition()
    {
        InitAfterLoading(animGraph);
    }


    AnimGraphTimeCondition::~AnimGraphTimeCondition()
    {
    }


    bool AnimGraphTimeCondition::InitAfterLoading(AnimGraph* animGraph)
    {
        if (!AnimGraphTransitionCondition::InitAfterLoading(animGraph))
        {
            return false;
        }

        InitInternalAttributesForAllInstances();

        Reinit();
        return true;
    }


    // get the palette name
    const char* AnimGraphTimeCondition::GetPaletteName() const
    {
        return "Time Condition";
    }


    // update the passed time of the condition
    void AnimGraphTimeCondition::Update(AnimGraphInstance* animGraphInstance, float timePassedInSeconds)
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        // increase the elapsed time of the condition
        uniqueData->m_elapsedTime += timePassedInSeconds;
    }


    // reset the condition
    void AnimGraphTimeCondition::Reset(AnimGraphInstance* animGraphInstance)
    {
        // find the unique data and reset it
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        // reset the elapsed time
        uniqueData->m_elapsedTime = 0.0f;

        // use randomized count downs?
        if (m_useRandomization)
        {
            // create a randomized count down value
            uniqueData->m_countDownTime =
                m_minRandomTime + (m_maxRandomTime - m_minRandomTime) * animGraphInstance->GetLcgRandom().GetRandomFloat();
        }
        else
        {
            // get the fixed count down value from the attribute
            uniqueData->m_countDownTime = m_countDownTime;
        }
    }


    // test the condition
    bool AnimGraphTimeCondition::TestCondition(AnimGraphInstance* animGraphInstance) const
    {
        // add the unique data for the condition to the anim graph
        UniqueData* uniqueData = static_cast<UniqueData*>(animGraphInstance->FindOrCreateUniqueObjectData(this));

        // in case the elapsed time is bigger than the count down time, we can trigger the condition
        if (uniqueData->m_elapsedTime + 0.0001f >= uniqueData->m_countDownTime)   // The 0.0001f is to counter floating point inaccuracies. The AZ float epsilon is too small.
        {
            return true;
        }

        // we have not reached the count down time yet, don't trigger yet
        return false;
    }

    // construct and output the information summary string for this object
    void AnimGraphTimeCondition::GetSummary(AZStd::string* outResult) const
    {
        *outResult = AZStd::string::format("%s: Countdown=%.2f secs, RandomizationUsed=%d, Random Count Down Range=(%.2f secs, %.2f secs)", RTTI_GetTypeName(), m_countDownTime, m_useRandomization, m_minRandomTime, m_maxRandomTime);
    }


    // construct and output the tooltip for this object
    void AnimGraphTimeCondition::GetTooltip(AZStd::string* outResult) const
    {
        AZStd::string columnName, columnValue;

        // add the condition type
        columnName = "Condition Type: ";
        columnValue = RTTI_GetTypeName();
        *outResult = AZStd::string::format("<table border=\"0\"><tr><td width=\"165\"><b>%s</b></td><td>%s</td>", columnName.c_str(), columnValue.c_str());

        // add the count down
        columnName = "Count Down: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%.2f secs</td>", columnName.c_str(), m_countDownTime);

        // add the randomization used flag
        columnName = "Randomization Used: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>%s</td>", columnName.c_str(), m_useRandomization ? "Yes" : "No");

        // add the random count down range
        columnName = "Random Count Down Range: ";
        *outResult += AZStd::string::format("</tr><tr><td><b>%s</b></td><td>(%.2f secs, %.2f secs)</td></tr></table>", columnName.c_str(), m_minRandomTime, m_maxRandomTime);
    }

    //--------------------------------------------------------------------------------
    // class AnimGraphTimeCondition::UniqueData
    //--------------------------------------------------------------------------------

    // constructor
    AnimGraphTimeCondition::UniqueData::UniqueData(AnimGraphObject* object, AnimGraphInstance* animGraphInstance)
        : AnimGraphObjectData(object, animGraphInstance)
    {
        m_elapsedTime    = 0.0f;
        m_countDownTime  = 0.0f;
    }


    // destructor
    AnimGraphTimeCondition::UniqueData::~UniqueData()
    {
    }


    void AnimGraphTimeCondition::SetCountDownTime(float countDownTime)
    {
        m_countDownTime = countDownTime;
    }


    float AnimGraphTimeCondition::GetCountDownTime() const
    {
        return m_countDownTime;
    }


    void AnimGraphTimeCondition::SetMinRandomTime(float minRandomTime)
    {
        m_minRandomTime = minRandomTime;
    }


    float AnimGraphTimeCondition::GetMinRandomTime() const
    {
        return m_minRandomTime;
    }


    void AnimGraphTimeCondition::SetMaxRandomTime(float maxRandomTime)
    {
        m_maxRandomTime = maxRandomTime;
    }


    float AnimGraphTimeCondition::GetMaxRandomTime() const
    {
        return m_maxRandomTime;
    }


    void AnimGraphTimeCondition::SetUseRandomization(bool useRandomization)
    {
        m_useRandomization = useRandomization;
    }


    bool AnimGraphTimeCondition::GetUseRandomization() const
    {
        return m_useRandomization;
    }


    AZ::Crc32 AnimGraphTimeCondition::GetRandomTimeVisibility() const
    {
        return m_useRandomization ? AZ::Edit::PropertyVisibility::Show : AZ::Edit::PropertyVisibility::Hide;
    }


    void AnimGraphTimeCondition::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<AnimGraphTimeCondition, AnimGraphTransitionCondition>()
            ->Version(1)
            ->Field("countDownTime", &AnimGraphTimeCondition::m_countDownTime)
            ->Field("useRandomization", &AnimGraphTimeCondition::m_useRandomization)
            ->Field("minRandomTime", &AnimGraphTimeCondition::m_minRandomTime)
            ->Field("maxRandomTime", &AnimGraphTimeCondition::m_maxRandomTime)
            ;

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (!editContext)
        {
            return;
        }

        editContext->Class<AnimGraphTimeCondition>("Time Condition", "Time condition attributes")
            ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AnimGraphTimeCondition::m_countDownTime, "Countdown Time", "The amount of seconds the condition will count down until the condition will trigger.")
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AnimGraphTimeCondition::m_useRandomization, "Use Randomization", "When randomization is enabled the count down time will be a random one between the min and max random time.")
                ->Attribute(AZ::Edit::Attributes::ChangeNotify, AZ::Edit::PropertyRefreshLevels::EntireTree)
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AnimGraphTimeCondition::m_minRandomTime, "Min Random Time", "The minimum randomized count down time. This will be only used then randomization is enabled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphTimeCondition::GetRandomTimeVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ->DataElement(AZ::Edit::UIHandlers::SpinBox, &AnimGraphTimeCondition::m_maxRandomTime, "Max Random Time", "The maximum randomized count down time. This will be only used then randomization is enabled.")
                ->Attribute(AZ::Edit::Attributes::Visibility, &AnimGraphTimeCondition::GetRandomTimeVisibility)
                ->Attribute(AZ::Edit::Attributes::Min, 0.0f)
                ->Attribute(AZ::Edit::Attributes::Max, std::numeric_limits<float>::max())
            ;
    }
} // namespace EMotionFX
