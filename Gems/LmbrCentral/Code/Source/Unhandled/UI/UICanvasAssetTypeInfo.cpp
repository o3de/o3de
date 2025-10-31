/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include "UICanvasAssetTypeInfo.h"

namespace LmbrCentral
{
    static AZ::Data::AssetType uiCanvasAssetType("{E48DDAC8-1F1E-4183-AAAB-37424BCC254B}");

    UICanvasAssetTypeInfo::~UICanvasAssetTypeInfo()
    {
        Unregister();
    }

    void UICanvasAssetTypeInfo::Register()
    {
        AZ::AssetTypeInfoBus::Handler::BusConnect(uiCanvasAssetType);
    }

    void UICanvasAssetTypeInfo::Unregister()
    {
        AZ::AssetTypeInfoBus::Handler::BusDisconnect(uiCanvasAssetType);
    }

    AZ::Data::AssetType UICanvasAssetTypeInfo::GetAssetType() const
    {
        return uiCanvasAssetType;
    }

    const char* UICanvasAssetTypeInfo::GetAssetTypeDisplayName() const
    {
        return "UI Canvas";
    }

    const char* UICanvasAssetTypeInfo::GetGroup() const
    {
        return "UI";
    }

    const char * UICanvasAssetTypeInfo::GetBrowserIcon() const
    {
        return "Icons/Components/UICanvasAssetRef.svg";
    }
} // namespace LmbrCentral
