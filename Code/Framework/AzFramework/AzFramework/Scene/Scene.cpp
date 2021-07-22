/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Scene/Scene.h>

namespace AzFramework
{
    Scene::Scene(AZStd::string name)
        : m_name(AZStd::move(name))
    {
    }

    Scene::Scene(AZStd::string name, AZStd::shared_ptr<Scene> parent)
        : m_name(AZStd::move(name))
        , m_parent(AZStd::move(parent))
    {
    }

    Scene::~Scene()
    {
        m_removalEvent.Signal(*this, RemovalEventType::Destroyed);
    }

    const AZStd::string& Scene::GetName() const
    {
        return m_name;
    }

    const AZStd::shared_ptr<Scene>& Scene::GetParent()
    {
        return m_parent;
    }

    AZStd::shared_ptr<const Scene> Scene::GetParent() const
    {
        return m_parent;
    }

    bool Scene::IsAlive() const
    {
        return m_isAlive;
    }

    void Scene::ConnectToEvents(RemovalEvent::Handler& handler)
    {
        handler.Connect(m_removalEvent);
    }

    void Scene::ConnectToEvents(SubsystemEvent::Handler& handler)
    {
        handler.Connect(m_subsystemEvent);
    }

    AZStd::any* Scene::FindSubsystem(const AZ::TypeId& typeId)
    {
        AZStd::any* result = FindSubsystemInScene(typeId);
        return (!result && m_parent) ? m_parent->FindSubsystem(typeId) : result;
    }

    const AZStd::any* Scene::FindSubsystem(const AZ::TypeId& typeId) const
    {
        return const_cast<Scene*>(this)->FindSubsystem(typeId);
    }

    AZStd::any* Scene::FindSubsystemInScene(const AZ::TypeId& typeId)
    {
        // Spot check that the internal arrays remain consistent.
        AZ_Assert(
            m_systemKeys.size() == m_systemObjects.size(), "Key and object list in AzFramework::Scene '%s' have gone out of sync.",
            m_name.c_str());

        const size_t m_systemKeysCount = m_systemKeys.size();
        for (size_t i = 0; i < m_systemKeysCount; ++i)
        {
            if (m_systemKeys[i] != typeId)
            {
                continue;
            }
            else
            {
                return &m_systemObjects[i];
            }
        }
        return nullptr;
    }

    const AZStd::any* Scene::FindSubsystemInScene(const AZ::TypeId& typeId) const
    {
        return const_cast<Scene*>(this)->FindSubsystemInScene(typeId);
    }

    void Scene::MarkForDestruction()
    {
        m_isAlive = false;
        m_removalEvent.Signal(*this, RemovalEventType::Zombified);
    }

    void Scene::RemoveSubsystem(size_t index, const AZ::TypeId& subsystemType)
    {
        m_systemKeys[index] = m_systemKeys.back();
        m_systemObjects[index] = AZStd::move(m_systemObjects.back());
        m_systemKeys.pop_back();
        m_systemObjects.pop_back();
        m_subsystemEvent.Signal(*this, SubsystemEventType::Removed, subsystemType);
    }
}
