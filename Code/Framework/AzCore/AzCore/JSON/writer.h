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

#include <AzCore/JSON/rapidjson.h>

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#endif

// Make you have available rapidjson/include folder. Currently 3rdParty\rapidjson\rapidjson-1.1.0\include
#include <rapidjson/writer.h>

#if AZ_TRAIT_JSON_CLANG_IGNORE_UNKNOWN_WARNING && defined(AZ_COMPILER_CLANG)
#pragma clang diagnostic pop
#endif
