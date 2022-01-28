/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/Math/Crc.h>
#include <AzCore/Math/Vector2.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Color.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Math/Sfmt.h>
#include <AzCore/Component/Entity.h>

#include <LyShine/UiAssetTypes.h>
#include <LyShine/UiBase.h>

////////////////////////////////////////////////////////////////////////////////////////////////////

namespace LyShine
{
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert an AZStd::string field to a simple asset reference
    // Inline to avoid DLL linkage issues
    template<typename T>
    inline bool ConvertSubElementFromAzStringToAssetRef(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* subElementName)
    {
        int index = classElement.FindElement(AZ_CRC(subElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            AZStd::string oldData;

            if (!elementNode.GetData(oldData))
            {
                // Error, old subElement was not a string or not valid
                AZ_Error("Serialization", false, "Cannot get string data for element %s.", subElementName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add a new element for the new data.
            int simpleAssetRefIndex = classElement.AddElement<AzFramework::SimpleAssetReference<T> >(context, subElementName);
            if (simpleAssetRefIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for simpleAssetRefIndex %s", subElementName);
                return false;
            }

            // add a sub element for the SimpleAssetReferenceBase within the SimpleAssetReference
            AZ::SerializeContext::DataElementNode& simpleAssetRefNode = classElement.GetSubElement(simpleAssetRefIndex);
            int simpleAssetRefBaseIndex = simpleAssetRefNode.AddElement<AzFramework::SimpleAssetReferenceBase>(context, "BaseClass1");
            if (simpleAssetRefBaseIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element BaseClass1");
                return false;
            }

            // add a sub element for the AssetPath within the SimpleAssetReference
            AZ::SerializeContext::DataElementNode& simpleAssetRefBaseNode = simpleAssetRefNode.GetSubElement(simpleAssetRefBaseIndex);
            int assetPathElementIndex = simpleAssetRefBaseNode.AddElement<AZStd::string>(context, "AssetPath");
            if (assetPathElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element AssetPath");
                return false;
            }

            AZStd::string newData(oldData.c_str());
            simpleAssetRefBaseNode.GetSubElement(assetPathElementIndex).SetData(context, newData);
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert a char to a uint32_t
    // Inline to avoid DLL linkage issues
    inline bool ConvertSubElementFromCharToUInt32(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* subElementName)
    {
        int index = classElement.FindElement(AZ_CRC(subElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            char oldData;

            if (!elementNode.GetData(oldData))
            {
                // Error, old subElement was not a CryString
                AZ_Error("Serialization", false, "Element %s is not a char.", subElementName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add a new element for the new data.
            int newElementIndex = classElement.AddElement<uint32_t>(context, subElementName);
            if (newElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", subElementName);
                return false;
            }

            uint32_t newData = oldData;
            classElement.GetSubElement(newElementIndex).SetData(context, newData);
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to get the value for a named sub element
    template<typename T>
    inline bool GetSubElementValue(
        AZ::SerializeContext::DataElementNode& classElement,
        const char* elementName, T& value)
    {
        int index = classElement.FindElement(AZ_CRC(elementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);
            if (!elementNode.GetData(value))
            {
                return false;
            }
        }
        else
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to make a ColorF out of an RGB color and an alpha
    inline ColorF MakeColorF(AZ::Vector3 color, float alpha)
    {
        return ColorF(color.GetX(), color.GetY(), color.GetZ(), alpha);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to make an AZ::Vector3 from the rgb elements of a ColorF
    inline AZ::Vector3 MakeColorVector3(ColorF color)
    {
        return AZ::Vector3(color.r, color.g, color.b);
    }

    // Helper function to make an AZ::Color from a ColorF
    inline AZ::Color MakeColorAZColor(ColorF color)
    {
        return AZ::Color(color.r, color.g, color.b, color.a);
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert a ColorF field to a Vector3 color and a float
    // alpha.
    // Inline to avoid DLL linkage issues
    inline bool ConvertSubElementFromColorToColorPlusAlpha(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* colorElementName,
        const char* alphaElementName)
    {
        int index = classElement.FindElement(AZ_CRC(colorElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            ColorF oldData;

            if (!(GetSubElementValue(elementNode, "r", oldData.r) &&
                  GetSubElementValue(elementNode, "g", oldData.g) &&
                  GetSubElementValue(elementNode, "b", oldData.b) &&
                  GetSubElementValue(elementNode, "a", oldData.a)))
            {
                // Error, old subElement was not a ColorF
                AZ_Error("Serialization", false, "Element %s is not a ColorF.", colorElementName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add new elements for the new data.

            int newColorElementIndex = classElement.AddElement<AZ::Vector3>(context, colorElementName);
            if (newColorElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", colorElementName);
                return false;
            }

            int newAlphaElementIndex = classElement.AddElement<float>(context, alphaElementName);
            if (newAlphaElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", alphaElementName);
                return false;
            }

            AZ::Vector3 newColorData(oldData.r, oldData.g, oldData.b);
            classElement.GetSubElement(newColorElementIndex).SetData(context, newColorData);

            float newAlphaData(oldData.a);
            classElement.GetSubElement(newAlphaElementIndex).SetData(context, newAlphaData);
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert a AZ::Vector3 field to a AZ::Color
    // Inline to avoid DLL linkage issues
    inline bool ConvertSubElementFromVector3ToAzColor(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* colorElementName)
    {
        int index = classElement.FindElement(AZ_CRC(colorElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            AZ::Vector3 oldData;

            if (!elementNode.GetData(oldData))
            {
                // Error, old subElement was not a Vector3 or not valid
                AZ_Error("Serialization", false, "Cannot get Vector3 data for element %s.", colorElementName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add new elements for the new data.

            int newColorElementIndex = classElement.AddElement<AZ::Color>(context, colorElementName);
            if (newColorElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", colorElementName);
                return false;
            }

            AZ::Color newColorData = AZ::Color::CreateFromVector3(oldData);
            classElement.GetSubElement(newColorElementIndex).SetData(context, newColorData);
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert a ColorF field to a AZ::Color
    // Inline to avoid DLL linkage issues
    inline bool ConvertSubElementFromColorFToAzColor(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* colorElementName)
    {
        int index = classElement.FindElement(AZ_CRC(colorElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            ColorF oldData;

            if (!(GetSubElementValue(elementNode, "r", oldData.r) &&
                  GetSubElementValue(elementNode, "g", oldData.g) &&
                  GetSubElementValue(elementNode, "b", oldData.b) &&
                  GetSubElementValue(elementNode, "a", oldData.a)))
            {
                // Error, old subElement was not a ColorF
                AZ_Error("Serialization", false, "Element %s is not a ColorF.", colorElementName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add new elements for the new data.

            int newColorElementIndex = classElement.AddElement<AZ::Color>(context, colorElementName);
            if (newColorElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", colorElementName);
                return false;
            }

            classElement.GetSubElement(newColorElementIndex).SetData(context, MakeColorAZColor(oldData));
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to convert a Vec2 field to a AZ::Vector2.
    // Inline to avoid DLL linkage issues
    inline bool ConvertSubElementFromVec2ToVector2(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* vec2ElementName)
    {
        int index = classElement.FindElement(AZ_CRC(vec2ElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& elementNode = classElement.GetSubElement(index);

            Vec2 oldData;

            if (!(GetSubElementValue(elementNode, "x", oldData.x) &&
                GetSubElementValue(elementNode, "y", oldData.y)))
            {
                // Error, old subElement was not a Vec2
                AZ_Error("Serialization", false, "Element %s is not a Vec2.", vec2ElementName);
                return false;
            }

            // Remove old version.
            classElement.RemoveElement(index);

            // Add new elements for the new data.

            int newVector2ElementIndex = classElement.AddElement<AZ::Vector2>(context, vec2ElementName);
            if (newVector2ElementIndex == -1)
            {
                // Error adding the new sub element
                AZ_Error("Serialization", false, "AddElement failed for converted element %s", vec2ElementName);
                return false;
            }

            AZ::Vector2 newVector2Data(oldData.x, oldData.y);
            classElement.GetSubElement(newVector2ElementIndex).SetData(context, newVector2Data);
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to move a sub-element from one DataElementNode to another
    // DataElementNode and rename it
    inline bool MoveElement(
        [[maybe_unused]] AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& srcElement,
        AZ::SerializeContext::DataElementNode& dstElement,
        const char* srcSubElementName,
        const char* dstSubElementName)
    {
        // find the sub-element (the field we will move)
        int srcSubElementIndex = srcElement.FindElement(AZ_CRC(srcSubElementName));
        if (srcSubElementIndex != -1)
        {
            // add a copy of the sub-element into the base class
            AZ::SerializeContext::DataElementNode subElementNode = srcElement.GetSubElement(srcSubElementIndex);
            subElementNode.SetName(dstSubElementName);
            dstElement.AddElement(subElementNode);

            // remove the sub-element from its original location
            srcElement.RemoveElement(srcSubElementIndex);
        }

        // if the field did not exist then we do not report an error
        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to VersionConverter to remove leading forward slashes from asset path
    inline bool RemoveLeadingForwardSlashesFromAssetPath(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& classElement,
        const char* simpleAssetRefSubElementName)
    {
        int index = classElement.FindElement(AZ_CRC(simpleAssetRefSubElementName));
        if (index != -1)
        {
            AZ::SerializeContext::DataElementNode& simpleAssetRefNode = classElement.GetSubElement(index);
            index = simpleAssetRefNode.FindElement(AZ_CRC("BaseClass1", 0xd4925735));

            if (index != -1)
            {
                AZ::SerializeContext::DataElementNode& baseClassNode = simpleAssetRefNode.GetSubElement(index);
                index = baseClassNode.FindElement(AZ_CRC("AssetPath", 0x2c355179));

                if (index != -1)
                {
                    AZ::SerializeContext::DataElementNode& assetPathNode = baseClassNode.GetSubElement(index);
                    AZStd::string assetPath;

                    if (!assetPathNode.GetData(assetPath))
                    {
                        AZ_Error("Serialization", false, "Element AssetPath is not a AZStd::string.");
                        return false;
                    }

                    // Count leading forward slashes
                    int i = 0;
                    while ((i < assetPath.length()) && (assetPath[i] == '/'))
                    {
                        i++;
                    }
                    if (i > 0)
                    {
                        // Remove leading forward slashes
                        assetPath.erase(0, i);

                        if (!assetPathNode.SetData(context, assetPath))
                        {
                            AZ_Error("Serialization", false, "Unable to set AssetPath data.");
                            return false;
                        }
                    }
                }
            }
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to find a component with the given UUID in an entity
    inline AZ::SerializeContext::DataElementNode* FindComponentNode(AZ::SerializeContext::DataElementNode& entityNode, const AZ::Uuid& uuid)
    {
        // get the component vector node
        int componentsIndex = entityNode.FindElement(AZ_CRC("Components", 0xee48f5fd));
        if (componentsIndex == -1)
        {
            return nullptr;
        }
        AZ::SerializeContext::DataElementNode& componentsNode = entityNode.GetSubElement(componentsIndex);

        // search the components vector for the first component with the given ID
        int numComponents = componentsNode.GetNumSubElements();
        for (int compIndex = 0; compIndex < numComponents; ++compIndex)
        {
            AZ::SerializeContext::DataElementNode& componentNode = componentsNode.GetSubElement(compIndex);

            if (componentNode.GetId() == uuid)
            {
                return &componentNode;
            }
        }

        return nullptr;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to create an EntityId node for a newly created entity node
    inline bool CreateEntityIdNode(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& entityNode)
    {
        // Generate a new EntityId and extract the raw u64
        AZ::EntityId newEntityId = AZ::Entity::MakeId();
        AZ::u64 rawId = static_cast<AZ::u64>(newEntityId);

        // Create the EntityId node within this entity
        int entityIdIndex = entityNode.AddElement<AZ::EntityId>(context, "Id");
        if (entityIdIndex == -1)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode& elementIdNode = entityNode.GetSubElement(entityIdIndex);

        // Create the sub node of the EntityID that actually stores the u64
        int u64Index = elementIdNode.AddElementWithData(context, "id", rawId);
        if (u64Index == -1)
        {
            return false;
        }

        return true;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // Helper function to create the AZ::Component base class for a newly created component
    inline bool CreateComponentBaseClassNode(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& componentNode)
    {
        // Generate a new component ID (using same technique that the AZ::Component uses)
        AZ::ComponentId compId = AZ::Sfmt::GetInstance().Rand64();

        // Create the base class node within this component node
        int baseClassIndex = componentNode.AddElement<AZ::Component>(context, "BaseClass1");
        if (baseClassIndex == -1)
        {
            return false;
        }
        AZ::SerializeContext::DataElementNode& baseClassNode = componentNode.GetSubElement(baseClassIndex);

        // Create the sub node of the component base class that stores the u64 component id
        int u64Index = baseClassNode.AddElementWithData(context, "Id", compId);
        if (u64Index == -1)
        {
            return false;
        }

        return true;
    }

}
