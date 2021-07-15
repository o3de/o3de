/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzQtComponents/Buses/DragAndDrop.h>
#include <AzToolsFramework/AssetBrowser/AssetBrowserBus.h>
#include <AzToolsFramework/AssetBrowser/Previewer/PreviewerBus.h>

namespace AZ
{
    class Entity;
}

namespace AzQtComponents
{
    class DragAndDropContextBase;
}

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;
        class PreviewerFactory;
        class ProductAssetBrowserEntry;
        class SourceAssetBrowserEntry;
    }
}

class AzAssetBrowserRequestHandler
    : protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    , protected AzQtComponents::DragAndDropEventsBus::Handler
{
public:
    AzAssetBrowserRequestHandler();
    ~AzAssetBrowserRequestHandler() override;

    //////////////////////////////////////////////////////////////////////////
    // AssetBrowserInteractionNotificationBus
    //////////////////////////////////////////////////////////////////////////
    void AddContextMenuActions(QWidget* caller, QMenu* menu, const AZStd::vector<AzToolsFramework::AssetBrowser::AssetBrowserEntry*>& entries) override;
    void AddSourceFileOpeners(const char* fullSourceFileName, const AZ::Uuid& sourceUUID, AzToolsFramework::AssetBrowser::SourceFileOpenerList& openers) override;
    void OpenAssetInAssociatedEditor(const AZ::Data::AssetId& assetId, bool& alreadyHandled) override;

    static bool OpenWithOS(const AZStd::string& fullEntryPath);
protected:

    //////////////////////////////////////////////////////////////////////////
    // AzQtComponents::DragAndDropEventsBus::Handler
    //////////////////////////////////////////////////////////////////////////
    void DragEnter(QDragEnterEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DragMove(QDragMoveEvent* event, AzQtComponents::DragAndDropContextBase& context) override;
    void DragLeave(QDragLeaveEvent* event) override;
    void Drop(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) override;

    bool CanAcceptDragAndDropEvent(
        QDropEvent* event, AzQtComponents::DragAndDropContextBase& context,
        AZStd::optional<AZStd::vector<const AzToolsFramework::AssetBrowser::SourceAssetBrowserEntry*>*> outSources = AZStd::nullopt,
        AZStd::optional<AZStd::vector<const AzToolsFramework::AssetBrowser::ProductAssetBrowserEntry*>*> outProducts = AZStd::nullopt) const;
};
