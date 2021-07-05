/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Break.h"

#include <ScriptCanvas/Execution/ErrorBus.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            AZ::Outcome<DependencyReport, void> Break::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }
        }
    }
}
