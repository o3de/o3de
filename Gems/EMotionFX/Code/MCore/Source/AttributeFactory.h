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

#include <AzCore/std/containers/vector.h>
#include "StandardHeaders.h"
#include "MemoryManager.h"
#include "Attribute.h"


namespace MCore
{
    /**
     * The attribute factory, which is used to create attribute objects.
     */
    class MCORE_API AttributeFactory
    {
    public:
        AttributeFactory();
        ~AttributeFactory();

        void UnregisterAllAttributes(bool delFromMem = true);
        void RegisterAttribute(Attribute* attribute);
        void UnregisterAttribute(Attribute* attribute, bool delFromMem = true);
        void RegisterStandardTypes();

        size_t GetNumRegisteredAttributes() const                  { return mRegistered.size(); }
        Attribute* GetRegisteredAttribute(uint32 index) const      { return mRegistered[index]; }

        uint32 FindAttributeIndexByType(uint32 typeID) const;
        Attribute* CreateAttributeByType(uint32 typeID) const;

    private:
        AZStd::vector<Attribute*> mRegistered;
    };
}   // namespace MCore
