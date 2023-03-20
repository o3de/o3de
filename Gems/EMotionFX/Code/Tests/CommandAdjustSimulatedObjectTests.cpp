/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzTest/AzTest.h>
#include <sstream>

#include <AzCore/Memory/Memory.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/optional.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/string/conversions.h>
#include <AzCore/std/string/string.h>
#include <AzFramework/Physics/SystemBus.h>

#include <EMotionFX/CommandSystem/Source/CommandManager.h>
#include <EMotionFX/Source/Actor.h>
#include <EMotionFX/Source/ActorManager.h>
#include <EMotionFX/Source/Allocators.h>
#include <EMotionFX/Source/AnimGraph.h>
#include <EMotionFX/Source/AnimGraphManager.h>
#include <EMotionFX/Source/AnimGraphNode.h>
#include <EMotionFX/Source/AnimGraphObjectIds.h>
#include <EMotionFX/Source/EMotionFXManager.h>
#include <EMotionFX/Source/Node.h>
#include <EMotionFX/Source/SimulatedObjectSetup.h>
#include <EMotionFX/Source/SimulatedObjectBus.h>
#include <EMotionFX/Source/AnimGraphStateTransition.h>
#include <EMotionFX/Source/AnimGraphTransitionCondition.h>
#include <MCore/Source/Command.h>
#include <MCore/Source/ReflectionSerializer.h>
#include <Tests/Printers.h>
#include <Tests/Matchers.h>

namespace CommandAdjustSimulatedObjectTests
{
    namespace EMotionFX
    {
        // Import real types from the production namespace
        using ::EMotionFX::AnimGraphConnectionId;
        using ::EMotionFX::AnimGraphNodeId;
        using ::EMotionFX::AnimGraphObject;
        using ::EMotionFX::BlendTreeConnection;
        using ::EMotionFX::ValueParameter;
        using ::EMotionFX::CommandAllocator;
        using ::EMotionFX::AnimGraphAllocator;
        using ::EMotionFX::PhysicsSetup;
        using ::EMotionFX::SimulatedObjectNotificationBus;

        // Forward-declare types that will be mocked
        class Actor;
        class ActorManager;
        class AnimGraph;
        class AnimGraphInstance;
        class AnimGraphManager;
        class AnimGraphNode;
        class AnimGraphStateTransition;
        class EMotionFXManager;
        class GroupParameter;
        class Parameter;
        class SimulatedJoint;
        class SimulatedObject;
        class SimulatedObjectSetup;
    } // namespace EMotionFX

namespace EMotionFX
{
    typedef AZStd::vector<GroupParameter*> GroupParameterVector;
    typedef AZStd::vector<Parameter*> ParameterVector;
    typedef AZStd::vector<ValueParameter*> ValueParameterVector;
}
#include <Tests/Mocks/Parameter.h>
#include <Tests/Mocks/GroupParameter.h>
#include <Tests/Mocks/Actor.h>
#include <Tests/Mocks/ActorManager.h>
#include <Tests/Mocks/AnimGraphTransitionCondition.h>
#include <Tests/Mocks/AnimGraph.h>
#include <Tests/Mocks/AnimGraphInstance.h>
#include <Tests/Mocks/AnimGraphManager.h>
#include <Tests/Mocks/AnimGraphNode.h>
#include <Tests/Mocks/AnimGraphStateTransition.h>
#include <Tests/Mocks/EMotionFXManager.h>
#include <Tests/Mocks/SimulatedJoint.h>
#include <Tests/Mocks/SimulatedObject.h>
#include <Tests/Mocks/SimulatedObjectSetup.h>
#include <EMotionFX/CommandSystem/Source/ParameterMixins_Interface.inl>
#include <EMotionFX/CommandSystem/Source/ParameterMixins_Impl.inl>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands_Interface.inl>
#include <EMotionFX/CommandSystem/Source/SimulatedObjectCommands_Impl.inl>
} // namespace CommandAdjustSimulatedObjectTests

