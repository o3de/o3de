/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/containers/vector.h>

class QMimeData;

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        class AssetBrowserEntry;

        //! A set of useful utilities that work with Asset Browser Entries
        namespace Utils
        {
            //! Write a vector of entries to a string that you can read back using FromString.
            //! Becuase entries are constantly changing as new assets are added and old are removed
            //! it is important to snapshot entries to a string, and then read them back from the string
            //! at a later time in any async operation, rather than hanging on to the pointers for any longer
            //! than one callstack.
            AZStd::string ToString(const AZStd::vector<const AssetBrowserEntry*>& entries);

            //! The opposite of ToString - given a string that contains encoded entries,
            //! resolve them to the real entries.
            //! Note that the entries returned are pointers to the actual real entries in the real
            //! asset browser view, and should not be cached for longer than this call.
            bool FromString(AZStd::string_view inputString, AZStd::vector<const AssetBrowserEntry*>& entries);

            //! Write the list of entries into a data format that can be read back by FromMimeData.
            void ToMimeData(QMimeData* mimeData, const AZStd::vector<const AssetBrowserEntry*>& entries);

            //! Things like filtering lists, making queries, etc.
            bool FromMimeData(const QMimeData* mimeData, AZStd::vector<const AssetBrowserEntry*>& entries);

            //! Write a single AssetBrowserEntry to a string in a stable and machine-readable way that can be read back later.
            //! Supports only products and sources currently.
            AZStd::string WriteEntryToString(const AssetBrowserEntry* entry);

            //! This function takes a string written by the WriteEntryToString function above,
            //! and reads it back, searches for the associated element and resolves it if it can.
            //! note that what is being returned is the actual entry, it is not deserializing or creating a new one,
            //! or nullptr if it cannot be found locally.
            const AssetBrowserEntry* FindFromString(AZStd::string_view data);
        } 
    }
}
