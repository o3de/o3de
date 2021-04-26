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
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/Factory.h>

namespace AZ
{
    namespace RHI
    {
        ScopeProducer::ScopeProducer()
        {
            m_scope = Factory::Get().CreateScope();
        }

        ScopeProducer::ScopeProducer(const ScopeId& scopeId)
            : m_scopeId{scopeId}
        {
            m_scope = Factory::Get().CreateScope();
            m_scope->Init(scopeId);
        }

        const ScopeId& ScopeProducer::GetScopeId() const
        {
            return m_scopeId;
        }

        Scope* ScopeProducer::GetScope()
        {
            return m_scope.get();
        }

        const Scope* ScopeProducer::GetScope() const
        {
            return m_scope.get();
        }

        void ScopeProducer::SetScopeId(const ScopeId& scopeId)
        {
            m_scopeId = scopeId;
            
            if (m_scope->IsInitialized())
            {
                m_scope->Shutdown();
            }
            
            m_scope->Init(scopeId);
        }
    }
}
