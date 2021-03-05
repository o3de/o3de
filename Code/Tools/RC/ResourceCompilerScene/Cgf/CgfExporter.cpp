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

#include <RC/ResourceCompilerScene/Cgf/CgfExporter.h>
#include <Cry_Geo.h>
#include <ConvertContext.h>
#include <CGFContent.h>

#include <AzToolsFramework/Debug/TraceContext.h>

#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneManifest.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/Events/ExportEventContext.h>

#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;

        CgfExporter::CgfExporter(IConvertContext* convertContext)
            : m_convertContext(convertContext)
        {
            BindToCall(&CgfExporter::ProcessContext);
            ActivateBindings();
        }

        SceneEvents::ProcessingResult CgfExporter::ProcessContext(SceneEvents::ExportEventContext& context)
        {
            AZ_TraceContext("Scene name", context.GetScene().GetName());
            AZ_TraceContext("Source file", context.GetScene().GetSourceFilename());
            AZ_TraceContext("Output path", context.GetOutputDirectory());

            const SceneContainers::SceneManifest& manifest = context.GetScene().GetManifest();
            auto valueStorage = manifest.GetValueStorage();
            auto view = SceneContainers::MakeDerivedFilterView<SceneDataTypes::IMeshGroup>(valueStorage);

            SceneEvents::ProcessingResultCombiner result;
            for (const SceneDataTypes::IMeshGroup& meshGroup : view)
            {
                AZ_TraceContext("Mesh group", meshGroup.GetName());
                result += SceneEvents::Process<CgfGroupExportContext>(context, meshGroup, Phase::Construction);
                result += SceneEvents::Process<CgfGroupExportContext>(context, meshGroup, Phase::Filling);
                result += SceneEvents::Process<CgfGroupExportContext>(context, meshGroup, Phase::Finalizing);
            }
            return result.GetResult();
        }
    } // namespace RC
} // namespace AZ
