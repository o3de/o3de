/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ProjectSettingsTool_precompiled.h"
#include "PropertyLinked.h"

#include "PlatformSettings_common.h"
#include "ValidationHandler.h"

#include <QLayout>
#include <QPushButton>
#include <QLineEdit>

namespace ProjectSettingsTool
{
    PropertyLinkedCtrl::PropertyLinkedCtrl(QWidget* pParent)
        : PropertyFuncValLineEditCtrl(pParent)
        , m_linkButton(nullptr)
        , m_linkedProperty(nullptr)
        , m_linkEnabled(false)
    {
        connect(this, &PropertyFuncValLineEditCtrl::ValueChangedByUser, this, &PropertyLinkedCtrl::MirrorToLinkedProperty);
    }

    void PropertyLinkedCtrl::SetLinkedProperty(PropertyLinkedCtrl* property)
    {
        m_linkedProperty = property;
        m_linkEnabled = true;
    }

    void PropertyLinkedCtrl::SetLinkTooltip(const QString& tip)
    {
        if (m_linkButton != nullptr)
        {
            m_linkButton->setToolTip("Linked to " + tip);
        }
    }

    void PropertyLinkedCtrl::MakeLinkButton()
    {
        QLayout* myLayout = layout();
        QIcon icon;
        icon.addFile("://link.svg", QSize(), QIcon::Normal, QIcon::On);
        icon.addFile("://broken_link.svg", QSize(), QIcon::Normal, QIcon::Off);

        m_linkButton = new QPushButton(this);
        m_linkButton->setIcon(icon);
        m_linkButton->setCheckable(true);
        m_linkButton->setFlat(true);
        m_linkButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_linkButton->setFixedSize(QSize(16, 16));
        m_linkButton->setContentsMargins(0, 0, 0, 0);
        m_linkButton->setToolTip("Linked to...");
        myLayout->addWidget(m_linkButton);

        connect(m_linkButton, &QPushButton::clicked, this, &PropertyLinkedCtrl::MirrorLinkButtonState);
    }

    bool PropertyLinkedCtrl::LinkIsOptional()
    {
        return m_linkButton;
    }

    void PropertyLinkedCtrl::SetOptionalLink(bool linked)
    {
        m_linkButton->setChecked(linked);
    }

    void PropertyLinkedCtrl::MirrorToLinkedProperty()
    {
        if (m_linkEnabled && m_linkedProperty != nullptr)
        {
            m_linkedProperty->MirrorToLinkedPropertyRecursive(this, m_pLineEdit->text());
        }
    }

    void PropertyLinkedCtrl::MirrorToLinkedPropertyRecursive(PropertyLinkedCtrl* caller, const QString& value)
    {
        if (caller != this)
        {
            if (m_linkButton == nullptr || m_linkButton->isChecked())
            {
                // Stop Property from mirroring again
                m_linkEnabled = false;
                m_pLineEdit->setText(value);
                m_linkEnabled = true;
            }
            if (m_linkedProperty != nullptr)
            {
                m_linkedProperty->MirrorToLinkedPropertyRecursive(caller, value);
            }
        }
    }

    void PropertyLinkedCtrl::MirrorLinkButtonState(bool checked)
    {
        if (m_linkedProperty != nullptr)
        {
            m_linkedProperty->MirrorLinkButtonStateRecursive(this, checked);

            // Mirror value of property linked was enabled on to all linked fields
            if (checked)
            {
                MirrorToLinkedProperty();
            }
        }
    }

    void PropertyLinkedCtrl::MirrorLinkButtonStateRecursive(PropertyLinkedCtrl* caller, bool state)
    {
        if (caller != this)
        {
            if (m_linkButton != nullptr)
            {
                m_linkButton->setChecked(state);
            }
            if (m_linkedProperty != nullptr)
            {
                m_linkedProperty->MirrorLinkButtonStateRecursive(caller, state);
            }
        }
    }

    bool PropertyLinkedCtrl::AllLinkedPropertiesEqual()
    {
        if (m_linkedProperty != nullptr)
        {
            return m_linkedProperty->AllLinkedPropertiesEqual(this, m_pLineEdit->text());
        }
        // No linked property so must be equal
        else
        {
            return true;
        }
    }

    bool PropertyLinkedCtrl::AllLinkedPropertiesEqual(PropertyLinkedCtrl* caller, const QString& value)
    {
        if (caller != this)
        {
            // Not equal
            if (m_pLineEdit->text() != value)
            {
                return false;
            }
            if (m_linkedProperty != nullptr)
            {
                return m_linkedProperty->AllLinkedPropertiesEqual(caller, value);
            }
            // All checked properties were equal
            else
            {
                return true;
            }
        }
        // All properties were equal
        else
        {
            return true;
        }
    }

    void PropertyLinkedCtrl::SetLinkEnabled(bool enabled)
    {
        m_linkEnabled = enabled;
    }

