/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
