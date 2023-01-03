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

            //! The opposite of ToString - given a string that may contain encoded entries,
            //! resolve them to the real entries.
            //! Note that the entries returned are pointers to the actual real entries in the real
            //! asset browser view, and should not be cached for longer than this call.
            //! Returns true if the inputString contained any AssetBrowserEntry encodings.  Note that this
            //! call de-duplicates, but will still return true if it found any valid entries in the data, 
            //! regardless of whether it added any NEW ones or not due to de-duplication.
            bool FromString(AZStd::string_view inputString, AZStd::vector<const AssetBrowserEntry*>& entries);

            //! Stores a given vector of AssetBrowserEntry*, writes it to the given QMimeData object
            //! in a way that can be read back even across applications (ie, no pointers are serialized, only
            //! identifiers that can be looked up).
            void ToMimeData(QMimeData* mimeData, const AZStd::vector<const AssetBrowserEntry*>& entries);

            //! Given a QMimeData object, parses any AssetBrowserEntry* objects encoded in it and writes
            //! them to the given vector.
            //! Note that this is a lookup/find operation, not a deserialize operation, so the pointers you recieve
            //! are to the actual AssetBrowserEntries in your actual Asset Browser tree local to this application.
            //! Returns true if any entries were encoded in the mimeData (regardless of whether they were added to the
            //! vector or not, since it de-duplicates).
            bool FromMimeData(const QMimeData* mimeData, AZStd::vector<const AssetBrowserEntry*>& entries);

            //! Write a single AssetBrowserEntry to a string in a stable and machine-readable way that can be read back later.
            //! Supports only products and sources currently.
            AZStd::string WriteEntryToString(const AssetBrowserEntry* entry);

            //! This function takes a string written by the WriteEntryToString function above,
            //! and reads it back, searches for the associated element in the Asset Browser Entry tree,
            //! and resolves it if it can.
            //! Note that what is being returned is a pointer to the actual entry in the actual Asset Browser tree,
            //! it is not deserializing or creating a new one.
            const AssetBrowserEntry* FindFromString(AZStd::string_view data);

            //! This function returns the folder in which given asset is located or the entry itself if it's already a folder
            const AssetBrowserEntry* FolderForEntry(const AssetBrowserEntry* entry);
        } 
    }
}