namespace EMotionFX
{
    namespace UnderTest
    {
        using namespace ::CommandAdjustSimulatedObjectTests::EMotionFX;
    } // namespace UnderTest

    ///////////////////////////////////////////////////////////////////////////
    // CommandAdjustSimulatedObject tests
    ///////////////////////////////////////////////////////////////////////////
    struct CommandAdjustSimulatedObjectTestsParam
    {
        AZStd::optional<std::string> objectName;
        AZStd::optional<float> gravityFactor;
        AZStd::optional<float> stiffnessFactor;
        AZStd::optional<float> dampingFactor;
        AZStd::optional<std::vector<std::string>> colliderTags;
        void (*setExecuteExpectations)(UnderTest::SimulatedObject*);
        void (*setUndoExpectations)(UnderTest::SimulatedObject*);
    };

    class CommandAdjustSimulatedObjectTestsFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<::testing::tuple<bool, bool, CommandAdjustSimulatedObjectTestsParam>>
    {
    public:
        static std::string buildCommandLineFromTestParam(const CommandAdjustSimulatedObjectTestsParam& param)
        {
            std::string string;
            std::stringstream stream(string);
            if (param.objectName.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedObject::s_objectNameParameterName << ' ' << param.objectName.value();
            }
            if (param.gravityFactor.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedObject::s_gravityFactorParameterName << ' ' << param.gravityFactor.value();
            }
            if (param.stiffnessFactor.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedObject::s_stiffnessFactorParameterName << ' ' << param.stiffnessFactor.value();
            }
            if (param.dampingFactor.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedObject::s_dampingFactorParameterName << ' ' << param.dampingFactor.value();
            }
            if (param.colliderTags.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedObject::s_colliderTagsParameterName << ' ';
                for (const std::string& val : *param.colliderTags)
                {
                    stream << val;
                    if (&val != std::addressof(*(param.colliderTags->end() - 1)))
                    {
                        stream << ";";
                    }
                }
            }
            return stream.str();
        }
    };

