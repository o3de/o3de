/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#include <Editor/EditorClassConverters.h>
#include <Source/EditorColliderComponent.h>
#include <PhysX/MeshAsset.h>
#include <AzCore/Asset/AssetSerializer.h>
#include <AzCore/Math/Matrix3x3.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>

namespace PhysX
{
    namespace ClassConverters
    {
        template <class T>
        bool FindElementAndGetData(AZ::SerializeContext::DataElementNode& dataElementNode, AZ::Crc32 fieldCrc, T& outValue)
        {
            const int index = dataElementNode.FindElement(fieldCrc);
            if (index == -1)
            {
                return false;
            }
            dataElementNode.GetSubElement(index).GetData<T>(outValue);
            return true;
        }

        template <class T>
        bool FindElementRecursiveAndGetData(AZ::SerializeContext::DataElementNode& recursiveRootNode, AZ::Crc32 fieldCrc, T& outValue)
        {
            bool result = false;
            const int index = recursiveRootNode.FindElement(fieldCrc);
            if (index != -1)
            {
                recursiveRootNode.GetSubElement(index).GetData<T>(outValue);
                return true;
            }

            int numSubElements = recursiveRootNode.GetNumSubElements();
            for (int subElementIndex = 0; subElementIndex < numSubElements; subElementIndex++)
            {
                result |= FindElementRecursiveAndGetData(recursiveRootNode.GetSubElement(subElementIndex), fieldCrc, outValue);
            }

            return result;
        }

        Physics::ColliderConfiguration FindColliderConfig(AZ::SerializeContext::DataElementNode& node)
        {
            // This function is only meant to be used for the other deprecation functions in this file.
            // Any new upgrade functions should steer clear of this, as it is not handling collision groups
            // correctly. But it is left here to maintain backwards compatibility.
            Physics::ColliderConfiguration colliderConfig;
            FindElementRecursiveAndGetData(node, AZ_CRC("CollisionLayer", 0x39931633), colliderConfig.m_collisionLayer);
            FindElementRecursiveAndGetData(node, AZ_CRC("Trigger", 0x1a6b0f5d), colliderConfig.m_isTrigger);
            FindElementRecursiveAndGetData(node, AZ_CRC("Rotation", 0x297c98f1), colliderConfig.m_rotation);
            FindElementRecursiveAndGetData(node, AZ_CRC("Position", 0x462ce4f5), colliderConfig.m_position);
            return colliderConfig;
        }

        bool ConvertToNewEditorColliderComponent(AZ::SerializeContext& context,
            AZ::SerializeContext::DataElementNode& classElement, EditorProxyShapeConfig& shapeConfig)
        {
            // collision group id
            AzPhysics::CollisionGroups::Id collisionGroupId;
            FindElementRecursiveAndGetData(classElement, AZ_CRC("CollisionGroupId", 0x84fe4bbe), collisionGroupId);

            // collider config
            Physics::ColliderConfiguration colliderConfig = FindColliderConfig(classElement);

            // convert to the new EditorColliderComponent and fill out the data
            const bool result = classElement.Convert(context, "{FD429282-A075-4966-857F-D0BBF186CFE6}");
            if (result)
            {
                classElement.AddElementWithData<AzPhysics::CollisionGroups::Id>(context, "CollisionGroupId", collisionGroupId);
                classElement.AddElementWithData<Physics::ColliderConfiguration>(context, "ColliderConfiguration", colliderConfig);
                classElement.AddElementWithData<EditorProxyShapeConfig>(context, "ShapeConfiguration", shapeConfig);
                AZ::Data::Asset<Pipeline::MeshAsset> meshAsset;
                if (shapeConfig.IsAssetConfig())
                {
                    FindElementAndGetData(classElement, AZ_CRC("PxMesh", 0xb369e226), meshAsset);
                }
                classElement.AddElementWithData<AZ::Data::Asset<Pipeline::MeshAsset>>(context, "MeshAsset", meshAsset);

                return true;
            }

            return false;
        }

        bool DeprecateEditorCapsuleColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // capsule specific geometry data
            Physics::CapsuleShapeConfiguration capsuleConfig;
            const int capsuleConfigIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (capsuleConfigIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& capsuleConfigNode = classElement.GetSubElement(capsuleConfigIndex);
            FindElementAndGetData(capsuleConfigNode, AZ_CRC("Height", 0xf54de50f), capsuleConfig.m_height);
            FindElementAndGetData(capsuleConfigNode, AZ_CRC("Radius", 0x3b7c6e5a), capsuleConfig.m_radius);

            EditorProxyShapeConfig shapeConfig(capsuleConfig);
            return ConvertToNewEditorColliderComponent(context, classElement, shapeConfig);
        }

        bool DeprecateEditorBoxColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // box specific geometry data
            Physics::BoxShapeConfiguration boxConfig;
            const int boxConfigIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (boxConfigIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& boxConfigNode = classElement.GetSubElement(boxConfigIndex);
            FindElementAndGetData(boxConfigNode, AZ_CRC("Configuration", 0xa5e2a5d7), boxConfig.m_dimensions);

            EditorProxyShapeConfig shapeConfig(boxConfig);
            return ConvertToNewEditorColliderComponent(context, classElement, shapeConfig);
        }

        bool DeprecateEditorSphereColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // sphere specific geometry data
            Physics::SphereShapeConfiguration sphereConfig;
            const int sphereConfigIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (sphereConfigIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& sphereConfigNode = classElement.GetSubElement(sphereConfigIndex);
            FindElementAndGetData(sphereConfigNode, AZ_CRC("Radius", 0x3b7c6e5a), sphereConfig.m_radius);

            EditorProxyShapeConfig shapeConfig(sphereConfig);
            return ConvertToNewEditorColliderComponent(context, classElement, shapeConfig);
        }

        bool DeprecateEditorMeshColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // native shape specific geometry data
            Physics::NativeShapeConfiguration nativeShapeConfig;
            const int nativeShapeConfigIndex = classElement.FindElement(AZ_CRC("Configuration", 0xa5e2a5d7));
            if (nativeShapeConfigIndex == -1)
            {
                return false;
            }

            AZ::SerializeContext::DataElementNode& nativeShapeConfigNode = classElement.GetSubElement(nativeShapeConfigIndex);
            FindElementAndGetData(nativeShapeConfigNode, AZ_CRC("Scale", 0xec462584), nativeShapeConfig.m_nativeShapeScale);

            EditorProxyShapeConfig shapeConfig(nativeShapeConfig);
            return ConvertToNewEditorColliderComponent(context, classElement, shapeConfig);
        }

