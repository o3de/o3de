/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#if !defined(AZ_MONOLITHIC_BUILD)

#include "UnitTestingLibrary.h"

#include <ScriptCanvas/Libraries/UnitTesting/Auxiliary/Auxiliary.h>
#include <ScriptCanvas/Libraries/UnitTesting/UnitTestBusSender.h>

namespace ScriptCanvas::UnitTestingLibrary
{
    void Reflect(AZ::ReflectContext* reflection)
    {
        ScriptCanvas::UnitTesting::EventSender::Reflect(reflection);
        ScriptCanvas::UnitTesting::Auxiliary::StringConversion::Reflect(reflection);
        ScriptCanvas::UnitTesting::Auxiliary::EBusTraits::Reflect(reflection);
        ScriptCanvas::UnitTesting::Auxiliary::TypeExposition::Reflect(reflection);
    }
} 

#endif
