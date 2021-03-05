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

#include <Atom/RPI.Reflect/Shader/ShaderResourceGroupAssetCreator.h>
#include <Atom/RPI.Public/Shader/ShaderResourceGroup.h>

namespace AZ
{
    namespace RPI
    {
        //////////////////////////////////////////////////////////////////////////////////////////
        //// ShaderResourceGroupAssetCreator

        void ShaderResourceGroupAssetCreator::Begin(const AZ::Data::AssetId& assetId, const Name& shaderResourceGroupName)
        {
            BeginCommon(assetId);

            if (ValidateIsReady())
            {
                m_asset->m_name = shaderResourceGroupName;
            }
        }

        void ShaderResourceGroupAssetCreator::BeginAPI(RHI::APIType type)
        {
            if (ValidateIsReady())
            {
                m_currentAPIType = type;
                m_shaderResourceGroupLayout = RHI::ShaderResourceGroupLayout::Create();
            }
        }

        void ShaderResourceGroupAssetCreator::SetBindingSlot(uint32_t bindingSlot)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->SetBindingSlot(bindingSlot);
            }
        }

        void ShaderResourceGroupAssetCreator::SetShaderVariantKeyFallback(const Name& shaderInputName, uint32_t bitSize)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->SetShaderVariantKeyFallback(shaderInputName, bitSize);
            }
        }


        void ShaderResourceGroupAssetCreator::AddStaticSampler(const RHI::ShaderInputStaticSamplerDescriptor& shaderInputStaticSampler)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->AddStaticSampler(shaderInputStaticSampler);
            }
        }

        void ShaderResourceGroupAssetCreator::AddShaderInput(const RHI::ShaderInputBufferDescriptor& shaderInputBuffer)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->AddShaderInput(shaderInputBuffer);
            }
        }

        void ShaderResourceGroupAssetCreator::AddShaderInput(const RHI::ShaderInputImageDescriptor& shaderInputImage)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->AddShaderInput(shaderInputImage);
            }
        }

        void ShaderResourceGroupAssetCreator::AddShaderInput(const RHI::ShaderInputSamplerDescriptor& shaderInputSampler)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->AddShaderInput(shaderInputSampler);
            }
        }

        void ShaderResourceGroupAssetCreator::AddShaderInput(const RHI::ShaderInputConstantDescriptor& shaderInputConstant)
        {
            if (ValidateIsReady())
            {
                m_shaderResourceGroupLayout->AddShaderInput(shaderInputConstant);
            }
        }

        void ShaderResourceGroupAssetCreator::Cleanup()
        {
            m_shaderResourceGroupLayout = nullptr;
            m_currentAPIType = RHI::APIType();
        }
        
        bool ShaderResourceGroupAssetCreator::End(Data::Asset<ShaderResourceGroupAsset>& result)
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (m_shaderResourceGroupLayout)
            {
                AZ_Assert(false, "Must EndAPI before ending calling End");
                return false;
            }

            if (m_asset->m_perAPILayout.empty())
            {
                AZ_Assert(false, "No Shader Resource Group Layout was added.");
                return false;
            }

            if (!m_asset->FinalizeAfterLoad())
            {
                ReportError("Failed to finalize the ShaderResourceGroupAsset.");
                return false;
            }

            m_asset->SetReady();
            return EndCommon(result);
        }

        bool ShaderResourceGroupAssetCreator::EndAPI()
        {
            if (!ValidateIsReady())
            {
                return false;
            }

            if (!m_shaderResourceGroupLayout->Finalize())
            {
                return false;
            }

            m_asset->m_perAPILayout.push_back({ m_currentAPIType, AZStd::move(m_shaderResourceGroupLayout) });

            Cleanup();

            return true;
        }
    } // namespace RPI
} // namespace AZ