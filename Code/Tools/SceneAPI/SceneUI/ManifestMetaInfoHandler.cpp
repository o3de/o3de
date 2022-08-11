/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <SceneAPI/SceneUI/ManifestMetaInfoHandler.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IMeshGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkeletonGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/ISkinGroup.h>
#include <SceneAPI/SceneCore/DataTypes/Groups/IAnimationGroup.h>

#include <AzToolsFramework/API/EditorAssetSystemAPI.h>

#include <QFile>

namespace AZ
{
    namespace SceneAPI
    {
        namespace UI
        {
            AZ_CLASS_ALLOCATOR_IMPL(ManifestMetaInfoHandler, SystemAllocator, 0)

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
                // Icons for scene node group nodes that have edit serialize contexts
                else if (target.RTTI_IsTypeOf(DataTypes::ISceneNodeGroup::TYPEINFO_Uuid()))
                {
                    const DataTypes::ISceneNodeGroup* sceneNodeGroup = azrtti_cast<const DataTypes::ISceneNodeGroup*>(&target);
                    
                    AZ::SerializeContext* serializeContext = nullptr;
                    AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationBus::Events::GetSerializeContext);
                    AZ_Assert(serializeContext, "No serialize context");

                    // Search the Edit Context for an icon.
                    // It will have been reflected like:
                    //  ->Attribute(AZ::Edit::Attributes::Icon, "Editor/Icons/MeshCollider.svg")
                    auto classData = serializeContext->FindClassData(target.RTTI_GetType());
                    if (classData && classData->m_editData)
                    {
                        auto editorElementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        if (editorElementData)
                        {
                            if (auto iconAttribute = editorElementData->FindAttribute(AZ::Edit::Attributes::Icon))
                            {
                                if (auto iconAttributeData = azdynamic_cast<const AZ::Edit::AttributeData<const char*>*>(iconAttribute))
                                {
                                    AZStd::string iconAttributeValue = iconAttributeData->Get(&sceneNodeGroup);
                                    if (!iconAttributeValue.empty())
                                    {
                                        iconPath = AZStd::move(iconAttributeValue);
                                        
                                        // The path is probably going to be relative to a scan directory,
                                        // especially if this node was defined in a Gem.
                                        // If a first check doesn't find the file, then see if the path can
                                        // be resolved via the asset system, and pull an absolute path from there.
                                        if (!QFile::exists(QString(iconPath.c_str())))
                                        {
                                            AZStd::string iconFullPath;
                                            bool pathFound = false;
                                            using AssetSysReqBus = AzToolsFramework::AssetSystemRequestBus;
                                            AssetSysReqBus::BroadcastResult(
                                                pathFound, &AssetSysReqBus::Events::GetFullSourcePathFromRelativeProductPath,
                                                iconPath, iconFullPath);
                                            if (pathFound)
                                            {
                                                iconPath = AZStd::move(iconFullPath);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } // SceneData
    } // SceneAPI
} // AZ
