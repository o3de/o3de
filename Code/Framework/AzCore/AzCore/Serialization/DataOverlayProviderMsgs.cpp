/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/Serialization/DataOverlayProviderMsgs.h>

namespace AZ
{
    //-------------------------------------------------------------------------
    // DataOverlayTarget
    //-------------------------------------------------------------------------
    void DataOverlayTarget::Parse(const void* classPtr, const SerializeContext::ClassData* classData)
    {
        AZStd::vector<SerializeContext::DataElementNode*> nodeStack;
        nodeStack.push_back(m_dataContainer);

        SerializeContext::EnumerateInstanceCallContext callContext(
            [this, &nodeStack](void* instancePointer, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* classElement)->bool
            {
                return ElementBegin(&nodeStack, instancePointer, classData, classElement);
            },
            [this, &nodeStack]()->bool { return ElementEnd(&nodeStack); },
            m_sc,
            SerializeContext::ENUM_ACCESS_FOR_READ,
            m_errorLogger
        );

        m_sc->EnumerateInstanceConst(
              &callContext
            , classPtr
            , classData->m_typeId
            , classData
            , nullptr
            );
    }
    //-------------------------------------------------------------------------
    bool DataOverlayTarget::ElementBegin(NodeStack* nodeStack, const void* elemPtr, const SerializeContext::ClassData* classData, const SerializeContext::ClassElement* elementData)
    {
        nodeStack->back()->m_subElements.push_back();
        SerializeContext::DataElementNode* node = &nodeStack->back()->m_subElements.back();
        nodeStack->push_back(node);

        node->m_classData = classData;
        node->m_element.m_id = classData->m_typeId;
        node->m_element.m_version = classData->m_version;
        if (elementData)
        {
            node->m_element.m_name = elementData->m_name;
            node->m_element.m_nameCrc = elementData->m_nameCrc;
        }
        if (classData->m_serializer)
        {
            node->m_element.m_dataSize = classData->m_serializer->Save(elemPtr, node->m_element.m_byteStream);
            node->m_element.m_dataType = SerializeContext::DataElement::DT_BINARY;
        }
        else
        {
            node->m_element.m_dataSize = 0;
        }
        return true;
    }
    //-------------------------------------------------------------------------
    bool DataOverlayTarget::ElementEnd(NodeStack* nodeStack)
    {
        nodeStack->pop_back();
        return true;
    }
    //-------------------------------------------------------------------------
}   // namespace AZ
