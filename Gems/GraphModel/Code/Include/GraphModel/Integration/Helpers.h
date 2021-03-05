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

#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

namespace GraphModelIntegration
{
    namespace Attributes
    {
        const static AZ::Crc32 TitlePaletteOverride = AZ_CRC("TitlePaletteOverride", 0x2faad537);
    }

    class Helpers
    {
    public:
        //! Helper method to retrieve the TitlePaletteOverride attribute (if exists) set on
        //! a given AZ type class, that will also check any base class that it is derived from
        static AZStd::string GetTitlePaletteOverride(const AZ::TypeId& typeId)
        {
            AZ::SerializeContext* serializeContext = nullptr;
            AZ::ComponentApplicationBus::BroadcastResult(serializeContext, &AZ::ComponentApplicationRequests::GetSerializeContext);
            AZ_Assert(serializeContext, "Failed to acquire application serialize context.");

            AZStd::string paletteOverride;

            const AZ::SerializeContext::ClassData* derivedClassData = serializeContext->FindClassData(typeId);
            if (!derivedClassData)
            {
                return paletteOverride;
            }

            // Use the EnumHierarchy API to retrive a list of TypeIds that this class derives from,
            // starting with the actual type and going backwards
            AZStd::vector<AZ::TypeId> typeIds;
            if (derivedClassData->m_azRtti)
            {
                derivedClassData->m_azRtti->EnumHierarchy(&RttiEnumHeirarchyHelper, &typeIds);
            }

            // Look through all the derived TypeIds to see if the TitlePaletteOverride attribute
            // was set in the EditContext at any level
            for (auto currentTypeId : typeIds)
            {
                auto classData = serializeContext->FindClassData(currentTypeId);
                if (classData)
                {
                    if (classData->m_editData)
                    {
                        const AZ::Edit::ElementData* elementData = classData->m_editData->FindElementData(AZ::Edit::ClassElements::EditorData);
                        if (elementData)
                        {
                            if (auto titlePaletteAttribute = elementData->FindAttribute(Attributes::TitlePaletteOverride))
                            {
                                AZ::AttributeReader nameReader(nullptr, titlePaletteAttribute);
                                nameReader.Read<AZStd::string>(paletteOverride);
                            }
                        }
                    }
                }
            }

            return paletteOverride;
        }

    private:
        //! Callback method needed for IRttiHelper::EnumHierarchy that gets invoked at every level
        //! allowing us to build a list of each TypeId it encounters
        static void RttiEnumHeirarchyHelper(const AZ::TypeId& typeId, void* userData)
        {
            AZStd::vector<AZ::TypeId>* typeIds = reinterpret_cast<AZStd::vector<AZ::TypeId>*>(userData);
            typeIds->push_back(typeId);
        }
    };
}
