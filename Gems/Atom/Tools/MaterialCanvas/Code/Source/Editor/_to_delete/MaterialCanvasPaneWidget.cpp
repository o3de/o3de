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
        //! Resolves the graph view settings before the base constructor runs. If the component is not active something is very
        //! wrong with component ordering, but returning an empty settings object keeps this from dereferencing null and lets
        //! the failure surface as an obviously blank pane rather than a crash on Editor startup.
        AtomToolsFramework::GraphViewSettingsPtr GetGraphViewSettingsForPane()
        {
            if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
            {
                return systemComponent->GetGraphViewSettings();
            }

            AZ_Error("MaterialCanvasPaneWidget", false, "Material Canvas pane opened before its system component activated.");
            return {};
        }

        //! Creates the untitled graph document the tool opens with, equivalent to MaterialCanvasApplication::InitDefaultDocument.
        //!
        //! THIS MUST STAY A FREE FUNCTION. It cannot be inlined into a MaterialCanvasPaneWidget member.
        //!
        //! AtomToolsDocumentMainWindow inherits AtomToolsDocumentNotificationBus::Handler *privately*, which makes the injected
        //! name AtomToolsDocumentNotifications an inaccessible member of every class derived from that window. Naming it inside
        //! a member of MaterialCanvasPaneWidget to form &AtomToolsDocumentNotifications::OnDocumentOpened is therefore
        //! ill-formed (MSVC C2247/C2248), regardless of whether it is spelled through ::Handler:: or ::Events:: -- both resolve
        //! to the same class, and it is the name lookup that is rejected, not the member. MaterialCanvasApplication writes the
        //! identical expression without trouble only because it is not part of that window's hierarchy.
        //!
        //! A namespace-scope function is not a member of the hierarchy, so no inherited-name access check applies.
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

        //! Free for the same reason as above, so that the two document calls in this file stay consistent and neither can be
        //! moved back into a member without the compiler explaining why at length.
        void CloseAllDocumentsForPane()
        {
            AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Event(
                MaterialCanvasEditorSystemComponent::ToolId,
                &AtomToolsFramework::AtomToolsDocumentSystemRequestBus::Events::CloseAllDocuments);
        }
    } // namespace

    MaterialCanvasPaneWidget::MaterialCanvasPaneWidget(QWidget* parent)
        // The trailing `true` marks this window as hosted, which stops AtomToolsMainWindow wrapping itself in a
        // WindowDecorationWrapper and floating away from the pane. It has to be passed explicitly: the Editor's
        // QtViewPaneManager::CreateWidget always calls the pane factory with a null parent -- "we need to set the parent
        // explicitly", as the comment there puts it -- and only reparents the finished widget into its DockWidget
        // afterwards, so `parent` is always null here and says nothing about whether the window is hosted.
        : Base(MaterialCanvasEditorSystemComponent::ToolId, GetGraphViewSettingsForPane(), parent, true)
    {
        if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
        {
            systemComponent->SetPaneWidget(this);
        }

        // The standalone application creates an untitled document at startup. Doing the equivalent here, on pane open rather
        // than on Editor start, keeps the tool usable immediately without paying for a graph document in every Editor session
        // that never opens the pane.
        CreateDefaultDocumentForPane();
    }

    MaterialCanvasPaneWidget::~MaterialCanvasPaneWidget()
    {
        // Close any open documents before the window goes away. The document views are children of this window and the
        // document system outlives it, so leaving documents open would leave the system holding views that Qt has destroyed.
        CloseAllDocumentsForPane();

        if (auto systemComponent = MaterialCanvasEditorSystemComponent::GetInstance())
        {
            systemComponent->SetPaneWidget(nullptr);
        }
    }
} // namespace MaterialCanvas
