/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "${GemName}RenderingSystemComponent.h"

#include "${Name}Pass.h"
#include "${Name}SettingsComponent.h"

#include <Atom/RPI.Public/Pass/PassSystemInterface.h>

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    void ${GemName}RenderingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        // Reflect every settings component the gem ships. Required for the
        // component to appear in the Add Component menu and for its data to
        // serialize.
        ${SanitizedCppName}SettingsComponent::Reflect(context);

        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${GemName}RenderingSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${GemName}RenderingSystemComponent>(
                    "${GemName} Rendering System",
                    "Registers ${GemName}'s PostVolume post-process passes with Atom.")
                    ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                    ->Attribute(AZ::Edit::Attributes::AppearsInAddComponentMenu, AZ_CRC_CE("System"))
                    ->Attribute(AZ::Edit::Attributes::AutoExpand, true)
                    ;
            }
        }
    }

    void ${GemName}RenderingSystemComponent::GetProvidedServices(
        AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("${GemName}RenderingService"));
    }

    void ${GemName}RenderingSystemComponent::GetIncompatibleServices(
        AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("${GemName}RenderingService"));
    }

    void ${GemName}RenderingSystemComponent::GetRequiredServices(
        AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void ${GemName}RenderingSystemComponent::Activate()
    {
        // -----------------------------------------------------------------
        // The wizard injects AddPassCreator(...) lines here, one per pass:
        //
        //   AZ::RPI::PassSystemInterface::Get()->AddPassCreator(
        //       AZ::Name("${Name}Pass"), &${Name}Pass::Create);
        //
        // The settings component is NOT registered here -- it lives on level
        // entities and is added by the artist in the editor. Atom finds it
        // through the standard component activation pipeline.
        // -----------------------------------------------------------------
    }

    void ${GemName}RenderingSystemComponent::Deactivate()
    {
        // -----------------------------------------------------------------
        // Mirror RemovePassCreator(...) lines injected here.
        // -----------------------------------------------------------------
    }

} // namespace ${GemName}
