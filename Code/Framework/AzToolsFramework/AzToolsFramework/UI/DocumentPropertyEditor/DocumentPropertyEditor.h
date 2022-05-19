/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>
#include <AzToolsFramework/UI/DocumentPropertyEditor/PropertyHandlerWidget.h>
#include <AzCore/Interface/Interface.h>
#include <QFrame>

class QVBoxLayout;
class QHBoxLayout;

#endif // Q_MOC_RUN

namespace AzToolsFramework
{
    class DocumentPropertyEditor;

    class DPERowWidget : public QWidget
    {
        Q_OBJECT

    public:
        DPERowWidget(QWidget* parentWidget);
        ~DPERowWidget();

        void Clear(); //!< destroy all layout contents and clear DOM children
        void AddChildFromDomValue(const AZ::Dom::Value& childValue, int domIndex);

        //! clears and repopulates all children from a given DOM array
        void SetValueFromDom(const AZ::Dom::Value& domArray);

        //! handles a patch operation at the given path, or delegates to a child that will
        void HandleOperationAtPath(const AZ::Dom::PatchOperation& domOperation, size_t pathIndex = 0);

    protected:
        DocumentPropertyEditor* GetDPE();

        QVBoxLayout* m_mainLayout = nullptr;
        QHBoxLayout* m_columnLayout = nullptr;
        QVBoxLayout* m_childRowLayout = nullptr;

        AZStd::deque<QWidget*> m_domOrderedChildren;

        // a map from the propertyHandler widgets to the propertyHandlers that created them
        AZStd::unordered_map<QWidget*, AZStd::unique_ptr<PropertyHandlerWidgetInterface>> m_widgetToPropertyHandler;
    };

    class DocumentPropertyEditor : public QFrame
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(DocumentPropertyEditor, AZ::SystemAllocator, 0);

        DocumentPropertyEditor(QWidget* parentWidget);
        ~DocumentPropertyEditor();

        //! set the DOM adapter for this DPE to inspect
        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter);
        AZ::DocumentPropertyEditor::DocumentAdapter* GetAdapter() { return m_adapter; }

    protected:
        QVBoxLayout* GetVerticalLayout();
        void AddRowFromValue(const AZ::Dom::Value& domValue, int rowIndex);

        void HandleReset();
        void HandleDomChange(const AZ::Dom::Patch& patch);

        AZ::DocumentPropertyEditor::DocumentAdapter* m_adapter = nullptr;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changedHandler;
    };
} // namespace AzToolsFramework
