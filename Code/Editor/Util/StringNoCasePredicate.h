/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
