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
#include <AzCore/std/string/string_view.h>
#include <IEditor.h>

#include <QMainWindow>
#include <QFileSystemWatcher>

#include <Source/Editor/ui_AudioControlsEditorMainWindow.h>
#endif

namespace AudioControls
{
    class CATLControlsModel;
    class CATLControlsPanel;
    class CInspectorPanel;
    class CAudioSystemPanel;
    class CATLControl;

    //-------------------------------------------------------------------------------------------//
    class CAudioControlsEditorWindow
        : public QMainWindow
        , public Ui::MainWindow
        , public IEditorNotifyListener
    {
        Q_OBJECT

    public:
        explicit CAudioControlsEditorWindow(QWidget* parent = nullptr);
        ~CAudioControlsEditorWindow();
        void OnEditorNotifyEvent(EEditorNotifyEvent event) override;

        // you are required to implement this to satisfy the unregister/registerclass requirements on "RegisterQtViewPane"
        // make sure you pick a unique GUID
        static const GUID& GetClassID()
        {
            // {5793D22F-3740-43FF-8474-5F4769E6E54F}
            static const GUID guid =
            {
                0x5793d22f, 0x3740, 0x43ff, { 0x84, 0x74, 0x5f, 0x47, 0x69, 0xe6, 0xe5, 0x4f }
            };
            return guid;
        }

    private slots:
        void Reload();
        void ReloadMiddlewareData();
        void Save();
        void UpdateFilterFromSelection();
        void UpdateInspector();
        void FilterControlType(EACEControlType type, bool bShow);
        void Update();

    protected:
        void keyPressEvent(QKeyEvent* pEvent) override;
        void closeEvent(QCloseEvent* pEvent) override;

    private:
        void UpdateAudioSystemData();
        void StartWatchingFolder(const AZStd::string_view folder);

        CATLControlsModel* m_pATLModel = nullptr;
        CATLControlsPanel* m_pATLControlsPanel = nullptr;     // Left ATL panel
        CInspectorPanel* m_pInspectorPanel = nullptr;         // Center Connection Editing panel
        CAudioSystemPanel* m_pAudioSystemPanel = nullptr;     // Right Middleware panel
        QFileSystemWatcher m_fileSystemWatcher;

        static bool m_wasClosed;        // true indicates that the window was once open and closed, used for refreshing data
    };
} // namespace AudioControls
