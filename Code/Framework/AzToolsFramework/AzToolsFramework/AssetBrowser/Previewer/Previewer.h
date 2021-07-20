/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/PlatformDef.h>

#if !defined(Q_MOC_RUN)
// Disables warning messages triggered by the Qt library
// 4251: class needs to have dll-interface to be used by clients of class 
// 4800: forcing value to bool 'true' or 'false' (performance warning)
AZ_PUSH_DISABLE_WARNING(4251 4800, "-Wunknown-warning-option")
#include <QWidget>
AZ_POP_DISABLE_WARNING
#endif

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        //! A base class for Asset Browser previewer.
        //! To implement your custom previewer:
        //! 1. Derive your previewer widget from this class.
        //! 2. Implement custom PreviewerFactory.
        //! 3. Register PreviewerFactory with PreviewerRequestBus::RegisterFactory EBus.
        //! Note: if there are multiple factories handling same entry type, last one registered will be selected.
        class Previewer
            : public QWidget
        {
            Q_OBJECT
        public:
            Previewer(QWidget* parent = nullptr);

            //! Clear previewer
            virtual void Clear() const = 0;
            //! Display asset preview for specific asset browser entry
            virtual void Display(const AssetBrowserEntry* entry) = 0;
            //! Get name of the previewer (this should be unique to other previewers)
            virtual const QString& GetName() const = 0;
        };
    } // namespace AssetBrowser
} // namespace AzToolsFramework
