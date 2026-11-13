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
    // =======================================================================
    // Outliner Procedural Prefab Colours
    // =======================================================================

    //! The capsule colours the Entity Outliner paints a procedural prefab row with over the prefab row's, named, so the legacy and the
    //! flattened GUI read one value. A named namespace, not an anonymous one: a unity build joins files into one translation unit, where
    //! anonymous namespaces merge.
    namespace ProceduralPrefabUiHandlerFlat
    {
        //! ProceduralPrefabUiHandler.cpp:17. TOKENIZE: OutlinerProceduralPrefabCapsule
        constexpr QColor Capsule(0x36, 0x15, 0x61);
        //! ProceduralPrefabUiHandler.cpp:18. TOKENIZE: OutlinerProceduralPrefabCapsuleDisabled
        constexpr QColor CapsuleDisabled(0x4B, 0x34, 0x55);
        //! ProceduralPrefabUiHandler.cpp:19. TOKENIZE: OutlinerProceduralPrefabCapsuleEdit
        constexpr QColor CapsuleEdit(0x36, 0x15, 0x61);
    }

    ProceduralPrefabUiHandler::ProceduralPrefabUiHandler()
    {
        m_prefabCapsuleColor = ProceduralPrefabUiHandlerFlat::Capsule;
        m_prefabCapsuleDisabledColor = ProceduralPrefabUiHandlerFlat::CapsuleDisabled;
        m_prefabCapsuleEditColor = ProceduralPrefabUiHandlerFlat::CapsuleEdit;
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
