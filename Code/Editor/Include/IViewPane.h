/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IVIEWPANE_H
#define CRYINCLUDE_EDITOR_INCLUDE_IVIEWPANE_H
#pragma once

#include "IEditorClassFactory.h"

#include <QSize>

class QWidget;
class QRect;

struct IViewPaneClass
    : public IClassDesc
{
    DEFINE_UUID(0x7E13EC7C, 0xF621, 0x4aeb, 0xB6, 0x42, 0x67, 0xD7, 0x8E, 0xD4, 0x68, 0xF8)

    enum EDockingDirection
    {
        DOCK_TOP,
        DOCK_LEFT,
        DOCK_RIGHT,
        DOCK_BOTTOM,
        DOCK_FLOAT,
    };

    virtual ~IViewPaneClass() = default;

    // Return text for view pane title.
    virtual QString GetPaneTitle() = 0;

    // Return the string resource ID for the title's text.
    virtual unsigned int GetPaneTitleID() const = 0;

    // Return preferable initial docking position for pane.
    virtual EDockingDirection GetDockingDirection() = 0;

    // Initial pane size.
    virtual QRect GetPaneRect() = 0;

    // Get Minimal view size
    virtual QSize GetMinSize() { return QSize(0, 0); }

    // Return true if only one pane at a time of time view class can be created.
    virtual bool SinglePane() = 0;

    // Return true if the view window wants to get ID_IDLE_UPDATE commands.
    virtual bool WantIdleUpdate() = 0;

    //////////////////////////////////////////////////////////////////////////
    // IUnknown
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE QueryInterface(const IID& riid, void** ppvObj)
    {
        if (riid == __uuidof(IViewPaneClass))
        {
            *ppvObj = this;
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    //////////////////////////////////////////////////////////////////////////

};

#endif // CRYINCLUDE_EDITOR_INCLUDE_IVIEWPANE_H
