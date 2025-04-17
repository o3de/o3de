/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Libraries.h"

#include <Libraries/Comparison/ComparisonLibrary.h>
#include <Libraries/Core/CoreLibrary.h>
#include <Libraries/Deprecated/DeprecatedNodeLibrary.h>
#include <Libraries/Logic/LogicLibrary.h>
#include <Libraries/Operators/OperatorsLibrary.h>
#include <Libraries/UnitTesting/UnitTestingLibrary.h>

#include <ScriptCanvas/Data/DataMacros.h>

#include <AzFramework/Spawnable/SpawnableEntitiesInterface.h>

// Compact
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactAddNodeable.h>
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactDecrementNodeable.h>
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactDivideNodeable.h>
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactIncrementNodeable.h>
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactMultiplyNodeable.h>
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactNegateNodeable.h>
#include <ScriptCanvas/Libraries/Compact/BasicOperators/CompactSubtractNodeable.h>

#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactCeilingNodeable.h>
#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactFloorNodeable.h>
#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactModuloNodeable.h>
#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactPowerNodeable.h>
#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactRoundNodeable.h>
#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactSquareNodeable.h>
#include <ScriptCanvas/Libraries/Compact/MathematicalFunctions/CompactSquareRootNodeable.h>

#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactArccosineNodeable.h>
#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactArcsineNodeable.h>
#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactArctangentNodeable.h>
#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactArctangent2Nodeable.h>
#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactCosineNodeable.h>
#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactSineNodeable.h>
#include <ScriptCanvas/Libraries/Compact/Trigonometry/CompactTangentNodeable.h>

// Comparison
#include <ScriptCanvas/Libraries/Comparison/EqualTo.h>
#include <ScriptCanvas/Libraries/Comparison/Greater.h>
#include <ScriptCanvas/Libraries/Comparison/GreaterEqual.h>
#include <ScriptCanvas/Libraries/Comparison/Less.h>
#include <ScriptCanvas/Libraries/Comparison/LessEqual.h>
#include <ScriptCanvas/Libraries/Comparison/NotEqualTo.h>

// Core
#include <ScriptCanvas/Libraries/Core/AzEventHandler.h>
#include <ScriptCanvas/Libraries/Core/BinaryOperator.h>
#include <ScriptCanvas/Libraries/Core/ContainerTypeReflection.h>
#include <ScriptCanvas/Libraries/Core/EBusEventHandler.h>
#include <ScriptCanvas/Libraries/Core/ExtractProperty.h>
#include <ScriptCanvas/Libraries/Core/ForEach.h>
#include <ScriptCanvas/Libraries/Core/FunctionCallNode.h>
#include <ScriptCanvas/Libraries/Core/FunctionDefinitionNode.h>
#include <ScriptCanvas/Libraries/Core/GetVariable.h>
#include <ScriptCanvas/Libraries/Core/Method.h>
#include <ScriptCanvas/Libraries/Core/ReceiveScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SendScriptEvent.h>
#include <ScriptCanvas/Libraries/Core/SetVariable.h>
#include <ScriptCanvas/Libraries/Core/Start.h>
#include <ScriptCanvas/Libraries/Core/UnaryOperator.h>

// Entity
#include <ScriptCanvas/Libraries/Entity/EntityFunctions.h>

// Logic
#include <ScriptCanvas/Libraries/Logic/And.h>
#include <ScriptCanvas/Libraries/Logic/Any.h>
#include <ScriptCanvas/Libraries/Logic/Break.h>
#include <ScriptCanvas/Libraries/Logic/Cycle.h>
#include <ScriptCanvas/Libraries/Logic/Gate.h>
#include <ScriptCanvas/Libraries/Logic/IsNull.h>
#include <ScriptCanvas/Libraries/Logic/Not.h>
#include <ScriptCanvas/Libraries/Logic/Once.h>
#include <ScriptCanvas/Libraries/Logic/Or.h>
#include <ScriptCanvas/Libraries/Logic/OrderedSequencer.h>
#include <ScriptCanvas/Libraries/Logic/TargetedSequencer.h>
#include <ScriptCanvas/Libraries/Logic/WeightedRandomSequencer.h>
#include <ScriptCanvas/Libraries/Logic/While.h>

