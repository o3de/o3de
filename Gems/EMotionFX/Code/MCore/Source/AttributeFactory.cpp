/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "StandardHeaders.h"
#include "AttributeFactory.h"
#include "LogManager.h"

// include default types
#include "AttributeFloat.h"
#include "AttributeInt32.h"
#include "AttributeString.h"
#include "AttributeBool.h"
#include "AttributeVector2.h"
#include "AttributeVector3.h"
#include "AttributeVector4.h"
#include "AttributeQuaternion.h"
#include "AttributeColor.h"
#include "AttributePointer.h"


namespace MCore
{
    AttributeFactory::AttributeFactory()
    {
        RegisterStandardTypes();
    }


    AttributeFactory::~AttributeFactory()
    {
        UnregisterAllAttributes();
    }


    void AttributeFactory::UnregisterAllAttributes(bool delFromMem)
    {
        if (delFromMem)
        {
            for (Attribute* attribute : m_registered)
            {
                delete attribute;
            }
        }

        m_registered.clear();
    }


    void AttributeFactory::RegisterAttribute(Attribute* attribute)
    {
        // check first if the type hasn't already been registered
        const size_t attribIndex = FindAttributeIndexByType(attribute->GetType());
        if (attribIndex != InvalidIndex)
        {
            MCore::LogWarning("MCore::AttributeFactory::RegisterAttribute() - There is already an attribute of the same type registered (typeID %d vs %d - typeString '%s' vs '%s')", attribute->GetType(), m_registered[attribIndex]->GetType(), attribute->GetTypeString(), m_registered[attribIndex]->GetTypeString());
            return;
        }

        m_registered.emplace_back(attribute);
    }


    void AttributeFactory::UnregisterAttribute(Attribute* attribute, bool delFromMem)
    {
        // check first if the type hasn't already been registered
        const size_t attribIndex = FindAttributeIndexByType(attribute->GetType());
        if (attribIndex == InvalidIndex)
        {
            MCore::LogWarning("MCore::AttributeFactory::UnregisterAttribute() - No attribute with the given type found (typeID=%d - typeString='%s'", attribute->GetType(), attribute->GetTypeString());
            return;
        }

        if (delFromMem)
        {
            delete m_registered[attribIndex];
        }

        m_registered.erase(m_registered.begin() + attribIndex);
    }


    size_t AttributeFactory::FindAttributeIndexByType(size_t typeID) const
    {
        const auto foundAttribute = AZStd::find_if(begin(m_registered), end(m_registered), [typeID](const Attribute* registeredAttribute)
        {
            return registeredAttribute->GetType() == typeID;
        });

        return foundAttribute != end(m_registered) ? AZStd::distance(begin(m_registered), foundAttribute) : InvalidIndex;
    }


    Attribute* AttributeFactory::CreateAttributeByType(size_t typeID) const
    {
        const size_t attribIndex = FindAttributeIndexByType(typeID);
        if (attribIndex == InvalidIndex)
        {
            return nullptr;
        }

        return m_registered[attribIndex]->Clone();
    }


    void AttributeFactory::RegisterStandardTypes()
    {
        m_registered.reserve(10);
        RegisterAttribute(aznew AttributeFloat());
        RegisterAttribute(aznew AttributeInt32());
        RegisterAttribute(aznew AttributeString());
        RegisterAttribute(aznew AttributeBool());
        RegisterAttribute(aznew AttributeVector2());
        RegisterAttribute(aznew AttributeVector3());
        RegisterAttribute(aznew AttributeVector4());
        RegisterAttribute(aznew AttributeQuaternion());
        RegisterAttribute(aznew AttributeColor());
        RegisterAttribute(aznew AttributePointer());
    }
}   // namespace MCore
