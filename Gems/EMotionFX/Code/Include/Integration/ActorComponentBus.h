/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/EBus/EBus.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Outcome/Outcome.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Physics/AnimationConfiguration.h>
#include <AzFramework/Physics/Character.h>
#include <Integration/Assets/ActorAsset.h>

namespace EMotionFX
{
    class ActorInstance;

    namespace Integration
    {
        /**
        * EMotion FX attachment type.
        */
        enum class AttachmentType : AZ::u32
        {
            None = 0,           ///< Do not attach to another actor.
            ActorAttachment,    ///< Attach to another actor as a separately animating attachment.
            SkinAttachment,     ///< Attach to another actor as a skinned attachment (using the same skeleton as the attachment target).
        };

        enum class Space : AZ::u32
        {
            LocalSpace,         ///< Relative to the parent.
            ModelSpace,         ///< Relative to the origin of the character.
            WorldSpace          ///< Relative to the world origin.
        };

        enum class SkinningMethod : AZ::u32
        {
            DualQuat = 0,       ///< Dual Quaternions will be used to blend joints during skinning.
            Linear,             ///< Matrices will be used to blend joints during skinning.
            None                ///< No skinning will be applied, the model will be rendered as-is. 
        };

        /**
        * EMotion FX Actor Component Request Bus
        * Used for making requests to EMotion FX Actor Components.
        */
        class ActorComponentRequests
            : public AZ::ComponentBus
        {
        public:
            using MutexType = AZStd::recursive_mutex;

            /// Retrieve component's actor instance.
            /// \return pointer to actor instance.
            virtual EMotionFX::ActorInstance* GetActorInstance() { return nullptr; }

            /// Retrieve the total number of joints.
            virtual size_t GetNumJoints() const { return 0; }

            /// Find the name index of a given joint by its name.
            /// \param name The name of the join to search for, case insensitive.
            /// \return The joint index, or s_invalidJointIndex if no found.
            virtual size_t GetJointIndexByName(const char* /*name*/) const  { return s_invalidJointIndex; }

            /// Retrieve the local transform (relative to the parent) of a given joint.
            /// \param jointIndex The joint index to get the transform from.
            /// \param Space the space to get the transform in.
            virtual AZ::Transform GetJointTransform(size_t /*jointIndex*/, Space /*space*/) const  { return AZ::Transform::CreateIdentity(); }
            virtual void GetJointTransformComponents(size_t /*jointIndex*/, Space /*space*/, AZ::Vector3& outPosition, AZ::Quaternion& outRotation, AZ::Vector3& outScale) const  { outPosition = AZ::Vector3::CreateZero(); outRotation = AZ::Quaternion::CreateIdentity(); outScale = AZ::Vector3::CreateOne(); }
            
            virtual Physics::AnimationConfiguration* GetPhysicsConfig() const { return nullptr; }

            /// Sockets are named joint-relative frames authored on the actor; a socket's forward direction is its local +Y axis.
            /// For the socket transform and forward, LocalSpace is the authored offset from the socket's joint,
            /// ModelSpace is relative to the origin of the character and WorldSpace is relative to the world origin.

            /// Retrieve the number of sockets on the actor.
            virtual size_t GetNumSockets() const { return 0; }

            /// Retrieve the name of a socket, or an empty string if the index is out of range.
            virtual AZStd::string GetSocketName(size_t /*socketIndex*/) const { return {}; }

            /// Find a socket index by its name, case sensitive.
            /// \return The socket index, or s_invalidSocketIndex if not found.
            virtual size_t GetSocketIndexByName(const char* /*name*/) const { return s_invalidSocketIndex; }

            /// Retrieve the transform of a socket in the given space; identity if the socket is invalid.
            virtual AZ::Transform GetSocketTransform(size_t /*socketIndex*/, Space /*space*/) const { return AZ::Transform::CreateIdentity(); }

            /// Retrieve the normalized forward direction (local +Y) of a socket in the given space; +Y if the socket is invalid.
            virtual AZ::Vector3 GetSocketForward(size_t /*socketIndex*/, Space /*space*/) const { return AZ::Vector3::CreateAxisY(); }

            /// Same as GetSocketTransform/GetSocketForward but looking the socket up by name on every call.
            virtual AZ::Transform GetSocketTransformByName(const char* /*name*/, Space /*space*/) const { return AZ::Transform::CreateIdentity(); }
            virtual AZ::Vector3 GetSocketForwardByName(const char* /*name*/, Space /*space*/) const { return AZ::Vector3::CreateAxisY(); }

            /// Socket transform in the bind pose, independent of animation so server and clients agree; WorldSpace uses the entity's current world transform.
            virtual AZ::Transform GetSocketBindTransform(size_t /*socketIndex*/, Space /*space*/) const { return AZ::Transform::CreateIdentity(); }
            virtual AZ::Transform GetSocketBindTransformByName(const char* /*name*/, Space /*space*/) const { return AZ::Transform::CreateIdentity(); }

            /// Animated model-space socket times the entity's current world transform; use it in network input processing, which runs before the animation update.
            virtual AZ::Transform GetSocketTransformFromEntity(size_t /*socketIndex*/) const { return AZ::Transform::CreateIdentity(); }
            virtual AZ::Transform GetSocketTransformFromEntityByName(const char* /*name*/) const { return AZ::Transform::CreateIdentity(); }

            /// Attach to the specified entity.
            /// \param targetEntityId - Id of the entity to attach to.
            /// \param attachmentType - Desired type of attachment.
            virtual void AttachToEntity(AZ::EntityId /*targetEntityId*/, AttachmentType /*attachmentType*/) {}

            /// Detach from parent entity, if attached.
            virtual void DetachFromEntity() {}

            /// Enables rendering of the actor.
            virtual bool GetRenderCharacter() const = 0;
            virtual void SetRenderCharacter(bool enabled) = 0;
            virtual bool GetRenderActorVisible() const = 0;

            /// Enables raytracing for the actor
            virtual void SetRayTracingEnabled(bool enabled) = 0;

            /// Returns skinning method used by the actor.
            virtual SkinningMethod GetSkinningMethod() const = 0;

            // Use this to alter the actor asset.
            virtual void SetActorAsset(AZ::Data::Asset<EMotionFX::Integration::ActorAsset> actorAsset) = 0;

            // Use this bus to enable or disable the actor instance update in the job scheduler system.
            // This could be useful if you want to manually update the actor instance.
            virtual void EnableInstanceUpdate(bool enableInstanceUpdate) = 0;

            static const size_t s_invalidJointIndex = std::numeric_limits<size_t>::max();
            static const size_t s_invalidSocketIndex = std::numeric_limits<size_t>::max();
        };