// Math
#include <ScriptCanvas/Libraries/Math/AABB.h>
#include <ScriptCanvas/Libraries/Math/Color.h>
#include <ScriptCanvas/Libraries/Math/CRC.h>
#include <ScriptCanvas/Libraries/Math/MathExpression.h>
#include <ScriptCanvas/Libraries/Math/MathFunctions.h>
#include <ScriptCanvas/Libraries/Math/Matrix3x3.h>
#include <ScriptCanvas/Libraries/Math/Matrix4x4.h>
#include <ScriptCanvas/Libraries/Math/MatrixMxN.h>
#include <ScriptCanvas/Libraries/Math/OBB.h>
#include <ScriptCanvas/Libraries/Math/Plane.h>
#include <ScriptCanvas/Libraries/Math/Quaternion.h>
#include <ScriptCanvas/Libraries/Math/Transform.h>
#include <ScriptCanvas/Libraries/Math/Vector2.h>
#include <ScriptCanvas/Libraries/Math/Vector3.h>
#include <ScriptCanvas/Libraries/Math/Vector4.h>
#include <ScriptCanvas/Libraries/Math/VectorN.h>

// Operators
#include <ScriptCanvas/Libraries/Operators/Math/OperatorAdd.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorDiv.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorLerpNodeable.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorLerpNodeableNode.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorMul.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorSub.h>

// Spawning
#include <ScriptCanvas/Libraries/Spawning/CreateSpawnTicketNodeable.h>
#include <ScriptCanvas/Libraries/Spawning/DespawnNodeable.h>
#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>

// String
#include <ScriptCanvas/Libraries/String/Format.h>
#include <ScriptCanvas/Libraries/String/Print.h>
#include <ScriptCanvas/Libraries/String/StringFunctions.h>

// Time
#include <ScriptCanvas/Libraries/Time/DelayNodeable.h>
#include <ScriptCanvas/Libraries/Time/DurationNodeable.h>
#include <ScriptCanvas/Libraries/Time/HeartBeatNodeable.h>
#include <ScriptCanvas/Libraries/Time/RepeaterNodeable.h>
#include <ScriptCanvas/Libraries/Time/TimeDelayNodeable.h>
#include <ScriptCanvas/Libraries/Time/TimerNodeable.h>


namespace ScriptCanvas
{
    void InitLibraries()
    {
        auto nodeRegistry = NodeRegistry::GetInstance();
        ComparisonLibrary::InitNodeRegistry(nodeRegistry);
        CoreLibrary::InitNodeRegistry(nodeRegistry);
        LogicLibrary::InitNodeRegistry(nodeRegistry);
        OperatorsLibrary::InitNodeRegistry(nodeRegistry);
    }

    void ResetLibraries()
    {
        NodeRegistry::ResetInstance();
    }

    void ReflectLibraries(AZ::ReflectContext* reflectContext)
    {
        CoreLibrary::Reflect(reflectContext);
        DeprecatedNodeLibrary::Reflect(reflectContext);
        LogicLibrary::Reflect(reflectContext);
        OperatorsLibrary::Reflect(reflectContext);

#if !defined(AZ_MONOLITHIC_BUILD)
        UnitTestingLibrary::Reflect(reflectContext);
#endif
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors(ComparisonLibrary::GetComponentDescriptors());

        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors = CoreLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = DeprecatedNodeLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = LogicLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = OperatorsLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        return libraryDescriptors;
    }

    AZ::EnvironmentVariable<NodeRegistry> GetNodeRegistry()
    {
        return AZ::Environment::FindVariable<NodeRegistry>(s_nodeRegistryName);
    }
}
