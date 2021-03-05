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

#ifndef CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_HELPERDATA_H
#define CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_HELPERDATA_H
#pragma once


struct SHelperData
{
public:
    enum EHelperType
    {
        eHelperType_UNKNOWN,
        eHelperType_Point,
        eHelperType_Dummy
    };

public:
    SHelperData()
        : m_eHelperType(eHelperType_UNKNOWN)
    {
    }

public:
    EHelperType m_eHelperType;
    float m_boundBoxMin[3];     // used for eHelperType_Dummy only
    float m_boundBoxMax[3];     // used for eHelperType_Dummy only
};

#endif // CRYINCLUDE_CRYCOMMONTOOLS_EXPORT_HELPERDATA_H
