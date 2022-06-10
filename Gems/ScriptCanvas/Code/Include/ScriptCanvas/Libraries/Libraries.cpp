/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Libraries.h"

#include <Libraries/Comparison/Comparison.h>
#include <Libraries/Core/CoreNodes.h>
#include <Libraries/Deprecated/DeprecatedNodeLibrary.h>
#include <Libraries/Logic/Logic.h>

namespace ScriptCanvas
{
    void InitLibraries()
    {
        auto nodeRegistry = GetNodeRegistry();
        using namespace Library;
        Core::InitNodeRegistry(*nodeRegistry);
        Logic::InitNodeRegistry(*nodeRegistry);
        Comparison::InitNodeRegistry(*nodeRegistry);
        Operators::InitNodeRegistry(*nodeRegistry);
    }

    void ResetLibraries()
    {
        ResetNodeRegistry();
    }

    void ReflectLibraries(AZ::ReflectContext* reflectContext)
    {
        using namespace Library;

        Core::Reflect(reflectContext);
        Math::Reflect(reflectContext);
        Logic::Reflect(reflectContext);
        Comparison::Reflect(reflectContext);
        Time::Reflect(reflectContext);
        Spawning::Reflect(reflectContext);
        String::Reflect(reflectContext);
        Operators::Reflect(reflectContext);
        CustomLibrary::Reflect(reflectContext);

        DeprecatedNodeLibrary::Reflect(reflectContext);

#ifndef _RELEASE
        Library::UnitTesting::Reflect(reflectContext);
#endif
    }

    AZStd::vector<AZ::ComponentDescriptor*> GetLibraryDescriptors()
    {
        using namespace Library;

        AZStd::vector<AZ::ComponentDescriptor*> libraryDescriptors(Core::GetComponentDescriptors());
        
        AZStd::vector<AZ::ComponentDescriptor*> componentDescriptors = Logic::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Comparison::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = Operators::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        componentDescriptors = DeprecatedNodeLibrary::GetComponentDescriptors();
        libraryDescriptors.insert(libraryDescriptors.end(), componentDescriptors.begin(), componentDescriptors.end());

        return libraryDescriptors;
    }

}
