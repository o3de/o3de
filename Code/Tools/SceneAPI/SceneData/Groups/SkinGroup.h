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
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <SceneAPI/SceneCore/Containers/RuleContainer.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneData/ManifestBase/SceneNodeSelectionList.h>


namespace AZ
{
    class ReflectContex;

    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IRule;
        }

        namespace SceneData
        {
            class SkinGroup : public DataTypes::ISkinGroup
            {
            public:
                AZ_RTTI(SkinGroup, "{A3217B13-79EA-4487-9A13-5D382EA9077A}", DataTypes::ISkinGroup);
                AZ_CLASS_ALLOCATOR_DECL

                SkinGroup();
                ~SkinGroup() override = default;

                const AZStd::string& GetName() const override;
                void SetName(const AZStd::string& name);
                void SetName(AZStd::string&& name);
                const Uuid& GetId() const override;
                void OverrideId(const Uuid& id);

                Containers::RuleContainer& GetRuleContainer();
                const Containers::RuleContainer& GetRuleContainerConst() const;

                DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() override;
                const DataTypes::ISceneNodeSelectionList& GetSceneNodeSelectionList() const override;

                static void Reflect(AZ::ReflectContext* context);
                static bool VersionConverter(SerializeContext& context, SerializeContext::DataElementNode& classElement);

            protected:
                SceneNodeSelectionList m_nodeSelectionList;
                Containers::RuleContainer m_rules;
                AZStd::string m_name;
                Uuid m_id;
            };

        } // SceneData
    } // SceneAPI
} // AZ