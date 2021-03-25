/*
* All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
* its licensors.
*
* For complete copyright and license terms please see the LICENSE at the root of this
* distribution (the "License"). All use of this software is governed by the License,
* or, if provided, by the license below or the license accompanying this file. Do not
* remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
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
