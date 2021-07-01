/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once


#include <Libraries/Core/BinaryOperator.h>

#include <Include/ScriptCanvas/Libraries/Math/Random.generated.h>


namespace ScriptCanvas
{
    namespace Nodes
    {
        namespace Math
        {
            //! Provides a random value within the specified range, uses std::uniform_real_distribution and std::mt19937
            class Random
                : public Node
            {
            public:

                SCRIPTCANVAS_NODE(Random);

                void OnInputSignal(const SlotId& slotId);

            };
        }
    }
}

