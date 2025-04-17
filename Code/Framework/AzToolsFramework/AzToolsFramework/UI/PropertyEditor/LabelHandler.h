/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#if !defined(Q_MOC_RUN)
#include <AzCore/Memory/SystemAllocator.h>

#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>

#include <QLabel>
#endif

namespace AzToolsFramework
{
    //! Handler for showing a generic QLabel
    class LabelHandler
        : public GenericPropertyHandler<QLabel>
    {
    public:
        AZ_CLASS_ALLOCATOR(LabelHandler, AZ::SystemAllocator);

        QWidget* CreateGUI(QWidget* parent) override;
        bool ResetGUIToDefaults(QLabel* GUI) override;
        AZ::u32 GetHandlerName() const override;

        void ConsumeAttribute(QLabel* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
    };

    void RegisterLabelHandler();
}
