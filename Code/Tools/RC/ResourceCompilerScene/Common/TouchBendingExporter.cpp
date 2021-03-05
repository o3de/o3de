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

#include <Cry_Geo.h> // Needed for CGFContent.h
#include <CGFContent.h>
#include <PropertyHelpers.h>

#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzFramework/StringFunc/StringFunc.h>
#include <AzToolsFramework/Debug/TraceContext.h>
#include <SceneAPI/SceneCore/Containers/Scene.h>
#include <SceneAPI/SceneCore/Containers/SceneGraph.h>
#include <SceneAPI/SceneCore/DataTypes/Rules/ITouchBendingRule.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/Containers/Views/PairIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphChildIterator.h>
#include <SceneAPI/SceneCore/Containers/Views/SceneGraphDownwardsIterator.h>
#include <SceneAPI/SceneCore/Containers/Utilities/Filters.h>
#include <SceneAPI/SceneCore/Utilities/SceneGraphSelector.h>
#include <SceneAPI/SceneCore/Utilities/Reporting.h>
#include <RC/ResourceCompilerScene/Common/ExportContextGlobal.h>
#include <RC/ResourceCompilerScene/Common/CommonExportContexts.h>
#include <RC/ResourceCompilerScene/Common/TouchBendingExporter.h>

#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h> //Needed by CgfExportContexts.h
#include <RC/ResourceCompilerScene/Cgf/CgfExportContexts.h>
#include <RC/ResourceCompilerScene/Cgf/CgfUtils.h>

namespace AZ
{
    namespace RC
    {
        namespace SceneEvents = AZ::SceneAPI::Events;
        namespace SceneDataTypes = AZ::SceneAPI::DataTypes;
        namespace SceneContainers = AZ::SceneAPI::Containers;
        namespace SceneUtil = AZ::SceneAPI::Utilities;
        namespace SceneViews = AZ::SceneAPI::Containers::Views;
        namespace AzStringFunc = AzFramework::StringFunc;

        TouchBendingExporter::TouchBendingExporter()
            : AZ::SceneAPI::SceneCore::RCExportingComponent()
        {
            BindToCall(&TouchBendingExporter::ConfigureContainer);
            BindToCall(&TouchBendingExporter::ProcessSkinnedMesh);
        }

        void TouchBendingExporter::Reflect(AZ::ReflectContext* context)
        {
            AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
            if (serializeContext)
            {
                serializeContext->Class<TouchBendingExporter, AZ::SceneAPI::SceneCore::RCExportingComponent>()->Version(1);
            }
        }

        SceneEvents::ProcessingResult TouchBendingExporter::ConfigureContainer(ContainerExportContext& context)
        {
            switch (context.m_phase)
            {
            case Phase::Filling:
            {
                AZStd::shared_ptr<const SceneDataTypes::ITouchBendingRule> touchBendingRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::ITouchBendingRule>();
                if (!touchBendingRule)
                {
                    return SceneEvents::ProcessingResult::Ignored;
                }
                const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
                AZStd::vector<AZStd::string> noCollideTargetNodes = SceneUtil::SceneGraphSelector::GenerateTargetNodes(graph, touchBendingRule->GetSceneNodeSelectionList(), SceneUtil::SceneGraphSelector::IsMesh);
                ProcessMeshType(context, context.m_container, noCollideTargetNodes, PHYS_GEOM_TYPE_NO_COLLIDE);
            }
            return SceneEvents::ProcessingResult::Success;
            case Phase::Finalizing:
            {
                AZStd::shared_ptr<const SceneDataTypes::ITouchBendingRule> touchBendingRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::ITouchBendingRule>();
                if (!touchBendingRule)
                {
                    return SceneEvents::ProcessingResult::Ignored;
                }
                //Let's make sure we have valid CSkinningInfo, otherwise,
                //there's no point in adding the Bone Tree helper nodes.
                CSkinningInfo* skinningInfo = context.m_container.GetSkinningInfo();
                if (skinningInfo->m_arrBonesDesc.size() < 1)
                {
                    return SceneEvents::ProcessingResult::Ignored;
                }
                AZStd::string rootBoneName = touchBendingRule->GetRootBoneName();
                AddHelperBoneNodes(context, context.m_container, rootBoneName,
                    touchBendingRule->ShouldOverrideDamping(),   touchBendingRule->GetOverrideDamping(),
                    touchBendingRule->ShouldOverrideStiffness(), touchBendingRule->GetOverrideStiffness(),
                    touchBendingRule->ShouldOverrideThickness(), touchBendingRule->GetOverrideThickness());
            }
            return SceneEvents::ProcessingResult::Success;
            default:
                return SceneEvents::ProcessingResult::Ignored;
            }
        }

