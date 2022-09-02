/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzFramework/Physics/ClassConverters.h>
#include <AzFramework/Physics/Collision/CollisionGroups.h>
#include <AzFramework/Physics/Collision/CollisionLayers.h>
#include <AzFramework/Physics/Ragdoll.h>
#include <AzFramework/Physics/Shape.h>
#include <AzFramework/Physics/SystemBus.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/smart_ptr/make_shared.h>
#include <AzCore/std/utils.h>

namespace Physics
{
    namespace ClassConverters
    {
        bool RagdollNodeConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 2)
            {
                // this conversion is to deal with m_shapes in the RagdollNodeConfiguration changing from
                // AZStd::vector<AZStd::shared_ptr<ShapeConfiguration>> to
                // AZStd::vector<AZStd::pair<AZStd::shared_ptr<ColliderConfiguration>, AZStd::shared_ptr<ShapeConfiguration>>>,
                // and the collider related information from ShapeConfiguration moving to ColliderConfiguration
                const int shapesIndex = classElement.FindElement(AZ_CRC_CE("shapes"));
                if (shapesIndex != -1)
                {
                    AZ::SerializeContext::DataElementNode& shapesElement = classElement.GetSubElement(shapesIndex);

                    // copy the old shape config data before removing the original vector
                    AZStd::vector<AZ::SerializeContext::DataElementNode> shapesCopy;
                    const int numSubElements = shapesElement.GetNumSubElements();
                    shapesCopy.reserve(numSubElements);

                    for (int i = 0; i < numSubElements; i++)
                    {
                        AZ::SerializeContext::DataElementNode& sharedPtrElement = shapesElement.GetSubElement(i);

                        if (sharedPtrElement.GetNumSubElements() > 0)
                        {
                            AZ::SerializeContext::DataElementNode& shape = sharedPtrElement.GetSubElement(0);
                            shapesCopy.push_back(shape);
                        }
                    }

                    // remove the old vector
                    classElement.RemoveElement(shapesIndex);

                    // add a new vector in the new format
                    const int newShapesIndex = classElement.AddElement<AzPhysics::ShapeColliderPairList>(context, "shapes");
                    if (newShapesIndex != -1)
                    {
                        AZ::SerializeContext::DataElementNode& newShapesElement = classElement.GetSubElement(newShapesIndex);

                        // convert the old shapes into the new format and add to the vector
                        for (AZ::SerializeContext::DataElementNode shape : shapesCopy)
                        {
                            const int pairIndex = newShapesElement.AddElementWithData<AzPhysics::ShapeColliderPair>(
                                context, "element", AzPhysics::ShapeColliderPair());

                            AZ::SerializeContext::DataElementNode& pairElement = newShapesElement.GetSubElement(pairIndex);

                            ColliderConfiguration colliderConfig;
                            if (AZ::SerializeContext::DataElementNode* baseClassNode = shape.FindSubElement(AZ_CRC_CE("BaseClass1")))
                            {
                                baseClassNode->FindSubElementAndGetData(AZ_CRC_CE("Trigger"), colliderConfig.m_isTrigger);
                                baseClassNode->FindSubElementAndGetData(AZ_CRC_CE("Position"), colliderConfig.m_position);
                                baseClassNode->FindSubElementAndGetData(AZ_CRC_CE("Rotation"), colliderConfig.m_rotation);
                                baseClassNode->FindSubElementAndGetData(AZ_CRC_CE("CollisionLayer"), colliderConfig.m_collisionLayer);
                            }

                            shape.RemoveElementByName(AZ_CRC_CE("BaseClass1"));

                            pairElement.GetSubElement(0).AddElementWithData<ColliderConfiguration>(context, "element", colliderConfig);
                            pairElement.GetSubElement(1).AddElement(shape);
                        }
                    }
                }
            }

            // Don't remove the 'shapes' element here, even though it got removed with v3. The version converter converts elements bottom-up
            // which means the ragdoll node configs get converted before the ragdoll config. If we remove the shapes element here, we would
            // not be able to pass the shapes over to the character collider config. The ragdoll config version converter takes care of this.
            //if (classElement.GetVersion() < 3)
            //{
            //    classElement.RemoveElementByName(AZ_CRC_CE("shapes"));
            //}

