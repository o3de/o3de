/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "ReplacementUtils.h"
#include <string>
#include <unordered_map>
#include <utility>

namespace ScriptCanvas
{
    BEGIN_METHOD_NODE_REPLACEMENT
    REPLACE_METHOD("Entity Transform::Rotate", "", "ScriptCanvas_EntityFunctions_Rotate")
    REPLACE_METHOD("String::Is Valid Find Position", "", "ScriptCanvas_StringFunctions_IsValidFindPosition")
    REPLACE_METHOD("String::Contains String", "", "ScriptCanvas_StringFunctions_ContainsString")
    REPLACE_METHOD("String::Starts With", "", "ScriptCanvas_StringFunctions_StartsWith")
    REPLACE_METHOD("String::Ends With", "", "ScriptCanvas_StringFunctions_EndsWith")
    REPLACE_METHOD("String::Split", "", "ScriptCanvas_StringFunctions_Split")
    REPLACE_METHOD("String::Join", "", "ScriptCanvas_StringFunctions_Join")
    REPLACE_METHOD("String::Replace String", "", "ScriptCanvas_StringFunctions_ReplaceString")
    END_METHOD_NODE_REPLACEMENT;

    static constexpr const char MethodNodeUUID[] = "{E42861BD-1956-45AE-8DD7-CCFC1E3E5ACF}";

    NodeReplacementConfiguration ReplacementUtils::GetReplacementMethodNode(const char* className, const char* methodName)
    {
        NodeReplacementConfiguration configuration{};
        AZStd::string oldMethodName = AZStd::string::format("%s::%s", className, methodName);
        if (m_replacementMethods.find(oldMethodName.c_str()) != m_replacementMethods.end())
        {
            configuration.m_type = AZ::Uuid(MethodNodeUUID);
            configuration.m_className = m_replacementMethods[oldMethodName.c_str()].first.c_str();
            configuration.m_methodName = m_replacementMethods[oldMethodName.c_str()].second.c_str();
        }
        return configuration;
    }
} // namespace ScriptCanvas
