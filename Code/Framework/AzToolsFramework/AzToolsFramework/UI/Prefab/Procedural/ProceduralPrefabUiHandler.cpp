/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/Prefab/Procedural/ProceduralPrefabUiHandler.h>

namespace AzToolsFramework
{
    ProceduralPrefabUiHandler::ProceduralPrefabUiHandler()
    {
        m_prefabCapsuleColor = QColor("#2B1037");
        m_prefabCapsuleDisabledColor = QColor("#4B3455");
        m_prefabCapsuleEditColor = QColor("#361561");
        m_prefabEditOpenIconPath = QString(":/Entity/prefab_edit_open_readonly.svg");
    }

    void ProceduralPrefabUiHandler::PaintItemBackground(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        PrefabUiHandler::PaintItemBackground(painter, option, index);
    }
}
