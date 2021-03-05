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

#ifndef CRYINCLUDE_EDITOR_CONTROLS_TOOLBUTTON_H
#define CRYINCLUDE_EDITOR_CONTROLS_TOOLBUTTON_H
#pragma once

// ToolButton.h : header file
//

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformDef.h>
#include <QPushButton>
#endif

AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
class SANDBOX_API QEditorToolButton
    : public QPushButton
    , public IEditorNotifyListener
{
AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    Q_OBJECT
    // Construction
public:
    QEditorToolButton(QWidget* parent = nullptr);
    virtual ~QEditorToolButton();

    void SetToolClass(const QMetaObject* toolClass, const QString& userDataKey = 0, void* userData = nullptr);
    void SetToolName(const QString& editToolName, const QString& userDataKey = 0, void* userData = nullptr);
    // Set if this tool button relies on a loaded level / ready document. By default every tool button only works if a level is loaded.
    // However some tools are also used without a loaded level (e.g. UI Emulator)
    void SetNeedDocument(bool needDocument) { m_needDocument = needDocument; }

    void SetSelected(bool selected);
    void OnEditorNotifyEvent(EEditorNotifyEvent event) override;
protected:
    void OnClicked();

    const QString m_styleSheet;

    //! Tool associated with this button.
    const QMetaObject* m_toolClass;
    CEditTool* m_toolCreated;
    QString m_userDataKey;
    void* m_userData;
    bool m_needDocument;
};


#endif // CRYINCLUDE_EDITOR_CONTROLS_TOOLBUTTON_H
