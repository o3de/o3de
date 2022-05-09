/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <QFrame>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/DOM/Backends/JSON/JsonBackend.h>
#include <AzFramework/DocumentPropertyEditor/DocumentAdapter.h>

class QVBoxLayout;
class QHBoxLayout;

#endif // Q_MOC_RUN

namespace AzToolsFramework
{
    class DPERowWidget : public QWidget
    {
        Q_OBJECT

    public:
        DPERowWidget(QWidget* parentWidget);
        ~DPERowWidget();

        void Clear();
        void PopulateFromValue(const AZ::Dom::Value& domValue);

    protected:
        QVBoxLayout* m_mainLayout = nullptr;
        QHBoxLayout* m_columnLayout = nullptr;
        QVBoxLayout* m_childLayout = nullptr;
    };

    class DocumentPropertyEditor : public QFrame
    {
        Q_OBJECT

    public:
        AZ_CLASS_ALLOCATOR(DocumentPropertyEditor, AZ::SystemAllocator, 0);

        DocumentPropertyEditor(QWidget* parentWidget);
        ~DocumentPropertyEditor();

        void SetAdapter(AZ::DocumentPropertyEditor::DocumentAdapter* theAdapter);

    protected:
        QVBoxLayout* GetVerticalLayout();

        void HandleReset();
        void HandleDomChange(const AZ::Dom::Patch& patch);

        AZ::DocumentPropertyEditor::DocumentAdapter* m_adapter = nullptr;
        AZ::DocumentPropertyEditor::DocumentAdapter::ResetEvent::Handler m_resetHandler;
        AZ::DocumentPropertyEditor::DocumentAdapter::ChangedEvent::Handler m_changedHandler;
    };
} // namespace AzToolsFramework