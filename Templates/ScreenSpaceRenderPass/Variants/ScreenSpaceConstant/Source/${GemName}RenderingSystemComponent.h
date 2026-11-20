/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${GemName}RenderingSystemComponent.h    (variant: ScreenSpaceConstant)
// -----------------------------------------------------------------------------
// Gem-level system component. Two responsibilities:
//   1. Register every ${Name}Pass class with PassSystemInterface so the
//      .pass templates can instantiate it.
//   2. Register every ${Name}FeatureProcessor with FeatureProcessorFactory so
//      Atom calls AddRenderPasses on it for each pipeline at startup.
//
// You picked "ScreenSpaceConstant" at generation time. That mode means: the
// pass is auto-injected into every default pipeline at the PostProcessPass
// anchor, runs every frame, and is toggled at runtime by a CVar
// (r_${Name}Enabled). No level setup, no PostProcess Volume entity, no manual
// pipeline edits.
//
// SCOPE LIMITS:
//   * No per-area control. Every viewport sees the effect.
//   * No artist-tunable parameters by default -- everything is via CVars or
//     hardcoded constants. If you need sliders on entities, regenerate with
//     integration_mode = "PostVolume".
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

        //! Atom pass registration AND feature processor registration happens here.
        //! The wizard injects:
        //!   * AddPassCreator(...) lines via add_pass_creator_call
        //!   * RegisterFeatureProcessor<...> lines via add_feature_processor_registration
        void Activate() override;

        //! Mirror block of RemovePassCreator(...) + UnregisterFeatureProcessor lines.
        void Deactivate() override;
    };

} // namespace ${GemName}
