/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Manages Textures used by Icon.
#pragma once

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
    ~CIconManager() override;

    void Init();
    void Done();

    // Unload all loaded resources.
    void Reset();

    // Operations
    virtual int GetIconTexture(EIcon icon);
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

    int m_icons[eIcon_COUNT];

    //////////////////////////////////////////////////////////////////////////
    // Icons bitmaps.
    //////////////////////////////////////////////////////////////////////////
    typedef std::map<QString, QImage*> IconsMap;
    IconsMap m_iconBitmapsMap;
};
