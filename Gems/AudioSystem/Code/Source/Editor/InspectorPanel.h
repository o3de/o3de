/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#if !defined(Q_MOC_RUN)
#include <ATLControlsModel.h>
#include <AudioControl.h>
#include <IAudioSystemEditor.h>
#include <QConnectionsWidget.h>

#include <QWidget>
#include <QMenu>
#include <QListWidget>

#include <Source/Editor/ui_InspectorPanel.h>
#endif

namespace AudioControls
{
    class CATLControl;

    //-------------------------------------------------------------------------------------------//
    class CInspectorPanel
        : public QWidget
        , public Ui::InspectorPanel
        , public IATLControlModelListener
    {
        Q_OBJECT
    public:
        explicit CInspectorPanel(CATLControlsModel* atlControlsModel);
        ~CInspectorPanel();
        void Reload();

    public slots:
        void SetSelectedControls(const ControlList& selectedControls);
        void UpdateInspector();

    private slots:
        void SetControlName(QString name);
        void SetControlScope(QString scope);
        void SetAutoLoadForCurrentControl(bool isAutoLoad);
        void FinishedEditingName();

    private:
        void UpdateNameControl();
        void UpdateScopeControl();
        void UpdateAutoLoadControl();
        void UpdateConnectionListControl();
        void UpdateConnectionData();
        void UpdateScopeData();

        void HideScope(bool hide);
        void HideAutoLoad(bool hide);

        //////////////////////////////////////////////////////////
        // IATLControlModelListener implementation
        void OnControlModified(CATLControl* control) override;
        //////////////////////////////////////////////////////////

        CATLControlsModel* m_atlControlsModel;

        EACEControlType m_selectedType;
        AZStd::vector<CATLControl*> m_selectedControls;
        bool m_allControlsSameType;

        QColor m_notFoundColor;
    };

} // namespace AudioControls