    TEST_P(CommandAdjustSimulatedObjectTestsFixture, TestExecute)
    {
        using ::testing::Return;
        using ::testing::ReturnRef;

        const AZStd::string nameString {"Old name"};

        UnderTest::EMotionFXManager& manager = UnderTest::GetEMotionFX();
        UnderTest::ActorManager actorManager;
        UnderTest::Actor actor;
        auto simulatedObjectSetup = AZStd::make_shared<UnderTest::SimulatedObjectSetup>();
        UnderTest::SimulatedObject simulatedObject;

        EXPECT_CALL(manager, GetActorManager())
            .WillRepeatedly(Return(&actorManager));

        EXPECT_CALL(actorManager, FindActorByID(0))
            .WillRepeatedly(Return(&actor));

        EXPECT_CALL(actor, GetSimulatedObjectSetup())
            .WillRepeatedly(ReturnRef(simulatedObjectSetup));
        EXPECT_CALL(actor, GetDirtyFlag())
            .WillOnce(Return(false));

        EXPECT_CALL(*simulatedObjectSetup, GetSimulatedObject(0))
            .WillRepeatedly(Return(&simulatedObject));
        EXPECT_CALL(*simulatedObjectSetup, IsSimulatedObjectNameUnique(StrEq("New name"), &simulatedObject))
            .WillRepeatedly(Return(true));
        EXPECT_CALL(*simulatedObjectSetup, IsSimulatedObjectNameUnique(StrEq("Old name"), &simulatedObject))
            .WillRepeatedly(Return(true));

        // GetName returns a reference, so the return value for it has to be
        // defined in a place where that reference will exist
        EXPECT_CALL(simulatedObject, GetName())
            .WillRepeatedly(::testing::ReturnRef(nameString));

        AZStd::vector<AZStd::string> defaultColliderTags;
        EXPECT_CALL(simulatedObject, GetColliderTags())
            .WillRepeatedly(testing::ReturnRef(defaultColliderTags));


        const bool doExecuteOnly = ::testing::get<0>(GetParam());
        const bool useCommandString = ::testing::get<1>(GetParam());
        const CommandAdjustSimulatedObjectTestsParam& testParams = ::testing::get<2>(GetParam());

        if (doExecuteOnly)
        {
            EXPECT_CALL(actor, SetDirtyFlag(true));

            testParams.setExecuteExpectations(&simulatedObject);
        }
        else
        {
            {
                testing::InSequence sequence;
                EXPECT_CALL(actor, SetDirtyFlag(true))
                    .RetiresOnSaturation();
                EXPECT_CALL(actor, SetDirtyFlag(false))
                    .RetiresOnSaturation();
            }
            testParams.setUndoExpectations(&simulatedObject);
        }

        AZStd::string paramString {"-actorId 0 -objectIndex 0"};
        paramString += AZStd::string(buildCommandLineFromTestParam(testParams).c_str());
        MCore::CommandLine parameters(paramString);
        AZStd::string outResult;

        UnderTest::CommandAdjustSimulatedObject command(/*actorId=*/0, /*objectIndex=*/0);
        if (useCommandString)
        {
            EXPECT_TRUE(command.SetCommandParameters(parameters));
        }
        else
        {
            if (testParams.objectName)
            {
                command.SetObjectName(AZStd::string(testParams.objectName->c_str(), testParams.objectName->size()));
            }
            command.SetGravityFactor(testParams.gravityFactor);
            command.SetStiffnessFactor(testParams.stiffnessFactor);
            command.SetDampingFactor(testParams.dampingFactor);
            if (testParams.colliderTags)
            {
                AZStd::vector<AZStd::string> value;
                for (const std::string& stdstring : testParams.colliderTags.value())
                {
                    value.emplace_back(stdstring.c_str(), stdstring.size());
                }
                command.SetColliderTags(value);
            }
        }
        EXPECT_TRUE(command.Execute(parameters, outResult)) << outResult.c_str();
        if (!doExecuteOnly)
        {
            EXPECT_TRUE(command.Undo(parameters, outResult)) << outResult.c_str();
        }
    }

