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
#include <AzCore/std/string/string.h>
#include <SceneAPI/SceneCore/Events/CallProcessorBus.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>
#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>
#include <CryHeaders.h>

class CContentCGF;
struct CNodeCGF;
class CMesh;

namespace AZ
{
    namespace SceneAPI
    {
        namespace DataTypes
        {
            class IGroup;
            class IMeshGroup;
        }
        namespace Events
        {
            class ExportProductList;
        }
    }
    namespace RC
    {
        // Called to export a specific Mesh (Cgf) Group.
        struct CgfGroupExportContext
            : public SceneAPI::Events::ICallContext
        {
            AZ_RTTI(CgfGroupExportContext, "{974FF54E-9724-4E8B-A2A1-D360C3B3DA80}", SceneAPI::Events::ICallContext);

            CgfGroupExportContext(SceneAPI::Events::ExportEventContext& parent,
                const SceneAPI::DataTypes::IMeshGroup& group, Phase phase);
            CgfGroupExportContext(SceneAPI::Events::ExportProductList& products, const SceneAPI::Containers::Scene& scene, const AZStd::string& outputDirectory,
                const SceneAPI::DataTypes::IMeshGroup& group, Phase phase);
            CgfGroupExportContext(const CgfGroupExportContext& copyContext, Phase phase);
            CgfGroupExportContext(const CgfGroupExportContext& copyContext) = delete;
            ~CgfGroupExportContext() override = default;

            CgfGroupExportContext& operator=(const CgfGroupExportContext& other) = delete;

            SceneAPI::Events::ExportProductList& m_products;
            const SceneAPI::Containers::Scene& m_scene;
            const AZStd::string& m_outputDirectory;
            const SceneAPI::DataTypes::IMeshGroup& m_group;
            const Phase m_phase; 
        };
    } // RC
} // AZ
