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

// Description : Utility class to help in STL algorithms on containers with
//               CStrings when intending to use case insensitive searches or sorting for
//               example.


#ifndef CRYINCLUDE_EDITOR_UTIL_STRINGNOCASEPREDICATE_H
#define CRYINCLUDE_EDITOR_UTIL_STRINGNOCASEPREDICATE_H
#pragma once

/*
Utility class to help in STL algorithms on containers with CStrings
when intending to use case insensitive searches or sorting for example.

e.g.
std::vector< CString > v;
...
std::sort( v.begin(), v.end(), CStringNoCasePredicate::LessThan() );
std::find_if( v.begin(), v.end(), CStringNoCasePredicate::Equal( stringImLookingFor ) );
*/
class CStringNoCasePredicate
{
public:
    //////////////////////////////////////////////////////////////////////////
    struct Equal
    {
        Equal(const QString& referenceString)
            : m_referenceString(referenceString)
        {
        }

        bool operator() (const QString& arg) const
        {
            return (m_referenceString.compare(arg, Qt::CaseInsensitive) == 0);
        }

    private:
        const QString& m_referenceString;
    };
    //////////////////////////////////////////////////////////////////////////

    //////////////////////////////////////////////////////////////////////////
    struct LessThan
    {
        bool operator() (const QString& arg1, const QString& arg2) const
        {
            return (arg1.CompareNoCase(arg2) < 0);
        }
    };
    //////////////////////////////////////////////////////////////////////////
};


#endif // CRYINCLUDE_EDITOR_UTIL_STRINGNOCASEPREDICATE_H
