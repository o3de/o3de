/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <algorithm>
#include <AzCore/RTTI/ReflectContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPIExt/Data/LodNodeSelectionList.h>


namespace EMotionFX
{
    namespace Pipeline
    {
        namespace Data
        {
            bool LodNodeSelectionList::ContainsNode(const AZStd::string & nodeName) const
            {
                for (const AZStd::string& nodePath : m_selectedNodes)
                {
                    if (nodePath.find(nodeName) != AZStd::string::npos)
                    {
                        return true;
                    }
                }
                return false;
            }

            AZStd::unique_ptr<SceneDataTypes::ISceneNodeSelectionList> LodNodeSelectionList::Copy() const
            {
                return AZStd::make_unique<LodNodeSelectionList>(*this);
            }

            void LodNodeSelectionList::Reflect(AZ::ReflectContext* context)
            {
                AZ::SerializeContext* serializeContext = azrtti_cast<AZ::SerializeContext*>(context);
                if (!serializeContext)
                {
                    return;
                }

                serializeContext->Class<LodNodeSelectionList, SceneNodeSelectionList>()->Version(2)
                    ->Field("lodLevel", &LodNodeSelectionList::m_lodLevel);

                AZ::EditContext* editContext = serializeContext->GetEditContext();
                if (editContext)
                {
                    editContext->Class<LodNodeSelectionList>("LOD Node selection list", "Select node for each LOD.")
                        ->ClassElement(AZ::Edit::ClassElements::EditorData, "")
                        ->Attribute("NarrowSelection", true)
                        ->Attribute("FilterName", "joints")
                        ->Attribute("FilterType", azrtti_typeid<SceneDataTypes::IBoneData>())
                        ->Attribute("HideUncheckable", true);
                }
            }
        }
    }
}