            // Version 4 adds visibility settings to hide rigid body settings that aren't relevant for the animation editor.
            if (classElement.GetVersion() < 4)
            {
                const int rigidBodyConfigIndex = classElement.FindElement(AZ_CRC_CE("RigidBodyConfiguration"));
                if (rigidBodyConfigIndex != -1)
                {
                    AZ::SerializeContext::DataElementNode& rigidBodyConfigElement = classElement.GetSubElement(rigidBodyConfigIndex);
                    // in the animation editor we want to show inertia, damping, sleep, interpolation, gravity and CCD properties
                    // so the value should be (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 7) = 190
                    rigidBodyConfigElement.AddElementWithData<AZ::u16>(context, "Property Visibility Flags", 190);
                }
            }

            return true;
        }

        bool RagdollConfigConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement)
        {
            if (classElement.GetVersion() < 3)
            {
                CharacterColliderConfiguration newColliderConfig;

                AZ::SerializeContext::DataElementNode* ragdollNodeConfig = classElement.FindSubElement(AZ_CRC_CE("nodes"));
                if (ragdollNodeConfig)
                {
                    int numNodes = ragdollNodeConfig->GetNumSubElements();
                    for (int i = 0; i < numNodes; ++i)
                    {
                        AZ::SerializeContext::DataElementNode& nodeElement = ragdollNodeConfig->GetSubElement(i);

                        AZStd::string name;
                        AZ::SerializeContext::DataElementNode* baseClass1 = nodeElement.FindSubElement(AZ_CRC_CE("BaseClass1"));
                        if (baseClass1)
                        {
                            AZ::SerializeContext::DataElementNode* baseBaseClass1 = baseClass1->FindSubElement(AZ_CRC_CE("BaseClass1"));
                            if (baseBaseClass1 && baseBaseClass1->FindSubElementAndGetData<AZStd::string>(AZ_CRC_CE("name"), name))
                            {
                                AzPhysics::ShapeColliderPairList shapes;
                                if (nodeElement.FindSubElementAndGetData<AzPhysics::ShapeColliderPairList>(AZ_CRC_CE("shapes"), shapes))
                                {
                                    CharacterColliderNodeConfiguration newColliderNodeConfig;
                                    newColliderNodeConfig.m_name = name;
                                    newColliderNodeConfig.m_shapes = shapes;
                                    newColliderConfig.m_nodes.push_back(newColliderNodeConfig);
                                }
                            }
                        }

                        nodeElement.RemoveElementByName(AZ_CRC_CE("shapes"));
                    }
                }

                classElement.AddElementWithData(context, "colliders", newColliderConfig);
            }

            return true;
        }

        bool ColliderConfigurationConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& dataElement)
        {
            // version 1->2
            if (dataElement.GetVersion() <= 1)
            {
                // Convert collision group to group id
                dataElement.RemoveElementByName(AZ_CRC_CE("CollisionGroup"));
                dataElement.AddElement<AzPhysics::CollisionGroups::Id>(context, "CollisionGroupId");
            }

            // version 2->3
            if (dataElement.GetVersion() <= 2)
            {
                // Force all new colliders to have exclusive shapes
                dataElement.RemoveElementByName(AZ_CRC_CE("Exclusive"));
                dataElement.AddElementWithData<bool>(context, "Exclusive", true);
            }

            // version 3->4
            if (dataElement.GetVersion() <= 3)
            {
                const int elementIndex = dataElement.FindElement(AZ_CRC_CE("Trigger"));

                if (elementIndex >= 0)
                {
                    bool isTrigger = false;
                    AZ::SerializeContext::DataElementNode& triggerElement = dataElement.GetSubElement(elementIndex);
                    const bool found = triggerElement.GetData<bool>(isTrigger);

                    if (found && isTrigger)
                    {
                        // Version 4 added "InSceneQueries" field set to true by default.
                        // The field is applicable to both trigger and simulated shapes.
                        // However before all trigger shapes were always invisible to scene queries.
                        // Setting "In Scene Queries" to false for all existing triggers to avoid breaking the existing content.
                        const int idx = dataElement.AddElement<bool>(context, "InSceneQueries");
                        if (idx != -1)
                        {
                            if (!dataElement.GetSubElement(idx).SetData<bool>(context, false))
                            {
                                return false;
                            }
                        }
                    }
                }
            }

            return true;
        }
    } // namespace ClassConverters
} // namespace Physics
