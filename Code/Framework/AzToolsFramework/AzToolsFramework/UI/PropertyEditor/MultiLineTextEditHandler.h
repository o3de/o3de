#pragma once

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

#if !defined(Q_MOC_RUN)
#include <AzCore/std/string/string.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzToolsFramework/UI/PropertyEditor/PropertyEditorAPI.h>
#include "GrowTextEdit.h"
#endif

/* 
=============================================================
= Handler Documentation                                     =
=============================================================

Handler Name: "MultiLineEdit"

Available Attributes:
    o PlaceholderText - Sets the placeholder text. Placeholder text is text that shows up 
                        when the text box is empty. Placeholder text acts as a prompt for
                        the user (similar to a tooltip) that helps the user understand
                        what the text box is for, and what they are supposed to put in it.

*/

namespace AzToolsFramework
{
    class MultiLineTextEditHandler
        : public QObject, public AzToolsFramework::PropertyHandler<AZStd::string, GrowTextEdit>
    {
        Q_OBJECT
    public:
        AZ_CLASS_ALLOCATOR(MultiLineTextEditHandler, AZ::SystemAllocator, 0);

        QWidget* CreateGUI(QWidget* parent) override;
        AZ::u32 GetHandlerName() const override;
        bool AutoDelete() const;

        void ConsumeAttribute(GrowTextEdit* widget, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName) override;
        void WriteGUIValuesIntoProperty(size_t index, GrowTextEdit* GUI, property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
        bool ReadValuesIntoGUI(size_t index, GrowTextEdit* GUI, const property_t& instance, AzToolsFramework::InstanceDataNode* node) override;
    };

    void RegisterMultiLineEditHandler();
}
