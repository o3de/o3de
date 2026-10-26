/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${GemName}RenderingSystemComponent.h    (variant: PostVolume)
// -----------------------------------------------------------------------------
// Gem-level system component. Two responsibilities:
//   1. Register every ${Name}Pass class with PassSystemInterface.
//   2. Reflect every ${Name}SettingsComponent so it appears in the Add
//      Component menu under Rendering/PostProcess.
//
// You picked "PostVolume" at generation time. The wiring story:
//   * The pass class is registered by this component on Activate.
//   * The artist drops a ${Name}SettingsComponent on a level entity.
//   * The settings component registers itself as the active provider via
//     AZ::Interface<${Name}SettingsProviderInterface>.
//   * The pass class queries the interface each frame in FrameBeginInternal
//     to pull current values into its SRG.
//
// PIPELINE INSERTION:
//   PostVolume mode does NOT auto-inject into the pipeline. The component
//   exists, but the pass only runs once you've added a PassRequest to your
//   project's pipeline asset (same paste step as ManualPassAsset). Why? In
//   Atom's real PostProcess Volume system the pass is part of the pipeline
//   permanently, and the component just modulates intensity. We replicate
//   that contract here.
//
//   See <gem>/Registry/${GemName}PostProcessRegistry.setreg for the snippet.
// =============================================================================

#pragma once

#include <AzCore/Component/Component.h>

namespace ${GemName}
{
    class ${GemName}RenderingSystemComponent
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(${GemName}::${GemName}RenderingSystemComponent,
                     "{${Random_Uuid}}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        ${GemName}RenderingSystemComponent() = default;
        ~${GemName}RenderingSystemComponent() override = default;

    protected:
        // AZ::Component overrides...

        //! AddPassCreator(...) lines are injected here per generated pass by
        //! the wizard's add_pass_creator_call command.
        void Activate() override;

        //! Mirror block of RemovePassCreator(...) is injected here.
        void Deactivate() override;
    };

} // namespace ${GemName}