        SceneEvents::ProcessingResult TouchBendingExporter::ProcessSkinnedMesh(AZ::RC::MeshNodeExportContext& context)
        {
            if (context.m_physicalizeType != PHYS_GEOM_TYPE_NONE)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZStd::shared_ptr<const SceneDataTypes::ITouchBendingRule> touchBendingRule = context.m_group.GetRuleContainerConst().FindFirstByType<SceneDataTypes::ITouchBendingRule>();
            if (!touchBendingRule)
            {
                return SceneEvents::ProcessingResult::Ignored;
            }

            AZStd::string rootBoneName = touchBendingRule->GetRootBoneName();
            SceneEvents::ProcessingResult result = SceneEvents::ProcessingResult::Ignored;
            switch (context.m_phase)
            {
                case  Phase::Filling:
                    result = SceneEvents::Process<TouchBendableMeshNodeExportContext>(
                        context, rootBoneName, Phase::Filling);
                    break;
                case Phase::Construction:
                {
                    //Add the Bones to CSkinningInfo only if they have not been added already.
                    CSkinningInfo* skinningInfo = context.m_container.GetSkinningInfo();
                    if (skinningInfo->m_arrBonesDesc.size() == 0)
                    {
                        result = SceneEvents::Process<AddBonesToSkinningInfoContext>(
                            *skinningInfo, context.m_scene, rootBoneName);
                    }
                }
                break;
                default:
                    break;
            }

            return result;
        }

        /*!
          The format is based on Cry's PropertyHelpers::SetPropertyValue.
          This version doesn't do any nullptr checking or white space trimming,
          because those errors are guaranteed not to happen.
        */
        static void AddPropertyValue(AZStd::string& inoutPropertiesString, const char* propertyName, float value, const char * propertySeparator)
        {
            char valueStr[16];
            snprintf(valueStr, sizeof(valueStr), "%f", value);

            AzStringFunc::Append(inoutPropertiesString, propertyName);
            AzStringFunc::Append(inoutPropertiesString, '=');
            AzStringFunc::Append(inoutPropertiesString, valueStr);
            if (propertySeparator)
            {
                AzStringFunc::Append(inoutPropertiesString, propertySeparator);
            }
        }

        bool TouchBendingExporter::AddHelperBoneNodes(AZ::RC::ContainerExportContext& context, CContentCGF& content, AZStd::string& rootBoneName,
            [[maybe_unused]] bool shouldOverrideDamping,   float damping,
            [[maybe_unused]] bool shouldOverrideStiffness, float stiffness,
            [[maybe_unused]] bool shouldOverrideThickness, float thickness)
        {
            AZ_TraceContext("AddHelperBoneNodes() rootBoneName:", rootBoneName);

            if (rootBoneName.empty())
            {
                AZ_TracePrintf(TraceWindowName, "Root bone name cannot be empty.");
                return false;
            }

            const SceneContainers::SceneGraph& graph = context.m_scene.GetGraph();
            SceneContainers::SceneGraph::NodeIndex nodeIndex = graph.Find(rootBoneName);
            if (!nodeIndex.IsValid())
            {
                AZ_TracePrintf(TraceWindowName, "Unable to find root bone in scene graph.");
                return false;
            }

            auto contentStorage = graph.GetContentStorage();
            auto nameStorage = graph.GetNameStorage();
            auto pairView = SceneViews::MakePairView(contentStorage, nameStorage);
            auto view = SceneViews::MakeSceneGraphDownwardsView<SceneViews::DepthFirst>(graph, nodeIndex, pairView.begin(), true);
            int index = 0;

            //Once SceneAPI supports Per Node Attributes, the string with properties
            //should be built per Node. In the meantime, because all the properties are
            //the same for all nodes, the string can be built once.
            AZStd::string nodeProperties;
            AddPropertyValue(nodeProperties, NODE_PROPERTY_DAMPING,   damping,   "\r\n");
            AddPropertyValue(nodeProperties, NODE_PROPERTY_STIFFNESS, stiffness, "\r\n");
            AddPropertyValue(nodeProperties, NODE_PROPERTY_THICKNESS, thickness, nullptr);
            for (auto it = view.begin(); it != view.end(); ++it)
            {
                if (it->first && it->first->RTTI_IsTypeOf(SceneDataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    //These very dummy nodes, are only used to define the name
                    //of the spines. It is not necessary to set transform matrices,
                    //nor it is relevant to set parent pointers, etc.
                    CNodeCGF* nodeCgf = new CNodeCGF();
                    SetNodeName(it->second.GetName(), *nodeCgf);
                    nodeCgf->type = CNodeCGF::NODE_HELPER;
                    nodeCgf->helperType = HP_POINT;
                    nodeCgf->properties = nodeProperties.c_str();
                    content.AddNode(nodeCgf);
                }
                else
                {
                    // End of bone chain or interruption in the bone chain. In both cases stop looking into this part of hierarchy further.
                    it.IgnoreNodeDescendants();
                }
            }

            return true;
        }
    } // namespace RC
} // namespace AZ
