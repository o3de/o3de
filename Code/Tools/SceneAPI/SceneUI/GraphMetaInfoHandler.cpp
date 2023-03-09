/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/GraphMetaInfoHandler.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IMeshData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/IBoneData.h>
#include <SceneAPI/SceneCore/DataTypes/GraphData/ITransform.h>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(GraphMetaInfoHandler, SystemAllocator);

            GraphMetaInfoHandler::GraphMetaInfoHandler()
            {
                BusConnect();
            }

            GraphMetaInfoHandler::~GraphMetaInfoHandler()
            {
                BusDisconnect();
            }

            void GraphMetaInfoHandler::GetIconPath(AZStd::string& iconPath, const DataTypes::IGraphObject* target)
            {
                if (target->RTTI_IsTypeOf(DataTypes::IMeshData::TYPEINFO_Uuid()))
                {
                    iconPath = ":/SceneUI/Graph/MeshIcon.png";
                }
                else if (target->RTTI_IsTypeOf(DataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    iconPath = ":/SceneUI/Graph/BoneIcon.png";
                }
            }

            void GraphMetaInfoHandler::GetToolTip(AZStd::string& toolTip, const DataTypes::IGraphObject* target)
            {
                if (target->RTTI_IsTypeOf(DataTypes::ITransform::TYPEINFO_Uuid()))
                {
                    toolTip = "Transform information changes the translation, rotation and/or scale. Multiple transform will be added together.";
                }
                else if (target->RTTI_IsTypeOf(DataTypes::IMeshData::TYPEINFO_Uuid()))
                {
                    toolTip = "MeshData contains the vertex information to create the mesh for the 3D model.";
                }
                else if (target->RTTI_IsTypeOf(DataTypes::IBoneData::TYPEINFO_Uuid()))
                {
                    toolTip = "Bones make up an animation skeleton. Usually bones are hierarchically chained together and the root bone will be available for selection.";
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
