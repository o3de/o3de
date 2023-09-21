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

#include <ScriptCanvas/Libraries/Entity/EntityFunctions.h>

#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>


#include <ScriptCanvas/Libraries/Math/AABB.h>
#include <ScriptCanvas/Libraries/Math/Color.h>
#include <ScriptCanvas/Libraries/Math/CRC.h>
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
#include <ScriptCanvas/Libraries/Spawning/SpawnNodeable.h>
#include <ScriptCanvas/Libraries/String/StringFunctions.h>


#include <ScriptCanvas/Libraries/Operators/Math/OperatorAdd.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorArithmetic.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorDiv.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorLerpNodeable.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorMul.h>
#include <ScriptCanvas/Libraries/Operators/Math/OperatorSub.h>

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

#ifndef _RELEASE
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
