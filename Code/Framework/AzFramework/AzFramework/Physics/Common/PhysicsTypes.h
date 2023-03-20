/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/tuple.h>
#include <AzCore/std/containers/variant.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/shared_ptr.h>
#include <AzCore/Casting/numeric_cast.h>

namespace Physics
{
    class ColliderConfiguration;
    class Shape;
    class ShapeConfiguration;
}

namespace AzPhysics
{
    //! Maximum number of scenes, Default Scene Names and Crc32
    constexpr AZ::u32 MaxNumberOfScenes = 64;
    constexpr const char* DefaultPhysicsSceneName = "DefaultScene";
    constexpr AZ::Crc32 DefaultPhysicsSceneId = AZ_CRC_CE(DefaultPhysicsSceneName);
    constexpr const char* EditorPhysicsSceneName = "EditorScene";
    constexpr AZ::Crc32 EditorPhysicsSceneId = AZ_CRC_CE(EditorPhysicsSceneName);

    //! Default gravity.
    static const AZ::Vector3 DefaultGravity = AZ::Vector3(0.0f, 0.0f, -9.81f);

    //! Helper for retrieving the values from the SceneHandle tuple.
    //! Example usage
    //! @code{ .cpp }
    //! SceneHandle someHandle;
    //! const AZ::Crc32 handleCrc = AZStd::get<HandleTypeIndex::Crc>(someHandle);
    //! const SceneIndex index = AZStd::get<HandleTypeIndex::Index>(someHandle);
    //! @endcode
    enum HandleTypeIndex
    {
        Crc = 0,
        Index
    };

    using SceneIndex = AZ::s8;
    using SimulatedBodyIndex = AZ::s32;
    using JointIndex = AZ::s32;
    static_assert(std::is_signed<SceneIndex>::value
        && std::is_signed<SimulatedBodyIndex>::value
        && std::is_signed<JointIndex>::value, "SceneIndex, SimulatedBodyIndex and JointIndex must be signed integers.");
    

    //! A handle to a Scene within the physics simulation.
    //! A SceneHandle is a tuple of a Crc of the scenes name and the index in the Scene list.
    using SceneHandle = AZStd::tuple<AZ::Crc32, SceneIndex>;
    static constexpr SceneHandle InvalidSceneHandle = { AZ::Crc32(), SceneIndex(-1) };

    //! Ease of use type for referencing a List of SceneHandle objects.
    using SceneHandleList = AZStd::vector<SceneHandle>;

    //! A handle to a Simulated body within a physics scene.
    //! A SimulatedBodyHandle is a tuple of a Crc of the scene's name and the index in the SimulatedBody list.
    using SimulatedBodyHandle = AZStd::tuple<AZ::Crc32, SimulatedBodyIndex>;
    static constexpr SimulatedBodyHandle InvalidSimulatedBodyHandle = { AZ::Crc32(), -1 };
    using SimulatedBodyHandleList = AZStd::vector<SimulatedBodyHandle>;

    //! A handle to a Joint within a physics scene.
    //! A JointHandle is a tuple of a Crc of the scene's name and the index in the Joint list.
    using JointHandle = AZStd::tuple<AZ::Crc32, JointIndex>;
    static constexpr JointHandle InvalidJointHandle = { AZ::Crc32(), -1 };

    //! Helper used for pairing the ShapeConfiguration and ColliderConfiguration together which is used when creating a Simulated Body.
    using ShapeColliderPair = AZStd::pair<
        AZStd::shared_ptr<Physics::ColliderConfiguration>,
        AZStd::shared_ptr<Physics::ShapeConfiguration>>;
    using ShapeColliderPairList = AZStd::vector<ShapeColliderPair>;

    //! Joint types are used to request for AZ::TypeId with the JointHelpersInterface::GetSupportedJointTypeId.
    //! If the Physics backend supports this joint type JointHelpersInterface::GetSupportedJointTypeId will return a AZ::TypeId.
    enum class JointType
    {
        D6Joint,
        FixedJoint,
        BallJoint,
        HingeJoint
    };

    //! Flags used to specifying which properties of a body to compute.
    enum class MassComputeFlags : AZ::u8
    {
        NONE = 0,

        //! Flags indicating whether a certain mass property should be auto-computed or not.
        COMPUTE_MASS = 1,
        COMPUTE_INERTIA = 1 << 1,
        COMPUTE_COM = 1 << 2,

        //! If set, non-simulated shapes will also be included in the mass properties calculation.
        INCLUDE_ALL_SHAPES = 1 << 3,

        DEFAULT = COMPUTE_COM | COMPUTE_INERTIA | COMPUTE_MASS
    };

    //! Bitwise operators for MassComputeFlags
    AZ_DEFINE_ENUM_BITWISE_OPERATORS(MassComputeFlags)

    //! Variant to allow support for the system to either create the Shape(s) or use the provide Shape(s) that have been created externally.
    //! Can be one of the following.
    //! @code{ .cpp }
    //! // A ShapeColliderPair, which contains a ColliderConfiguration and ShapeConfiguration.
    //! AzPhysics::StaticRigidBodyConfiguration staticRigidBodyConfig;
    //! staticRigidBodyConfig.m_colliderAndShapeData = AzPhysics::ShapeColliderPair(&colliderConfig, &shapeConfig);
    //! 
    //! // A pointer to a Physics::Shape. The Simulated Body will take ownership of the pointer.
    //! AZStd::shared_ptr<Physics::Shape> shapePtr /*Created through other means*/;
    //! AzPhysics::StaticRigidBodyConfiguration staticRigidBodyConfig;
    //! staticRigidBodyConfig.m_colliderAndShapeData = shapePtr;
    //! 
    //! // A list of ShapeColliderPairs.
    //! AZStd::vector<AzPhysics::ShapeColliderPair> shapeColliderPairList;
    //! shapeColliderPairList.emplace_back(&colliderConfig, &shapeConfig); //add as many configs as required.
    //! AzPhysics::StaticRigidBodyConfiguration staticRigidBodyConfig;
    //! staticRigidBodyConfig.m_colliderAndShapeData = shapeColliderPairList;
    //! 
    //! // A list of Physics::Shape pointers. The Simulated Body will take ownership of these pointers.
    //! AZStd::vector<AZStd::shared_ptr<Physics::Shape>> shapePtrList;
    //! shapePtrList.emplace_back(/*Shape created through other means*/);
    //! AzPhysics::StaticRigidBodyConfiguration staticRigidBodyConfig;
    //! staticRigidBodyConfig.m_colliderAndShapeData = shapePtrList;
    //! @endcode
    using ShapeVariantData = AZStd::variant<
                                AZStd::monostate,
                                ShapeColliderPair,
                                AZStd::shared_ptr<Physics::Shape>,
                                AZStd::vector<ShapeColliderPair>,
                                AZStd::vector<AZStd::shared_ptr<Physics::Shape>>>;
}
