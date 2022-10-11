/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "CvarDPE.h"
#include <AzFramework/DocumentPropertyEditor/CvarAdapter.h>
#include "QtViewPaneManager.h"


namespace AzToolsFramework
{
    CvarDPE::CvarDPE(QWidget* parentWidget)
        : FilteredDPE(parentWidget)
        , m_cvarAdapter(AZStd::make_shared<AZ::DocumentPropertyEditor::CvarAdapter>())
    {
        setWindowTitle(tr("Console Variables"));
        SetAdapter(m_cvarAdapter);
    }

    void CvarDPE::RegisterViewClass()
    {
        ViewPaneOptions opts;
        opts.paneRect = QRect(100, 100, 700, 600);
        opts.isDeletable = false;
        RegisterViewPane<CvarDPE>(LyViewPane::ConsoleVariables, LyViewPane::CategoryOther, opts);
    }

} // namespace AzToolsFramework
