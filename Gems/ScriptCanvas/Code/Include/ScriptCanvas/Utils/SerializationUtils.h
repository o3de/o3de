/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Serialization/SerializeContext.h>

namespace ScriptCanvas
{
    class SerializationUtils
    {
    public:

        // Removes one layer of base class from the reflection hiearchy.
        // i.e. takes Derivied -> Parent -> Grandparent and converts it to Derived -> Grandparent
        //
        // Will keep all data in Grandparent while discarding all data in Parent.
        static bool RemoveBaseClass([[maybe_unused]] AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement)
        {
            AZ::SerializeContext::DataElementNode* operatorBaseClass = classElement.FindSubElement(AZ::Crc32("BaseClass1"));

            if (operatorBaseClass == nullptr)
            {
                return false;
            }

            int nodeElementIndex = operatorBaseClass->FindElement(AZ_CRC_CE("BaseClass1"));

            if (nodeElementIndex < 0)
            {
                return false;
            }

            // The DataElementNode is being copied purposefully in this statement to clone the data
            AZ::SerializeContext::DataElementNode baseNodeElement = operatorBaseClass->GetSubElement(nodeElementIndex);

            classElement.RemoveElementByName(AZ::Crc32("BaseClass1"));
            classElement.AddElement(baseNodeElement);

            return true;
        }

        // Used to shim in a new base class i.e. A > C becomes A > B > C with InsertNewBaseClass<B>
        template<class ClassType>
        static bool InsertNewBaseClass(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement)
        {
            bool addedBaseClass = true;

            // Need to shim in a new class
            AZ::SerializeContext::DataElementNode baseClassElement = (*classElement.FindSubElement(AZ_CRC_CE("BaseClass1")));
            classElement.RemoveElementByName(AZ_CRC_CE("BaseClass1"));

            classElement.AddElement<ClassType>(serializeContext, "BaseClass1");

            AZ::SerializeContext::DataElementNode* newClassNode = classElement.FindSubElement(AZ_CRC_CE("BaseClass1"));

            if (newClassNode)
            {
                newClassNode->AddElement(baseClassElement);
            }
            else
            {
                addedBaseClass = false;
            }

            return addedBaseClass;
        }

        template<typename T>
        static bool GetElementData(AZ::SerializeContext& serializeContext, AZ::SerializeContext::DataElementNode& classElement, T& outData, uint32_t crc)
        {
            AZ::SerializeContext::DataElementNode* dataElement = classElement.FindSubElement(crc);
            if (!dataElement)
            {
                return false;
            }

            if (!dataElement->GetData(outData))
            {
                return false;
            }

            return true;
        }

        static AZ::SerializeContext::DataElementNode* FindFirstSubElement(AZ::SerializeContext::DataElementNode& classElement, uint32_t crc, int depthLimit = 10)
        {
            if (depthLimit <= 0)
            {
                return nullptr;
            }

            if (auto element = classElement.FindSubElement(crc))
            {
                return element;
            }

            for (int index = 0; index < classElement.GetNumSubElements(); index ++)
            {
                if (auto element = FindFirstSubElement(classElement.GetSubElement(index), crc, depthLimit - 1))
                {
                    return element;
                }
            }

            return nullptr;
        }
    };
}
