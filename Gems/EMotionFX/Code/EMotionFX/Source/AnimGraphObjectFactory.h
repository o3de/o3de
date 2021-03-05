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

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/containers/unordered_set.h>
#include <EMotionFX/Source/EMotionFXConfig.h>
#include <MCore/Source/StaticAllocator.h>

namespace AZ
{
    class ReflectContext;
}

namespace EMotionFX
{
    // forward declarations
    class AnimGraphObject;
    class AnimGraph;

    /**
     *
     *
     */
    class EMFX_API AnimGraphObjectFactory
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL

        using UITypesSet = AZStd::unordered_set<AZ::TypeId, AZStd::hash<AZ::TypeId>, AZStd::equal_to<AZ::TypeId>, MCore::StaticAllocator>;

        AnimGraphObjectFactory();
        ~AnimGraphObjectFactory();

        static void ReflectTypes(AZ::ReflectContext* context);

        static UITypesSet& GetUITypes();

        const AZStd::vector<AnimGraphObject*>& GetUiObjectPrototypes() const { return m_animGraphObjectPrototypes; }

        static AnimGraphObject* Create(const AZ::TypeId& type, AnimGraph* animGraph = nullptr);

    private:
        AZStd::vector<AnimGraphObject*> m_animGraphObjectPrototypes;
    };
}   // namespace EMotionFX
