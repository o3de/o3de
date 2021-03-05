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
// Original file Copyright Crytek GMBH or its affiliates, used under license.

// Description : Part of CryEngine's extension framework.


#ifndef CRYINCLUDE_CRYEXTENSION_IMPL_CONVERSION_H
#define CRYINCLUDE_CRYEXTENSION_IMPL_CONVERSION_H
#pragma once


namespace TC
{
    //template <class T, class U>
    //struct Conversion
    //{
    //private:
    //  typedef char y[1];
    //  typedef char n[2];
    //  static y& Test(U);
    //  static n& Test(...);
    //  static T MakeT();

    //public:
    //  enum
    //  {
    //      exists = sizeof(Test(MakeT())) == sizeof(y),
    //      sameType = false
    //  };
    //};

    //template <class T>
    //struct Conversion<T, T>
    //{
    //public:
    //  enum
    //  {
    //      exists = true,
    //      sameType = true
    //  };
    //};

    //template<typename Base, typename Derived>
    //struct CheckInheritance
    //{
    //  enum
    //  {
    //      exists = Conversion<const Derived*, const Base*>::exists && !Conversion<const Base*, const void*>::sameType
    //  };
    //};

    //template<typename Base, typename Derived>
    //struct CheckStrictInheritance
    //{
    //  enum
    //  {
    //      exists = CheckInheritance<Base, Derived>::exists && !Conversion<const Base*, const Derived*>::sameType
    //  };
    //};


    template <typename Base, typename Derived>
    struct SuperSubClass
    {
    private:
        typedef char y[1];
        typedef char n[2];

        template<typename T>
        static y& check(const volatile Derived&, T);
        static n& check(const volatile Base&, int);

        struct C
        {
            operator const volatile Base&() const;
            operator const volatile Derived&();
        };

        static C getC();

    public:
        enum
        {
            exists = sizeof(check(getC(), 0)) == sizeof(y),
            sameType = false
        };
    };

    template <typename T>
    struct SuperSubClass<T, T>
    {
        enum
        {
            exists = true
        };
    };
} // namespace TC

#endif // CRYINCLUDE_CRYEXTENSION_IMPL_CONVERSION_H
