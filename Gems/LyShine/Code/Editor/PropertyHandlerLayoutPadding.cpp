/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include "EditorCommon.h"

#include "PropertyHandlerLayoutPadding.h"

QWidget* PropertyHandlerLayoutPadding::CreateGUI(QWidget* pParent)
{
    AzQtComponents::VectorInput* ctrl = m_common.ConstructGUI(pParent);
    ctrl->setLabel(0, "Left");
    ctrl->setLabel(1, "Top");
    ctrl->setLabel(2, "Right");
    ctrl->setLabel(3, "Bottom");

    return ctrl;
}

void PropertyHandlerLayoutPadding::Register()
{
    AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
        &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Events::RegisterPropertyType, aznew PropertyHandlerLayoutPadding());
}
