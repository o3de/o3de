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

            int nodeElementIndex = operatorBaseClass->FindElement(AZ_CRC("BaseClass1", 0xd4925735));

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
            AZ::SerializeContext::DataElementNode baseClassElement = (*classElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735)));            
            classElement.RemoveElementByName(AZ_CRC("BaseClass1", 0xd4925735));

            classElement.AddElement<ClassType>(serializeContext, "BaseClass1");

            AZ::SerializeContext::DataElementNode* newClassNode = classElement.FindSubElement(AZ_CRC("BaseClass1", 0xd4925735));

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
    };
}