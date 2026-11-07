/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/algorithm.h>

#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/Skeleton.h>
#include <EMotionFX/Source/SocketSetup.h>

namespace EMotionFX
{
    AZ_CLASS_ALLOCATOR_IMPL(ActorSocket, EMotionFX::ActorAllocator)
    AZ_CLASS_ALLOCATOR_IMPL(SocketSetup, EMotionFX::ActorAllocator)

    ActorSocket::ActorSocket(AZStd::string name, AZStd::string jointName, const AZ::Vector3& position, const AZ::Quaternion& rotation)
        : m_name(AZStd::move(name))
        , m_jointName(AZStd::move(jointName))
        , m_position(position)
        , m_rotation(rotation)
    {
    }

    AZ::Transform ActorSocket::GetLocalTransform() const
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(m_rotation, m_position);
    }

    void ActorSocket::Reflect(AZ::ReflectContext* context)
    {
        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<ActorSocket>()
            ->Version(1)
            ->Field("name", &ActorSocket::m_name)
            ->Field("jointName", &ActorSocket::m_jointName)
            ->Field("position", &ActorSocket::m_position)
            ->Field("rotation", &ActorSocket::m_rotation);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<ActorSocket>("Socket", "A named joint-relative frame; local +Y is forward")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->DataElement(AZ::Edit::UIHandlers::Default, &ActorSocket::m_name, "Name", "Unique socket name")
                ->DataElement(AZ::Edit::UIHandlers::Default, &ActorSocket::m_jointName, "Joint", "Parent joint")
                ->DataElement(AZ::Edit::UIHandlers::Default, &ActorSocket::m_position, "Position", "Offset from the joint")
                ->DataElement(AZ::Edit::UIHandlers::Default, &ActorSocket::m_rotation, "Rotation", "Orientation relative to the joint");
        }
    }

    size_t SocketSetup::FindSocketIndex(const AZStd::string& name) const
    {
        const auto it = AZStd::find_if(m_sockets.begin(), m_sockets.end(), [&name](const ActorSocket& socket)
        {
            return socket.GetName() == name;
        });
        return it != m_sockets.end() ? static_cast<size_t>(it - m_sockets.begin()) : InvalidIndex;
    }

    const ActorSocket* SocketSetup::FindSocket(const AZStd::string& name) const
    {
        const size_t index = FindSocketIndex(name);
        return index != InvalidIndex ? &m_sockets[index] : nullptr;
    }

    ActorSocket* SocketSetup::AddSocket(const AZStd::string& name, const AZStd::string& jointName, const AZ::Vector3& position, const AZ::Quaternion& rotation)
    {
        if (name.empty() || FindSocketIndex(name) != InvalidIndex)
        {
            return nullptr;
        }

        m_sockets.emplace_back(name, jointName, position, rotation);
        return &m_sockets.back();
    }

    ActorSocket* SocketSetup::InsertSocketAt(size_t index, const ActorSocket& socket)
    {
        if (socket.GetName().empty() || FindSocketIndex(socket.GetName()) != InvalidIndex)
        {
            return nullptr;
        }

        index = AZStd::min(index, m_sockets.size());
        return &*m_sockets.insert(m_sockets.begin() + index, socket);
    }

    bool SocketSetup::RemoveSocket(const AZStd::string& name)
    {
        const size_t index = FindSocketIndex(name);
        if (index == InvalidIndex)
        {
            return false;
        }

        m_sockets.erase(m_sockets.begin() + index);
        return true;
    }

    bool SocketSetup::RenameSocket(const AZStd::string& oldName, const AZStd::string& newName)
    {
        const size_t index = FindSocketIndex(oldName);
        if (index == InvalidIndex || newName.empty())
        {
            return false;
        }

        const size_t existing = FindSocketIndex(newName);
        if (existing != InvalidIndex && existing != index)
        {
            return false;
        }

        m_sockets[index].SetName(newName);
        return true;
    }

    bool SocketSetup::ResolveJointIndices(const Skeleton& skeleton)
    {
        bool allResolved = true;
        for (ActorSocket& socket : m_sockets)
        {
            const Node* joint = skeleton.FindNodeByNameNoCase(socket.GetJointName().c_str());
            socket.SetJointIndex(joint ? joint->GetNodeIndex() : InvalidIndex);
            allResolved = allResolved && joint;
        }
        return allResolved;
    }

    void SocketSetup::Reflect(AZ::ReflectContext* context)
    {
        ActorSocket::Reflect(context);

        AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
        if (!serializeContext)
        {
            return;
        }

        serializeContext->Class<SocketSetup>()
            ->Version(1)
            ->Field("sockets", &SocketSetup::m_sockets);

        AZ::EditContext* editContext = serializeContext->GetEditContext();
        if (editContext)
        {
            editContext->Class<SocketSetup>("SocketSetup", "Actor socket setup")
                ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                ->Attribute(AZ::Edit::Attributes::AutoExpand, "")
                ->Attribute(AZ::Edit::Attributes::Visibility, AZ::Edit::PropertyVisibility::ShowChildrenOnly);
        }
    }
} // namespace EMotionFX
