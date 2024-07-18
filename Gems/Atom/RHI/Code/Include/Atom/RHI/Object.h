/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <Atom/RHI.Reflect/Base.h>
#include <AzCore/std/smart_ptr/intrusive_base.h>
#include <AzCore/Name/Name.h>

namespace AZ::RHI
{
    //! Base class for any persistent resource in the RHI library. Provides a name, reference
    //! counting, and a common RTTI base class for all objects in the RHI.
    class Object
    {
    public:
        AZ_RTTI(Object, "{E43378F1-2331-4173-94B8-990ED20E6003}");
        virtual ~Object() = default;

        //! Sets the name of the object.
        void SetName(const Name& name);

        //! Returns the name set on the object by SetName
        const Name& GetName() const;

        //! Returns the current use count of the object
        uint32_t use_count()
        {
            return static_cast<uint32_t>(m_useCount);
        }

    protected:
        Object() = default;

        // IntrusivePtrCountPolicy overrides
        template<typename Type>
        friend struct AZStd::IntrusivePtrCountPolicy;
        void add_ref() const;

        //! All objects have an explicit Init / Shutdown path in addition to
        //! creation / deletion. The user is expected to Init the derived variant
        //! in order to use it. This extension allows the user to forgo shutdown
        //! explicitly and depend on the Ptr ref-counting if necessary.
        //!
        //! This requires that Shutdown properly account for being called multiple times.
        void release() const;
        mutable AZStd::atomic_int m_useCount = 0;

    private:

        //! Shuts down the object. Derived classes can make this public if it fits
        //! with their lifecycle model (i.e. if they use an explicit Init / Shutdown).
        //! By default, it is private in order to maintain consistency with a simpler RAII lifecycle.
        virtual void Shutdown() {};

        virtual void SetNameInternal(const AZStd::string_view& name);

        Name m_name;
    };
}
