/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
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
