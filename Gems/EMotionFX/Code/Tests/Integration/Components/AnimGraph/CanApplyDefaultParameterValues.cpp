/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Script/ScriptProperty.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzFramework/Components/TransformComponent.h>

#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeFloat.h>
#include <MCore/Source/AttributeBool.h>
#include <EMotionFX/Source/Parameter/BoolParameter.h>
#include <EMotionFX/Source/Parameter/FloatSliderParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/MotionSet.h>
#include <Integration/Assets/MotionSetAsset.h>
#include <Integration/Components/ActorComponent.h>
#include <Integration/Components/AnimGraphComponent.h>
#include <Tests/SystemComponentFixture.h>
#include <Tests/TestAssetCode/ActorAssetFactory.h>
#include <Tests/TestAssetCode/ActorFactory.h>
#include <Tests/TestAssetCode/AnimGraphAssetFactory.h>
#include <Tests/TestAssetCode/AnimGraphFactory.h>
#include <Tests/TestAssetCode/MotionSetAssetFactory.h>
#include <Tests/TestAssetCode/SimpleActors.h>

namespace EMotionFX
{
    class AnimGraphComponentDefaultParameterValuesFixture
        : public SystemComponentFixture
    {
        // The AnimGraphComponent supports setting default values for parameters in the AnimGraph, overriding the default value specified in
        // the AnimGraph itself.
        // This template correlates value types with how they are stored in the different places. The underlying parameter uses a typed
        // MCore::Attribute, the Component uses a typed ScriptProperty, and the parameter itself is also typed. This correlates all the
        // types together.
        template <typename ValueType>
        struct ParameterTypeTraits;

    public:
        void SetUp() override
        {
            SystemComponentFixture::SetUp();

            m_app.RegisterComponentDescriptor(Integration::ActorComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(Integration::AnimGraphComponent::CreateDescriptor());
            m_app.RegisterComponentDescriptor(AzFramework::TransformComponent::CreateDescriptor());
        }

        template<typename T, typename Matcher>
        void CanApplyDefaultParameterValues(T initialDefaultValue, T customDefaultValue, const Matcher& matcherFactory)
        {
            using ParameterType = typename ParameterTypeTraits<T>::ParameterType;
            using ScriptPropertyType = typename ParameterTypeTraits<T>::ScriptPropertyType;
            using AttributeType = typename ParameterTypeTraits<T>::AttributeType;

            const char* parameterName{"Parameter"};

            auto parameter = aznew ParameterType(parameterName);
            parameter->SetDefaultValue(initialDefaultValue);

            auto animGraphAsset = AnimGraphAssetFactory::Create(AZ::Data::AssetId("{B4EE9F32-84F7-4F89-B643-A2B9905242ED}"), AnimGraphFactory::Create<EmptyAnimGraph>());
            animGraphAsset->GetAnimGraph()->AddParameter(parameter);

            auto motionSetAsset = MotionSetAssetFactory::Create(AZ::Data::AssetId("{D4CB9179-2388-473D-9B04-D88BC7B9B990}"), aznew MotionSet());

            auto actorAsset = ActorAssetFactory::Create(AZ::Data::AssetId("{A0E136B5-636F-4E10-9D09-0BF40A774760}"), ActorFactory::CreateAndInit<SimpleJointChainActor>(1));

            auto entity = AZStd::make_unique<AZ::Entity>(AZ::EntityId(8934213));
            entity->CreateComponent<AzFramework::TransformComponent>();

            Integration::AnimGraphComponent::Configuration animGraphConfig{animGraphAsset, motionSetAsset, {}, false, {{aznew ScriptPropertyType(parameterName, customDefaultValue)}}};
            auto animGraphComponent = entity->CreateComponent<EMotionFX::Integration::AnimGraphComponent>(&animGraphConfig);

            Integration::ActorComponent::Configuration actorConfig{actorAsset};
            auto actorComponent = entity->CreateComponent<EMotionFX::Integration::ActorComponent>(&actorConfig);

            entity->Init();
            entity->Activate();
            actorComponent->SetActorAsset(actorAsset);

            const auto matchesInitialValue = AZStd::invoke(matcherFactory, initialDefaultValue);
            const auto matchesCustomValue = AZStd::invoke(matcherFactory, customDefaultValue);

            EXPECT_THAT(parameter->GetDefaultValue(), matchesInitialValue);
            EXPECT_THAT(static_cast<AttributeType*>(animGraphComponent->GetAnimGraphInstance()->FindParameter(parameterName))->GetValue(), matchesCustomValue);
        }
    };

    template<>
    struct AnimGraphComponentDefaultParameterValuesFixture::ParameterTypeTraits<float>
    {
        using ParameterType = FloatSliderParameter;
        using ScriptPropertyType = AZ::ScriptPropertyNumber;
        using AttributeType = MCore::AttributeFloat;
    };

    template<>
    struct AnimGraphComponentDefaultParameterValuesFixture::ParameterTypeTraits<const char*>
    {
        using ParameterType = StringParameter;
        using ScriptPropertyType = AZ::ScriptPropertyString;
        using AttributeType = MCore::AttributeString;
    };

    template<>
    struct AnimGraphComponentDefaultParameterValuesFixture::ParameterTypeTraits<bool>
    {
        using ParameterType = BoolParameter;
        using ScriptPropertyType = AZ::ScriptPropertyBoolean;
        using AttributeType = MCore::AttributeBool;
    };

    // To support changing the test's comparison function based on its type, a factory function to create a matcher from a value is passed
    // to the test function. These functions are overloaded, so a static_cast is necessary to choose the right one. And their return types
    // use gtest-internal types, so decltype is used to get the function's return type
    #define MatcherFnAddress(matcher, exampleValue, argType) static_cast<decltype(matcher(exampleValue)) (*)(argType)>(matcher)
    TEST_F(AnimGraphComponentDefaultParameterValuesFixture, CanApplyDefaultParameterValuesFloat)
    {
        CanApplyDefaultParameterValues<float>(5.0f, 10.0f, MatcherFnAddress(testing::FloatEq, 0.0f, float));
    }

    TEST_F(AnimGraphComponentDefaultParameterValuesFixture, CanApplyDefaultParameterValuesBool)
    {
        CanApplyDefaultParameterValues<bool>(false, true, MatcherFnAddress(testing::Eq, true, bool));
    }

    TEST_F(AnimGraphComponentDefaultParameterValuesFixture, CanApplyDefaultParameterValuesString)
    {
        CanApplyDefaultParameterValues<const char*>("defaultString", "customString", MatcherFnAddress(StrEq, "", const AZStd::string&));
    }
} // namespace EMotionFX
