/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/Prefab/PrefabFocusHandler.h>

#include <AzToolsFramework/Entity/PrefabEditorEntityOwnershipInterface.h>
#include <AzToolsFramework/Prefab/Instance/Instance.h>
#include <AzToolsFramework/Prefab/Instance/InstanceEntityMapperInterface.h>

namespace AzToolsFramework::Prefab
{
    PrefabFocusHandler::~PrefabFocusHandler()
    {
        AZ::Interface<PrefabFocusInterface>::Unregister(this);
    }

    void PrefabFocusHandler::Initialize()
    {
        m_focusModeInterface = AZ::Interface<FocusModeInterface>::Get();
        AZ_Assert(
            m_focusModeInterface,
            "Prefab - PrefabFocusHandler - "
            "Focus Mode Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_instanceEntityMapperInterface = AZ::Interface<InstanceEntityMapperInterface>::Get();
        AZ_Assert(
            m_instanceEntityMapperInterface,
            "Prefab - PrefabFocusHandler - "
            "Instance Entity Mapper Interface could not be found. "
            "Check that it is being correctly initialized.");

        m_prefabEditorEntityOwnershipInterface = AZ::Interface<PrefabEditorEntityOwnershipInterface>::Get();
        AZ_Assert(
            m_prefabEditorEntityOwnershipInterface,
            "Prefab - PrefabFocusHandler - "
            "Prefab Editor Entity Ownership Interface could not be found. "
            "Check that it is being correctly initialized.");

        AZ::Interface<PrefabFocusInterface>::Register(this);
    }

    PrefabFocusOperationResult PrefabFocusHandler::FocusOnOwningPrefab(AZ::EntityId entityId)
    {
        InstanceOptionalReference focusedInstance = (entityId == AZ::EntityId())
            ? m_prefabEditorEntityOwnershipInterface->GetRootPrefabInstance()
            : m_instanceEntityMapperInterface->FindOwningInstance(entityId);

        if (!focusedInstance.has_value())
        {
            return AZ::Failure(AZStd::string(
                "Prefab Focus Handler: Couldn't find owning instance of entityId provided."));
        }

        m_focusedInstance = focusedInstance;
        m_focusedTemplateId = focusedInstance->get().GetTemplateId();
        m_focusModeInterface->SetFocusRoot(focusedInstance->get().GetContainerEntityId());

        return AZ::Success();
    }
    
    TemplateId PrefabFocusHandler::GetFocusedPrefabTemplateId()
    {
        return m_focusedTemplateId;
    }

    InstanceOptionalReference PrefabFocusHandler::GetFocusedPrefabInstance()
    {
        return m_focusedInstance;
    }

    bool PrefabFocusHandler::IsOwningPrefabBeingFocused(AZ::EntityId entityId)
    {
        if (entityId == AZ::EntityId())
        {
            return false;
        }

        InstanceOptionalReference instance = m_instanceEntityMapperInterface->FindOwningInstance(entityId);

        return instance.has_value() && (&instance->get() == &m_focusedInstance->get());
    }

} // namespace AzToolsFramework::Prefab
