/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */



#include <Source/Framework/ScriptCanvasTestFixture.h>
#include <Source/Framework/ScriptCanvasTestUtilities.h>
#include <Source/Framework/ScriptCanvasTestNodes.h>

#include <ScriptCanvas/Libraries/Core/Start.h>
#include <AzCore/Math/Uuid.h>
#include <AzCore/std/string/string.h>

using namespace ScriptCanvasTests;
using namespace ScriptCanvasEditor;
using namespace TestNodes;

void ReflectSignCorrectly()
{
    AZ::BehaviorContext* behaviorContext(nullptr);
    AZ::ComponentApplicationBus::BroadcastResult(behaviorContext, &AZ::ComponentApplicationRequests::GetBehaviorContext);
    AZ_Assert(behaviorContext, "A behavior context is required!");
    behaviorContext->Method("Sign", AZ::GetSign);
}
