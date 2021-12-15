/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabDomUtils.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabDocument.h>
#include <AzToolsFramework/Prefab/Spawnable/PrefabProcessorContext.h>

namespace AzToolsFramework::Prefab::PrefabConversionUtils
{
    PrefabDocument::PrefabDocument(AZStd::string name)
        : m_name(AZStd::move(name))
        , m_instance(AZStd::make_unique<AzToolsFramework::Prefab::Instance>())
    {
        m_instance->SetTemplateSourcePath(AZ::IO::Path("InMemory") / name);
    }

    bool PrefabDocument::SetPrefabDom(const PrefabDom& prefab)
    {
        if (ConstructInstanceFromPrefabDom(prefab))
        {
            constexpr bool copyConstStrings = true;
            m_dom.CopyFrom(prefab, m_dom.GetAllocator(), copyConstStrings);
            return true;
        }
        else
        {
            return false;
        }
    }

    bool PrefabDocument::SetPrefabDom(PrefabDom&& prefab)
    {
        if (ConstructInstanceFromPrefabDom(prefab))
        {
            m_dom = AZStd::move(prefab);
            return true;
        }
        else
        {
            return false;
        }
    }

    const AZStd::string& PrefabDocument::GetName() const
    {
        return m_name;
    }

    const PrefabDom& PrefabDocument::GetDom() const
    {
        if (m_isDirty)
        {
            m_isDirty = !PrefabDomUtils::StoreInstanceInPrefabDom(*m_instance, m_dom);
        }
        return m_dom;
    }

    PrefabDom&& PrefabDocument::TakeDom()
    {
        if (m_isDirty)
        {
            m_isDirty = !PrefabDomUtils::StoreInstanceInPrefabDom(*m_instance, m_dom);
        }
        return AZStd::move(m_dom);
    }

    void PrefabDocument::ListEntitiesWithComponentType(
        AZ::TypeId componentType, const AZStd::function<bool(AzToolsFramework::Prefab::AliasPath&&)>& callback) const
    {
        m_instance->GetAllEntitiesInHierarchyConst(
            [this, &componentType, &callback](const AZ::Entity& entity) -> bool
            {
                if (entity.FindComponent(componentType))
                {
                    return callback(m_instance->GetAliasPathRelativeToInstance(entity.GetId()));
                }
                else
                {
                    return true;
                }
            });
    }

    AZ::Entity* PrefabDocument::CreateEntityAlias(
        PrefabDocument& source,
        AzToolsFramework::Prefab::AliasPathView entity,
        AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasType aliasType,
        AzToolsFramework::Prefab::PrefabConversionUtils::EntityAliasSpawnableLoadBehavior loadBehavior,
        uint32_t tag,
        AzToolsFramework::Prefab::PrefabConversionUtils::PrefabProcessorContext& context)
    {
        auto&& [sourceInstance, entityId] = source.m_instance->GetInstanceAndEntityIdFromAliasPath(entity);
        if (sourceInstance != nullptr && entityId.IsValid())
        {
            return SpawnableUtils::CreateEntityAlias(
                source.m_name, *sourceInstance, m_name, *m_instance, entityId, aliasType, loadBehavior, tag, context);
        }
        else
        {
            return nullptr;
        }
    }

    AzToolsFramework::Prefab::Instance& PrefabDocument::GetInstance()
    {
        // Assume that changes will be made to the instance.
        m_isDirty = true;
        return *m_instance;
    }

    const AzToolsFramework::Prefab::Instance& PrefabDocument::GetInstance() const
    {
        return *m_instance;
    }

    bool PrefabDocument::ConstructInstanceFromPrefabDom(const PrefabDom& prefab)
    {
        using namespace AzToolsFramework::Prefab;

        m_instance->Reset();
        if (PrefabDomUtils::LoadInstanceFromPrefabDom(*m_instance, prefab, PrefabDomUtils::LoadFlags::AssignRandomEntityId))
        {
            return true;
        }
        else
        {
#ifdef AZ_ENABLE_TRACING
            AZStd::string_view sourceName = m_name;
            PrefabDomValueConstReference sourceReference = PrefabDomUtils::FindPrefabDomValue(prefab, PrefabDomUtils::SourceName);
            if (sourceReference.has_value() && sourceReference->get().IsString() && sourceReference->get().GetStringLength() != 0)
            {
                sourceName = AZStd::string_view(sourceReference->get().GetString(), sourceReference->get().GetStringLength());
            }
            AZ_Error(
                "PrefabDocument", false, "Failed to construct Prefab instance from given PrefabDOM '%.*s'.", AZ_STRING_ARG(sourceName));
#endif
            return false;
        }
    }
} // namespace AzToolsFramework::Prefab::PrefabConversionUtils
