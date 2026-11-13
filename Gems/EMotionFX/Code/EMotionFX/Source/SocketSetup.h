/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/RTTI/RTTI.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace EMotionFX
{
    class Skeleton;

    // A named, joint-relative frame; local +Y is the socket's forward direction.
    class ActorSocket
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(ActorSocket, "{0E333BBE-F292-417E-9068-3ABA63D819EA}");

        ActorSocket() = default;
        ActorSocket(AZStd::string name, AZStd::string jointName, const AZ::Vector3& position, const AZ::Quaternion& rotation);
        virtual ~ActorSocket() = default;

        const AZStd::string& GetName() const { return m_name; }
        void SetName(const AZStd::string& name) { m_name = name; }

        const AZStd::string& GetJointName() const { return m_jointName; }
        void SetJointName(const AZStd::string& jointName) { m_jointName = jointName; }

        // Skeleton node index of the parent joint; not serialized, filled in by SocketSetup::ResolveJointIndices().
        size_t GetJointIndex() const { return m_jointIndex; }
        void SetJointIndex(size_t jointIndex) { m_jointIndex = jointIndex; }

        const AZ::Vector3& GetLocalPosition() const { return m_position; }
        void SetLocalPosition(const AZ::Vector3& position) { m_position = position; }

        const AZ::Quaternion& GetLocalRotation() const { return m_rotation; }
        void SetLocalRotation(const AZ::Quaternion& rotation) { m_rotation = rotation; }

        // Offset from the parent joint's transform to the socket frame.
        AZ::Transform GetLocalTransform() const;

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::string m_name;
        AZStd::string m_jointName;
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Quaternion m_rotation = AZ::Quaternion::CreateIdentity();
        size_t m_jointIndex = static_cast<size_t>(-1);
    };

    // The sockets authored for an actor; saved in the source asset's manifest and exported into the .actor.
    class SocketSetup
    {
    public:
        AZ_CLASS_ALLOCATOR_DECL
        AZ_RTTI(SocketSetup, "{1AB393CF-BFCA-44C0-B691-4C49603D34A2}");

        SocketSetup() = default;
        virtual ~SocketSetup() = default;

        static constexpr size_t InvalidIndex = static_cast<size_t>(-1);

        size_t GetNumSockets() const { return m_sockets.size(); }
        const ActorSocket& GetSocket(size_t index) const { return m_sockets[index]; }
        ActorSocket& GetSocket(size_t index) { return m_sockets[index]; }
        const AZStd::vector<ActorSocket>& GetSockets() const { return m_sockets; }

        size_t FindSocketIndex(const AZStd::string& name) const;
        const ActorSocket* FindSocket(const AZStd::string& name) const;

        // Returns nullptr when the name is empty or already used.
        ActorSocket* AddSocket(const AZStd::string& name, const AZStd::string& jointName, const AZ::Vector3& position, const AZ::Quaternion& rotation);
        // Re-inserts a socket at an index, clamped to the end; fails if its name is empty or already used.
        ActorSocket* InsertSocketAt(size_t index, const ActorSocket& socket);
        bool RemoveSocket(const AZStd::string& name);
        void RemoveAllSockets() { m_sockets.clear(); }

        // Renames a socket; fails when the new name is empty or taken by a different socket.
        bool RenameSocket(const AZStd::string& oldName, const AZStd::string& newName);

        // Looks up every socket's parent joint (case insensitive); returns false if any joint is missing, leaving its index invalid.
        bool ResolveJointIndices(const Skeleton& skeleton);

        static void Reflect(AZ::ReflectContext* context);

    private:
        AZStd::vector<ActorSocket> m_sockets;
    };
} // namespace EMotionFX
