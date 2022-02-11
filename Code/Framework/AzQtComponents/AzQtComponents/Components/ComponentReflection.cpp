/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Script/ScriptContext.h>
#include <AzCore/Script/ScriptSystemBus.h>

#include <AzQtComponents/Components/ComponentReflection.h>
#include <AzQtComponents/Components/ButtonDivider.h>
#include <AzQtComponents/Components/ButtonStripe.h>
#include <AzQtComponents/Components/DockBar.h>
#include <AzQtComponents/Components/DockBarButton.h>
#include <AzQtComponents/Components/DockMainWindow.h>
#include <AzQtComponents/Components/DockTabBar.h>
#include <AzQtComponents/Components/DockTabWidget.h>
#include <AzQtComponents/Components/ExtendedLabel.h>
#include <AzQtComponents/Components/FancyDocking.h>
#include <AzQtComponents/Components/FancyDockingDropZoneWidget.h>

namespace AzQtComponents
{
    void ComponentReflection::Reflect(AZ::ReflectContext* context, const AZ::EnvironmentInstance& env)
    {
        AZ::Environment::Attach(env);

        AzQtComponents::ButtonDivider::Reflect(context);
        AzQtComponents::ButtonStripe::Reflect(context);
        AzQtComponents::DockBar::Reflect(context);
        AzQtComponents::DockBarButton::Reflect(context);
        AzQtComponents::DockMainWindow::Reflect(context);
        AzQtComponents::DockTabBar::Reflect(context);
        AzQtComponents::DockTabWidget::Reflect(context);
        AzQtComponents::ExtendedLabel::Reflect(context);
        AzQtComponents::FancyDocking::Reflect(context);
        AzQtComponents::FancyDockingDropZoneWidget::Reflect(context);

        AZ::Environment::Detach();
    }
} // namespace AzQtComponents
