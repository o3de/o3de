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
#ifndef AZCORE_SCRIPTDEBUG_H_
#define AZCORE_SCRIPTDEBUG_H_

#include <AzCore/base.h>
#include <AzCore/Memory/SystemAllocator.h>

namespace AZ
{
    class ReflectContext;

    /**
     * Reflects global Log, Warning, Error, and Assert functions to script under the "Debug." prefix.
     */
    class ScriptDebug
    {
    public:
        AZ_TYPE_INFO(ScriptDebug, "{FBAFA9F8-9861-41F9-A9EF-3B943B91FF4E}");
        AZ_CLASS_ALLOCATOR(ScriptDebug, SystemAllocator, 0);

        ScriptDebug() = default;
        ~ScriptDebug() = default;

        static void Reflect(AZ::ReflectContext* context);
    };
}


#endif // AZCORE_SCRIPTDEBUG_H_
