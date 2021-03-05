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

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class ISkeletonGroup;
        }
        namespace Events
        {
            class ExportProductList;
        }
    }

    namespace RC
    {
        // Called to export a specific Skeleton (Chr) Group
        struct ChrGroupExportContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(ChrGroupExportContext, "{294BB98B-DE9D-4B03-B219-A8A94657E81E}", SceneAPI::Events::ICallContext);

            ChrGroupExportContext(SceneAPI::Events::ExportEventContext& parent,
                const SceneAPI::DataTypes::ISkeletonGroup& group, Phase phase);
            ChrGroupExportContext(SceneAPI::Events::ExportProductList& products, const SceneAPI::Containers::Scene& scene, 
                const AZStd::string& outputDirectory, const SceneAPI::DataTypes::ISkeletonGroup& group, Phase phase);
            ChrGroupExportContext(const ChrGroupExportContext& copyContent, Phase phase);
            ChrGroupExportContext(const ChrGroupExportContext& copyContent) = delete;
            ~ChrGroupExportContext() override = default;

            ChrGroupExportContext& operator=(const ChrGroupExportContext& other) = delete;

            SceneAPI::Events::ExportProductList& m_products;
            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_outputDirectory;
            const SceneAPI::DataTypes::ISkeletonGroup& m_group;
            const Phase m_phase;
        };
    }
}