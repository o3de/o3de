/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "APIArguments.h"

namespace ScriptCanvas
{
    namespace Debugger
    {
        void ReflectArguments(AZ::ReflectContext* context)
        {
            if (AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
            {
                serializeContext->Class<ScriptTarget>()
                    ->Version(1)
                    ->Field("logExecution", &ScriptTarget::m_logExecution)
                    ->Field("entities", &ScriptTarget::m_entities)
                    ->Field("static_entities", &ScriptTarget::m_staticEntities)
                    ->Field("graphs", &ScriptTarget::m_graphs)
                    ;
                
                serializeContext->Class<Target>()
                    ->Version(0)
                    ->Field("info", &Target::m_info)
                    ->Field("script", &Target::m_script)
                    ;
            }
        }
        
        bool ScriptTarget::operator==(const ScriptTarget& other) const
        {
            return m_logExecution == other.m_logExecution
                && m_entities == other.m_entities
                && m_graphs == other.m_graphs;
        }

        Target::Target(const AzFramework::TargetInfo& info)
            : m_info(info)
        {}

        bool Target::operator==(const Target& other) const
        {
            return IsNetworkIdentityEqualTo(other)
                && m_script == other.m_script;
        }

        bool Target::IsNetworkIdentityEqualTo(const Target& other) const
        {
            return m_info.IsIdentityEqualTo(other.m_info);
        }

        void Target::CopyScript(const Target& other)
        {
            m_script = other.m_script;
        }

        Target::operator bool() const
        {
            return m_info.IsValid();
        }

    }
} 
