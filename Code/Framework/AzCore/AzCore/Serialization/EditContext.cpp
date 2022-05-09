/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/EditContext.h>

namespace AZ
{
    //=========================================================================
    // EditContext
    // [10/26/2012]
    //=========================================================================
    EditContext::EditContext(SerializeContext& serializeContext)
        : m_serializeContext(serializeContext)
    {
    }

    //=========================================================================
    // ~EditContext
    // [10/26/2012]
    //=========================================================================
    EditContext::~EditContext()
    {
        for (ClassDataListType::iterator classIt = m_classData.begin(); classIt != m_classData.end(); ++classIt)
        {
            if (classIt->m_classData)
            {
                classIt->m_classData->m_editData = nullptr;
                classIt->m_classData = nullptr;
            }
            classIt->ClearElements();
        }
        for (auto& enumIt : m_enumData)
        {
            enumIt.second.ClearAttributes();
        }
        m_enumData.clear();
    }

    //=========================================================================
    // ~RemoveClassData
    //=========================================================================
    void EditContext::RemoveClassData(SerializeContext::ClassData* classData)
    {
        Edit::ClassData* data = classData->m_editData;
        if (data)
        {
            data->m_classData->m_editData = nullptr;
            data->m_classData = nullptr;
            data->ClearElements();
            for (auto editClassDataIt = m_classData.begin(); editClassDataIt != m_classData.end(); ++editClassDataIt)
            {
                if (&(*editClassDataIt) == data)
                {
                    m_classData.erase(editClassDataIt);
                    break;
                }
            }
        }
    }

    //=========================================================================
    // GetEnumElementData
    //=========================================================================
    const Edit::ElementData* EditContext::GetEnumElementData(const AZ::Uuid& enumId) const
    {
        auto enumIt = m_enumData.find(enumId);
        const Edit::ElementData* data = nullptr;
        if (enumIt != m_enumData.end())
        {
            data = &enumIt->second;
        }
        return data;
    }

    namespace Edit
    {
        void GetComponentUuidsWithSystemComponentTag(
            const SerializeContext* serializeContext,
            const AZStd::vector<AZ::Crc32>& requiredTags,
            AZStd::unordered_set<AZ::Uuid>& componentUuids)
        {
            componentUuids.clear();

            serializeContext->EnumerateAll(
                [&requiredTags, &componentUuids](const AZ::SerializeContext::ClassData* data, const AZ::Uuid& typeId) -> bool
            {
                if (SystemComponentTagsMatchesAtLeastOneTag(data, requiredTags))
                {
                    componentUuids.emplace(typeId);
                }

                return true;
            });
        }

        //=========================================================================
        // ClearAttributes
        //=========================================================================
        void ElementData::ClearAttributes()
        {
            for (auto& attrib : m_attributes)
            {
                delete attrib.second;
            }
            m_attributes.clear();
        }

        //=========================================================================
        // FindAttribute
        //=========================================================================
        Edit::Attribute* ElementData::FindAttribute(AttributeId attributeId) const
        {
            for (const AttributePair& attributePair : m_attributes)
            {
                if (attributePair.first == attributeId)
                {
                    return attributePair.second;
                }
            }
            return nullptr;
        }

        //=========================================================================
        // ClearElements
        //=========================================================================
        void ClassData::ClearElements()
        {
            for (auto element : m_elements)
            {
                if (element.m_serializeClassElement)
                {
                    element.m_serializeClassElement->m_editData = nullptr;
                    element.m_serializeClassElement = nullptr;
                }
                element.ClearAttributes();
            }
            m_elements.clear();
        }

        //=========================================================================
        // FindElementData
        //=========================================================================
        const ElementData* ClassData::FindElementData(AttributeId elementId) const
        {
            for (const ElementData& element : m_elements)
            {
                if (element.m_elementId == elementId)
                {
                    return &element;
                }
            }
            return nullptr;
        }
    } // namespace Edit
} // namespace AZ
