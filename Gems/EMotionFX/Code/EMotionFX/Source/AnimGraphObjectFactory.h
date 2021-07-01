/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
