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
        void SelectedConnectionChanged();
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
