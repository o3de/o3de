/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "KeyQueryDPE.h"

namespace AzToolsFramework
{
    KeyQueryDPE::KeyQueryDPE(AZ::DocumentPropertyEditor::DocumentAdapterPtr keyQueryAdapter, QWidget* parentWidget)
        : QDialog(parentWidget)
    {
        setupUi(this);
        dpe->SetAdapter(keyQueryAdapter);
    }
} // namespace AzToolsFramework
