/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : This file declares a control which objective is to display
//               multiple assets allowing selection and preview of such things
//               It also handles scrolling and changes in the thumbnail display size


#ifndef CRYINCLUDE_EDITOR_INCLUDE_IASSETVIEWER_H
#define CRYINCLUDE_EDITOR_INCLUDE_IASSETVIEWER_H
#pragma once
#include "IObservable.h"
#include "IAssetItemDatabase.h"

struct IAssetItem;
struct IAssetItemDatabase;

// Description:
//  Observer for the asset viewer events
struct IAssetViewerObserver
{
    virtual void OnChangeStatusBarInfo(UINT nSelectedItems, UINT nVisibleItems, UINT nTotalItems) {};
    virtual void OnSelectionChanged() {};
    virtual void OnChangedPreviewedAsset(IAssetItem* pAsset) {};
    virtual void OnAssetDblClick(IAssetItem* pAsset) {};
    virtual void OnAssetFilterChanged() {};
};

// Description:
//  The asset viewer interface for the asset database plugins to use
struct IAssetViewer
{
    DEFINE_OBSERVABLE_PURE_METHODS(IAssetViewerObserver);

    virtual HWND GetRenderWindow() = 0;
    virtual void ApplyFilters(const IAssetItemDatabase::TAssetFieldFiltersMap& rFieldFilters) = 0;
    virtual const IAssetItemDatabase::TAssetFieldFiltersMap& GetCurrentFilters() = 0;
    virtual void ClearFilters() = 0;
};
#endif // CRYINCLUDE_EDITOR_INCLUDE_IASSETVIEWER_H
