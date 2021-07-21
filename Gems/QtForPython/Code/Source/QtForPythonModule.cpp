/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include <QtForPythonSystemComponent.h>

namespace QtForPython
{
    class QtForPythonModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(QtForPythonModule, "{81545CD5-79FA-47CE-96F2-1A9C5D59B4B9}", AZ::Module);
        AZ_CLASS_ALLOCATOR(QtForPythonModule, AZ::SystemAllocator, 0);

        QtForPythonModule()
            : AZ::Module()
        {
            m_descriptors.insert(m_descriptors.end(), {
                QtForPythonSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList
            {
                azrtti_typeid<QtForPythonSystemComponent>(),
            };
        }
    };
}

// DO NOT MODIFY THIS LINE UNLESS YOU RENAME THE GEM
// The first parameter should be GemName_GemIdLower
// The second should be the fully qualified name of the class above
AZ_DECLARE_MODULE_CLASS(Gem_QtForPython, QtForPython::QtForPythonModule)
