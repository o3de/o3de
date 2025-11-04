/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <AzCore/Module/Module.h>
#include <AzCore/Component/ComponentApplicationBus.h>

namespace AZ
{
    Module::Module()
    {
    }

    Module::~Module()
    {
        for (AZ::ComponentDescriptor* descriptor : m_descriptors)
        {
            // Deletes and "un-reflects" the descriptor
            descriptor->ReleaseDescriptor();
        }
    }

    void Module::RegisterComponentDescriptors()
    {
        for (const ComponentDescriptor* descriptor : m_descriptors)
        {
            AZ_Warning("AZ::Module", descriptor, "Null module descriptor is being skipped (%s)", RTTI_GetType().ToString<AZStd::string>().c_str());
            ComponentApplicationBus::Broadcast(&ComponentApplicationBus::Events::RegisterComponentDescriptor, descriptor);
        }
    }
}
