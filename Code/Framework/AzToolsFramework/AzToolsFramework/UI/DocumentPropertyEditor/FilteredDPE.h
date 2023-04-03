/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
#include <AzToolsFramework/UI/DocumentPropertyEditor/ValueStringFilter.h>

namespace Ui
{
    class FilteredDPE;
}

namespace AzToolsFramework
{
    class DocumentPropertyEditor;

    class FilteredDPE
        : public QWidget
    {
    public:
        FilteredDPE(QWidget* parentWidget = nullptr);
        virtual ~FilteredDPE();

        void SetAdapter(AZStd::shared_ptr<AZ::DocumentPropertyEditor::DocumentAdapter> sourceAdapter);
        DocumentPropertyEditor* GetDPE();

    protected:
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::DocumentAdapter> m_sourceAdapter;
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::ValueStringFilter> m_filterAdapter;

        Ui::FilteredDPE* m_ui;
    };
} // namespace AzToolsFramework
