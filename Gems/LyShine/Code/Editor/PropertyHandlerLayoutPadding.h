/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
*/
#pragma once

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

#include <LyShine/UiBase.h>
#include <LyShine/Bus/UiTransform2dBus.h>

#include "UIVectorPropertyHandlerBase.h"

class PropertyHandlerLayoutPadding
    : public UIVectorPropertyHandlerBase<UiLayoutInterface::Padding>
{
public:
    AZ_CLASS_ALLOCATOR(PropertyHandlerLayoutPadding, AZ::SystemAllocator, 0);

    PropertyHandlerLayoutPadding()
        : UIVectorPropertyHandlerBase(4, 2)
    {
    }

    AZ::u32 GetHandlerName(void) const override  { return AZ::Edit::UIHandlers::LayoutPadding; }

    QWidget* CreateGUI(QWidget* pParent) override;

    static void Register();
};
