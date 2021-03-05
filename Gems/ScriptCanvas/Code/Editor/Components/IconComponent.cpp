
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

#include "precompiled.h"

#include "IconComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Component/ComponentApplicationBus.h>


namespace ScriptCanvasEditor
{
    AZStd::string IconComponent::LookupClassIcon(const AZ::Uuid& classId)
    {
        AZStd::string iconPath  = "Editor/Icons/ScriptCanvas/Placeholder.png";

        AZ::SerializeContext* serializeContext = nullptr;
        AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
        AZ_Assert(serializeContext, "Failed to acquire application serialize context.");
        const AZ::SerializeContext::ClassData* classData = serializeContext->FindClassData(classId);

        // Find the icon's path in the editor data attributes
        if (classData && classData->m_editData)
        {
            auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
            if (editorElementData)
            {
                if (auto iconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Icon))
                {
                    if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
                    {
                        AZStd::string iconAttributeValue = iconAttributeData->Get(nullptr);
                        if (!iconAttributeValue.empty())
                        {
                            iconPath = AZStd::move(iconAttributeValue);
                        }
                    }
                }
            }
        }

        return iconPath;
    }

    IconComponent::IconComponent(const AZ::Uuid& classId)
    {
        m_iconPath = LookupClassIcon(classId);
    }

    void IconComponent::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (serializeContext)
        {
            serializeContext->Class<IconComponent, AZ::Component>()
                ->Version(1)
                ->Field("m_iconPath", &IconComponent::m_iconPath)
                ;

            AZ::EditContext* editContext = serializeContext->GetEditContext();
            if (editContext)
            {
                editContext->Class<IconComponent>("Icon", "Represents a icon path")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "Icon Components class attributes")
                    ;
            }
        }
    }

    void IconComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("GraphCanvas_IconService", 0x2037454a));
    }

    void IconComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("GraphCanvas_IconService", 0x2037454a));
    }

    void IconComponent::GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        (void)dependent;
    }

    void IconComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        (void)required;
    }

    void IconComponent::Init()
    {
    }

    void IconComponent::Activate()
    {
        IconBus::Handler::BusConnect(GetEntityId());
    }

    void IconComponent::Deactivate()
    {
        IconBus::Handler::BusDisconnect();
    }

    AZStd::string IconComponent::GetIconPath() const
    {
        return m_iconPath;
    }

} // namespace ScriptCanvasEditor
