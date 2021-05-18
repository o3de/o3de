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

#include "AtomViewportDisplayIconsSystemComponent.h"

#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/EditContextConstants.inl>

namespace AZ::Render
{
    void AtomViewportDisplayIconsSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (AZ::SerializeContext* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<AtomViewportDisplayIconsSystemComponent, AZ::Component>()
                ->Version(0)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<AtomViewportDisplayIconsSystemComponent>("Viewport Display Icons", "Provides an interface for drawing simple icons to the Editor viewport")
                    ->ClassElement(Edit::ClassElements::EditorData, "")
                        ->Attribute(Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC("System", 0xc94d118b))
                        ->Attribute(Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void AtomViewportDisplayIconsSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC("ViewportDisplayIconsService"));
    }

    void AtomViewportDisplayIconsSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC("ViewportDisplayIconsService"));
    }

    void AtomViewportDisplayIconsSystemComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC("RPISystem", 0xf2add773));
    }

    void AtomViewportDisplayIconsSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    void AtomViewportDisplayIconsSystemComponent::Activate()
    {
        AzToolsFramework::EditorViewportIconDisplay::Register(this);
    }

    void AtomViewportDisplayIconsSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorViewportIconDisplay::Unregister(this);
    }

    void AtomViewportDisplayIconsSystemComponent::DrawIcon(const DrawParameters& drawParameters)
    {
        (void)drawParameters;
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconId AtomViewportDisplayIconsSystemComponent::GetOrLoadIconForPath(
        AZStd::string_view path)
    {
        (void)path;
        return IconId();
    }

    AzToolsFramework::EditorViewportIconDisplayInterface::IconLoadStatus AtomViewportDisplayIconsSystemComponent::GetIconLoadStatus(
        IconId icon)
    {
        (void)icon;
        return IconLoadStatus();
    }
} // namespace AZ::Render
