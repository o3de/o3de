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

// Description : This file declares the container for the assotiaon of
//               enumeration name to enumeration values


#ifndef CRYINCLUDE_EDITOR_UTIL_UIENUMERATIONS_H
#define CRYINCLUDE_EDITOR_UTIL_UIENUMERATIONS_H
#pragma once


class CUIEnumerations
{
public:
    // For XML standard values.
    typedef QStringList                    TDValues;
    typedef std::map<QString, TDValues>      TDValuesContainer;
protected:
private:

public:
    static CUIEnumerations& GetUIEnumerationsInstance();

    TDValuesContainer&      GetStandardNameContainer();
protected:
private:
};


#endif // CRYINCLUDE_EDITOR_UTIL_UIENUMERATIONS_H
