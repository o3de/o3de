/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Uuid.h>
#include <AzCore/std/typetraits/is_enum.h>

namespace AZ
{
    using TypeId = AZ::Uuid;
}

#define AZ_TYPE_INFO_INTERNAL_1(_ClassName) static_assert(false, "You must provide a ClassName,ClassUUID")
#define AZ_TYPE_INFO_INTERNAL_2(_ClassName, _ClassUuid)        \
    void TYPEINFO_Enable(){}                                   \
    static const char* TYPEINFO_Name() { return #_ClassName; } \
    static const AZ::TypeId& TYPEINFO_Uuid() { static AZ::TypeId s_uuid(_ClassUuid); return s_uuid; }

#define AZ_TYPE_INFO_1 AZ_TYPE_INFO_INTERNAL_1
#define AZ_TYPE_INFO_2 AZ_TYPE_INFO_INTERNAL_2

/**
* Use this macro inside a class to allow it to be identified across modules and serialized (in different contexts).
* The expected input is the class and the assigned uuid as a string or an instance of a uuid.
* Example:
*   class MyClass
*   {
*   public:
*       AZ_TYPE_INFO(MyClass, "{BD5B1568-D232-4EBF-93BD-69DB66E3773F}");
*       ...
*/
#define AZ_TYPE_INFO(...) AZ_MACRO_SPECIALIZE(AZ_TYPE_INFO_, AZ_VA_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

