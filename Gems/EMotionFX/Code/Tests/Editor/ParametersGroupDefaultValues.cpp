/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>

#include <string>
#include <type_traits>

#include <QTreeWidget>

#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Parameter/RotationParameter.h>
#include <EMotionFX/Source/Parameter/StringParameter.h>
#include <EMotionFX/Source/Parameter/Vector2Parameter.h>
#include <EMotionFX/Source/Parameter/Vector3Parameter.h>
#include <EMotionFX/Source/Parameter/Vector4Parameter.h>
#include <EMotionFX/Tools/EMotionStudio/EMStudioSDK/Source/EMStudioManager.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/AnimGraphPlugin.h>
#include <EMotionFX/Tools/EMotionStudio/Plugins/StandardPlugins/Source/AnimGraph/ParameterWindow.h>
#include <MCore/Source/AttributeQuaternion.h>
#include <MCore/Source/AttributeString.h>
#include <MCore/Source/AttributeVector2.h>
#include <MCore/Source/AttributeVector3.h>
#include <MCore/Source/AttributeVector4.h>

#include <Tests/UI/CommandRunnerFixture.h>
#include <Tests/Matchers.h>
#include <Tests/TestAssetCode/SimpleActors.h>
#include <Tests/TestAssetCode/ActorFactory.h>

namespace EMotionFX
{
    namespace
    {
        template <class ValueType>
        ValueType GetExpectedValue();

        template <>
        AZ::Quaternion GetExpectedValue<AZ::Quaternion>()
        {
            return AZ::Quaternion::CreateRotationX(0.5f);
        }
        template <>
        AZStd::string GetExpectedValue<AZStd::string>()
        {
            return "New default value for a string";
        }
        template <>
        AZ::Vector4 GetExpectedValue<AZ::Vector4>()
        {
            return AZ::Vector4(2.0f, 3.0f, 4.0f, 5.0f);
        }
        template <>
        AZ::Vector3 GetExpectedValue<AZ::Vector3>()
        {
            return AZ::Vector3(2.0f, 3.0f, 4.0f);
        }
        template <>
        AZ::Vector2 GetExpectedValue<AZ::Vector2>()
        {
            return AZ::Vector2(2.0f, 3.0f);
        }

        template <class ValueType>
        void TestEquality(const ValueType& lhs, const ValueType& rhs)
        {
            EXPECT_THAT(lhs, IsClose(rhs));
        }
        template <>
        void TestEquality<AZStd::string>(const AZStd::string& lhs, const AZStd::string& rhs)
        {
            EXPECT_EQ(lhs, rhs);
        }
        template <class ValueType>
        void TestInequality(const ValueType& lhs, const ValueType& rhs)
        {
            EXPECT_THAT(lhs, ::testing::Not(IsClose(rhs)));
        }
        template <>
        void TestInequality<AZStd::string>(const AZStd::string& lhs, const AZStd::string& rhs)
        {
            EXPECT_NE(lhs, rhs);
        }
    } // namespace

    template <typename T>
    class CanSetParameterToDefaultValueWhenInGroupFixture
        : public CommandRunnerFixture
    {
    public:
        using TestParameterT = T;
    };

    template <class ParameterType, class AttributeType>
    class TestParameterT
    {
    public:
        using ParameterT = ParameterType;
        using AttributeT = AttributeType;
        // Use the return type from the attribute's GetValue method as the type
        // that this attribute contains
        using ValueT = AZStd::remove_const_t<AZStd::remove_reference_t<decltype(AZStd::declval<AttributeType>().GetValue())>>;
    };

    using RotationParameterT = TestParameterT<RotationParameter, MCore::AttributeQuaternion>;
    using StringParameterT = TestParameterT<StringParameter, MCore::AttributeString>;
    using Vector2ParameterT = TestParameterT<Vector2Parameter, MCore::AttributeVector2>;
    using Vector3ParameterT = TestParameterT<Vector3Parameter, MCore::AttributeVector3>;
    using Vector4ParameterT = TestParameterT<Vector4Parameter, MCore::AttributeVector4>;

