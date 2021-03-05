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

#ifndef CRYINCLUDE_EDITOR_UTIL_PREDEFINEDASPECTRATIOS_H
#define CRYINCLUDE_EDITOR_UTIL_PREDEFINEDASPECTRATIOS_H
#pragma once


#include <vector>

class CPredefinedAspectRatios
{
public:
    CPredefinedAspectRatios();
    virtual ~CPredefinedAspectRatios();

    void AddAspectRatio(float x, int y);
    void AddAspectRatio(int x, int y);

    float GetCurrentValue() const;

    bool IsEmpty() const;
    size_t GetCount() const;

    const QString& GetName(size_t aspectRatioId) const;
    float GetValue(size_t aspectRatioId) const;
    bool IsCurrent(size_t aspectRatioId) const;

private:
    struct SAspectRatio
    {
        QString name;
        float value;
    };
    std::vector< SAspectRatio > m_aspectRatios;
};

#endif // CRYINCLUDE_EDITOR_UTIL_PREDEFINEDASPECTRATIOS_H
