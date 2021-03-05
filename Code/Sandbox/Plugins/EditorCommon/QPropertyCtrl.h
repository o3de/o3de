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
#pragma once


#include "EditorCommonAPI.h"
#include "Controls/PropertyCtrl.h"
#include <QtWinMigrate/qwinhost.h>

template<class T>
class TemplatePropertyCtrl
    : public QWinHost
{
public:
    TemplatePropertyCtrl(QWidget* parent)
        : QWinHost(parent) { }

    T     m_props;
protected:
    virtual HWND createWindow(HWND parent, HINSTANCE instance)
    {
        CWnd* parentWindow = CWnd::FromHandle(parent);
        m_props.Create(WS_CHILD | WS_VISIBLE, CRect(0, 0, 100, 100), parentWindow /*, IDC_GRAPH_PROPERTIES*/);
        m_props.ModifyStyleEx(0, WS_EX_CLIENTEDGE);
        m_props.SetParent(parentWindow);
        return m_props.m_hWnd;
    }
};

class QPropertyCtrl
    : public TemplatePropertyCtrl <CPropertyCtrl>
{
public:
    QPropertyCtrl(QWidget* parent)
        : TemplatePropertyCtrl<CPropertyCtrl>(parent)
    {
    }
};


