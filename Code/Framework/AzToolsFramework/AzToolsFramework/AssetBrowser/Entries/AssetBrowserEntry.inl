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

namespace AzToolsFramework
{
    namespace AssetBrowser
    {
        template<typename EntryType>
        void AssetBrowserEntry::GetChildren(AZStd::vector<const EntryType*>& entries) const
        {
            entries.reserve(entries.size() + m_children.size());
            for (auto child : m_children)
            {
                if (auto newEntry = azrtti_cast<const EntryType*>(child))
                {
                    entries.push_back(newEntry);
                }
            }
        }

        template<typename EntryType>
        void AssetBrowserEntry::GetChildrenRecursively(AZStd::vector<const EntryType*>& entries) const
        {
            if (auto newEntry = azrtti_cast<const EntryType*>(this))
            {
                entries.push_back(newEntry);
            }

            for (auto child : m_children)
            {
                child->GetChildrenRecursively<EntryType>(entries);
            }
        }

        template <typename EntryType>
        void AssetBrowserEntry::ForEachEntryInMimeData(const QMimeData* mimeData, AZStd::function<void(const EntryType*)> callbackFunction)
        {
            if ((!mimeData) || (!callbackFunction))
            {
                return;
            }

            AZStd::vector<AssetBrowserEntry*> entries;
            if (!AssetBrowserEntry::FromMimeData(mimeData, entries))
            {
                // if mimedata does not even contain product entries, no point in proceeding.
                return;
            }

            for (auto entry : entries)
            {
                // note that this works even if entry itself is a product already.
                AZStd::vector<const EntryType*> matchingEntries;
                entry->GetChildrenRecursively<EntryType>(matchingEntries);
                for (const EntryType* matchingEntry : matchingEntries)
                {
                    callbackFunction(matchingEntry);
                }
            }
        }
    } // namespace AssetBrowser
} // namespace AzToolsFramework
