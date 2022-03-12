/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/string/string_view.h>
#include <AzCore/std/parallel/shared_mutex.h>
#include <AzCore/Memory/Memory.h>
#include <AzCore/Memory/OSAllocator.h>
#include <AzCore/Name/Name.h>

namespace MaterialEditor
{
    class MaterialEditorCoreComponent;
}

namespace UnitTest
{
    class NameDictionaryTester;
}

namespace AZ
{
    class Module;

    namespace Internal
    {
        class NameData;
    };

    //! Maintains a list of unique strings for Name objects.
    //! The main benefit of the Name system is very fast string equality comparison, because every
    //! unique name has a unique ID. The NameDictionary's purpose is to guarantee name IDs do not
    //! collide. It also saves memory by removing duplicate strings.
    //!
    //! Benchmarks have shown that creating a new Name object can be quite slow when the name doesn't
    //! already exist in the NameDictionary, but is comparable to creating an AZStd::string for names
    //! that already exist.
    class NameDictionary final
    {
        AZ_CLASS_ALLOCATOR(NameDictionary, AZ::OSAllocator, 0);

        friend Module;
        friend Name;
        friend Internal::NameData;
        friend UnitTest::NameDictionaryTester;
        template<typename T, typename... Args> friend constexpr auto AZStd::construct_at(T*, Args&&... args)
            -> AZStd::enable_if_t<AZStd::is_void_v<AZStd::void_t<decltype(new (AZStd::declval<void*>()) T(AZStd::forward<Args>(args)...))>>, T*>;
        template<typename T> constexpr friend void AZStd::destroy_at(T*);

    public:

        static void Create();

        static void Destroy();
        static bool IsReady();
        static NameDictionary& Instance();

        //! Makes a Name from the provided raw string. If an entry already exists in the dictionary, it is shared.
        //! Otherwise, it is added to the internal dictionary.
        //!
        //! @param name The name to resolve against the dictionary.
        //! @return A Name instance holding a dictionary entry associated with the provided raw string.
        Name MakeName(AZStd::string_view name);

        //! Search for an existing name in the dictionary by hash.
        //! @param hash The key by which to search for the name.
        //! @return A Name instance. If the hash was not found, the Name will be empty.
        Name FindName(Name::Hash hash) const;

    private:
        NameDictionary();
        ~NameDictionary();

        void ReportStats() const;

        //////////////////////////////////////////////////////////////////////////
        // Private API for NameData

        // Attempts to release the name from the dictionary, but checks to make sure
        // a reference wasn't taken by another thread.
        void TryReleaseName(Name::Hash hash);

        //////////////////////////////////////////////////////////////////////////

        // Calculates a hash for the provided name string.
        // Does not attempt to resolve hash collisions; that is handled elsewhere.
        Name::Hash CalcHash(AZStd::string_view name);

        AZStd::unordered_map<Name::Hash, Internal::NameData*> m_dictionary;
        mutable AZStd::shared_mutex m_sharedMutex;
    };
}
