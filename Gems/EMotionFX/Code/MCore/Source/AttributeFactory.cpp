/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
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
            for (Attribute* attribute : mRegistered)
            {
                delete attribute;
            }
        }

        mRegistered.clear();
    }


    void AttributeFactory::RegisterAttribute(Attribute* attribute)
    {
        // check first if the type hasn't already been registered
        const uint32 attribIndex = FindAttributeIndexByType(attribute->GetType());
        if (attribIndex != MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("MCore::AttributeFactory::RegisterAttribute() - There is already an attribute of the same type registered (typeID %d vs %d - typeString '%s' vs '%s')", attribute->GetType(), mRegistered[attribIndex]->GetType(), attribute->GetTypeString(), mRegistered[attribIndex]->GetTypeString());
            return;
        }

        mRegistered.emplace_back(attribute);
    }


    void AttributeFactory::UnregisterAttribute(Attribute* attribute, bool delFromMem)
    {
        // check first if the type hasn't already been registered
        const uint32 attribIndex = FindAttributeIndexByType(attribute->GetType());
        if (attribIndex == MCORE_INVALIDINDEX32)
        {
            MCore::LogWarning("MCore::AttributeFactory::UnregisterAttribute() - No attribute with the given type found (typeID=%d - typeString='%s'", attribute->GetType(), attribute->GetTypeString());
            return;
        }

        if (delFromMem)
        {
            delete mRegistered[attribIndex];
        }

        mRegistered.erase(mRegistered.begin() + attribIndex);
    }


    uint32 AttributeFactory::FindAttributeIndexByType(uint32 typeID) const
    {
        const size_t numAttributes = mRegistered.size();
        for (size_t i = 0; i < numAttributes; ++i)
        {
            if (mRegistered[i]->GetType() == typeID) // we found one with the same type
            {
                return static_cast<uint32>(i);
            }
        }

        // no attribute of this type found
        return MCORE_INVALIDINDEX32;
    }


    Attribute* AttributeFactory::CreateAttributeByType(uint32 typeID) const
    {
        const uint32 attribIndex = FindAttributeIndexByType(typeID);
        if (attribIndex == MCORE_INVALIDINDEX32)
        {
            return nullptr;
        }

        return mRegistered[attribIndex]->Clone();
    }


    void AttributeFactory::RegisterStandardTypes()
    {
        mRegistered.reserve(10);
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
