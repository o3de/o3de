/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneData/SceneDataConfiguration.h>


namespace AZ
{
    class ReflectContext;
    namespace SceneAPI
    {
        namespace Containers
        {
            class Scene;
        }

        namespace DataTypes
        {
            class IGroup;
        }
    }
}

namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Rule
        {
            /**
             * Assignment operator needs to be defined for ReflectableData
             * Needs to be reflectable.
             * Needs IsEmpty() and Clear() functions?
             */
            template<class ReflectableData>
            class ExternalToolRule
                : public AZ::SceneAPI::DataTypes::IRule
            {
            public:
                AZ_RTTI(ExternalToolRule, "{75B41D83-D432-4D29-908D-CF26762B2399}", AZ::SceneAPI::DataTypes::IRule);
                AZ_CLASS_ALLOCATOR(ExternalToolRule, AZ::SystemAllocator)

                virtual const ReflectableData& GetData() const = 0;
                virtual void SetData(const ReflectableData& data) = 0;
            };

            template<class RuleClass, class ReflectableData>
            static bool LoadFromGroup(const AZ::SceneAPI::DataTypes::IGroup& group, ReflectableData& outData);

            template<class RuleClass, class ReflectableData>
            static void SaveToGroup(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group, const ReflectableData& data);

            template<class RuleClass, class ReflectableData>
            static void RemoveRuleFromGroup(AZ::SceneAPI::Containers::Scene& scene, AZ::SceneAPI::DataTypes::IGroup& group);
        } // Rule
    } // Pipeline
} // EMotionFX
#include <SceneAPIExt/Rules/ExternalToolRule.inl>
