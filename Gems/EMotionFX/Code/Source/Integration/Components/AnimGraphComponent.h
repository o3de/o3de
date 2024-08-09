/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>

#include <Integration/Assets/AnimGraphAsset.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/ActorComponentBus.h>
#include <Integration/AnimGraphComponentBus.h>
#include <Integration/AnimGraphNetworkingBus.h>

namespace EMotionFX
{
    namespace Integration
    {
        class AnimGraphComponent
            : public AZ::Component
            , private AZ::Data::AssetBus::MultiHandler
            , private ActorComponentNotificationBus::Handler
            , private AnimGraphComponentRequestBus::Handler
            , private AnimGraphComponentNotificationBus::Handler
            , private AnimGraphComponentNetworkRequestBus::Handler
        {
        public:

            friend class EditorAnimGraphComponent;

            AZ_COMPONENT(AnimGraphComponent, "{77624349-D5C4-4902-9F08-665814520999}");

            /**
             * Structure containing data-driven properties extracted from the anim graph,
             * to allow override control per-entity via the component inspector UI.
             */
            struct ParameterDefaults
            {
                AZ_TYPE_INFO(ParameterDefaults, "{E6826EB9-C79B-43F3-A03F-3298DD3C724E}")

                ~ParameterDefaults();
                ParameterDefaults& operator=(const ParameterDefaults& rhs)
                {
                    if (this == &rhs)
                    {
                        return *this;
                    }

                    Reset();

                    m_parameters.reserve(rhs.m_parameters.size());

                    for (AZ::ScriptProperty* p : rhs.m_parameters)
                    {
                        if (p)
                        {
                            m_parameters.push_back(p->Clone());
                        }
                    }

                    return *this;
                }

                using ParameterList = AZStd::vector<AZ::ScriptProperty*>;
                ParameterList m_parameters;

                void Reset();
                static void Reflect(AZ::ReflectContext* context);
            };

            /**
            * Configuration struct for procedural configuration of Actor Components.
            */
            struct Configuration
            {
                AZ_TYPE_INFO(Configuration, "{F5A93340-60CD-4A16-BEF3-1014D762B217}")

                AZ::Data::Asset<AnimGraphAsset>     m_animGraphAsset;           ///< Selected anim graph.
                AZ::Data::Asset<MotionSetAsset>     m_motionSetAsset;           ///< Selected motion set asset.
                AZStd::string                       m_activeMotionSetName;      ///< Selected motion set.
                bool                                m_visualize = false;        ///< Debug visualization.
                ParameterDefaults                   m_parameterDefaults;        ///< Defaults for parameter values.

                static void Reflect(AZ::ReflectContext* context);
            };

            AnimGraphComponent(const Configuration* config = nullptr);
            ~AnimGraphComponent() override;
            
            //////////////////////////////////////////////////////////////////////////
            // AZ::Component interface implementation
            void Init() override;
            void Activate() override;
            void Deactivate() override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // AnimGraphComponentRequestBus::Handler
            EMotionFX::AnimGraphInstance* GetAnimGraphInstance() override;
            size_t FindParameterIndex(const char* parameterName) override;
            const char* FindParameterName(size_t parameterIndex) override;
            void SetParameterFloat(size_t parameterIndex, float value) override;
            void SetParameterBool(size_t parameterIndex, bool value) override;
            void SetParameterString(size_t parameterIndex, const char* value) override;
            void SetParameterVector2(size_t parameterIndex, const AZ::Vector2& value) override;
            void SetParameterVector3(size_t parameterIndex, const AZ::Vector3& value) override;
            void SetParameterRotationEuler(size_t parameterIndex, const AZ::Vector3& value) override;
            void SetParameterRotation(size_t parameterIndex, const AZ::Quaternion& value) override;
            void SetNamedParameterFloat(const char* parameterName, float value) override;
            void SetNamedParameterBool(const char* parameterName, bool value) override;
            void SetNamedParameterString(const char* parameterName, const char* value) override;
            void SetNamedParameterVector2(const char* parameterName, const AZ::Vector2& value) override;
            void SetNamedParameterVector3(const char* parameterName, const AZ::Vector3& value) override;
            void SetNamedParameterRotationEuler(const char* parameterName, const AZ::Vector3& value) override;
            void SetNamedParameterRotation(const char* parameterName, const AZ::Quaternion& value) override;
            void SetVisualizeEnabled(bool enabled) override;
            float GetParameterFloat(size_t parameterIndex) override;
            bool GetParameterBool(size_t parameterIndex) override;
            AZStd::string GetParameterString(size_t parameterIndex) override;
            AZ::Vector2 GetParameterVector2(size_t parameterIndex) override;
            AZ::Vector3 GetParameterVector3(size_t parameterIndex) override;
            AZ::Vector3 GetParameterRotationEuler(size_t parameterIndex) override;
            AZ::Quaternion GetParameterRotation(size_t parameterIndex) override;
            float GetNamedParameterFloat(const char* parameterName) override;
            bool GetNamedParameterBool(const char* parameterName) override;
            AZStd::string GetNamedParameterString(const char* parameterName) override;
            AZ::Vector2 GetNamedParameterVector2(const char* parameterName) override;
            AZ::Vector3 GetNamedParameterVector3(const char* parameterName) override;
            AZ::Vector3 GetNamedParameterRotationEuler(const char* parameterName) override;
            AZ::Quaternion GetNamedParameterRotation(const char* parameterName) override;
            bool GetVisualizeEnabled() override;
            void SyncAnimGraph(AZ::EntityId leaderEntityId) override;
            void DesyncAnimGraph(AZ::EntityId leaderEntityId) override;
            void SetActiveMotionSet(const char* activeMotionSetName) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            // ActorComponentNotificationBus::Handler
            void OnActorInstanceCreated(EMotionFX::ActorInstance* /*actorInstance*/) override;
            void OnActorInstanceDestroyed(EMotionFX::ActorInstance* /*actorInstance*/) override;
            //////////////////////////////////////////////////////////////////////////

            // AnimGraphComponentNetworkRequestBus
            bool IsAssetReady() const override;
            bool HasSnapshot() const override;
            void CreateSnapshot(bool isAuthoritative) override;
            void SetActiveStates(const NodeIndexContainer& activeStates) override;
            static NodeIndexContainer s_emptyNodeIndexContainer;
            const NodeIndexContainer& GetActiveStates() const override;
            static MotionNodePlaytimeContainer s_emptyMotionNodePlaytimeContainer;
            void SetMotionPlaytimes(const MotionNodePlaytimeContainer& motionNodePlaytimes) override;
            const MotionNodePlaytimeContainer& GetMotionPlaytimes() const override;
            void UpdateActorExternal(float deltatime) override;
            void SetNetworkRandomSeed(AZ::u64 seed) override;
            AZ::u64 GetNetworkRandomSeed() const override;
            void SetActorThreadIndex(AZ::u32 threadIndex) override;
            AZ::u32 GetActorThreadIndex() const override;
            //////////////////////////////////////////////////////////////////////////
            // AnimGraphComponentNotificationBus::Handler
            void OnAnimGraphSynced(EMotionFX::AnimGraphInstance* /*animGraphInstance(Follower)*/) override;
            void OnAnimGraphDesynced(EMotionFX::AnimGraphInstance* /*animGraphInstance(Follower)*/) override;
            //////////////////////////////////////////////////////////////////////////

            //////////////////////////////////////////////////////////////////////////
            static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
            {
                provided.push_back(AZ_CRC_CE("EMotionFXAnimGraphService"));
            }

            static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
            {
                incompatible.push_back(AZ_CRC_CE("EMotionFXAnimGraphService"));
                incompatible.push_back(AZ_CRC_CE("EMotionFXSimpleMotionService"));
            }

            static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent)
            {
                dependent.push_back(AZ_CRC_CE("MeshService"));
            }

