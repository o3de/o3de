/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include <QWidget>
#include <QMenuBar>
#include <QMainWindow>
#include <QLabel>

#include <Editor/EditorParticleSystemComponentRequestBus.h>
#include <Document/ParticleDocument.h>
#include <OpenParticleSystemEditorWindowRequests.h>
#endif
#include <AzQtComponents/Components/StyledDockWidget.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/DockMainWindow.h>

namespace OpenParticleSystemEditor
{
    class EffectorInspector;
    class OpenParticleSystemEditorWindow
        : public AzQtComponents::DockMainWindow
        , public EditorWindowRequestsBus::Handler
        , public OpenParticle::EditorParticleOpenParticleRequestsBus::Handler
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(OpenParticleSystemEditorWindow, AZ::SystemAllocator);

        explicit OpenParticleSystemEditorWindow(QWidget* parent = nullptr);
        ~OpenParticleSystemEditorWindow() override;

    private:
        void SetupMenu();
        void SetupDocking();
        void SetupCentral();
        void SetDockWidget(const AZStd::string& name, QWidget* widget);
        void SetTabWidget(const AZStd::string& path);

        // ParticleEditorWindowRequestsBus
        void ActivateWindow() override;
        void OpenDocument(const AZStd::string& path) override;
        void CreateParticleFile(const AZStd::string& path) override;
        void SaveDocument() override;
        void SaveAllDocuments();

        // EditorParticleOpenParticleRequestsBus
        void OpenParticleFile(const AZStd::string& sourcePath) override;
        bool SaveDialog();

        void closeEvent(QCloseEvent* event) override;
        bool RaiseOpendEmitter(const QString& path);
        void SetStatusMessage(const QString& message);
        bool IsDockWidgetVisible(const AZStd::string& name) const;
        void SetDockWidgetVisible(const AZStd::string& name, bool visible);
        void SetEmitterDockWidget(const AZStd::string& name, QWidget* widget, AZ::u32 area);
        bool AddDockWidget(const AZStd::string& name, QWidget* widget, AZ::u32 area);
        void Checked(const QString& name);
        void UnChecked(const QString& name);
        void RemoveDockWidget(const AZStd::string& name);
        AZStd::vector<AZStd::string> GetDockWidgetNames() const;

        QMenuBar* m_menuBar = nullptr;

        bool m_opened = false;
        AZStd::unique_ptr<ParticleDocument> m_document;
        AZStd::unordered_map<AZStd::string, AZStd::unique_ptr<ParticleDocument>> m_tabWidgetDocument;

        QLabel* m_statusMessage = {};
        QMenu* m_menuFile = {};
        QMenu* m_menuView = {};

        AZStd::unordered_map<AZStd::string, QDockWidget*> m_dockWidgets;
        AZStd::unordered_map<AZStd::string, QAction*> m_dockActions;

        AzQtComponents::FancyDocking* m_fancyDockingManager = nullptr;
        AzQtComponents::DockTabWidget* m_emitterTabWidget = nullptr;
        EffectorInspector* m_effectorInspector = nullptr;
        AZStd::string m_currentTabName;
    };
} // namespace OpenParticleSystemEditor
