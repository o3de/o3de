/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <SceneAPI/SceneUI/ManifestMetaInfoHandler.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>


namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ManifestMetaInfoHandler, SystemAllocator);

            ManifestMetaInfoHandler::ManifestMetaInfoHandler()
            {
                BusConnect();
            }

            ManifestMetaInfoHandler::~ManifestMetaInfoHandler()
            {
                BusDisconnect();
            }

            void ManifestMetaInfoHandler::GetIconPath(AZStd::string& iconPath, const DataTypes::IManifestObject& target)
            {
                // Icons for classes that don't have edit serialize contexts
                if (target.RTTI_IsTypeOf(DataTypes::IMeshGroup::TYPEINFO_Uuid()))
                {
                    iconPath = ":/SceneUI/Manifest/MeshGroupIcon.svg";
                }
                else if (target.RTTI_IsTypeOf(DataTypes::ISkeletonGroup::TYPEINFO_Uuid()))
                {
                    iconPath = ":/SceneUI/Manifest/SkeletonGroupIcon.png";
                }
                else if (target.RTTI_IsTypeOf(DataTypes::ISkinGroup::TYPEINFO_Uuid()))
                {
                    iconPath = ":/SceneUI/Manifest/SkinGroupIcon.png";
                }
                else if (target.RTTI_IsTypeOf(DataTypes::IAnimationGroup::TYPEINFO_Uuid()))
                {
                    iconPath = ":/SceneUI/Manifest/AnimationGroupIcon.png";
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
