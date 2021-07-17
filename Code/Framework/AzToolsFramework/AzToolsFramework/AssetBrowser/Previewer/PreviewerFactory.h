/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

class QWidget;
class QString;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class Previewer;
        class AssetBrowserEntry;

        //! Handles creating concrete instances of previewers.
        class PreviewerFactory
        {
        public:
            //! Create new instance of previewer.
            //! Its lifecycle is managed by the parent widget, so you should not destroy it manually.
            virtual Previewer* CreatePreviewer(QWidget* parent = nullptr) const = 0;
            //! Checks if previewers created by this factory can display provided entry.
            virtual bool IsEntrySupported(const AssetBrowserEntry* entry) const = 0;
            //! Returns unique name for the factory (typically it's the name of previewer type it generates).
            virtual const QString& GetName() const = 0;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
