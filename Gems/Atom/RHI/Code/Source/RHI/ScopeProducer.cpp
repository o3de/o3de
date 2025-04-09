/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Atom/RHI/ScopeProducer.h>
#include <Atom/RHI/Scope.h>
#include <Atom/RHI/Factory.h>

namespace AZ::RHI
{
    ScopeProducer::ScopeProducer()
    {
        m_scope = Factory::Get().CreateScope();
    }

    ScopeProducer::ScopeProducer(const ScopeId& scopeId, int deviceIndex)
        : m_scopeId{ scopeId }
        , m_deviceIndex{ deviceIndex }
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

    int ScopeProducer::GetDeviceIndex() const
    {
        return m_deviceIndex;
    }

    void ScopeProducer::SetHardwareQueueClass(HardwareQueueClass hardwareQueueClass)
    {
        m_scope->SetHardwareQueueClass(hardwareQueueClass);
    }

    // DEPRECATED: use InitScope instead
    void ScopeProducer::SetScopeId(const ScopeId& scopeId)
    {
        InitScope(scopeId);
    }

    void ScopeProducer::InitScope(const ScopeId& scopeId, HardwareQueueClass hardwareQueueClass, int deviceIndex)
    {
        m_scopeId = scopeId;
        m_deviceIndex = deviceIndex;
            
        if (m_scope->IsInitialized())
        {
            m_scope->Shutdown();
        }
            
        m_scope->Init(scopeId, hardwareQueueClass);
    }
}
