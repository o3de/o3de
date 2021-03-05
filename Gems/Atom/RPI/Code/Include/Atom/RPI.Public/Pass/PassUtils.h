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
#pragma once

// This header file is for declaring types used for RPI System classes to avoid recursive includes
 
#include <Atom/RPI.Public/Base.h>
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
            bool BindDataMappingsToSrg(const PassDescriptor& descriptor, ShaderResourceGroup* shaderResourceGroup);

            //! Retrieves PassData from a PassDescriptor
            const PassData* GetPassData(const PassDescriptor& descriptor);

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

        }   // namespace PassUtils
    }   // namespace RPI
}   // namespace AZ

