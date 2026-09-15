/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AtomToolsFramework/Document/AtomToolsDocumentNotificationBus.h>
#include <AtomToolsFramework/Document/AtomToolsDocumentSystemRequestBus.h>
#include <AtomToolsFramework/Util/Util.h>
#include <Editor/MaterialCanvasEditorSystemComponent.h>
#include <Editor/MaterialCanvasPaneWidget.h>

namespace MaterialCanvas
{
    namespace
    {
        //! Resolves graph view settings before the base constructor; empty if the component is inactive, so failure is a blank pane.
        AtomToolsFramework::GraphViewSettingsPtr GetGraphViewSettingsForPane()
        {
            if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
            {
                return systemComponent->GetGraphViewSettings();
            }

            AZ_Error("MaterialCanvasPaneWidget", false, "Material Canvas pane opened before its system component activated.");
            return {};
        }

        //! Creates the untitled startup graph; must stay free since the private Handler base makes the notification name inaccessible.
        void CreateDefaultDocumentForPane()
        {
            if (!AtomToolsFramework::GetSettingsValue("/O3DE/Atom/MaterialCanvas/CreateDefaultDocumentOnStart", true))
            {
                return;
            }

            AZ::Uuid documentId = AZ::Uuid::CreateNull();
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::EventResult(
                documentId,
                MaterialCanvasEditorSystemComponent::ToolId,
                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CreateDocumentFromTypeName,
                "Material Graph");

            AtomToolsFramework::AtomToolsDocumentNotificationBus::Event(
                MaterialCanvasEditorSystemComponent::ToolId,
                &AtomToolsFramework::AtomToolsDocumentNotificationBus::Events::OnDocumentOpened,
                documentId);
        }

        //! Free for the same reason as above, keeping both document calls in this file consistent.
        void CloseAllDocumentsForPane()
        {
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                MaterialCanvasEditorSystemComponent::ToolId,
                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        }
    } // namespace

    MaterialCanvasPaneWidget::MaterialCanvasPaneWidget(QWidget* parent)
        // `true` marks the window as hosted (no WindowDecorationWrapper); parent is always null here, so it can't be inferred.
        : Base(MaterialCanvasEditorSystemComponent::ToolId, GetGraphViewSettingsForPane(), parent, true)
    {
        if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
        {
            systemComponent->SetPaneWidget(this);
        }

        // Create the untitled document on pane open, as the standalone app does at startup, so unused Editor sessions pay nothing.
        CreateDefaultDocumentForPane();
    }

    MaterialCanvasPaneWidget::~MaterialCanvasPaneWidget()
    {
        // Close documents first: their views are children of this window, but the document system outlives it.
        CloseAllDocumentsForPane();

        if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
        {
            systemComponent->SetPaneWidget(nullptr);
        }
    }
} // namespace MaterialCanvas
