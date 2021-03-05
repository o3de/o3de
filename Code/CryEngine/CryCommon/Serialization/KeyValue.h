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

#ifndef CRYINCLUDE_CRYCOMMON_SERIALIZATION_KEYVALUE_H
#define CRYINCLUDE_CRYCOMMON_SERIALIZATION_KEYVALUE_H
#pragma once
namespace Serialization {
    class IArchive;

    class IKeyValue
        : IString
    {
    public:
        virtual const char* get() const = 0;
        virtual void set(const char* key) = 0;
        virtual bool serializeValue(IArchive& ar, const char* name, const char* label) = 0;
        template<class TArchive>
        void Serialize(TArchive& ar)
        {
            ar(*(IString*)this, "", "^");
            serializeValue(ar, "", "^");
        }
    };
}


#endif // CRYINCLUDE_CRYCOMMON_SERIALIZATION_KEYVALUE_H
