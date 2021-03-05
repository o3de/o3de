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
    }
}

class LegacyPreviewerFactory;

class AzAssetBrowserRequestHandler
    : protected AzToolsFramework::AssetBrowser::AssetBrowserInteractionNotificationBus::Handler
    , protected AzQtComponents::DragAndDropEventsBus::Handler
    , protected AzToolsFramework::AssetBrowser::PreviewerRequestBus::Handler
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

    //////////////////////////////////////////////////////////////////////////
    // PreviewerRequestBus::Handler
    //////////////////////////////////////////////////////////////////////////
    const AzToolsFramework::AssetBrowser::PreviewerFactory* GetPreviewerFactory(const AzToolsFramework::AssetBrowser::AssetBrowserEntry* entry) const override;

    bool CanAcceptDragAndDropEvent(QDropEvent* event, AzQtComponents::DragAndDropContextBase& context) const;

private:
    AZStd::unique_ptr<const LegacyPreviewerFactory> m_previewerFactory;
};
