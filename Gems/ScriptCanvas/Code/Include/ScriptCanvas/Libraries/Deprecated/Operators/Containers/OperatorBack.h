/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "Operator.h"

#include <ScriptCanvas/Core/Node.h>
#include <Include/ScriptCanvas/Libraries/Deprecated/Operators/Containers/OperatorBack.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //! Deprecated: see MethodOverloaded for "Get Last Element"
            class OperatorBack : public OperatorBase
            {
            public:

                SCRIPTCANVAS_NODE(OperatorBack);


                OperatorBack()
                    : OperatorBase(DefaultContainerInquiryOperatorConfiguration())
                {
                }

            protected:

                void ConfigureContracts(SourceType sourceType, AZStd::vector<ContractDescriptor>& contractDescs) override;
                void OnSourceTypeChanged() override;
            };
        }
    }
}
