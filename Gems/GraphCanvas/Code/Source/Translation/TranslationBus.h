/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include "TranslationAsset.h"

#include <AzCore/StringFunc/StringFunc.h>

namespace GraphCanvas
{
    namespace Translation
    {
        using Handle = size_t;
    }

    class TranslationKey
    {
    public:

        TranslationKey() = default;
        explicit TranslationKey(const AZStd::string& key)
            : m_key(key)
        {}

        TranslationKey(const TranslationKey& rhs)
        {
            m_key = rhs.m_key;
        }

        TranslationKey& operator = (const AZStd::string& key)
        {
            m_key = key;
            return *this;
        }

        bool operator == (const TranslationKey& rhs) const
        {
            return m_key.compare(rhs.m_key) == 0;
        }

        template <typename T>
        auto operator << (T value) -> AZStd::enable_if_t<AZStd::is_same_v<std::void_t<decltype(AZStd::to_string(value))>, void>, TranslationKey&>
        {
            if (!m_key.empty() && !AZStd::to_string(value).empty())
            {
                m_key.append(".");
            }

            if (!AZStd::to_string(value).empty())
            {
                m_key.append(AZStd::to_string(value));
            }

            return *this;
        }

        TranslationKey& operator << (const AZStd::string& value)
        {
            if (!m_key.empty() && !value.empty())
            {
                m_key.append(".");
            }
            if (!value.empty())
            {
                m_key.append(value.c_str());
            }

            return *this;
        }

        const AZStd::string operator + (const AZStd::string& value)
        {
            return m_key + value;
        }

        void clear() { m_key.clear(); }

        const AZStd::string& ToString() const { return m_key; }

        operator AZStd::string() { return m_key; }

        static AZStd::string Sanitize(const AZStd::string& text)
        {
            AZStd::string result = text;
            AZ::StringFunc::Replace(result, "*", "x");
            AZ::StringFunc::Replace(result, "(", "_");
            AZ::StringFunc::Replace(result, ")", "_");
            AZ::StringFunc::Replace(result, "{", "_");
            AZ::StringFunc::Replace(result, "}", "_");
            AZ::StringFunc::Replace(result, ":", "_");
            AZ::StringFunc::Replace(result, "<", "_");
            AZ::StringFunc::Replace(result, ",", "_");
            AZ::StringFunc::Replace(result, ">", " ");
            AZ::StringFunc::Replace(result, "/", "");
            AZ::StringFunc::Strip(result, " ");
            AZ::StringFunc::Path::Normalize(result);
            return result;
        }

    protected:

        AZStd::string m_key;

    };

    //! Requests to access the database
    class TranslationRequests : public AZ::EBusTraits
    {
    public:

        using MutexType = AZStd::recursive_mutex;

        //! Restores the database from all the assets
        virtual void Restore() {}

        //! Returns true if they database has the specified key
        virtual bool HasKey(const AZStd::string& /*key*/) { return false; }

        //! Returns the text value for a given key
        virtual bool Get(const AZStd::string& /*key*/, AZStd::string& /*value*/) { return false; }

        struct Details
        {
            AZStd::string Name;
            AZStd::string Tooltip;
            AZStd::string Category;
            AZStd::string Subtitle;

            bool Valid = false;

            Details() = default;

            Details(const Details& rhs)
            {
                Name = rhs.Name;
                Tooltip = rhs.Tooltip;
                Subtitle = rhs.Subtitle;
                Category = rhs.Category;
                Valid = rhs.Valid;
            }

            Details(const char* name, const char* tooltip, const char* subtitle, const char* category)
                : Name(name), Tooltip(tooltip), Subtitle(subtitle), Category(category)
            {
                Valid = !Name.empty();
            }
        };

        //! Adds an entry into the database, returns true if there were any warnings while adding to the database
        virtual bool Add(const TranslationFormat& /*translationFormat*/) { return false;  }

        //! Get the details associated with a given key (assumes they are within a "details" object)
        virtual Details GetDetails(const AZStd::string& /*key*/, const Details& /*fallbackDetails*/) { return Details(); }

        //! Generates the source JSON assets for all reflected elements
        virtual void GenerateSourceAssets() {}

        //! Stores the runtime database into a json file for debugging only
        virtual void DumpDatabase(const AZStd::string& /*filename*/) {}
    };

    using TranslationRequestBus = AZ::EBus<TranslationRequests>;

}

namespace AZStd
{
    template <typename T>
    struct hash;

    // hashing support for STL containers
    template <>
    struct hash<GraphCanvas::TranslationKey>
    {
        using argument_type = GraphCanvas::TranslationKey;
        using result_type = AZStd::size_t;

        inline size_t operator()(const GraphCanvas::TranslationKey& value) const
        {
            size_t h = 0;
            hash_combine(h, value.ToString().c_str());
            return h;
        }
    };
}
