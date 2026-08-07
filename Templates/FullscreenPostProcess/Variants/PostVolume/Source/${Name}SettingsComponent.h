/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}SettingsComponent.h    (variant: PostVolume)
// -----------------------------------------------------------------------------
// Component that the artist drops on a level entity to expose the
// ${Name}Settings struct in the Inspector. The first active component in the
// scene becomes "the active provider" for ${Name}Pass.
//
// SINGLETON-BY-COMPONENT PATTERN:
//   On Activate(), the component registers itself via
//   ${Name}SettingsProviderInterface::Register. On Deactivate(), it
//   unregisters. ${Name}Pass::FrameBeginInternal queries the interface to get
//   the current settings each frame.
//
//   If you need true volume blending (priority + radius + falloff), graduate
//   to Atom's full PostProcessFeatureProcessor pattern; this component is the
//   minimum viable artist surface, not a replacement for that machinery.
//
// HOW TO TEST AFTER GENERATION:
//   1. Build, open the editor.
//   2. Create or open a level. Add an entity (or use the level entity).
//   3. Add Component -> "${Name} Settings" (under the Rendering category).
//   4. Tick "Enabled". The Pass should immediately become live.
//   5. To see something visible without writing C++ first, uncomment the PROBE
//      block in Assets/Shaders/PostProcessing/${Name}.azsl. The viewport
//      should turn red as soon as the entity activates.
// =============================================================================

#pragma once

#include "${Name}Settings.h"
#include "${Name}SettingsProviderInterface.h"

#include <AzCore/Component/Component.h>

namespace ${GemName}
{
    class ${SanitizedCppName}SettingsComponent
        : public AZ::Component
        , public ${SanitizedCppName}SettingsProviderInterface
    {
    public:
        AZ_COMPONENT(${GemName}::${SanitizedCppName}SettingsComponent,
                     "{${Random_Uuid}}");

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);

        ${SanitizedCppName}SettingsComponent() = default;
        ~${SanitizedCppName}SettingsComponent() override = default;

        // ${Name}SettingsProviderInterface
        const ${SanitizedCppName}Settings& GetSettings() const override { return m_settings; }

    protected:
        // AZ::Component
        void Activate() override;
        void Deactivate() override;

    private:
        ${SanitizedCppName}Settings m_settings;
    };

} // namespace ${GemName}
