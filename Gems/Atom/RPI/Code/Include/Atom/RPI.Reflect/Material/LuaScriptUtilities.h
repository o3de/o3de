/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <Atom/RPI.Reflect/Configuration.h>
#include <AzCore/std/string/string.h>

namespace AZ
{
    class BehaviorContext;

    namespace RPI
    {
        class ATOM_RPI_REFLECT_API LuaScriptUtilities
        {
        public:
            static void Reflect(BehaviorContext* behaviorContext);
            static constexpr char const DebugName[] = "LuaScriptUtilities";

            static void Error(const AZStd::string& message);
            static void Warning(const AZStd::string& message);
            static void Print(const AZStd::string& message);
        };


    } // namespace Render

} // namespace AZ
