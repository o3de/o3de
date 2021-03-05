#pragma once

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

#include <AzCore/RTTI/RTTI.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>

struct CSkinningInfo;

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISkinGroup;
        }
    }

    namespace RC
    {
        // Called to export a specific Skin Group
        struct SkinGroupExportContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(SkinGroupExportContext, "{F2C0DF6D-84F7-4692-9626-C981FA599755}", SceneAPI::Events::ICallContext);

            SkinGroupExportContext(SceneAPI::Events::ExportEventContext& parent, 
                const SceneAPI::DataTypes::ISkinGroup& group, Phase phase);
            SkinGroupExportContext(SceneAPI::Events::ExportProductList& products, const SceneAPI::Containers::Scene& scene, 
                const AZStd::string& outputDirectory, const SceneAPI::DataTypes::ISkinGroup& group, Phase phase);
            SkinGroupExportContext(const SkinGroupExportContext& copyContent, Phase phase);
            SkinGroupExportContext(const SkinGroupExportContext& copyContent) = delete;
            ~SkinGroupExportContext() override = default;

            SkinGroupExportContext& operator=(const SkinGroupExportContext& other) = delete;

            SceneAPI::Events::ExportProductList& m_products;
            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_outputDirectory;
            const SceneAPI::DataTypes::ISkinGroup& m_group;
            const Phase m_phase;
        };
    }
}