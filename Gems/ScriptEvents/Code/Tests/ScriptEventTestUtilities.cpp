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

#include "precompiled.h"
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
