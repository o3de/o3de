#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(Q_MOC_RUN)
#include <AzCore/Debug/TraceMessageBus.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/Asset/AssetSystemBus.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzToolsFramework/Debug/TraceContextMultiStackHandler.h>
#include <AzQtComponents/Components/StyledDetailsTableModel.h>
#include <SceneAPI/SceneUI/SceneUIConfiguration.h>
#include <QScopedPointer>
#include <QWidget>
#include <SceneAPI/SceneUI/CommonWidgets/OverlayWidget.h>
#include <QSortFilterProxyModel>
#endif

class QCloseEvent;
class QLabel;
class QTimer;

namespace AzQtComponents
{
    class StyledBusyLabel;
    class StyledDetailsTableView;
}

namespace AzToolsFramework
{
    namespace Logging
    {
        class LogEntry;
    }
}

namespace AZ
{
    namespace SceneAPI
    {
        namespace SceneUI
        {
            // The qt-generated ui code (from the .ui)
            namespace Ui
            {
                class ProcessingOverlayWidget;
            }

            namespace Internal
            {
                // This QSortFilterProxyModel filters out an erroneous message.
                // ResourceCompiler loads all gems. There are some gems which depend on
                // EditorLib, which loads QtWebEngineWidgets. QtWebEngineWidgets prints a
                // warning if it is loaded after a QCoreApplication has been instantiated,
                // but no QOpenGLContext exists, which is always the case with
                // ResourceCompiler.
                // The correct fix would be to remove all dependencies on EditorLib from
                // gems.
                class QtWebEngineMessageFilter
                    : public QSortFilterProxyModel
                {
                    Q_OBJECT
                public:
                    explicit QtWebEngineMessageFilter(QObject* parent = nullptr);
                    ~QtWebEngineMessageFilter() override;

                protected:
                    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
                };
            }

            class ProcessingHandler;
            AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
            AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
            class SCENE_UI_API ProcessingOverlayWidget 
                : public QWidget
                , public Debug::TraceMessageBus::Handler
            {
            AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
            AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
                Q_OBJECT
            public:
                AZ_CLASS_ALLOCATOR(ProcessingOverlayWidget, SystemAllocator, 0);

                //! Layout configurations for the various stages the Scene Settings can be in.
                enum class Layout
                {
                    Loading,
                    Resetting,
                    Exporting
                };

                ProcessingOverlayWidget(UI::OverlayWidget* overlay, Layout layout, Uuid traceTag);
                ~ProcessingOverlayWidget() override;

                bool OnPrintf(const char* window, const char* message) override;
                bool OnError(const char* window, const char* message) override;
                bool OnWarning(const char* window, const char* message) override;
                bool OnAssert(const char* message) override;

                int PushToOverlay();

                void SetAndStartProcessingHandler(const AZStd::shared_ptr<ProcessingHandler>& handler);
                AZStd::shared_ptr<ProcessingHandler> GetProcessingHandler() const;

                bool GetAutoCloseOnSuccess() const;
                void SetAutoCloseOnSuccess(bool autoCloseOnSuccess);
                bool HasProcessingCompleted() const;

                void BlockClosing();
                void UnblockClosing();

            signals:
                void Closing();

            public slots:
                void AddLogEntry(const AzToolsFramework::Logging::LogEntry& entry);

                void OnLayerRemoved(int layerId);

                void OnSetStatusMessage(const AZStd::string& message);
                void OnProcessingComplete();

                void UpdateColumnSizes();

            private:
                void SetUIToCompleteState();

                bool CanClose() const;
                bool ShouldProcessMessage() const;
                void CopyTraceContext(AzQtComponents::StyledDetailsTableModel::TableEntry& entry) const;

                AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
                AzToolsFramework::Debug::TraceContextMultiStackHandler m_traceStackHandler;
                Uuid m_traceTag;
                QScopedPointer<Ui::ProcessingOverlayWidget> ui;
                AZStd::shared_ptr<ProcessingHandler> m_targetHandler;
                AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
                UI::OverlayWidget* m_overlay;
                AzQtComponents::StyledBusyLabel* m_busyLabel;
                AzQtComponents::StyledDetailsTableView* m_reportView;
                AzQtComponents::StyledDetailsTableModel* m_reportModel;
                QLabel* m_progressLabel;
                int m_layerId;
                QTimer* m_resizeTimer;
                
                bool m_isProcessingComplete;
                bool m_isClosingBlocked;
                bool m_autoCloseOnSuccess;
                bool m_encounteredIssues;
            };
        }
    }
}