    INSTANTIATE_TEST_CASE_P(TestCommandAdjustSimulatedObject, CommandAdjustSimulatedObjectTestsFixture,
        ::testing::Combine(
            ::testing::Bool(), // Test execute or test undo
            ::testing::Bool(), // Use command strings or not
            ::testing::ValuesIn({
                CommandAdjustSimulatedObjectTestsParam
                {
                    "New name",
                    AZStd::nullopt,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        EXPECT_CALL(*simulatedObject, SetName(StrEq("New name")))
                            .Times(1);
                    },
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, SetName(StrEq("New name")))
                            .Times(1);
                        EXPECT_CALL(*simulatedObject, SetName(StrEq("Old name")))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedObjectTestsParam
                {
                    AZStd::nullopt,
                    2.2f,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, GetGravityFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(1.2f));
                        EXPECT_CALL(*simulatedObject, SetGravityFactor(::testing::FloatEq(2.2f)))
                            .Times(1);
                    },
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, GetGravityFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(1.2f));
                        EXPECT_CALL(*simulatedObject, SetGravityFactor(::testing::FloatEq(2.2f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedObject, SetGravityFactor(::testing::FloatEq(1.2f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedObjectTestsParam
                {
                    AZStd::nullopt,
                    AZStd::nullopt,
                    3.2f,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, GetStiffnessFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(2.2f));
                        EXPECT_CALL(*simulatedObject, SetStiffnessFactor(::testing::FloatEq(3.2f)))
                            .Times(1);
                    },
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, GetStiffnessFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(2.2f));
                        EXPECT_CALL(*simulatedObject, SetStiffnessFactor(::testing::FloatEq(3.2f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedObject, SetStiffnessFactor(::testing::FloatEq(2.2f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedObjectTestsParam
                {
                    AZStd::nullopt,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    4.2f,
                    AZStd::nullopt,
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, GetDampingFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(3.2f));
                        EXPECT_CALL(*simulatedObject, SetDampingFactor(::testing::FloatEq(4.2f)))
                            .Times(1);
                    },
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, GetDampingFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(3.2f));
                        EXPECT_CALL(*simulatedObject, SetDampingFactor(::testing::FloatEq(4.2f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedObject, SetDampingFactor(::testing::FloatEq(3.2f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedObjectTestsParam
                {
                    AZStd::nullopt,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    AZStd::nullopt,
                    std::vector<std::string>{"left_knee", "right_knee"},
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        EXPECT_CALL(*simulatedObject, SetColliderTags(::testing::Pointwise(StrEq(), std::vector<std::string> {"left_knee", "right_knee"})))
                            .Times(1);
                    },
                    [](UnderTest::SimulatedObject* simulatedObject)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedObject, SetColliderTags(::testing::Pointwise(StrEq(), std::vector<std::string> {"left_knee", "right_knee"})))
                            .Times(1);
                        EXPECT_CALL(*simulatedObject, SetColliderTags(::testing::Pointwise(StrEq(), std::vector<std::string> {})))
                            .Times(1);
                    }
                },
            })
        ),
        [](const ::testing::TestParamInfo<::testing::tuple<bool, bool, CommandAdjustSimulatedObjectTestsParam>>& info)
        {
            std::string cmdline =
                (::testing::get<0>(info.param) ? std::string {"Execute_"} : std::string {"Undo_"})
                + (::testing::get<1>(info.param) ? std::string {"UseCommandString"} : std::string {"UseSetters"})
                + CommandAdjustSimulatedObjectTestsFixture::buildCommandLineFromTestParam(::testing::get<2>(info.param));
            std::replace(cmdline.begin(), cmdline.end(), ' ', '_');
            std::replace(cmdline.begin(), cmdline.end(), ';', '_');
            cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '-'), cmdline.end());
            cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '.'), cmdline.end());
            return cmdline;
        }
    );

    ///////////////////////////////////////////////////////////////////////////
    // CommandAdjustSimulatedJoint tests
    ///////////////////////////////////////////////////////////////////////////
    struct CommandAdjustSimulatedJointTestsParam
    {
        AZStd::optional<float> coneAngleLimit;
        AZStd::optional<float> mass;
        AZStd::optional<float> stiffness;
        AZStd::optional<float> damping;
        AZStd::optional<float> gravityFactor;
        AZStd::optional<float> friction;
        AZStd::optional<bool>  pinned;
        AZStd::optional<std::vector<std::string>> colliderExclusionTags;
        AZStd::optional<SimulatedJoint::AutoExcludeMode> autoExcludeMode;
        AZStd::optional<bool> geometricAutoExclusion;
        void (*setExecuteExpectations)(UnderTest::SimulatedJoint*);
        void (*setUndoExpectations)(UnderTest::SimulatedJoint*);
    };

    class CommandAdjustSimulatedJointTestsFixture
        : public UnitTest::LeakDetectionFixture
        , public ::testing::WithParamInterface<::testing::tuple<bool, bool, CommandAdjustSimulatedJointTestsParam>>
    {
    public:
        static std::string buildCommandLineFromTestParam(const CommandAdjustSimulatedJointTestsParam& param)
        {
            std::string string;
            std::stringstream stream(string);

            if (param.coneAngleLimit.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_coneAngleLimitParameterName << ' ' << param.coneAngleLimit.value();
            }
            if (param.mass.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_massParameterName << ' ' << param.mass.value();
            }
            if (param.stiffness.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_stiffnessParameterName << ' ' << param.stiffness.value();
            }
            if (param.damping.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_dampingParameterName << ' ' << param.damping.value();
            }
            if (param.gravityFactor.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_gravityFactorParameterName << ' ' << param.gravityFactor.value();
            }
            if (param.friction.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_frictionParameterName << ' ' << param.friction.value();
            }
            if (param.pinned.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_pinnedParameterName << ' ' << (param.pinned.value() ? "true" : "false");
            }
            if (param.colliderExclusionTags.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_colliderExclusionTagsParameterName << ' ';
                for (const std::string& val : *param.colliderExclusionTags)
                {
                    stream << val;
                    if (&val != std::addressof(*(param.colliderExclusionTags->end() - 1)))
                    {
                        stream << ";";
                    }
                }
            }
            if (param.autoExcludeMode.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_autoExcludeModeParameterName << ' ';
                switch (param.autoExcludeMode.value())
                {
                    case SimulatedJoint::AutoExcludeMode::None:
                        stream << "None";
                        break;
                    case SimulatedJoint::AutoExcludeMode::Self:
                        stream << "Self";
                        break;
                    case SimulatedJoint::AutoExcludeMode::SelfAndNeighbors:
                        stream << "SelfAndNeighbors";
                        break;
                    case SimulatedJoint::AutoExcludeMode::All:
                        stream << "All";
                        break;
                };
            }
            if (param.geometricAutoExclusion.has_value())
            {
                stream << " -" << UnderTest::CommandAdjustSimulatedJoint::s_geometricAutoExclusionParameterName << ' ' << (param.geometricAutoExclusion.value() ? "true" : "false");
            }
            return stream.str();
        }
    };

    TEST_P(CommandAdjustSimulatedJointTestsFixture, TestExecute)
    {
        using ::testing::Return;
        using ::testing::ReturnRef;

        UnderTest::EMotionFXManager& manager = UnderTest::GetEMotionFX();
        UnderTest::ActorManager actorManager;
        UnderTest::Actor actor;
        auto simulatedObjectSetup = AZStd::make_shared<UnderTest::SimulatedObjectSetup>();
        UnderTest::SimulatedObject simulatedObject;
        UnderTest::SimulatedJoint simulatedJoint;

        EXPECT_CALL(manager, GetActorManager())
            .WillRepeatedly(Return(&actorManager));

        EXPECT_CALL(actorManager, FindActorByID(0))
            .WillRepeatedly(Return(&actor));

        EXPECT_CALL(actor, GetSimulatedObjectSetup())
            .WillRepeatedly(ReturnRef(simulatedObjectSetup));
        EXPECT_CALL(actor, GetDirtyFlag())
            .WillOnce(Return(false));

        EXPECT_CALL(*simulatedObjectSetup, GetSimulatedObject(0))
            .WillRepeatedly(Return(&simulatedObject));

        EXPECT_CALL(simulatedObject, GetSimulatedJoint(0))
            .WillRepeatedly(Return(&simulatedJoint));

        AZStd::vector<AZStd::string> defaultColliderExclusionTags;
        EXPECT_CALL(simulatedJoint, GetColliderExclusionTags())
            .WillRepeatedly(testing::ReturnRef(defaultColliderExclusionTags));

        const bool doExecuteOnly = ::testing::get<0>(GetParam());
        const bool useCommandString = ::testing::get<1>(GetParam());
        const auto& testParams = ::testing::get<2>(GetParam());

        if (doExecuteOnly)
        {
            EXPECT_CALL(actor, SetDirtyFlag(true));
            testParams.setExecuteExpectations(&simulatedJoint);
        }
        else
        {
            {
                testing::InSequence sequence;
                EXPECT_CALL(actor, SetDirtyFlag(true))
                    .RetiresOnSaturation();
                EXPECT_CALL(actor, SetDirtyFlag(false))
                    .RetiresOnSaturation();
            }
            testParams.setUndoExpectations(&simulatedJoint);
        }

        AZStd::string paramString {"-actorId 0 -objectIndex 0 -jointIndex 0"};
        paramString += AZStd::string(buildCommandLineFromTestParam(testParams).c_str());
        MCore::CommandLine parameters(paramString);
        AZStd::string outResult;

        UnderTest::CommandAdjustSimulatedJoint command(/*actorId=*/0, /*objectIndex=*/0, /*jointIndex=*/0);
        if (useCommandString)
        {
            EXPECT_TRUE(command.SetCommandParameters(parameters));
        }
        else
        {
            if (testParams.coneAngleLimit.has_value())
            {
                command.SetConeAngleLimit(testParams.coneAngleLimit.value());
            }
            if (testParams.mass.has_value())
            {
                command.SetMass(testParams.mass.value());
            }
            if (testParams.stiffness.has_value())
            {
                command.SetStiffness(testParams.stiffness.value());
            }
            if (testParams.damping.has_value())
            {
                command.SetDamping(testParams.damping.value());
            }
            if (testParams.gravityFactor.has_value())
            {
                command.SetGravityFactor(testParams.gravityFactor.value());
            }
            if (testParams.friction.has_value())
            {
                command.SetFriction(testParams.friction.value());
            }
            if (testParams.pinned.has_value())
            {
                command.SetPinned(testParams.pinned.value());
            }
            if (testParams.colliderExclusionTags.has_value())
            {
                AZStd::vector<AZStd::string> value;
                for (const std::string& stdstring : testParams.colliderExclusionTags.value())
                {
                    value.emplace_back(stdstring.c_str(), stdstring.size());
                }
                command.SetColliderExclusionTags(value);
            }
            if (testParams.autoExcludeMode.has_value())
            {
                command.SetAutoExcludeMode(testParams.autoExcludeMode.value());
            }
            if (testParams.geometricAutoExclusion.has_value())
            {
                command.SetGeometricAutoExclusion(testParams.geometricAutoExclusion.has_value());
            }
        }

        EXPECT_TRUE(command.Execute(parameters, outResult)) << outResult.c_str();

        if (!doExecuteOnly)
        {
            EXPECT_TRUE(command.Undo(parameters, outResult)) << outResult.c_str();
        }
    }

    INSTANTIATE_TEST_CASE_P(TestCommandAdjustSimulatedJoint, CommandAdjustSimulatedJointTestsFixture,
        ::testing::Combine(
            ::testing::Bool(), // Test execute or test undo
            ::testing::Bool(), // Use command strings or not
            ::testing::ValuesIn(
            {
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ 0.3f,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetConeAngleLimit())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetConeAngleLimit(::testing::FloatEq(0.3f)))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetConeAngleLimit())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetConeAngleLimit(::testing::FloatEq(0.3f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetConeAngleLimit(::testing::FloatEq(0.8f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ 0.3f,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetMass())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetMass(::testing::FloatEq(0.3f)))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetMass())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetMass(::testing::FloatEq(0.3f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetMass(::testing::FloatEq(0.8f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ 0.3f,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetStiffness())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetStiffness(::testing::FloatEq(0.3f)))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetStiffness())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetStiffness(::testing::FloatEq(0.3f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetStiffness(::testing::FloatEq(0.8f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ 0.3f,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetDamping())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetDamping(::testing::FloatEq(0.3f)))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetDamping())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetDamping(::testing::FloatEq(0.3f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetDamping(::testing::FloatEq(0.8f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ 0.3f,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetGravityFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetGravityFactor(::testing::FloatEq(0.3f)))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetGravityFactor())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetGravityFactor(::testing::FloatEq(0.3f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetGravityFactor(::testing::FloatEq(0.8f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ 0.3f,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetFriction())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetFriction(::testing::FloatEq(0.3f)))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetFriction())
                            .Times(1)
                            .WillOnce(::testing::Return(0.8f));
                        EXPECT_CALL(*simulatedJoint, SetFriction(::testing::FloatEq(0.3f)))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetFriction(::testing::FloatEq(0.8f)))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ true,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, IsPinned())
                            .Times(1)
                            .WillOnce(::testing::Return(false));
                        EXPECT_CALL(*simulatedJoint, SetPinned(true))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint* simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, IsPinned())
                            .Times(1)
                            .WillOnce(::testing::Return(false));
                        EXPECT_CALL(*simulatedJoint, SetPinned(true))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetPinned(false))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ std::vector<std::string>{"left_knee", "right_knee"},
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint * simulatedJoint)
                    {
                        EXPECT_CALL(*simulatedJoint, SetColliderExclusionTags(::testing::Pointwise(StrEq(), std::vector<std::string> {"left_knee", "right_knee"})))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint * simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, SetColliderExclusionTags(::testing::Pointwise(StrEq(), std::vector<std::string> {"left_knee", "right_knee"})))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetColliderExclusionTags(::testing::Pointwise(StrEq(), std::vector<std::string> {})))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ SimulatedJoint::AutoExcludeMode::All,
                    /* geometricAutoExclusion = */ AZStd::nullopt,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint * simulatedJoint)
                    {
                        EXPECT_CALL(*simulatedJoint, GetAutoExcludeMode())
                            .Times(1)
                            .WillOnce(::testing::Return(SimulatedJoint::AutoExcludeMode::None));
                        EXPECT_CALL(*simulatedJoint, SetAutoExcludeMode(SimulatedJoint::AutoExcludeMode::All))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint * simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, GetAutoExcludeMode())
                            .Times(1)
                            .WillOnce(::testing::Return(SimulatedJoint::AutoExcludeMode::None));
                        EXPECT_CALL(*simulatedJoint, SetAutoExcludeMode(SimulatedJoint::AutoExcludeMode::All))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetAutoExcludeMode(SimulatedJoint::AutoExcludeMode::None))
                            .Times(1);
                    }
                },
                CommandAdjustSimulatedJointTestsParam
                {
                    /* coneAngleLimit = */ AZStd::nullopt,
                    /* mass = */ AZStd::nullopt,
                    /* stiffness = */ AZStd::nullopt,
                    /* damping = */ AZStd::nullopt,
                    /* gravityFactor = */ AZStd::nullopt,
                    /* friction = */ AZStd::nullopt,
                    /* pinned = */ AZStd::nullopt,
                    /* colliderExclusionTags = */ AZStd::nullopt,
                    /* autoExcludeMode = */ AZStd::nullopt,
                    /* geometricAutoExclusion = */ true,
                    /* setExecuteExpectations = */ [](UnderTest::SimulatedJoint * simulatedJoint)
                    {
                        EXPECT_CALL(*simulatedJoint, IsGeometricAutoExclusion())
                            .Times(1)
                            .WillOnce(::testing::Return(false));
                        EXPECT_CALL(*simulatedJoint, SetGeometricAutoExclusion(true))
                            .Times(1);
                    },
                    /* setUndoExpectations = */ [](UnderTest::SimulatedJoint * simulatedJoint)
                    {
                        ::testing::InSequence sequence;
                        EXPECT_CALL(*simulatedJoint, IsGeometricAutoExclusion())
                            .Times(1)
                            .WillOnce(::testing::Return(false));
                        EXPECT_CALL(*simulatedJoint, SetGeometricAutoExclusion(true))
                            .Times(1);
                        EXPECT_CALL(*simulatedJoint, SetGeometricAutoExclusion(false))
                            .Times(1);
                    }
                },
            })
        ),
        [](const ::testing::TestParamInfo<::testing::tuple<bool, bool, CommandAdjustSimulatedJointTestsParam>>& info)
        {
            std::string cmdline =
                (::testing::get<0>(info.param) ? std::string {"Execute_"} : std::string {"Undo_"})
                + (::testing::get<1>(info.param) ? std::string {"UseCommandString"} : std::string {"UseSetters"})
                + CommandAdjustSimulatedJointTestsFixture::buildCommandLineFromTestParam(::testing::get<2>(info.param));
            std::replace(cmdline.begin(), cmdline.end(), ' ', '_');
            std::replace(cmdline.begin(), cmdline.end(), ';', '_');
            cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '-'), cmdline.end());
            cmdline.erase(std::remove(cmdline.begin(), cmdline.end(), '.'), cmdline.end());
            return cmdline;
        }
    );
} // namespace EMotionFX
