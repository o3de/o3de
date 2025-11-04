/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>
#include <AzCore/Debug/Trace.h>

#include <QtForPythonSystemComponent.h>

#include "InitializeEmbeddedPyside2.h"

namespace QtForPython
{
    class QtForPythonModule
        : public AZ::Module
        , private InitializeEmbeddedPyside2
    {
    public:
        AZ_RTTI(QtForPythonModule, "{81545CD5-79FA-47CE-96F2-1A9C5D59B4B9}", AZ::Module);
        AZ_CLASS_ALLOCATOR(QtForPythonModule, AZ::SystemAllocator);

        QtForPythonModule()
            : AZ::Module()
            , InitializeEmbeddedPyside2()
        {
            m_descriptors.insert(m_descriptors.end(), {
                QtForPythonSystemComponent::CreateDescriptor(),
            });
        }
        ~QtForPythonModule() override = default;

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

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), QtForPython::QtForPythonModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_QtForPython_Editor, QtForPython::QtForPythonModule)
#endif
