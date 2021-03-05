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

#include <AzCore/std/hash.h>

#include <ScriptCanvas/Core/Endpoint.h>

#include <ScriptCanvas/Core/Core.h>
#include <ScriptCanvas/Variable/VariableCore.h>

namespace ScriptCanvas
{
    template<class T>
    class GraphScopedIdentifier
    {
    public:
        AZ_RTTI((GraphScopedIdentifier, "{1B01849B-57BA-4926-8DF6-252966E2BF8F}", T));
        
        GraphScopedIdentifier() = default;
        GraphScopedIdentifier(const ScriptCanvasId& scriptCanvasId, const T& identifier)
            : m_scriptCanvasId(scriptCanvasId)
            , m_identifier(identifier)
        {

        }

        GraphScopedIdentifier(const GraphScopedIdentifier& other)
            : m_scriptCanvasId(other.m_scriptCanvasId)
            , m_identifier(other.m_identifier)
        {

        }

        virtual ~GraphScopedIdentifier() = default;

        bool operator==(const GraphScopedIdentifier& other) const
        {
            return m_scriptCanvasId == other.m_scriptCanvasId && m_identifier == other.m_identifier;
        }
        
        void Clear()
        {
            m_scriptCanvasId.SetInvalid();
            m_identifier = T();
        }
        
        bool IsValid()
        {
            return m_scriptCanvasId.IsValid() && m_identifier.IsValid();
        }        
        
        ScriptCanvasId m_scriptCanvasId;
        T           m_identifier;
    };
    
    typedef GraphScopedIdentifier<VariableId> GraphScopedVariableId;
    typedef GraphScopedIdentifier<AZ::EntityId> GraphScopedNodeId;
    typedef GraphScopedIdentifier<Endpoint> GraphScopedEndpoint;
}

namespace AZStd
{
    template<class T>
    struct hash<ScriptCanvas::GraphScopedIdentifier<T>>
    {
        size_t operator()(const ScriptCanvas::GraphScopedIdentifier<T>& key)
        {
            size_t seed = 0;
            AZStd::hash_combine(seed, AZStd::hash<T>{}(key.m_identifier), AZStd::hash<ScriptCanvas::ScriptCanvasId>{}(key.m_scriptCanvasId));

            return seed;
        }
    };
}