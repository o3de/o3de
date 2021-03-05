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

// Description : implementation file


#include "EditorDefs.h"

// Editor
#include "CryEditDoc.h"
#include "EditTool.h"
#include "ToolButton.h"


QEditorToolButton::QEditorToolButton(QWidget* parent /* = nullptr */)
    : QPushButton(parent)
    , m_styleSheet(styleSheet())
    , m_toolClass(nullptr)
    , m_toolCreated(nullptr)
    , m_needDocument(true)
{
    setSizePolicy({ QSizePolicy::Expanding, QSizePolicy::Fixed });
    connect(this, &QAbstractButton::clicked, this, &QEditorToolButton::OnClicked);
    GetIEditor()->RegisterNotifyListener(this);
}

QEditorToolButton::~QEditorToolButton()
{
    GetIEditor()->UnregisterNotifyListener(this);
}

void QEditorToolButton::SetToolName(const QString& editToolName, const QString& userDataKey, void* userData)
{
    IClassDesc* klass = GetIEditor()->GetClassFactory()->FindClass(editToolName.toUtf8().data());
    if (!klass)
    {
        Warning(QStringLiteral("Editor Tool %1 not registered.").arg(editToolName).toUtf8().data());
        return;
    }
    if (klass->SystemClassID() != ESYSTEM_CLASS_EDITTOOL)
    {
        Warning(QStringLiteral("Class name %1 is not a valid Edit Tool class.").arg(editToolName).toUtf8().data());
        return;
    }

    QScopedPointer<QObject> o(klass->CreateQObject());
    if (!qobject_cast<CEditTool*>(o.data()))
    {
        Warning(QStringLiteral("Class name %1 is not a valid Edit Tool class.").arg(editToolName).toUtf8().data());
        return;
    }
    SetToolClass(o->metaObject(), userDataKey, userData);
}

//////////////////////////////////////////////////////////////////////////
void QEditorToolButton::SetToolClass(const QMetaObject* toolClass, const QString& userDataKey, void* userData)
{
    m_toolClass = toolClass;

    m_userData = userData;
    if (!userDataKey.isEmpty())
    {
        m_userDataKey = userDataKey;
    }
}

void QEditorToolButton::OnEditorNotifyEvent(EEditorNotifyEvent event)
{
    switch (event)
    {
    case eNotify_OnBeginNewScene:
    case eNotify_OnBeginLoad:
    case eNotify_OnBeginSceneOpen:
    {
        if (m_needDocument)
        {
            setEnabled(false);
        }
        break;
    }

    case eNotify_OnEndNewScene:
    case eNotify_OnEndLoad:
    case eNotify_OnEndSceneOpen:
    {
        if (m_needDocument)
        {
            setEnabled(true);
        }
        break;
    }
    case eNotify_OnEditToolChange:
    {
        CEditTool* tool = GetIEditor()->GetEditTool();

        if (!tool || tool != m_toolCreated || tool->metaObject() != m_toolClass)
        {
            m_toolCreated = nullptr;
            SetSelected(false);
        }
    }
    default:
        break;
    }
}

void QEditorToolButton::OnClicked()
{
    if (!m_toolClass)
    {
        return;
    }

    if (m_needDocument && !GetIEditor()->GetDocument()->IsDocumentReady())
    {
        return;
    }

    CEditTool* tool = GetIEditor()->GetEditTool();
    if (tool && tool->IsMoveToObjectModeAfterEnd() && tool->metaObject() == m_toolClass && tool == m_toolCreated)
    {
        GetIEditor()->SetEditTool(nullptr);
        SetSelected(false);
    }
    else
    {
        CEditTool* newTool = qobject_cast<CEditTool*>(m_toolClass->newInstance());
        if (!newTool)
        {
            return;
        }

        m_toolCreated = newTool;

        SetSelected(true);

        if (m_userData)
        {
            newTool->SetUserData(m_userDataKey.toUtf8().data(), (void*)m_userData);
        }

        update();

        // Must be last function, can delete this.
        GetIEditor()->SetEditTool(newTool);
    }
}

void QEditorToolButton::SetSelected(bool selected)
{
    if (selected)
    {
        setStyleSheet(QStringLiteral("QPushButton { background-color: palette(highlight); color: palette(highlighted-text); }"));
    }
    else
    {
        setStyleSheet(m_styleSheet);
    }
}

#include <Controls/moc_ToolButton.cpp>
