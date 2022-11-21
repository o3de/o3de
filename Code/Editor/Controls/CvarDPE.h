/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QWidget>
#include <AzToolsFramework/UI/DocumentPropertyEditor/FilteredDPE.h>

namespace AZ::DocumentPropertyEditor
{
    class CvarAdapter;
    class ValueStringSort;
}

namespace AzToolsFramework
{
    class CvarDPE : public FilteredDPE
    {
    public:
        CvarDPE(QWidget* parentWidget = nullptr);
        static void RegisterViewClass();

    protected:
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::CvarAdapter> m_cvarAdapter;
        AZStd::shared_ptr<AZ::DocumentPropertyEditor::ValueStringSort> m_sortAdapter;
    };
} // namespace AzToolsFramework
