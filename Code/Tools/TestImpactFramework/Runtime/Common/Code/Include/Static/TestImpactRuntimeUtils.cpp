/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <TestImpactFramework/TestImpactUtils.h>
#include <TestImpactFramework/TestImpactRuntimeException.h>

#include <Artifact/Factory/TestImpactTargetDescriptorFactory.h>
#include <TestImpactRuntimeUtils.h>

#include <filesystem>

namespace TestImpact
{
    AZStd::vector<TargetDescriptor> ReadTargetDescriptorFiles(const BuildTargetDescriptorConfig& buildTargetDescriptorConfig)
    {
        AZStd::vector<TargetDescriptor> descriptors;
        for (const auto& targetDescriptorFile :
             std::filesystem::directory_iterator(buildTargetDescriptorConfig.m_mappingDirectory.c_str()))
        {
            const auto targetDescriptorContents = ReadFileContents<RuntimeException>(targetDescriptorFile.path().string().c_str());
            auto targetDescriptor = TargetDescriptorFactory(
                targetDescriptorContents,
                buildTargetDescriptorConfig.m_staticInclusionFilters,
                buildTargetDescriptorConfig.m_inputInclusionFilters,
                buildTargetDescriptorConfig.m_inputOutputPairer);
            descriptors.emplace_back(AZStd::move(targetDescriptor));
        }

        return descriptors;
    }
} // namespace TestImpact
