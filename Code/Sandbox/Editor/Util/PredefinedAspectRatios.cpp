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

#include "EditorDefs.h"

#include "PredefinedAspectRatios.h"

// Editor
#include "Settings.h"

CPredefinedAspectRatios::CPredefinedAspectRatios()
{
    m_aspectRatios.reserve(10);

    AddAspectRatio(5, 4);
    AddAspectRatio(4, 3);
    AddAspectRatio(3, 2);
    AddAspectRatio(16, 10);
    AddAspectRatio(16, 9);
    AddAspectRatio(1.85f, 1);
    AddAspectRatio(2.39f, 1);
}

CPredefinedAspectRatios::~CPredefinedAspectRatios()
{
}

void CPredefinedAspectRatios::AddAspectRatio(float x, int y)
{
    if (y == 0)
    {
        return;
    }

    SAspectRatio aspectRatio;
    aspectRatio.name = QStringLiteral("%1:%2").arg(x, 0, 'f', 2).arg(y);
    aspectRatio.value = x / y;

    m_aspectRatios.push_back(aspectRatio);
}

void CPredefinedAspectRatios::AddAspectRatio(int x, int y)
{
    if (y == 0)
    {
        return;
    }

    SAspectRatio aspectRatio;
    aspectRatio.name = QStringLiteral("%1:%2").arg(x).arg(y);
    aspectRatio.value = float( x ) / y;

    m_aspectRatios.push_back(aspectRatio);
}


float CPredefinedAspectRatios::GetCurrentValue() const
{
    return gSettings.viewports.fDefaultAspectRatio;
}

bool CPredefinedAspectRatios::IsEmpty() const
{
    return m_aspectRatios.empty();
}

size_t CPredefinedAspectRatios::GetCount() const
{
    return m_aspectRatios.size();
}


const QString& CPredefinedAspectRatios::GetName(size_t aspectRatioId) const
{
    bool validAspectRatioId = (aspectRatioId < GetCount());
    assert(validAspectRatioId);
    if (!validAspectRatioId)
    {
        static QString dummyAspectRatioName("1:1");
        return dummyAspectRatioName;
    }

    const SAspectRatio& aspectRatio = m_aspectRatios[ aspectRatioId ];

    return aspectRatio.name;
}

float CPredefinedAspectRatios::GetValue(size_t aspectRatioId) const
{
    bool validAspectRatioId = (aspectRatioId < GetCount());
    assert(validAspectRatioId);
    if (!validAspectRatioId)
    {
        return 1;
    }

    const SAspectRatio& aspectRatio = m_aspectRatios[ aspectRatioId ];

    return aspectRatio.value;
}

bool CPredefinedAspectRatios::IsCurrent(size_t aspectRatioId) const
{
    float selectedValue = GetValue(aspectRatioId);
    float currentValue = GetCurrentValue();

    const float THRESHOLD = 0.01f;

    bool valuesCloseEnough = (fabs(selectedValue - currentValue) <= THRESHOLD);

    return valuesCloseEnough;
}
