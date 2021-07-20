/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <Libraries/Logic/Once.h>
#include <ScriptCanvas/Utils/VersionConverters.h>

#include <AzCore/RTTI/BehaviorContext.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Logic
        {
            Once::Once()
                : Node()
            {}

            AZ::Outcome<DependencyReport, void> Once::GetDependencies() const
            {
                return AZ::Success(DependencyReport());
            }

            ExecutionNameMap Once::GetExecutionNameMap() const
            {
                return { {"In", "Out"}, {"Reset", "On Reset" } };
            }
        }
    }
}
