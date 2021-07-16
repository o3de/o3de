/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


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
