/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#include "EditorDefs.h"

#include "ClassDesc.h"

// Editor
#include "IconManager.h"

int CObjectClassDesc::GetTextureIconId()
{
    if (!m_nTextureIcon)
    {
        QString pTexName = GetTextureIcon();

        if (!pTexName.isEmpty())
        {
            m_nTextureIcon = GetIEditor()->GetIconManager()->GetIconTexture(pTexName.toUtf8().data());
        }
    }

    return m_nTextureIcon;
}
