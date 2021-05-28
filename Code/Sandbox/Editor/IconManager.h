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

// Description : Manages Textures used by Icon.


#ifndef CRYINCLUDE_EDITOR_ICONMANAGER_H
#define CRYINCLUDE_EDITOR_ICONMANAGER_H

#pragma once

struct IStatObj;
struct IMaterial;

#include "Include/IIconManager.h"   // for IIconManager
#include "IEditor.h"                // for IDocListener

/*!
 *  CIconManager contains map of icon names to icon textures,
 *  Ensuring that only one instance of texture for specified icon will be allocated.
 *  Also release textures when editor exit.
 *
 */
class CIconManager
    : public IIconManager
    , public IDocListener
{
public:
    // Construction
    CIconManager();
    ~CIconManager();

    void Init();
    void Done();

    // Unload all loaded resources.
    void Reset();

    // Operations
    virtual int GetIconTexture(EIcon icon);

    virtual IStatObj*   GetObject(EStatObject object);
    virtual int GetIconTexture(const char* iconName);

    //////////////////////////////////////////////////////////////////////////
    // Icon bitmaps.
    //////////////////////////////////////////////////////////////////////////
    virtual QImage* GetIconBitmap(const char* filename, bool& bHaveAlpha, uint32 effects = 0);

    //////////////////////////////////////////////////////////////////////////
    // Implementation of IDocListener.
    //////////////////////////////////////////////////////////////////////////
    virtual void OnNewDocument() { Reset(); };
    virtual void OnLoadDocument() { Reset(); };
    virtual void OnCloseDocument() { Reset(); };
    //////////////////////////////////////////////////////////////////////////

private:
    StdMap<QString, int> m_textures;

    IStatObj*   m_objects[eStatObject_COUNT];
    int m_icons[eIcon_COUNT];

    //////////////////////////////////////////////////////////////////////////
    // Icons bitmaps.
    //////////////////////////////////////////////////////////////////////////
    typedef std::map<QString, QImage*> IconsMap;
    IconsMap m_iconBitmapsMap;
};

#endif // CRYINCLUDE_EDITOR_ICONMANAGER_H
