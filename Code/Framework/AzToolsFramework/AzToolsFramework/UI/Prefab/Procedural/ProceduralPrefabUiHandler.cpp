/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/Procedural/ProceduralPrefabUiHandler.h>

#include <AzToolsFramework/Prefab/PrefabPublicInterface.h>

namespace AzToolsFramework
{
    ProceduralPrefabUiHandler::ProceduralPrefabUiHandler()
    {
        m_prefabCapsuleColor = QColor("#361561");
        m_prefabCapsuleDisabledColor = QColor("#4B3455");
        m_prefabCapsuleEditColor = QColor("#361561");
        m_prefabIconPath = QString(":/Entity/prefab_edit.svg");
        m_prefabEditOpenIconPath = QString(":/Entity/prefab_edit_open_readonly.svg");
    }

    QString ProceduralPrefabUiHandler::GenerateItemTooltip(AZ::EntityId entityId) const
    {
        if (AZ::IO::Path path = m_prefabPublicInterface->GetOwningInstancePrefabPath(entityId); !path.empty())
        {
            return QObject::tr("Double click to inspect.\n%1").arg(path.Native().data());
        }
        return QString();
    }
}
