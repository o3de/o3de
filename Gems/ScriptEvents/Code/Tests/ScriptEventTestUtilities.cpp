/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ScriptEventTestUtilities.h"
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzTest/AzTest.h>

namespace ScriptEventsTests
{
    namespace Utilities
    {
        void Reflect(AZ::ReflectContext* context)
        {
            if (AZ::BehaviorContext* behaviorContext = azrtti_cast<AZ::BehaviorContext*>(context))
            {
                behaviorContext->Method("ScriptExpectTrue", &ScriptExpectTrue);
                behaviorContext->Method("ScriptTrace", &ScriptTrace);
            }
        }
        void ScriptExpectTrue(bool check, const char* msg)
        {
            (void)check; (void)msg;
            EXPECT_TRUE(check) << msg;
        }

        void ScriptTrace(const char* txt)
        {
            static bool showTraces = true;
            if (showTraces)
            {
                std::cerr << txt << std::endl;
            }
        }

    }


}
