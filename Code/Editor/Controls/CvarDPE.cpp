/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CvarDPE.h"
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>
#include <AzFramework/DocumentPropertyEditor/ValueStringSort.h>
#include "QtViewPaneManager.h"


namespace AzToolsFramework
{
    CvarDPE::CvarDPE(QWidget* parentWidget)
        : FilteredDPE(parentWidget)
        , m_cvarAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::CvarAdapter>())
        , m_sortAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::ValueStringSort>())
    {
        setWindowTitle(tr("Console Variables"));
        m_sortAdapter->SetSourceAdapter(m_cvarAdapter);
        SetAdapter(m_sortAdapter);
    }

    void CvarDPE::RegisterViewClass()
    {
        ViewPaneOptions opts;
        opts.paneRect = QRect(100, 100, 700, 600);
        RegisterViewPane<CvarDPE>(LyViewPane::ConsoleVariables, LyViewPane::CategoryOther, opts);
    }

} // namespace AzToolsFramework