    using TypesToTest = ::testing::Types<
        RotationParameterT,
        StringParameterT,
        Vector2ParameterT,
        Vector3ParameterT,
        Vector4ParameterT
    >;

    TYPED_TEST_CASE(CanSetParameterToDefaultValueWhenInGroupFixture, TypesToTest);

    TYPED_TEST(CanSetParameterToDefaultValueWhenInGroupFixture, CanSetParameterToDefaultValueWhenInGroup)
    {
        using ParameterT = typename TestFixture::TestParameterT::ParameterT;
        using AttributeT = typename TestFixture::TestParameterT::AttributeT;
        using ValueT = typename TestFixture::TestParameterT::ValueT;

        AZStd::unique_ptr<Actor> actor = ActorFactory::CreateAndInit<SimpleJointChainActor>(1);

        ActorInstance* actorInstance = ActorInstance::Create(actor.get());

        this->ExecuteCommands({
            R"(Select -actorInstanceID )" + std::to_string(actorInstance->GetID()),
            R"(CreateMotionSet -name CanSetParameterToDefaultValueWhenInGroupMotionSet -motionSetID 200)",
            R"(CreateAnimGraph -animGraphID 100)",
            R"(AnimGraphAddGroupParameter -animGraphID 100 -name GroupParam)",
            R"(AnimGraphCreateParameter -animGraphID 100 -parent GroupParam -name Param -type )" + std::string(azrtti_typeid<ParameterT>().ToFixedString().c_str()),
            R"(ActivateAnimGraph -animGraphID 100 -motionSetID 200 -actorInstanceID )" + std::to_string(actorInstance->GetID()),
        });

        AnimGraph* animGraph = GetEMotionFX().GetAnimGraphManager()->FindAnimGraphByID(100);

        const ValueParameter* valueParameter = animGraph->FindValueParameter(0);
        const ParameterT* defaultValueParameter = azdynamic_cast<const ParameterT*>(valueParameter);
        ASSERT_TRUE(defaultValueParameter) << "Found parameter does not inherit from DefaultValueParameter";

        const ValueT expectedValue = GetExpectedValue<ValueT>();
        TestInequality(defaultValueParameter->GetDefaultValue(), expectedValue);

        AnimGraphInstance* animGraphInstance = animGraph->GetAnimGraphInstance(0);
        auto instanceValue = static_cast<AttributeT*>(animGraphInstance->GetParameterValue(static_cast<uint32>(animGraph->FindValueParameterIndex(valueParameter).GetValue())));
        ASSERT_EQ(instanceValue->GetType(), AttributeT::TYPE_ID);

        // Set the parameter's current value
        instanceValue->SetValue(expectedValue);

        auto animGraphPlugin = static_cast<EMStudio::AnimGraphPlugin*>(EMStudio::GetPluginManager()->FindActivePlugin(EMStudio::AnimGraphPlugin::CLASS_ID));
        ASSERT_TRUE(animGraphPlugin) << "Expected to find the AnimGraph plugin. Is it loaded?";

        QTreeWidget* treeWidget = animGraphPlugin->GetParameterWindow()->findChild<QTreeWidget*>("AnimGraphParamWindow");
        ASSERT_TRUE(treeWidget) << "Expected to find the QTreeWidget inside the AnimGraph plugin's parameter window";
        const QTreeWidgetItem* groupParameterItem = treeWidget->topLevelItem(0);
        QTreeWidgetItem* valueParameterItem = groupParameterItem->child(0);
        valueParameterItem->setSelected(true);
        // Make the current value of the parameter from the current animgraph the parameter's default value
        animGraphPlugin->GetParameterWindow()->OnMakeDefaultValue();

        TestEquality(defaultValueParameter->GetDefaultValue(), expectedValue);

        this->ExecuteCommands({
            R"(RemoveAnimGraph -animGraphId )" + std::to_string(animGraph->GetID())
        });
        actorInstance->Destroy();
    }
} // namespace EMotionFX
