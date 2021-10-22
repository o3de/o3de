/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <PrefabGroup/PrefabGroup.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/JSON/error/error.h>
#include <AzCore/JSON/error/en.h>
#include <AzCore/Serialization/Json/JsonUtils.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/optional.h>
#include <AzFramework/FileFunc/FileFunc.h>

namespace AZ::SceneAPI::SceneData
{
     // PrefabGroup

     PrefabGroup::PrefabGroup()
        : m_id(Uuid::CreateNull())
        , m_name()
    {
    }

    const AZStd::string& PrefabGroup::GetName() const
    {
        return m_name;
    }

    void PrefabGroup::SetName(AZStd::string name)
    {
        m_name = AZStd::move(name);
    }

    const Uuid& PrefabGroup::GetId() const
    {
        return m_id;
    }

    void PrefabGroup::SetId(Uuid id)
    {
        m_id = AZStd::move(id);
    }

    Containers::RuleContainer& PrefabGroup::GetRuleContainer()
    {
        return m_rules;
    }

    const Containers::RuleContainer& PrefabGroup::GetRuleContainerConst() const
    {
        return m_rules;
    }
            
    DataTypes::ISceneNodeSelectionList& PrefabGroup::GetSceneNodeSelectionList()
    {
        return m_nodeSelectionList;
    }

    const DataTypes::ISceneNodeSelectionList& PrefabGroup::GetSceneNodeSelectionList() const
    {
        return m_nodeSelectionList;
    }

    void PrefabGroup::SetPrefabDom(AzToolsFramework::Prefab::PrefabDom prefabDom)
    {
        m_prefabDomData = AZStd::make_shared<Prefab::PrefabDomData>();
        m_prefabDomData->CopyValue(prefabDom);
    }

    AzToolsFramework::Prefab::PrefabDomConstReference PrefabGroup::GetPrefabDomRef() const
    {
        if (m_prefabDomData)
        {
            return m_prefabDomData->GetValue();
        }
        return {};
    }

    void PrefabGroup::Reflect(ReflectContext* context)
    {
        SerializeContext* serializeContext = azrtti_cast<SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<DataTypes::IPrefabGroup, DataTypes::ISceneNodeGroup>()
                ->Version(1);

            serializeContext->Class<PrefabGroup, DataTypes::IPrefabGroup>()
                ->Version(1)
                ->Field("name", &PrefabGroup::m_name)
                ->Field("nodeSelectionList", &PrefabGroup::m_nodeSelectionList)
                ->Field("rules", &PrefabGroup::m_rules)
                ->Field("id", &PrefabGroup::m_id)
                ->Field("prefabDomData", &PrefabGroup::m_prefabDomData);
        }

        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            auto setPrefabDomData = [](PrefabGroup& self, const AZStd::string& json)
            {
                auto jsonOutcome = JsonSerializationUtils::ReadJsonString(json);
                if (jsonOutcome.IsSuccess())
                {
                    self.SetPrefabDom(AZStd::move(jsonOutcome.GetValue()));
                    return true;
                }
                AZ_Error("prefab", false, "Set PrefabDom failed (%s)", jsonOutcome.GetError().c_str());
                return false;
            };

            auto getPrefabDomData = [](const PrefabGroup& self) -> AZStd::string
            {
                if (self.GetPrefabDomRef().has_value() == false)
                {
                    return {};
                }
                AZStd::string buffer;
                JsonSerializationUtils::WriteJsonString(self.GetPrefabDomRef().value(), buffer);
                return buffer;
            };

            behaviorContext->Class<PrefabGroup>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)
                ->Attribute(Script::Attributes::Module, "prefab")
                ->Property("name", BehaviorValueProperty(&PrefabGroup::m_name))
                ->Property("id", BehaviorValueProperty(&PrefabGroup::m_id))
                ->Property("prefabDomData", getPrefabDomData, setPrefabDomData);
        }
    }
}
