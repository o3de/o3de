/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

// This header file is for declaring types used for RPI System classes to avoid recursive includes
 
#include <Atom/RPI.Public/Base.h>
#include <Atom/RPI.Public/Configuration.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

#include <Atom/RPI.Reflect/Pass/PassData.h>
#include <Atom/RPI.Reflect/Pass/PassDescriptor.h>
#include <Atom/RPI.Reflect/Pass/PassRequest.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

namespace AZ
{
    namespace RPI
    {
        namespace PassUtils
        {
            //! Function for applying shader data mappings from a PassDescriptor to a shader resource group
            ATOM_RPI_PUBLIC_API bool BindDataMappingsToSrg(const PassDescriptor& descriptor, ShaderResourceGroup* shaderResourceGroup);

            //! Retrieves PassData from a PassDescriptor
            ATOM_RPI_PUBLIC_API const PassData* GetPassData(const PassDescriptor& descriptor);

            //! Finds all PipelineGlobalConnections in the descriptor and adds them to the provided list
            ATOM_RPI_PUBLIC_API void ExtractPipelineGlobalConnections(const AZStd::shared_ptr<PassData>& passData, PipelineGlobalConnectionList& outList);

            //! Retrieves PassData from a PassDescriptor
            ATOM_RPI_PUBLIC_API AZStd::shared_ptr<PassData> GetPassDataPtr(const PassDescriptor& descriptor);

            //! Templated function for retrieving specific data types from a PassDescriptor
            template<typename PassDataType>
            const PassDataType* GetPassData(const PassDescriptor& descriptor)
            {
                const PassDataType* passData = nullptr;

                // Try custom data from PassRequest
                if (descriptor.m_passRequest != nullptr)
                {
                    passData = azrtti_cast<const PassDataType*>(descriptor.m_passRequest->m_passData.get());
                }

                // Try custom data from PassTemplate
                if (passData == nullptr && descriptor.m_passTemplate != nullptr)
                {
                    passData = azrtti_cast<const PassDataType*>(descriptor.m_passTemplate->m_passData.get());
                }

                if (passData == nullptr)
                {
                    passData = azrtti_cast<const PassDataType*>(descriptor.m_passData.get());
                }

                return passData;
            }

            ATOM_RPI_PUBLIC_API void SortPassListAscending(AZStd::vector< Ptr<Pass> >& passList);
            ATOM_RPI_PUBLIC_API void SortPassListDescending(AZStd::vector< Ptr<Pass> >& passList);
        }
    }
}

