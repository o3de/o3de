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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#include "EditorDefs.h"

#include "IconManager.h"

#include <AzCore/Interface/Interface.h>

// AzToolsFramework
#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

// Editor
#include "Settings.h"
#include "Util/Image.h"
#include "Util/ImageUtil.h"


#define HELPER_MATERIAL "Objects/Helper"

namespace
{
    // Object names in this array must correspond to EObject enumeration.
    const char* g_ObjectNames[eStatObject_COUNT] =
    {
        "Objects/Arrow.cgf",
        "Objects/Axis.cgf",
        "Objects/Sphere.cgf",
        "Objects/Anchor.cgf",
        "Objects/entrypoint.cgf",
        "Objects/hidepoint.cgf",
        "Objects/hidepoint_sec.cgf",
        "Objects/reinforcement_point.cgf",
    };

    const char* g_IconNames[eIcon_COUNT] =
    {
        "Icons/ScaleWarning.png",
        "Icons/RotationWarning.png",
    };
};

//////////////////////////////////////////////////////////////////////////
CIconManager::CIconManager()
{
    ZeroStruct(m_icons);
    ZeroStruct(m_objects);
}

//////////////////////////////////////////////////////////////////////////
CIconManager::~CIconManager()
{
}

//////////////////////////////////////////////////////////////////////////
void CIconManager::Init()
{
}

//////////////////////////////////////////////////////////////////////////
void CIconManager::Done()
{
    Reset();
}

//////////////////////////////////////////////////////////////////////////
void CIconManager::Reset()
{
    I3DEngine* pEngine = GetIEditor()->Get3DEngine();
    // Do not unload objects. but clears them.
    int i;
    for (i = 0; i < sizeof(m_objects) / sizeof(m_objects[0]); i++)
    {
        if (m_objects[i] && pEngine)
        {
            m_objects[i]->Release();
        }
        m_objects[i] = 0;
    }
    for (i = 0; i < eIcon_COUNT; i++)
    {
        m_icons[i] = 0;
    }

    // Free icon bitmaps.
    for (IconsMap::iterator it = m_iconBitmapsMap.begin(); it != m_iconBitmapsMap.end(); ++it)
    {
        delete it->second;
    }
    m_iconBitmapsMap.clear();
}

//////////////////////////////////////////////////////////////////////////
int CIconManager::GetIconTexture(const char* iconName)
{
    int id = 0;
    if (m_textures.Find(iconName, id))
    {
        return id;
    }

    if ((!iconName) || (iconName[0] == 0))
    {
        return 0;
    }

    ITexture* texture = GetIEditor()->GetRenderer() ? GetIEditor()->GetRenderer()->EF_LoadTexture(iconName) : nullptr;
    if (texture)
    {
        id = texture->GetTextureID();
        m_textures[iconName] = id;
    }

    return id;
}

//////////////////////////////////////////////////////////////////////////
int CIconManager::GetIconTexture(EIcon icon)
{
    assert(icon >= 0 && icon < eIcon_COUNT);
    if (m_icons[icon])
    {
        return m_icons[icon];
    }

    m_icons[icon] = GetIconTexture(g_IconNames[icon]);
    return m_icons[icon];
}


//////////////////////////////////////////////////////////////////////////
_smart_ptr<IMaterial>  CIconManager::GetHelperMaterial()
{
    if (!m_pHelperMtl)
    {
        m_pHelperMtl = GetIEditor()->Get3DEngine()->GetMaterialManager()->LoadMaterial(HELPER_MATERIAL);
    }
    return m_pHelperMtl;
};

//////////////////////////////////////////////////////////////////////////
IStatObj*   CIconManager::GetObject(EStatObject object)
{
    assert(object >= 0 && object < eStatObject_COUNT);

    if (m_objects[object])
    {
        return m_objects[object];
    }

    // Try to load this object.
    m_objects[object] = GetIEditor()->Get3DEngine()->LoadStatObjUnsafeManualRef(g_ObjectNames[object], NULL, NULL, false);
    if (!m_objects[object])
    {
        CLogFile::FormatLine("Error: Load Failed: %s", g_ObjectNames[object]);
    }
    m_objects[object]->AddRef();
    if (GetHelperMaterial())
    {
        m_objects[object]->SetMaterial(GetHelperMaterial());
    }
    return m_objects[object];
}

