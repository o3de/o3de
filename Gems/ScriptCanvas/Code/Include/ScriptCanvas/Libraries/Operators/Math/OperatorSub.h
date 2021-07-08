/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "OperatorArithmetic.h"
#include <Include/ScriptCanvas/Libraries/Operators/Math/OperatorSub.generated.h>

namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Operators
        {
            //! Node that provides subtraction
            class OperatorSub : public OperatorArithmetic
            {
            public:

                SCRIPTCANVAS_NODE(OperatorSub);

                OperatorSub() = default;

                AZStd::unordered_set< Data::Type > GetSupportedNativeDataTypes() const override;

                void Operator(Data::eType type, const ArithmeticOperands& operands, Datum& result) override;
            };
        }
    }
}
