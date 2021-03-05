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

#include <AzCore/Memory/Memory.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>


namespace AZ
{
    class ReflectContext;

    namespace SceneAPI
    {
        namespace SceneData
        {
            class SkeletonGroup
                : public DataTypes::ISkeletonGroup
            {
            public:
                AZ_RTTI(SkeletonGroup, "{F5F8D1BF-3A24-45E8-8C3F-6A682CA02520}", DataTypes::ISkeletonGroup);
                AZ_CLASS_ALLOCATOR_DECL

                SkeletonGroup();
                ~SkeletonGroup() override = default;

                const AZStd::string& GetName() const override;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);
                const Uuid& GetId() const override;
                void OverrideId(const Uuid& id);

                Containers::RuleContainer& GetRuleContainer();
                const Containers::RuleContainer& GetRuleContainerConst() const;

                const AZStd::string& GetSelectedRootBone() const override;
                void SetSelectedRootBone(const AZStd::string& selectedRootBone) override;

                static void Reflect(ReflectContext* context);
                static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement);

            protected:
                Containers::RuleContainer m_rules;
                AZStd::string m_name;
                AZStd::string m_selectedRootBone;
                Uuid m_id;
            };
        }
    }
}
