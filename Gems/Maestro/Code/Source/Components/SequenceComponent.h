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
#include <IMovieSystem.h>

#include <AzCore/Component/Component.h>
#include <Maestro/Bus/SequenceComponentBus.h>

namespace Maestro
{

    namespace AnimSerialize
    {
        //! Wrapper class for animation system data file. This allows us to use the old Cry
        //! serialize for the animation data
        class AnimationData
        {
        public:
            virtual ~AnimationData() { }
            AZ_CLASS_ALLOCATOR(AnimationData, AZ::SystemAllocator, 0);
            AZ_RTTI(AnimationData, "{1CC687A8-9331-4314-A0F9-C75C50C10268}");
            AZStd::string m_serializedData;
        };
    }

    class SequenceComponent
        : public AZ::Component
        , public Maestro::SequenceComponentRequestBus::Handler
    {
        friend class EditorSequenceComponent;

    public:
        AZ_COMPONENT(SequenceComponent, "{027CE988-CF48-4589-A73A-73CD8D02F783}");

        SequenceComponent();
        
        //////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        //////////////////////////////////////////////////////////////////////////

        //////////////////////////////////////////////////////////////////////////
        // SequenceComponentRequestBus::Handler Interface

        /**
        * Set a value for an animated property at the given address on the given entity.
        * @param animatedEntityId the entity Id of the entity containing the animatedAddress
        * @param animatedAddress identifies the component and property to be set
        * @param value the value to set - this must be instance of one of the concrete subclasses of AnimatedValue, corresponding to the type of the property referenced by the animatedAdresss
        * @return true if the value was changed.
        */
        bool SetAnimatedPropertyValue(const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress, const AnimatedValue& value) override;

        /**
        * Get the current value for a property
        * @param returnValue holds the value to get - this must be instance of one of the concrete subclasses of AnimatedValue, corresponding to the type of the property referenced by the animatedAdresss
        * @param animatedEntityId the entity Id of the entity containing the animatedAddress
        * @param animatedAddress identifies the component and property to be set
        */
        void GetAnimatedPropertyValue(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, const AnimatablePropertyAddress& animatableAddress) override;

        AZ::Uuid GetAnimatedAddressTypeId(const AZ::EntityId& animatedEntityId, const Maestro::SequenceComponentRequests::AnimatablePropertyAddress& animatableAddress) override;

        //! Track View will expect some components (those using AZ::Data::AssetBlends as a virtual property) to supply a GetAssetDuration event
        //! so Track View can query the duration of an asset (like a motion) without having any knowledge of that that asset is.
        void GetAssetDuration(AnimatedValue& returnValue, const AZ::EntityId& animatedEntityId, AZ::ComponentId componentId, const AZ::Data::AssetId& assetId)  override;

        /////////////////////////////////////////
        // Behaviors
        void Play() override;
        void PlayBetweenTimes(float startTime, float endTime) override;
        void Stop() override;
        void Pause() override;
        void Resume() override;
        void SetPlaySpeed(float newSpeed) override;
        void JumpToTime(float newTime) override;
        void JumpToEnd() override;
        void JumpToBeginning() override;
        float GetCurrentPlayTime() override;
        float GetPlaySpeed() override;
        //////////////////////////////////////////////////////////////////////////

    protected:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
        {
            provided.push_back(AZ_CRC("SequenceService", 0x7cbe5938));
        }

        // Required Reflect function.
        static void Reflect(AZ::ReflectContext* context);
    private:
        // pointer and id of the CryMovie anim sequence responsible for playback/recording
        AZStd::intrusive_ptr<IAnimSequence> m_sequence;

        // Reflects the entire CryMovie library
        static void ReflectCinematicsLib(AZ::SerializeContext* context);
    };

} // namespace Maestro
