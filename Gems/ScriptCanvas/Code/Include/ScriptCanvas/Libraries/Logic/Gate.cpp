/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Logic/Gate.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Gate::Gate()
                : Node()
            {}

            AZ::Outcome<DependencyReport, void> Gate::GetDependencies() const
            {
                return AZ::Success(DependencyReport{});
            }
        }
    }
}
