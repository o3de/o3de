/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
