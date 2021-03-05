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

#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/smart_ptr/intrusive_ptr.h>

namespace AZ
{
    class ReflectContext;
} // namespace AZ

namespace ScriptCanvas
{
    class BehaviorContextObject;
    using BehaviorContextObjectPtr = AZStd::intrusive_ptr<BehaviorContextObject>;
    void BehaviorContextObjectPtrReflect(AZ::ReflectContext* context);
} // namespace ScriptCanvas
