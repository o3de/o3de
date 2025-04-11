/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzToolsFramework/UI/PropertyEditor/LabelHandler.h>
#include <QSignalBlocker>

namespace AzToolsFramework
{
    QWidget* LabelHandler::CreateGUI(QWidget* parent)
    {
        QLabel* label = new QLabel(parent);

        // By default let the text wrap so that the label can grow to fit any text specified
        label->setWordWrap(true);

        // Also enable link activation in case the user wants to embed links to content such as documentation
        label->setOpenExternalLinks(true);

        return label;
    }

    bool LabelHandler::ResetGUIToDefaults(QLabel* GUI)
    {
        GUI->setText(QString());
        return true;
    }

    AZ::u32 LabelHandler::GetHandlerName() const
    {
        return AZ::Edit::UIHandlers::Label;
    }

    void LabelHandler::ConsumeAttribute(QLabel* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, [[maybe_unused]] const char* debugName)
    {
        if (attrib == AZ::Edit::Attributes::ValueText)
        {
            AZStd::string valueText;
            if (attrValue->Read<AZStd::string>(valueText))
            {
                QSignalBlocker blocker(GUI);
                GUI->setText(valueText.c_str());
            }
            else
            {
                AZ_WarningOnce("AzToolsFramework", false,
                    "Failed to read 'ValueText' attribute from property '%s' into label field.", debugName);
            }
        }
    }

    void RegisterLabelHandler()
    {
        PropertyTypeRegistrationMessageBus::Broadcast(&PropertyTypeRegistrationMessages::RegisterPropertyType, aznew LabelHandler());
    }
}
