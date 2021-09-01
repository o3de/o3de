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

namespace AZ::SceneAPI::SceneData
{
     PrefabGroup::PrefabGroup()
        : m_id(Uuid::CreateNull())
        , m_name()
    {
         m_prefabDom.SetNull();
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
        m_prefabDom = AZStd::move(prefabDom);
        m_prefabDomBuffer.clear();
    }

    void PrefabGroup::SetPrefabDomBuffer(AZStd::string prefabDomBuffer)
    {
        m_prefabDomBuffer = AZStd::move(prefabDomBuffer);
        m_prefabDom.SetNull();
    }

    const AzToolsFramework::Prefab::PrefabDom& PrefabGroup::GetPrefabDom() const
    {
        if (!m_prefabDom.HasParseError() && !m_prefabDom.IsNull())
        {
            // DOM is ready
            return m_prefabDom;
        }
        else if (m_prefabDomBuffer.empty())
        {
            m_prefabDom.SetNull();
            return m_prefabDom;
        }

        m_prefabDom.Parse<rapidjson::kParseCommentsFlag>(m_prefabDomBuffer.c_str());
        if (m_prefabDom.HasParseError())
        {
            AZ_Warning(
                "prefab",
                false,
                "JSON error during parsing at offset %zu: %s",
                m_prefabDom.GetErrorOffset(),
                rapidjson::GetParseError_En(m_prefabDom.GetParseError()));

            m_prefabDom.SetNull();
            return m_prefabDom;
        }

        return m_prefabDom;
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
                ->Field("prefabDomBuffer", &PrefabGroup::m_prefabDomBuffer);
        }

        BehaviorContext* behaviorContext = azrtti_cast<BehaviorContext*>(context);
        if (behaviorContext)
        {
            behaviorContext->Class<PrefabGroup>()
                ->Attribute(AZ::Script::Attributes::ExcludeFrom, AZ::Script::Attributes::ExcludeFlags::All)
                ->Attribute(Script::Attributes::Scope, Script::Attributes::ScopeFlags::Common)
                ->Attribute(Script::Attributes::Module, "prefab")
                ->Property("name", BehaviorValueProperty(&PrefabGroup::m_name))
                ->Property("id", BehaviorValueProperty(&PrefabGroup::m_id))
                ->Property("prefabDomBuffer", BehaviorValueProperty(&PrefabGroup::m_prefabDomBuffer));
        }
    }
}
