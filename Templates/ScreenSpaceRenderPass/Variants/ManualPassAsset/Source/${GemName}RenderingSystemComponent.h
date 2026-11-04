/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${GemName}RenderingSystemComponent.h    (variant: ManualPassAsset)
// -----------------------------------------------------------------------------
// Minimal gem-level system component. Registers pass class creators with
// Atom's PassSystem -- nothing more.
//
// WHY THIS VARIANT?
//   You picked "ManualPassAsset" at generation time. That mode ships the
//   pass class + the .pass / .shader / .azsl assets only; it does NOT modify
//   your render pipeline. After build:
//     1. Open your project's pipeline (typically <Project>/Passes/MainPipeline.pass).
//     2. Paste the PassRequest snippet from the _README_ block of
//        <gem>/Registry/${GemName}PostProcessRegistry.setreg into PassRequests.
//     3. Restart the editor.
//   This is the most flexible mode -- you control exactly where the pass
//   sits in the pipeline graph -- but it is the most manual. Switch to
//   PostVolume or ScreenSpaceConstant on a future generation if you'd rather
//   have the wiring done for you.
//
// IDEMPOTENCY:
//   The wizard's add_pass_creator_call command is idempotent. Re-running the
//   ScreenSpaceRenderPass template with a NEW ${Name} appends a second
//   AddPassCreator/RemovePassCreator pair to this same file. Re-running with
//   the same ${Name} is a no-op.
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

        //! Atom pass registration happens here. The wizard's add_pass_creator_call
        //! command injects PassSystemInterface::AddPassCreator(...) lines into
        //! Activate() for every pass produced by the ScreenSpaceRenderPass template.
        void Activate() override;

        //! Mirror block of RemovePassCreator(...) calls is injected here.
        void Deactivate() override;
    };

} // namespace ${GemName}
