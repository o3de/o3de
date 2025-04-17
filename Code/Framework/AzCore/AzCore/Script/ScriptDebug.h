/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
        AZ_CLASS_ALLOCATOR(ScriptDebug, SystemAllocator);

        ScriptDebug() = default;
        ~ScriptDebug() = default;

        static void Reflect(AZ::ReflectContext* context);
    };
}


#endif // AZCORE_SCRIPTDEBUG_H_