        bool UpgradeEditorColliderComponent(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            // v1->v2
            if (dataElement.GetVersion() <= 1)
            {
                // Remove the collision group id field from the EditorColliderComponent
                const int index = dataElement.FindElement(AZ_CRC("CollisionGroupId", 0x84fe4bbe));
                AzPhysics::CollisionGroups::Id groupId;
                dataElement.GetChildData<AzPhysics::CollisionGroups::Id>(AZ_CRC("CollisionGroupId", 0x84fe4bbe), groupId);
                dataElement.RemoveElement(index);

                // Find the collider configuration on the EditorColliderComponent
                auto colliderConfigurationDataElement = dataElement.FindSubElement(AZ_CRC("ColliderConfiguration", 0x55d2e97a));

                // Replace the CollisionGroupId with the old one from the component
                auto collisionGroupIdDataElement = colliderConfigurationDataElement->FindSubElement(AZ_CRC("CollisionGroupId", 0x84fe4bbe));
                if (!collisionGroupIdDataElement)
                {
                    return false;
                }
                collisionGroupIdDataElement->SetData(context, groupId);
            }

            if (dataElement.GetVersion() <= 2)
            {
                // Find the shape configuration on the EditorColliderComponent
                AZ::SerializeContext::DataElementNode* shapeConfigurationElement = dataElement.FindSubElement(AZ_CRC("ShapeConfiguration", 0xe29d5a5c));
                if (!shapeConfigurationElement)
                {
                    return false;
                }

                Physics::ShapeType shapeType = Physics::ShapeType::Sphere;
                FindElementAndGetData(*shapeConfigurationElement, AZ_CRC("ShapeType", 0xf6826a28), shapeType);

                if (shapeType == Physics::ShapeType::PhysicsAsset)
                {
                    // Set the asset from the component to the configuration
                    AZ::Data::Asset<Pipeline::MeshAsset> meshAsset;
                    FindElementAndGetData(dataElement, AZ_CRC("MeshAsset", 0x2e843642), meshAsset);

                    AZ::SerializeContext::DataElementNode* assetConfigNode = shapeConfigurationElement->FindSubElement(AZ_CRC("PhysicsAsset", 0x4a3b5e62));
                    AZ::SerializeContext::DataElementNode* assetNode = assetConfigNode->FindSubElement(AZ_CRC("PhysicsAsset", 0x4a3b5e62));
                    assetNode->SetData<AZ::Data::Asset<AZ::Data::AssetData>>(context, meshAsset);
                }
            }


            if (dataElement.GetVersion() <= 5)
            {
                // version 6 moves the settings "DebugDraw" and "DebugDrawButtonState" into a separate object,
                // "DebugDrawSettings", which is owned by the editor collider component.
                //AZ::SerializeContext::DataElementNode* debugDrawElement = dataElement.FindSubElement(AZ_CRC("DebugDraw", 0x42ef6229));

                bool debugDraw = false;
                const int debugDrawIndex = dataElement.FindElement(AZ_CRC("DebugDraw", 0x42ef6229));
                if (debugDrawIndex != -1)
                {
                    dataElement.GetChildData<bool>(AZ_CRC("DebugDraw", 0x42ef6229), debugDraw);
                    dataElement.RemoveElement(debugDrawIndex);
                }

                bool debugDrawButtonState = false;
                const int debugDrawButtonStateIndex = dataElement.FindElement(AZ_CRC("DebugDrawButtonState", 0x7a4f440f));
                if (debugDrawButtonStateIndex != -1)
                {
                    dataElement.GetChildData<bool>(AZ_CRC("DebugDrawButtonState", 0x7a4f440f), debugDrawButtonState);
                    dataElement.RemoveElement(debugDrawButtonStateIndex);
                }

                dataElement.AddElement<DebugDraw::Collider>(context, "DebugDrawSettings");

                const int debugDrawSettingsIndex = dataElement.FindElement(AZ_CRC("DebugDrawSettings", 0xda74260a));
                if (debugDrawSettingsIndex != -1)
                {
                    AZ::SerializeContext::DataElementNode& debugDrawSettingsNode = dataElement.GetSubElement(debugDrawSettingsIndex);
                    debugDrawSettingsNode.AddElementWithData<bool>(context, "LocallyEnabled", debugDraw);
                    debugDrawSettingsNode.AddElementWithData<bool>(context, "GlobalButtonState", debugDrawButtonState);
                }
            }

            if (dataElement.GetVersion() <= 6)
            {
                // version 7 is just a version bump to force a recompile of dynamic slices because the runtime component
                // serialization changed.
            }


            // Mesh Asset and ShapeConfiguration moved so edit context is better for UX purposes
            if (dataElement.GetVersion() <= 7)
            {
                // Find the shape configuration on the EditorColliderComponent
                AZ::SerializeContext::DataElementNode* shapeConfigurationElement = dataElement.FindSubElement(AZ_CRC("ShapeConfiguration", 0xe29d5a5c));
                if (!shapeConfigurationElement)
                {
                    return false;
                }

                // Moved:
                //    EditorColliderComponent::MeshAsset                        -> EditorColliderComponent::ShapeConfiguration::PhysicsAsset::Asset
                //    EditorColliderComponent::ShapeConfiguration::PhysicsAsset -> EditorColliderComponent::ShapeConfiguration::PhysicsAsset::Configuration
                
                Physics::PhysicsAssetShapeConfiguration physAssetConfig;
                FindElementAndGetData(*shapeConfigurationElement, AZ_CRC("PhysicsAsset", 0x4a3b5e62), physAssetConfig);
                shapeConfigurationElement->RemoveElementByName(AZ_CRC("PhysicsAsset", 0x4a3b5e62));

                AZ::Data::Asset<Pipeline::MeshAsset> meshAsset;
                FindElementAndGetData(dataElement, AZ_CRC("MeshAsset", 0x2e843642), meshAsset);
                dataElement.RemoveElementByName(AZ_CRC("MeshAsset", 0x2e843642));

                EditorProxyAssetShapeConfig newAssetShapeConfig;
                newAssetShapeConfig.m_pxAsset = meshAsset;
                newAssetShapeConfig.m_configuration = physAssetConfig;

                shapeConfigurationElement->AddElementWithData(context, "PhysicsAsset", newAssetShapeConfig);
            }
            
            if (dataElement.GetVersion() <= 8)
            {
                dataElement.RemoveElementByName(AZ_CRC("LinkedRenderMeshAssetId", 0x466f4230));
            }

            if (dataElement.GetVersion() <= 9)
            {
                // version 9 is just a version bump to force a recompile of dynamic slices because the runtime component
                // serialization changed.
            }

            return true;
        }

        bool EditorProxyShapeConfigVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() <= 1)
            {
                // Remove the old NativeShape configuration
                Physics::NativeShapeConfiguration nativeShapeConfiguration;
                if (!FindElementAndGetData(classElement, AZ_CRC("Mesh", 0xe16f3a56), nativeShapeConfiguration))
                {
                    return false;
                }

                classElement.RemoveElementByName(AZ_CRC("Mesh", 0xe16f3a56));

                // Change shapeType from Native to PhysicsAsset
                Physics::ShapeType currentShapeType = Physics::ShapeType::Sphere;
                AZ::SerializeContext::DataElementNode* shapeTypeElement = classElement.FindSubElement(AZ_CRC("ShapeType", 0xf6826a28));
                if (shapeTypeElement && shapeTypeElement->GetData<Physics::ShapeType>(currentShapeType) &&
                    currentShapeType == Physics::ShapeType::Native)
                {
                    shapeTypeElement->SetData<Physics::ShapeType>(context, Physics::ShapeType::PhysicsAsset);
                 
                    // Insert PhysicsAsset configuration instead of NativeShape. Save the mesh scale.
                    Physics::PhysicsAssetShapeConfiguration assetConfiguration;
                    assetConfiguration.m_assetScale = nativeShapeConfiguration.m_nativeShapeScale;
                    
                    classElement.AddElementWithData<Physics::PhysicsAssetShapeConfiguration>(context, "PhysicsAsset", assetConfiguration);
                }
            }

            return true;
        }

        bool EditorRigidBodyConfigVersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Version 1 had a field "Inertia diagonal values" which was used to edit just the diagonal elements of the
            // inertia tensor, which is represented in the underlying Physics::RigidBodyConfiguration as a Matrix3x3.
            // Version 2 removes that field and instead uses a custom UI handler to allow editing of the diagonal elements.
            if (classElement.GetVersion() <= 1)
            {
                // Get the diagonal values from the old field
                AZ::Vector3 diagonalElements;
                if (FindElementAndGetData(classElement, AZ_CRC("Inertia diagonal values", 0x00d27839), diagonalElements))
                {
                    // Remove the old field
                    classElement.RemoveElementByName(AZ_CRC("Inertia diagonal values", 0x00d27839));

                    const int rigidBodyConfigIndex = classElement.FindElement(AZ_CRC("BaseClass1", 0xd4925735));
                    if (rigidBodyConfigIndex != -1)
                    {
                        AZ::SerializeContext::DataElementNode rigidBodyConfigElement = classElement.GetSubElement(rigidBodyConfigIndex);
                        // Update the inertia tensor
                        if (rigidBodyConfigElement.FindElement(AZ_CRC("Inertia tensor", 0xdd452265)) != -1)
                        {
                            AZ::Matrix3x3 inertiaTensor = AZ::Matrix3x3::CreateDiagonal(diagonalElements);
                            rigidBodyConfigElement.RemoveElementByName(AZ_CRC("Inertia tensor", 0xdd452265));
                            rigidBodyConfigElement.AddElementWithData<AZ::Matrix3x3>(context, "Inertia tensor", inertiaTensor);
                        }
                    }
                }
            }
            return true;
        }

        bool EditorTerrainComponentConverter([[maybe_unused]] AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            // Version 1 had a field 'ExportOnSave'.
            // This field was made redundant by the in-memory terrain asset introduced in version 2.
            if (classElement.GetVersion() <= 1)
            {
                classElement.RemoveElementByName(AZ_CRC("ExportOnSave", 0xa7865698));
            }

            return true;
        }
    } // namespace ClassConverters
} // namespace PhysX
