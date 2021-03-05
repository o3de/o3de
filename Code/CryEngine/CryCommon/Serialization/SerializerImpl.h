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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_SERIALIZERIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_SERIALIZERIMPL_H
#pragma once

#include "Serializer.h"
#include "IClassFactory.h"
#include "ClassFactory.h"

// IArchive.h is supposed to be pre-included

namespace Serialization {
    inline bool SStruct::operator()(IArchive& ar) const
    {
        YASLI_ESCAPE(serializeFunc_ && object_, return false);
        return serializeFunc_(object_, ar);
    }

    inline bool SStruct::operator()(IArchive& ar, const char* name, const char* label) const
    {
        return ar(*this, name, label);
    }


    inline void IPointer::Serialize(IArchive& ar) const
    {
        const bool noEmptyNames = ar.GetCaps(IArchive::NO_EMPTY_NAMES);
        const char* const typePropertyName = noEmptyNames ? "type" : "";
        const char* const dataPropertyName = noEmptyNames ? "data" : "";

        TypeID baseTypeID = baseType();
        const char* oldRegisteredName = registeredTypeName();
        if (!oldRegisteredName)
        {
            oldRegisteredName = "";
        }
        IClassFactory* factory = this->factory();

        if (ar.IsOutput())
        {
            if (oldRegisteredName[0] != '\0')
            {
                TypeNameWithFactory pair(oldRegisteredName, factory);
                if (ar(pair, typePropertyName))
                {
                    ar(serializer(), dataPropertyName);
                }
                else
                {
                    ar.Warning(pair, "Unable to write typeID!");
                }
            }
        }
        else
        {
            TypeNameWithFactory pair("", factory);
            if (!ar(pair, typePropertyName))
            {
                if (oldRegisteredName[0] != '\0')
                {
                    create(""); // 0
                }
                return;
            }

            if (oldRegisteredName[0] != '\0' && (pair.registeredName.empty() || (pair.registeredName != oldRegisteredName)))
            {
                create(""); // 0
            }
            if (!pair.registeredName.empty())
            {
                if (!get())
                {
                    create(pair.registeredName.c_str());
                }
                ar(serializer(), dataPropertyName);
            }
        }
    }
}
// vim:sw=4 ts=4:

#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_SERIALIZERIMPL_H
