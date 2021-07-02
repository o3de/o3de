/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "StandaloneTools_precompiled.h"

#include "Source/Driller/ChannelConfigurationDialog.hxx"
#include <Source/Driller/moc_ChannelConfigurationDialog.cpp>

namespace Driller
{
    ChannelConfigurationDialog::ChannelConfigurationDialog(QWidget* parent)
        : QDialog(parent)
    {
        setAttribute(Qt::WA_DeleteOnClose, true);
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowMinimizeButtonHint);

        QMargins margins(0, 0, 0, 0);
        setContentsMargins(margins);
    }

    ChannelConfigurationDialog::~ChannelConfigurationDialog()
    {
        emit DialogClosed(this);
    }    
}
