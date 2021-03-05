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

#include "Libraries.h"

#include <Libraries/Core/CoreNodes.h>
#include <Libraries/Logic/Logic.h>
#include <Libraries/Math/Math.h>
#include <Libraries/Comparison/Comparison.h>
#include <Libraries/UnitTesting/UnitTestingLibrary.h>
#include <AzCore/std/containers/vector.h>

namespace ScriptCanvas
{
    static AZ::EnvironmentVariable<NodeRegistry> g_nodeRegistry;

    void InitNodeRegistry()
    {
        g_nodeRegistry = AZ::Environment::CreateVariable<NodeRegistry>(s_nodeRegistryName);
        using namespace Library;
        Core::InitNodeRegistry(*g_nodeRegistry);
        Math::InitNodeRegistry(*g_nodeRegistry);
        Logic::InitNodeRegistry(*g_nodeRegistry);
        Entity::InitNodeRegistry(*g_nodeRegistry);
        Comparison::InitNodeRegistry(*g_nodeRegistry);
        Time::InitNodeRegistry(*g_nodeRegistry);
        String::InitNodeRegistry(*g_nodeRegistry);
        Operators::InitNodeRegistry(*g_nodeRegistry);

#ifndef _RELEASE
        Library::UnitTesting::InitNodeRegistry(*g_nodeRegistry);
#endif
    }

    void ResetNodeRegistry()
    {
        g_nodeRegistry.Reset();
    }

    AZ::EnvironmentVariable<NodeRegistry> GetNodeRegistry()
    {
        return AZ::Environment::FindVariable<NodeRegistry>(s_nodeRegistryName);
    }

    void ReflectLibraries(AZ::ReflectContext* reflectContext)
    {
        using namespace Library;

        Core::Reflect(reflectContext);
        Math::Reflect(reflectContext);
        Logic::Reflect(reflectContext);
        Entity::Reflect(reflectContext);
        Comparison::Reflect(reflectContext);
        Time::Reflect(reflectContext);
        String::Reflect(reflectContext);
        Operators::Reflect(reflectContext);

#ifndef _RELEASE
        Library::UnitTesting::Reflect(reflectContext);
#endif
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        using namespace Library;

        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors(Core::GetComponentDescriptors());
        
        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors(Math::GetComponentDescriptors());
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        
        componentDescriptors = Logic::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
        
        componentDescriptors = Entity::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Comparison::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Time::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = String::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Operators::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

#ifndef _RELEASE
        componentDescriptors = Library::UnitTesting::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());
#endif


        return libraryDescriptors;
    }

}