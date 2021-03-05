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
#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Math/Color.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
class UiDynamicContentDatabaseInterface
    : public AZ::EBusTraits
{
public: // types

    enum ColorType
    {
        FreeColors,
        PaidColors,

        NumColorTypes
    };

public:

    virtual int GetNumColors(ColorType colorType) = 0;
    virtual AZ::Color GetColor(ColorType colorType, int index) = 0;
    virtual AZStd::string GetColorName(ColorType colorType, int index) = 0;
    virtual AZStd::string GetColorPrice(ColorType colorType, int index) = 0;

    virtual void Refresh(ColorType colorType, const AZStd::string& filePath) = 0;
};
typedef AZ::EBus<UiDynamicContentDatabaseInterface> UiDynamicContentDatabaseBus;
