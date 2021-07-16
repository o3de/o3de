/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Libraries/Operators/Operator.h>
#include <Include/ScriptCanvas/Libraries/Operators/Containers/OperatorPushBack.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //! Deprecated: see MethodOverloaded for "Add Element at End"
            class OperatorPushBack : public OperatorBase
            {
            public:

                SCRIPTCANVAS_NODE(OperatorPushBack);

                OperatorPushBack()
                    : OperatorBase(DefaultContainerManipulationOperatorConfiguration())
                {
                }

            protected:

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;
            };
        }
    }
}
