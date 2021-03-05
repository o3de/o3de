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

#include <AzCore/Interface/Interface.h>

#include <Atom/RPI.Public/Pass/ComputePass.h>
#include <Atom/RPI.Public/Pass/CopyPass.h>
#include <Atom/RPI.Public/Pass/FullscreenTrianglePass.h>
#include <Atom/RPI.Public/Pass/ParentPass.h>
#include <Atom/RPI.Public/Pass/PassLibrary.h>
#include <Atom/RPI.Public/Pass/Specific/DownsampleMipChainPass.h>
#include <Atom/RPI.Public/Pass/Pass.h>
#include <Atom/RPI.Public/Pass/PassFactory.h>
#include <Atom/RPI.Public/Pass/PassFilter.h>
#include <Atom/RPI.Public/Pass/RasterPass.h>
#include <Atom/RPI.Public/Pass/MSAAResolvePass.h>
#include <Atom/RPI.Public/Pass/Specific/RenderToTexturePass.h>
#include <Atom/RPI.Public/Pass/Specific/EnvironmentCubeMapPass.h>

#include <Atom/RPI.Reflect/Pass/PassAsset.h>
#include <Atom/RPI.Reflect/Pass/PassTemplate.h>

namespace AZ
{
    namespace RPI
    {
        void PassFactory::Init(PassLibrary* passLibrary)
        {
            m_passLibary = passLibrary;
            AddCorePasses();
        }

        void PassFactory::Shutdown()
        {
            m_passClassNames.clear();
            m_creationFunctions.clear();
        }

        // --- Entry related functions ---

        void PassFactory::AddPassCreator(Name passClassName, PassCreator createFunction)
        {
            // Ensure we don't already have a PassCreator registered with this Name
            AZ_Assert(FindCreatorIndex(passClassName) == CreatorIndex::Null,
                "The Pass Factory already has a creator for the class name %s",
                passClassName.GetCStr());

            m_passClassNames.push_back(passClassName);
            m_creationFunctions.push_back(createFunction);
        }

        void PassFactory::AddCorePasses()
        {
            AddPassCreator(Name("ParentPass"), &ParentPass::Create);
            AddPassCreator(Name("RasterPass"), &RasterPass::Create);
            AddPassCreator(Name("CopyPass"), &CopyPass::Create);
            AddPassCreator(Name("FullScreenTriangle"), &FullscreenTrianglePass::Create);
            AddPassCreator(Name("ComputePass"), &ComputePass::Create);
            AddPassCreator(Name("MSAAResolvePass"), &MSAAResolvePass::Create);
            AddPassCreator(Name("DownsampleMipChainPass"), &DownsampleMipChainPass::Create);
            AddPassCreator(Name("EnvironmentCubeMapPass"), &EnvironmentCubeMapPass::Create);
            AddPassCreator(Name("RenderToTexturePass"), &RenderToTexturePass::Create);
        }

        PassFactory::CreatorIndex PassFactory::FindCreatorIndex(Name passClassName)
        {
            for (uint32_t i = 0; i < m_passClassNames.size(); ++i)
            {
                if (passClassName == m_passClassNames[i])
                {
                    return CreatorIndex(i);
                }
            }
            return CreatorIndex::Null;
        }

        bool PassFactory::HasCreatorForClass(Name passClassName)
        {
            return FindCreatorIndex(passClassName) != CreatorIndex::Null;
        }

        // --- Pass Creation Functions ---

        Ptr<Pass> PassFactory::CreatePassFromIndex(CreatorIndex index, Name passName, const AZStd::shared_ptr<PassTemplate>& passTemplate, const PassRequest* passRequest)
        {
            if (index.IsNull() || index.GetIndex() >= m_creationFunctions.size())
            {
                AZ_Error("PassFactory", false, "FAILED TO CREATE PASS [%s].", passName.GetCStr());
                return nullptr;
            }
            PassCreator passCreator = m_creationFunctions[index.GetIndex()];
            PassDescriptor passDescriptor(passName, passTemplate, passRequest);
            Ptr<Pass> pass = passCreator(passDescriptor);

            return pass;
        }
        
        Ptr<Pass> PassFactory::CreatePassFromClass(Name passClassName, Name passName)
        {
            CreatorIndex index = FindCreatorIndex(passClassName);
            return CreatePassFromIndex(index, passName, nullptr, nullptr);
        }

        Ptr<Pass> PassFactory::CreatePassFromTemplate(const AZStd::shared_ptr<PassTemplate>& passTemplate, Name passName)
        {
            if (!passTemplate)
            {
                AZ_Assert(false, "PassFactory::CreatePassFromTemplate() was handed a null PassTemplate!");
                return nullptr;
            }
            CreatorIndex index = FindCreatorIndex(passTemplate->m_passClass);
            return CreatePassFromIndex(index, passName, passTemplate, nullptr);
        }

        Ptr<Pass> PassFactory::CreatePassFromTemplate(Name templateName, Name passName)
        {
            const AZStd::shared_ptr<PassTemplate>& passTemplate = m_passLibary->GetPassTemplate(templateName);
            if (passTemplate == nullptr)
            {
                AZ_Error("PassFactory", false, "FAILED TO CREATE PASS [%s]. Could not find pass template [%s]", passName.GetCStr(), templateName.GetCStr());
                return nullptr;
            }

            return CreatePassFromTemplate(passTemplate, passName);
        }

        Ptr<Pass> PassFactory::CreatePassFromRequest(const PassRequest* passRequest)
        {
            if (!passRequest)
            {
                AZ_Assert(false, "PassFactory::CreatePassFromRequest() was handed a null PassRequest!");
                return nullptr;
            }

            const AZStd::shared_ptr<PassTemplate>& passTemplate = m_passLibary->GetPassTemplate(passRequest->m_templateName);
            if (passTemplate == nullptr)
            {
                AZ_Error("PassFactory", false, "FAILED TO CREATE PASS [%s]. Could not find pass template [%s]", passRequest->m_passName.GetCStr(), passRequest->m_templateName.GetCStr());
                return nullptr;
            }

            CreatorIndex index = FindCreatorIndex(passTemplate->m_passClass);
            if (!index.IsValid())
            {
                AZ_Error("PassFactory", false, "FAILED TO CREATE PASS [%s]. Could not find pass class [%s]", passRequest->m_passName.GetCStr(), passTemplate->m_passClass.GetCStr());
                return nullptr;
            }

            return CreatePassFromIndex(index, passRequest->m_passName, passTemplate, passRequest);
        }

    }   // namespace RPI
}   // namespace AZ
