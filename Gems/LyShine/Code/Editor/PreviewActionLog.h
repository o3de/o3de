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

#if !defined(Q_MOC_RUN)
#include <QTextEdit>
#include <LyShine/Bus/UiCanvasBus.h>
#endif

class EditorWindow;

class PreviewActionLog
    : public QTextEdit
    , public UiCanvasNotificationBus::Handler
{
    Q_OBJECT

public: // member functions

    PreviewActionLog(EditorWindow* editorWindow);
    ~PreviewActionLog();

    //! Start logging: Clear the log and register with the given canvas as a listener
    void Activate(AZ::EntityId canvasEntityId);

    //! Stop logging: Unregister as an action listener
    void Deactivate();

private: // member functions

    // UiCanvasActionNotification
    void OnAction(AZ::EntityId entityId, const LyShine::ActionName& actionName) override;
    // ~UiCanvasActionNotification

    QSize sizeHint() const override;

private: // data

    AZ::EntityId m_canvasEntityId;
    QString m_lastMessage;
    int m_repeatCount;
};
