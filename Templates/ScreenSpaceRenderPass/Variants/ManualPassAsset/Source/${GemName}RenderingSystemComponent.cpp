/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${GemName}RenderingSystemComponent.cpp
// -----------------------------------------------------------------------------
// Atom pass registration entry point for this gem.
//
// WHAT THE WIZARD INJECTS:
//   * #include "${Name}Pass.h" lines, one per generated pass.
//   * One AddPassCreator(...) line in Activate() per generated pass.
//   * One matching RemovePassCreator(...) line in Deactivate() per generated pass.
//
// MANUAL EDITS YOU CAN MAKE SAFELY:
//   * Add LoadPassTemplateMappings() if you want this gem to ship its own
//     PassTemplates.azasset (see commented block in Activate() below). Most
//     small effects do NOT need this -- the .pass file is enough as long as
//     the project's render pipeline references it directly. The PassTemplates
//     mechanism only matters when you want to alias templates by name and
//     reuse them across multiple pipelines.
//   * Reflect() additional pass-data types if you subclass FullscreenTrianglePassData.
//
// EDIT WITH CAUTION:
//   The wizard re-injects code by string-matching the surrounding shape of
//   Activate() / Deactivate(). If you wrap them in conditionals, lambdas, or
//   helper methods, future runs of the wizard will fail to find the right
//   insertion point and will log a warning instead of corrupting the file.
// =============================================================================

#include "${GemName}RenderingSystemComponent.h"

#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>

namespace ${GemName}
{
    void ${GemName}RenderingSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto* serialize = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serialize->Class<${GemName}RenderingSystemComponent, AZ::Component>()
                ->Version(1)
                ;

            if (AZ::EditContext* ec = serialize->GetEditContext())
            {
                ec->Class<${GemName}RenderingSystemComponent>(
                    "${GemName} Rendering System",
                    "Registers ${GemName}'s custom Atom passes (post-process, etc.) with the renderer.")
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
        // RPISystem is required so PassSystemInterface is available when we
        // register pass creators in Activate().
        required.push_back(AZ_CRC_CE("RPISystem"));
    }

    void ${GemName}RenderingSystemComponent::Activate()
    {
        // -----------------------------------------------------------------
        // The wizard injects pass-creator registrations into this method.
        // Generated lines look like:
        //
        //   AZ::RPI::PassSystemInterface::Get()->AddPassCreator(
        //       AZ::Name("MyPass"), &MyPass::Create);
        //
        // Do not move them out of Activate() -- the wizard's
        // add_pass_creator_call command relies on this method's shape to
        // re-enter on subsequent template runs.
        // -----------------------------------------------------------------

        // Optional: load a gem-scoped PassTemplates azasset so .pass templates
        // can be referenced by name across pipelines. Uncomment if you have
        // generated Assets/Passes/${GemName}PassTemplates.azasset and want it
        // active everywhere.
        //
        //   if (auto* passSystem = AZ::RPI::PassSystemInterface::Get())
        //   {
        //       passSystem->LoadPassTemplateMappings(
        //           "Passes/${GemName}PassTemplates.azasset");
        //   }
    }

    void ${GemName}RenderingSystemComponent::Deactivate()
    {
        // -----------------------------------------------------------------
        // The wizard injects matching RemovePassCreator(...) lines here.
        // -----------------------------------------------------------------
    }

} // namespace ${GemName}