//////////////////////////////////////////////////////////////////////////
QImage* CIconManager::GetIconBitmap(const char* filename, bool& bHaveAlpha, uint32 effects /*=0*/)
{
    QImage* pBitmap = 0;

    QString iconFilename = filename;

    if (Path::GetPath(iconFilename).isEmpty())
    {
        QString iconsPath;
        if (!gSettings.searchPaths[EDITOR_PATH_UI_ICONS].empty())
        {
            iconsPath = gSettings.searchPaths[EDITOR_PATH_UI_ICONS][0];
        }
        iconFilename = Path::Make(iconsPath, iconFilename);
    }

    if (Path::GetExt(iconFilename).isEmpty())
    {
        // By default add .bmp extension to the filename without extension.
        pBitmap = GetIconBitmap((iconFilename + ".png").toUtf8().data(), bHaveAlpha);
        if (!pBitmap)
        {
            pBitmap = GetIconBitmap((iconFilename + ".bmp").toUtf8().data(), bHaveAlpha);
        }
        return pBitmap;
    }

    BOOL bAlphaBitmap = FALSE;
    QPixmap pm(iconFilename);
    bAlphaBitmap = pm.hasAlpha();

    bHaveAlpha = (bAlphaBitmap == TRUE);
    if (!pm.isNull())
    {
        pBitmap = new QImage;
        *pBitmap = pm.toImage();
        m_iconBitmapsMap[filename] = pBitmap;

        // apply image effects
        if (bAlphaBitmap)
        {
            const DWORD dataSize = pm.width() * pm.height() * 4;
            BYTE* pImage = pBitmap->bits();

              if (effects & eIconEffect_ColorEnabled)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 0] = (BYTE)((DWORD)pImage[i + 0] * 109 / 255);
                    pImage[i + 1] = (BYTE)((DWORD)pImage[i + 1] * 97 / 255);
                    pImage[i + 2] = (BYTE)((DWORD)pImage[i + 2] * 89 / 255);
                }
            }

            if (effects & eIconEffect_ColorDisabled)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 0] = (BYTE)((DWORD)pImage[i + 0] * 168 / 255);
                    pImage[i + 1] = (BYTE)((DWORD)pImage[i + 1] * 164 / 255);
                    pImage[i + 2] = (BYTE)((DWORD)pImage[i + 2] * 162 / 255);
                }
            }

            if (effects & eIconEffect_Dim)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 0] /= 2;
                    pImage[i + 1] /= 2;
                    pImage[i + 2] /= 2;
                }
            }

            if (effects & eIconEffect_HalfAlpha)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 3] /= 2;
                }
            }

            if (effects & eIconEffect_TintGreen)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 0] /= 2;
                    pImage[i + 2] /= 2;
                }
            }

            if (effects & eIconEffect_TintRed)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 0] /= 2;
                    pImage[i + 1] /= 2;
                }
            }

            if (effects & eIconEffect_TintYellow)
            {
                for (DWORD i = 0; i < dataSize; i += 4)
                {
                    pImage[i + 0] /= 2;
                }
            }

            // alpha premultiply
            for (DWORD i = 0; i < dataSize; i += 4)
            {
                pImage[i + 0] = ((DWORD)pImage[i + 0] * (DWORD)pImage[i + 3]) / 255;
                pImage[i + 1] = ((DWORD)pImage[i + 1] * (DWORD)pImage[i + 3]) / 255;
                pImage[i + 2] = ((DWORD)pImage[i + 2] * (DWORD)pImage[i + 3]) / 255;
            }
        }

        return pBitmap;
    }
    return NULL;
}