            static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
            {
                required.push_back(AZ_CRC_CE("TransformService"));
                required.push_back(AZ_CRC_CE("EMotionFXActorService"));
            }

            static void Reflect(AZ::ReflectContext* context);
            //////////////////////////////////////////////////////////////////////////

            // AZ::Data::AssetBus::Handler
            void OnAssetReady(AZ::Data::Asset<AZ::Data::AssetData> asset) override;
            void OnAssetReloaded(AZ::Data::Asset<AZ::Data::AssetData> asset) override;

            void SetAnimGraphAssetId(const AZ::Data::AssetId& assetId);
            void SetMotionSetAssetId(const AZ::Data::AssetId& assetId);

        private:
            void CheckCreateAnimGraphInstance();
            void DestroyAnimGraphInstance();

            // Helper functions to wrap special logic required for EMFX anim graph ref-counting.
            void AnimGraphInstancePostCreate();
            void AnimGraphInstancePreDestroy();

            Configuration                               m_configuration;        ///< Component configuration.

            EMotionFXPtr<EMotionFX::ActorInstance>      m_actorInstance;        ///< Associated actor instance (retrieved from Actor Component).
            EMotionFXPtr<EMotionFX::AnimGraphInstance>  m_animGraphInstance;    ///< Live anim graph instance.
        };

    } // namespace Integration
} // namespace EMotionFX

