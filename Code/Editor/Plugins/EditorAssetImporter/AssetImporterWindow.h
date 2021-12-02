#pragma once

/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/////////////////////////////////////////////////////////////////////////////
//
// Asset Importer Qt Interface
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(Q_MOC_RUN)
#include <AzCore/PlatformIncl.h>
#include <QMainWindow>
#include <AssetImporterDocument.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzCore/Math/Guid.h>
#endif

namespace AZStd
{
    class thread;
}

namespace Ui
{
    class AssetImporterWindow;
}

namespace AZ
{
    class Module;

    namespace SceneAPI
    {
        namespace UI
        {
            class OverlayWidget;
        }
        namespace SceneUI
        {
            class ProcessingOverlayWidget;
        }
    }
}

class ImporterRootDisplay;
class QCloseEvent;
class QMenu;
class QAction;

class AssetImporterWindow
    : public QMainWindow
{
    Q_OBJECT
public:
    AssetImporterWindow();
    explicit AssetImporterWindow(QWidget* parent);
    ~AssetImporterWindow();

    // This implementation is required for unregister/register on "RegisterQtViewPane"
    static const GUID& GetClassID()
    {
        // {c50c09d6-5bfa-4d49-8542-e350656ed1bc}
        static const GUID guid = {
            0xc50c09d6, 0x5bfa, 0x4d49, { 0x85, 0x42, 0xe3, 0x50, 0x65, 0x6e, 0xd1, 0xbc }
        };
        return guid;
    }
    
    void OpenFile(const AZStd::string& filePath);
    
    void closeEvent(QCloseEvent* ev);

public slots:
    void OnSceneResetRequested();
    void OnOpenDocumentation();
    void OnInspect();

private:
    void Init();
    void OpenFileInternal(const AZStd::string& filePath);
    bool IsAllowedToChangeSourceFile();
    
    enum class WindowState
    {
        InitialNothingLoaded,
        FileLoaded,
        OverlayShowing
    };

    void ResetMenuAccess(WindowState state);
    void SetTitle(const char* filePath);
    void HandleAssetLoadingCompleted();
    void ClearProcessingOverlay();

private slots:
    void UpdateClicked();
    
    void OverlayLayerAdded();
    void OverlayLayerRemoved();

private:
    static const AZ::Uuid s_browseTag;
    static const char* s_documentationWebAddress;

    QScopedPointer<Ui::AssetImporterWindow> ui;
    QScopedPointer<AssetImporterDocument> m_assetImporterDocument;
    QScopedPointer<AZ::SceneAPI::UI::OverlayWidget> m_overlay;

    AZ::SerializeContext* m_serializeContext;
    AZStd::string m_fullSourcePath;

    QScopedPointer<ImporterRootDisplay> m_rootDisplay;
    bool m_isClosed;

    int m_processingOverlayIndex;
    QSharedPointer<AZ::SceneAPI::SceneUI::ProcessingOverlayWidget> m_processingOverlay;
};
