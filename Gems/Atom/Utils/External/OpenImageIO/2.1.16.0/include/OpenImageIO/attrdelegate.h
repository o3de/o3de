// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once

#include <memory>
#include <vector>

#include <OpenImageIO/export.h>
#include <OpenImageIO/string_view.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/ustring.h>


OIIO_NAMESPACE_BEGIN


namespace pvt {

// Helper template to detect if a type is one if the string types that
// OIIO tends to use.
// clang-format off
template<typename T> struct is_string : std::false_type {};
template<> struct is_string<ustring> : std::true_type {};
template<> struct is_string<string_view> : std::true_type {};
template<> struct is_string<std::string> : std::true_type {};
// clang-format on

}  // namespace pvt



// AttrDelegate should not be used directly in user code!
//
// This is a helper template that allows the notation
//    Class object;
//    object["name"] = value;
// to work as a shorthand for
//    object.attribute ("name", TypeDesc_of_value, &value);
// and for
//    value = object["name"].get<type>();
// to work as a shorthand for extracting the named attribute via
// object.getattribute() and assigning it to the result.
//
// Basically, the class in question needs to posess these three methods,
// working in the usual idiomatic way for OIIO:
//
//      void attribute (string_view name, TypeDesc type, void* data);
//      bool getattribute (string_view name, TypeDesc type, void *data);
//      TypeDesc getattributetype (string_view name);
//
// Then the array access notation can be added with the following members
// of the class C:
//
//     AttrDelegate<const C> operator[](string_view name) const
//     {
//         return { this, name };
//     }
//     AttrDelegate<C> operator[](string_view name) {
//         return { this, name };
//     }
//
// This allows the following convenient notation:
//
// 1. Adding attributes, type implied by the C++ type of what's assigned:
//        C obj;
//        obj["foo"] = 42;       // adds integer
//        obj["bar"] = 39.8f;    // adds float
//        obj["baz"] = "hello";  // adds string
// 2. Retrieving attributes:
//         int i = obj["foo"].get<int>();
//         float f = obj["bar"].get<float>();
//         std::string s = obj["baz"].get<std::string>();
//    If the object does not posess an attribute of that name (and/or of
//    that type), it will instead return the default value for that type.
//    A specific default value override may be provided as an argument to
//    the get() method:
//         float aspect = obj["aspectratio"].get<float>(1.0f);
//

template<class C> class AttrDelegate {
public:
    AttrDelegate(C* obj, string_view name)
        : m_obj(obj)
        , m_name(name)
        , m_readonly(std::is_const<C>::value)
    {
    }

    // Assignment to a delegate should copy the value into an attribute,
    // calling attribute(name,typedesc,&data). Except for strings, which are
    // handled separately.
    template<typename T,
             typename std::enable_if<!pvt::is_string<T>::value, int>::type = 0>
    inline const T& operator=(const T& val)
    {
        if (!m_readonly)
            const_cast<C*>(m_obj)->attribute(m_name, TypeDescFromC<T>::value(),
                                             &val);
        return val;
    }

    // String types are a special case because we don't directly copy their
    // data, instead call the attribute(name,string_view) variant.
    template<typename T,
             typename std::enable_if<pvt::is_string<T>::value, int>::type = 1>
    inline const T& operator=(const T& val)
    {
        if (!m_readonly)
            const_cast<C*>(m_obj)->attribute(m_name, string_view(val));
        return val;
    }
    // char arrays are special
    inline const char* operator=(const char* val)
    {
        if (!m_readonly)
            const_cast<C*>(m_obj)->attribute(m_name, string_view(val));
        return val;
    }


    // `Delegate->type()` returns the TypeDesc describing the data, or
    // TypeUnknown if no such attribute exists.
    TypeDesc type() const { return m_obj->getattributetype(m_name); }

    // `Delegate->get<T>(defaultval=T())` retrieves the data as type T,
    // or the defaultval if no such named data exists or is not the
    // designated type.
    template<typename T,
             typename std::enable_if<!pvt::is_string<T>::value, int>::type = 0>
    inline T get(const T& defaultval = T()) const
    {
        T result;
        if (!m_obj->getattribute(m_name, TypeDescFromC<T>::value(), &result))
            result = defaultval;
        return result;
    }

    // Using enable_if, make a slightly different version of get<> for
    // strings, which need to do some ustring magic because we can't
    // directly store in a std::string or string_view.
    template<typename T = string_view,
             typename std::enable_if<pvt::is_string<T>::value, int>::type = 1>
    inline T get(const T& defaultval = T()) const
    {
        ustring s;
        return m_obj->getattribute(m_name, TypeString, &s) ? T(s) : defaultval;
    }

    // `Delegate->get_indexed<T>(int index, defaultval=T())` retrieves the
    // index-th base value in the data as type T, or the defaultval if no
    // such named data exists or is not the designated type.
    template<typename T,
             typename std::enable_if<!pvt::is_string<T>::value, int>::type = 0>
    inline T get_indexed(int index, const T& defaultval = T()) const
    {
        T result;
        if (!m_obj->getattribute_indexed(m_name, index,
                                         TypeDescFromC<T>::value(), &result))
            result = defaultval;
        return result;
    }

    // Using enable_if, make a slightly different version of get_indexed<>
    // for strings, which need to do some ustring magic because we can't
    // directly store in a std::string or string_view.
    template<typename T = string_view,
             typename std::enable_if<pvt::is_string<T>::value, int>::type = 1>
    inline T get_indexed(int index, const T& defaultval = T()) const
    {
        ustring s;
        return m_obj->getattribute_indexed(m_name, index, TypeString, &s)
                   ? T(s)
                   : defaultval;
    }

    // `Delegate->as_string(defaultval="")` returns the data, no matter its
    // type, as a string. Returns the defaultval if no such data exists at
    // all.
    inline std::string as_string(const std::string& defaultval = std::string())
    {
        std::string s;
        TypeDesc t = m_obj->getattributetype(m_name);
        if (t == TypeString) {  // Attrib is a string? Return it.
            s = get<std::string>();
        } else if (t != TypeUnknown) {  // Non-string attrib? Convert.
            const int localsize = 64;   // Small types copy to stack, avoid new
            char localbuffer[localsize];
            char* buffer = localbuffer;
            std::unique_ptr<char[]> allocbuffer;
            if (t.size() > localsize) {
                allocbuffer.reset(new char[t.size()]);
                buffer = allocbuffer.get();
            }
            if (m_obj->getattribute(m_name, t, buffer))
                s = tostring(t, buffer);
            else
                s = defaultval;
        } else {  // No attrib? Return default.
            s = defaultval;
        }
        return s;
    }

    // Return the entire attribute (even if an array or aggregate) as a
    // `std::vector<T>`, calling `get_indexed` on each base element.
    template<typename T, typename Allocator = std::allocator<T>>
    inline std::vector<T, Allocator> as_vec() const
    {
        TypeDesc t      = m_obj->getattributetype(m_name);
        size_t basevals = t.basevalues();
        using Vec       = std::vector<T, Allocator>;
        Vec result;
        result.reserve(basevals);
        for (size_t i = 0; i < basevals; ++i)
            result.push_back(get_indexed<T>(int(i)));
        return result;
    }

    // Allow direct assignment to string, equivalent to calling as_string().
    inline operator std::string() { return as_string(); }

protected:
    C* m_obj;
    string_view m_name;
    bool m_readonly = false;
};


OIIO_NAMESPACE_END
