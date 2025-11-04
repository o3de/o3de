/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include "UIVectorPropertyHandlerBase.h"

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/base.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include <LyShine/Bus/UiTransform2dBus.h>
#include <LyShine/Bus/UiLayoutBus.h>
#include <LyShine/UiBase.h>

class PropertyHandlerLayoutPadding : public UIVectorPropertyHandlerBase<UiLayoutInterface::Padding>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerLayoutPadding, AZ::SystemAllocator);

    PropertyHandlerLayoutPadding()
        : UIVectorPropertyHandlerBase(4, 2)
    {
    }

    AZ::u32 GetHandlerName(void) const override
    {
        return AZ::Edit::UIHandlers::LayoutPadding;
    }

    QWidget* CreateGUI(QWidget* pParent) override;

    static void Register();
};
