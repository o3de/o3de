/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <AudioControl.h>

#include <QWidget>
#include <QListWidget>
#include <QColor>

#include <Source/Editor/ui_ConnectionsWidget.h>
#endif

namespace AudioControls
{
    class CATLControl;
    class IAudioConnectionInspectorPanel;

    //-------------------------------------------------------------------------------------------//
    class QConnectionsWidget
        : public QWidget
        , public Ui::ConnectionsWidget
    {
        Q_OBJECT
    public:
        QConnectionsWidget(QWidget* parent = nullptr);

    public slots:
        void SetControl(CATLControl* control);

    private slots:
        void ShowConnectionContextMenu(const QPoint& pos);
        void CurrentConnectionModified();
        void RemoveSelectedConnection();

    private:
        bool eventFilter(QObject* object, QEvent* event) override;
        void MakeConnectionTo(IAudioSystemControl* middlewareControl);
        void UpdateConnections();
        void CreateItemFromConnection(IAudioSystemControl* middlewareControl);

        CATLControl* m_control;
        QColor m_notFoundColor;
        QColor m_localizedColor;
    };

} // namespace AudioControls
