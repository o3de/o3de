/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "PythonCoverageEditorModule.h"
#include "PythonCoverageEditorSystemComponent.h"

namespace PythonCoverage
{
    AZ_CLASS_ALLOCATOR_IMPL(PythonCoverageEditorModule, AZ::SystemAllocator);

    PythonCoverageEditorModule::PythonCoverageEditorModule()
    {
        m_descriptors.insert(
            m_descriptors.end(),
            {
                PythonCoverageEditorSystemComponent::CreateDescriptor()
            });
    }

    PythonCoverageEditorModule::~PythonCoverageEditorModule() = default;

    AZ::ComponentTypeList PythonCoverageEditorModule::GetRequiredSystemComponents() const
    {
        // add required SystemComponents to the SystemEntity
        return AZ::ComponentTypeList{ azrtti_typeid<PythonCoverageEditorSystemComponent>() };
    }
} // namespace PythonCoverage

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), PythonCoverage::PythonCoverageEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_PythonCoverage_Editor, PythonCoverage::PythonCoverageEditorModule)
#endif
