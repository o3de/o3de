/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/IRule.h>
#include <SceneAPI/SceneCore/SceneCoreConfiguration.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace Containers
        {
            class RuleContainer
            {
            public:
                AZ_RTTI(RuleContainer, "{2C20D3DF-57FF-4A31-8680-A4D45302B9CF}");
                virtual ~RuleContainer() {}
                
                SCENE_CORE_API size_t GetRuleCount() const;
                SCENE_CORE_API AZStd::shared_ptr<DataTypes::IRule> GetRule(size_t index) const;

                /**
                 * Find the first rule of the template type.
                 * @result The first rule of the template type. nullptr if not found.
                 */
                template<typename T>
                AZStd::shared_ptr<T> FindFirstByType() const;

                /**
                 * Check if there is a rule of the given template type.
                 * @result True in case a rule of the given template type got found, false if not.
                 */
                template<typename T>
                bool ContainsRuleOfType() const;

                SCENE_CORE_API void AddRule(const AZStd::shared_ptr<DataTypes::IRule>& rule);
                SCENE_CORE_API void AddRule(AZStd::shared_ptr<DataTypes::IRule>&& rule);

                SCENE_CORE_API void InsertRule(const AZStd::shared_ptr<DataTypes::IRule>& rule, size_t position);

                SCENE_CORE_API void RemoveRule(size_t index);
                SCENE_CORE_API void RemoveRule(const AZStd::shared_ptr<DataTypes::IRule>& rule);

                static void Reflect(ReflectContext* context);
                static SCENE_CORE_API bool VectorToRuleContainerConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);

            private:
                AZStd::vector<AZStd::shared_ptr<DataTypes::IRule>> m_rules;
            };

        } // Containers
    } // SceneAPI
} // AZ

#include <SceneAPI/SceneCore/Containers/RuleContainer.inl>
