/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzQtComponents/Components/ButtonDivider.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace AzQtComponents
{
    ButtonDivider::ButtonDivider(QWidget* parent)
        : QFrame(parent)
    {

    }

    void ButtonDivider::Reflect(AZ::ReflectContext* context)
    {
        if (auto* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
        {
            behaviorContext->Class<ButtonDivider>("ButtonDivider")
                ->Attribute(AZ::Script::Attributes::Scope, AZ::Script::Attributes::ScopeFlags::Automation)
                ->Attribute(AZ::Script::Attributes::Module, "azqtcomponents")
                ->Attribute(AZ::Script::Attributes::Category, "azqtcomponents");
        }
    }

} // namespace AzQtComponents

#include "Components/moc_ButtonDivider.cpp"
