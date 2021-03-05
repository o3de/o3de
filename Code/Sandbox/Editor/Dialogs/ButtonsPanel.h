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

#ifndef CRYINCLUDE_EDITOR_DIALOGS_BUTTONSPANEL_H
#define CRYINCLUDE_EDITOR_DIALOGS_BUTTONSPANEL_H
#pragma once

#if !defined(Q_MOC_RUN)
#include <QWidget>
#endif

class QEditorToolButton;

/////////////////////////////////////////////////////////////////////////////
// Panel with custom auto arranged buttons
class CButtonsPanel
    : public QWidget
{
    Q_OBJECT
public:

    struct SButtonInfo
    {
        QString name;
        QString toolClassName;
        QString toolUserDataKey;
        std::string toolUserData;
        QString toolTip;
        bool bNeedDocument;
        const QMetaObject* pToolClass;

        SButtonInfo()
            : pToolClass(nullptr)
            , bNeedDocument(true) {};
    };

    CButtonsPanel(QWidget* parent);
    virtual ~CButtonsPanel();

    virtual void AddButton(const SButtonInfo& button);
    virtual void AddButton(const QString& name, const QString& toolClass);
    virtual void AddButton(const QString& name, const QMetaObject* pToolClass);
    virtual void EnableButton(const QString& buttonName, bool disable);
    virtual void ClearButtons();

    virtual void OnButtonPressed([[maybe_unused]] const SButtonInfo& button) {};
    virtual void UncheckAll();

protected:
    void ReleaseGuiButtons();

    virtual void OnInitDialog();

    //////////////////////////////////////////////////////////////////////////
    struct SButton
    {
        SButtonInfo info;
        QEditorToolButton* pButton;
        SButton()
            : pButton(nullptr) {};
    };

    std::vector<SButton> m_buttons;
};

#endif // CRYINCLUDE_EDITOR_DIALOGS_BUTTONSPANEL_H
