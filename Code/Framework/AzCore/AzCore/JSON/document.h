/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/JSON/rapidjson.h>

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#endif

// Make sure rapidjson/include folder is available. Currently 3rdParty\rapidjson\rapidjson-1.1.0\include

#include <rapidjson/document.h>

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif

// Check if a Value is a valid object, has the specified key with correct type.
inline bool IsValidMember(const rapidjson::Value& val, const char* key, bool (rapidjson::Value::*func)() const)
{
    return val.IsObject() && val.HasMember(key) && (val[key].*func)();
}
#define RAPIDJSON_IS_VALID_MEMBER(node, key, isTypeFuncPtr) (IsValidMember(node, key, &rapidjson::Value::isTypeFuncPtr))
