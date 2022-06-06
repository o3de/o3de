/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas
{
#define BEGIN_METHOD_NODE_REPLACEMENT \
    static std::unordered_map<std::string, std::pair<std::string, std::string>> m_replacementMethods = {

#define END_METHOD_NODE_REPLACEMENT };

#define REPLACE_METHOD(OLD_METHOD_SIGNATURE, NEW_CLASS_NAME, NEW_METHOD_NAME) \
    { OLD_METHOD_SIGNATURE, { NEW_CLASS_NAME, NEW_METHOD_NAME } },

    // Use this as starting point for node replacement config lookup
    // Later we can replace it by autogen or disk file if necessary
    class ReplacementUtils
    {
    public:
        static NodeReplacementConfiguration GetReplacementMethodNode(const char* className, const char* methodName);
    };
} // namespace ScriptCanvas
