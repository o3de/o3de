/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <QColor>

namespace AzToolsFramework::AssetBrowser
{
    // =======================================================================
    // Asset Browser Paint Colours
    // =======================================================================

    //! The colours the views of the Asset Browser and their delegates paint with themselves, named once, so the legacy and the
    //! flattened GUI read one value. A named namespace, not an anonymous one: a header is read by many files, and a unity build joins
    //! files into one translation unit, where anonymous namespaces merge.
    namespace AssetBrowserFlat
    {
        //! The fill of the row of a top-level entry (one with no parent index) that is neither hovered nor selected, and of the branch
        //! cell beside it: EntryDelegate.cpp:135, AssetBrowserTreeView.cpp:202, FavoritesEntryDelegate.cpp:97,
        //! AssetBrowserFavoritesView.cpp:219. TOKENIZE: AssetBrowserRootRowFill
        inline constexpr QColor RootRowFill(0x33, 0x33, 0x33);
    } // namespace AssetBrowserFlat
} // namespace AzToolsFramework::AssetBrowser
