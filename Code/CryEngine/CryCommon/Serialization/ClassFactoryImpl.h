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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORYIMPL_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORYIMPL_H
#pragma once

#include "IArchive.h"
#include "IClassFactory.h"
#include "STL.h"
#include "ClassFactory.h"
#include "Strings.h"

namespace Serialization {
    inline bool Serialize(Serialization::IArchive& ar, Serialization::TypeNameWithFactory& value, const char* name, [[maybe_unused]] const char* label)
    {
        if (!ar(value.registeredName, name))
        {
            return false;
        }

        if (ar.IsInput())
        {
            const TypeDescription* desc = value.factory->descriptionByRegisteredName(value.registeredName.c_str());
            if (!desc)
            {
                ar.Error(value, "Unable to read TypeID: unregistered type name: \'%s\'", value.registeredName.c_str());
                value.registeredName.clear();
                return false;
            }
        }
        return true;
    }
}


#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_CLASSFACTORYIMPL_H