    void PropertyLinkedCtrl::ConsumeAttribute(AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == Attributes::LinkOptional && m_linkButton == nullptr)
        {
            bool optional = false;
            if (attrValue->Read<bool>(optional) && optional)
            {
                MakeLinkButton();
            }
        }
        else
        {
            PropertyFuncValLineEditCtrl::ConsumeAttribute(attrib, attrValue, debugName);
        }
    }

    //  Handler  ///////////////////////////////////////////////////////////////////

    PropertyLinkedHandler::PropertyLinkedHandler(ValidationHandler* valHdlr)
        : AzToolsFramework::PropertyHandler<AZStd::string, PropertyLinkedCtrl>()
        , m_validationHandler(valHdlr)
    {}

    AZ::u32 PropertyLinkedHandler::GetHandlerName(void) const
    {
        return Handlers::LinkedLineEdit;
    }

    QWidget* PropertyLinkedHandler::CreateGUI(QWidget* pParent)
    {
        PropertyLinkedCtrl* ctrl = aznew PropertyLinkedCtrl(pParent);
        m_validationHandler->AddValidatorCtrl(ctrl);
        return ctrl;
    }

    void PropertyLinkedHandler::ConsumeAttribute(PropertyLinkedCtrl* GUI, AZ::u32 attrib, AzToolsFramework::PropertyAttributeReader* attrValue, const char* debugName)
    {
        if (attrib == Attributes::PropertyIdentfier)
        {
            AZStd::string ident;
            if (attrValue->Read<AZStd::string>(ident))
            {
                m_identToCtrl.insert(AZStd::pair<AZStd::string, PropertyLinkedCtrl*>(ident, GUI));

                auto result = m_ctrlToIdentAndLink.find(GUI);
                if (result != m_ctrlToIdentAndLink.end())
                {
                    result->second.identifier = ident;
                }
                else
                {
                    m_ctrlToIdentAndLink.insert(AZStd::pair<PropertyLinkedCtrl*, IdentAndLink>(GUI, IdentAndLink{ ident, "" }));
                    m_ctrlInitOrder.push_back(GUI);
                }
            }
        }
        else if (attrib == Attributes::LinkedProperty)
        {
            AZStd::string linked;
            if (attrValue->Read<AZStd::string>(linked))
            {
                auto result = m_ctrlToIdentAndLink.find(GUI);
                if (result != m_ctrlToIdentAndLink.end())
                {
                    result->second.linkedIdentifier = linked;
                }
                else
                {
                    m_ctrlToIdentAndLink.insert(AZStd::pair<PropertyLinkedCtrl*, IdentAndLink>(GUI, IdentAndLink{ "", linked }));
                    m_ctrlInitOrder.push_back(GUI);
                }

                GUI->SetLinkTooltip(linked.data());
            }
        }
        else
        {
            GUI->ConsumeAttribute(attrib, attrValue, debugName);
        }
    }

    void PropertyLinkedHandler::WriteGUIValuesIntoProperty([[maybe_unused]] size_t index, PropertyLinkedCtrl* GUI, property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        instance = GUI->GetValue().toUtf8().data();
    }

    bool PropertyLinkedHandler::ReadValuesIntoGUI([[maybe_unused]] size_t index, PropertyLinkedCtrl* GUI, const property_t& instance, [[maybe_unused]] AzToolsFramework::InstanceDataNode* node)
    {
        GUI->SetValue(instance.data());
        GUI->ForceValidate();
        return true;
    }

    PropertyLinkedHandler* PropertyLinkedHandler::Register(ValidationHandler* valHdlr)
    {
        PropertyLinkedHandler* handler = aznew PropertyLinkedHandler(valHdlr);
        AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Broadcast(
            &AzToolsFramework::PropertyTypeRegistrationMessages::Bus::Handler::RegisterPropertyType,
            handler);
        return handler;
    }

    void PropertyLinkedHandler::LinkAllProperties()
    {
        // Link all the properties
        for (PropertyLinkedCtrl* ctrlPointer : m_ctrlInitOrder)
        {
            auto property = m_ctrlToIdentAndLink.find(ctrlPointer);
            auto link = m_identToCtrl.find(property->second.linkedIdentifier);
            if (link != m_identToCtrl.end())
            {
                property->first->SetLinkedProperty(link->second);
                //Force mirror non-optional links.
                property->first->MirrorToLinkedProperty();
            }
            else
            {
                AZ_Assert(false, "Property \"%s\" not found while linking to \"%s\".", property->second.linkedIdentifier.data(), property->second.identifier.data());
            }
        }
        // Enable optional links if all properties in link chain are the same value
        EnableOptionalLinksIfAllPropertiesEqual();
    }

    void PropertyLinkedHandler::EnableOptionalLinksIfAllPropertiesEqual()
    {
        // Enable optional links if all properties in link chain are the same value
        for (const AZStd::pair<PropertyLinkedCtrl*, IdentAndLink>& property : m_ctrlToIdentAndLink)
        {
            if (property.first->LinkIsOptional())
            {
                property.first->SetOptionalLink(property.first->AllLinkedPropertiesEqual());
            }
        }
    }

    void PropertyLinkedHandler::MirrorAllLinkedProperties()
    {
        for (PropertyLinkedCtrl* ctrlPointer : m_ctrlInitOrder)
        {
            ctrlPointer->MirrorToLinkedProperty();
        }
    }

    void PropertyLinkedHandler::DisableAllPropertyLinks()
    {
        for (PropertyLinkedCtrl* ctrlPointer : m_ctrlInitOrder)
        {
            ctrlPointer->SetLinkEnabled(false);
        }
    }

    void PropertyLinkedHandler::EnableAllPropertyLinks()
    {
        for (PropertyLinkedCtrl* ctrlPointer : m_ctrlInitOrder)
        {
            ctrlPointer->SetLinkEnabled(true);
        }
    }
} // namespace ProjectSettingsTool

#include <moc_PropertyLinked.cpp>
