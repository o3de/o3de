/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

// =============================================================================
// ${Name}SettingsProviderInterface.h    (variant: PostVolume)
// -----------------------------------------------------------------------------
// AZ::Interface used by ${Name}Pass to read the currently active
// ${Name}Settings. The first ${Name}SettingsComponent in the scene to call
// AZ::Interface<>::Register wins. Pass queries the interface in
// FrameBeginInternal; if no provider is registered, the pass treats the
// effect as disabled (intensity 0).
// =============================================================================

#pragma once

#include "${Name}Settings.h"

#include <AzCore/Interface/Interface.h>

namespace ${GemName}
{
    class ${SanitizedCppName}SettingsProviderInterface
    {
    public:
        AZ_RTTI(${GemName}::${SanitizedCppName}SettingsProviderInterface, "{${Random_Uuid}}");

        virtual ~${SanitizedCppName}SettingsProviderInterface() = default;

        //! Returns the active settings. Caller does not retain ownership;
        //! pointer / reference validity is one frame.
        virtual const ${SanitizedCppName}Settings& GetSettings() const = 0;
    };

} // namespace ${GemName}
