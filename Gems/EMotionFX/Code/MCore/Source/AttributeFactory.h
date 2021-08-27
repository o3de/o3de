/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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

        size_t GetNumRegisteredAttributes() const                  { return m_registered.size(); }
        Attribute* GetRegisteredAttribute(size_t index) const      { return m_registered[index]; }

        size_t FindAttributeIndexByType(size_t typeID) const;
        Attribute* CreateAttributeByType(size_t typeID) const;

    private:
        AZStd::vector<Attribute*> m_registered;
    };
}   // namespace MCore