        using ActorComponentRequestBus = AZ::EBus<ActorComponentRequests>;

        /**
        * EMotion FX Actor Component Notification Bus
        * Used for monitoring events from actor components.
        */
        class ActorComponentNotifications
            : public AZ::ComponentBus
        {
        public:

            //////////////////////////////////////////////////////////////////////////
            /**
            * Custom connection policy notifies connecting listeners immediately if actor instance is already created.
            */
            template<class Bus>
            struct AssetConnectionPolicy
                : public AZ::EBusConnectionPolicy<Bus>
            {
                static void Connect(typename Bus::BusPtr& busPtr, typename Bus::Context& context, typename Bus::HandlerNode& handler, typename Bus::Context::ConnectLockGuard& connectLock, const typename Bus::BusIdType& id = 0)
                {
                    AZ::EBusConnectionPolicy<Bus>::Connect(busPtr, context, handler, connectLock, id);

                    EMotionFX::ActorInstance* instance = nullptr;
                    ActorComponentRequestBus::EventResult(instance, id, &ActorComponentRequestBus::Events::GetActorInstance);
                    if (instance)
                    {
                        handler->OnActorInstanceCreated(instance);
                    }
                }
            };
            template<typename Bus>
            using ConnectionPolicy = AssetConnectionPolicy<Bus>;
            //////////////////////////////////////////////////////////////////////////

            /// Notifies listeners when the component has created an actor instance.
            /// \param actorInstance - pointer to actor instance
            virtual void OnActorInstanceCreated(EMotionFX::ActorInstance* /*actorInstance*/) {};

            /// Notifies listeners when the component is destroying an actor instance.
            /// \param actorInstance - pointer to actor instance
            virtual void OnActorInstanceDestroyed(EMotionFX::ActorInstance* /*actorInstance*/) {};
        };

        using ActorComponentNotificationBus = AZ::EBus<ActorComponentNotifications>;

        /**
        * EMotion FX Editor Actor Component Request Bus
        * Used for making requests to EMotion FX Actor Components.
        */
        class EditorActorComponentRequests
            : public AZ::ComponentBus
        {
        public:
            virtual const AZ::Data::AssetId& GetActorAssetId() = 0;
            virtual AZ::EntityId GetAttachedToEntityId() const = 0;
        };

        using EditorActorComponentRequestBus = AZ::EBus<EditorActorComponentRequests>;
    } //namespace Integration
} // namespace EMotionFX


namespace AZ
{
    AZ_TYPE_INFO_SPECIALIZE(EMotionFX::Integration::Space, "{7606E4DD-B7CB-408B-BD0D-3A95636BB017}");
}
