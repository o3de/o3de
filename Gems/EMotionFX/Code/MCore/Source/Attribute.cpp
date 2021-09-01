/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "Attribute.h"
#include "AttributeFactory.h"

namespace MCore
{
    Attribute::Attribute(AZ::u32 typeID)
    {
        m_typeId = typeID;
    }

    Attribute::~Attribute()
    {
    }

    Attribute& Attribute::operator=(const Attribute& other)
    {
        if (&other != this)
        {
            InitFrom(&other);
        }
        return *this;
    }
} // namespace MCore
