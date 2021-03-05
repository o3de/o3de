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

#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Aabb.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Component/Entity.h>
#include <AzFramework/Physics/Casts.h>

namespace Physics
{
    class WorldBody;
    class World;
    struct RayCastRequest;
    struct RayCastHit;

    class WorldBodyConfiguration
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldBodyConfiguration, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldBodyConfiguration, "{6EEB377C-DC60-4E10-AF12-9626C0763B2D}");
        
        WorldBodyConfiguration() = default;
        WorldBodyConfiguration(const WorldBodyConfiguration& settings) = default;
        virtual ~WorldBodyConfiguration() = default;

        static void Reflect(AZ::ReflectContext* context);

        // Basic initial settings.
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Quaternion m_orientation = AZ::Quaternion::CreateIdentity();
        AZ::Vector3 m_scale = AZ::Vector3::CreateOne();

        // Entity/object association.
        AZ::EntityId m_entityId;
        void* m_customUserData = nullptr;

        // For debugging/tracking purposes only.
        AZStd::string m_debugName;
    };

    class WorldBody
    {
    public:
        AZ_CLASS_ALLOCATOR(WorldBody, AZ::SystemAllocator, 0);
        AZ_RTTI(WorldBody, "{4F1D9B44-FC21-4E93-83F0-41B6A78D9B4B}");

        friend class World;

    public:
        WorldBody() = default;
        WorldBody(const WorldBodyConfiguration& /*settings*/) {};
        virtual ~WorldBody() = default;

        virtual AZ::EntityId GetEntityId() const = 0;

        void SetUserData(void* userData);
        template<typename T>
        T* GetUserData() const;

        virtual Physics::World* GetWorld() const = 0;

        virtual AZ::Transform GetTransform() const = 0;
        virtual void SetTransform(const AZ::Transform& transform) = 0;

        virtual AZ::Vector3 GetPosition() const = 0;
        virtual AZ::Quaternion GetOrientation() const = 0;

        virtual AZ::Aabb GetAabb() const = 0;
        virtual Physics::RayCastHit RayCast(const RayCastRequest& request) = 0;

        virtual AZ::Crc32 GetNativeType() const = 0;
        virtual void* GetNativePointer() const = 0;

        virtual void AddToWorld(Physics::World&) = 0;
        virtual void RemoveFromWorld(Physics::World&) = 0;

    private:
        void* m_customUserData = nullptr;
    };

    template<typename T>
    T* WorldBody::GetUserData() const
    {
        return static_cast<T*>(m_customUserData);
    }
    
} // namespace Physics
