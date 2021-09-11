/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/EBus/EBus.h>

namespace ScriptCanvasEditor
{
    namespace VersionExplorer
    {
        class ViewRequestsTraits : public AZ::EBusTraits
        {
        public:
            // flush logs, or add log text
            // set progress update
            virtual void ClearProgress() {}
            virtual void SetInProgress() {}
        };
        using ViewRequestsBus = AZ::EBus<ViewRequestsTraits>;
    }
}
