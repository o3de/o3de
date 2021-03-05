// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


/// \file
///
/// Define the ParamValue and ParamValueList classes, which are used to
/// store lists of arbitrary name/data pairs for internal storage of
/// parameter lists, attributes, geometric primitive data, etc.


#pragma once

#include <vector>

#include <OpenImageIO/attrdelegate.h>
#include <OpenImageIO/export.h>
#include <OpenImageIO/typedesc.h>
#include <OpenImageIO/ustring.h>


OIIO_NAMESPACE_BEGIN

/// ParamValue holds a parameter and a pointer to its value(s)
///
/// Nomenclature: if you have an array of 4 colors for each of 15 points...
///  - There are 15 VALUES
///  - Each value has an array of 4 ELEMENTS, each of which is a color
///  - A color has 3 COMPONENTS (R, G, B)
///
class OIIO_API ParamValue {
public:
    /// Interpolation types
    ///
    enum Interp {
        INTERP_CONSTANT = 0,  ///< Constant for all pieces/faces
        INTERP_PERPIECE = 1,  ///< Piecewise constant per piece/face
        INTERP_LINEAR   = 2,  ///< Linearly interpolated across each piece/face
        INTERP_VERTEX   = 3   ///< Interpolated like vertices
    };

    ParamValue() noexcept { m_data.ptr = nullptr; }
    ParamValue(const ustring& _name, TypeDesc _type, int _nvalues,
               const void* _value, bool _copy = true) noexcept
    {
        init_noclear(_name, _type, _nvalues, _value, _copy);
    }
    ParamValue(const ustring& _name, TypeDesc _type, int _nvalues,
               Interp _interp, const void* _value, bool _copy = true) noexcept
    {
        init_noclear(_name, _type, _nvalues, _interp, _value, _copy);
    }
    ParamValue(string_view _name, TypeDesc _type, int _nvalues,
               const void* _value, bool _copy = true) noexcept
    {
        init_noclear(ustring(_name), _type, _nvalues, _value, _copy);
    }
    ParamValue(string_view _name, TypeDesc _type, int _nvalues, Interp _interp,
               const void* _value, bool _copy = true) noexcept
    {
        init_noclear(ustring(_name), _type, _nvalues, _interp, _value, _copy);
    }
    ParamValue(string_view _name, int value) noexcept
    {
        init_noclear(ustring(_name), TypeDesc::INT, 1, &value);
    }
    ParamValue(string_view _name, float value) noexcept
    {
        init_noclear(ustring(_name), TypeDesc::FLOAT, 1, &value);
    }
    ParamValue(string_view _name, ustring value) noexcept
    {
        init_noclear(ustring(_name), TypeDesc::STRING, 1, &value);
    }
    ParamValue(string_view _name, string_view value) noexcept
    {
        ustring u(value);
        init_noclear(ustring(_name), TypeDesc::STRING, 1, &u);
    }

    // Set from string -- parse
    ParamValue(string_view _name, TypeDesc type, string_view value);

    // Copy constructor
    ParamValue(const ParamValue& p) noexcept
    {
        init_noclear(p.name(), p.type(), p.nvalues(), p.interp(), p.data(),
                     true);
    }
    ParamValue(const ParamValue& p, bool _copy) noexcept
    {
        init_noclear(p.name(), p.type(), p.nvalues(), p.interp(), p.data(),
                     _copy);
    }

    // Rvalue (move) constructor
    ParamValue(ParamValue&& p) noexcept
    {
        init_noclear(p.name(), p.type(), p.nvalues(), p.interp(), p.data(),
                     false);
        m_copy       = p.m_copy;
        m_nonlocal   = p.m_nonlocal;
        p.m_data.ptr = nullptr;  // make sure the old one won't free
    }

    ~ParamValue() noexcept { clear_value(); }

    void init(ustring _name, TypeDesc _type, int _nvalues, Interp _interp,
              const void* _value, bool _copy = true) noexcept
    {
        clear_value();
        init_noclear(_name, _type, _nvalues, _interp, _value, _copy);
    }
    void init(ustring _name, TypeDesc _type, int _nvalues, const void* _value,
              bool _copy = true) noexcept
    {
        init(_name, _type, _nvalues, INTERP_CONSTANT, _value, _copy);
    }
    void init(string_view _name, TypeDesc _type, int _nvalues,
              const void* _value, bool _copy = true) noexcept
    {
        init(ustring(_name), _type, _nvalues, _value, _copy);
    }
    void init(string_view _name, TypeDesc _type, int _nvalues, Interp _interp,
              const void* _value, bool _copy = true) noexcept
    {
        init(ustring(_name), _type, _nvalues, _interp, _value, _copy);
    }

    // Assignment
    const ParamValue& operator=(const ParamValue& p) noexcept
    {
        if (this != &p)
            init(p.name(), p.type(), p.nvalues(), p.interp(), p.data(),
                 p.m_copy);
        return *this;
    }
    const ParamValue& operator=(ParamValue&& p) noexcept
    {
        if (this != &p) {
            init(p.name(), p.type(), p.nvalues(), p.interp(), p.data(), false);
            m_copy       = p.m_copy;
            m_nonlocal   = p.m_nonlocal;
            p.m_data.ptr = nullptr;
        }
        return *this;
    }

    // FIXME -- some time in the future (after more cleanup), we should make
    // name() return a string_view, and use uname() for the rare time when
    // the caller truly requires the ustring.
    const ustring& name() const noexcept { return m_name; }
    const ustring& uname() const noexcept { return m_name; }
    TypeDesc type() const noexcept { return m_type; }
    int nvalues() const noexcept { return m_nvalues; }
    const void* data() const noexcept
    {
        return m_nonlocal ? m_data.ptr : &m_data;
    }
    int datasize() const noexcept
    {
        return m_nvalues * static_cast<int>(m_type.size());
    }
    Interp interp() const noexcept { return (Interp)m_interp; }
    void interp(Interp i) noexcept { m_interp = (unsigned char)i; }
    bool is_nonlocal() const noexcept { return m_nonlocal; }

    friend void swap(ParamValue& a, ParamValue& b) noexcept
    {
        auto tmp = std::move(a);
        a        = std::move(b);
        b        = std::move(tmp);
    }

    // Use with extreme caution! This is just doing a cast. You'd better
    // be really sure you are asking for the right type. Note that for
    // "string" data, you can get<ustring> or get<char*>, but it's not
    // a std::string.
    template<typename T> const T& get(int i = 0) const noexcept
    {
        return (reinterpret_cast<const T*>(data()))[i];
    }

    /// Retrive an integer, with converstions from a wide variety of type
    /// cases, including unsigned, short, byte. Not float. It will retrive
    /// from a string, but only if the string is entirely a valid int
    /// format. Unconvertable types return the default value.
    int get_int(int defaultval = 0) const;
    int get_int_indexed(int index, int defaultval = 0) const;

    /// Retrive a float, with converstions from a wide variety of type
    /// cases, including integers. It will retrive from a string, but only
    /// if the string is entirely a valid float format. Unconvertable types
    /// return the default value.
    float get_float(float defaultval = 0) const;
    float get_float_indexed(int index, float defaultval = 0) const;

    /// Convert any type to a string value. An optional maximum number of
    /// elements is also passed. In the case of a single string, just the
    /// string directly is returned. But for an array of strings, the array
    /// is returned as one string that's a comma-separated list of double-
    /// quoted, escaped strings.
    std::string get_string(int maxsize = 64) const;
    std::string get_string_indexed(int index) const;
    /// Convert any type to a ustring value. An optional maximum number of
    /// elements is also passed. Same behavior as get_string, but returning
    /// a ustring.
    ustring get_ustring(int maxsize = 64) const;
    ustring get_ustring_indexed(int index) const;

private:
    ustring m_name;   ///< data name
    TypeDesc m_type;  ///< data type, which may itself be an array
    union {
        char localval[16];
        const void* ptr;
    } m_data;  ///< Our data, either a pointer or small local value
    int m_nvalues          = 0;  ///< number of values of the given type
    unsigned char m_interp = INTERP_CONSTANT;  ///< Interpolation type
    bool m_copy            = false;
    bool m_nonlocal        = false;

    void init_noclear(ustring _name, TypeDesc _type, int _nvalues,
                      const void* _value, bool _copy = true) noexcept;
    void init_noclear(ustring _name, TypeDesc _type, int _nvalues,
                      Interp _interp, const void* _value,
                      bool _copy = true) noexcept;
    void clear_value() noexcept;
};



/// A list of ParamValue entries, that can be iterated over or searched.
/// It's really just a std::vector<ParamValue>, but with a few more handy
/// methods.
class OIIO_API ParamValueList : public std::vector<ParamValue> {
public:
    ParamValueList() {}

    /// Add space for one more ParamValue to the list, and return a
    /// reference to its slot.
    reference grow()
    {
        resize(size() + 1);
        return back();
    }

    /// Find the first entry with matching name, and if type != UNKNOWN,
    /// then also with matching type. The name search is case sensitive if
    /// casesensitive == true.
    iterator find(string_view name, TypeDesc type = TypeDesc::UNKNOWN,
                  bool casesensitive = true);
    iterator find(ustring name, TypeDesc type = TypeDesc::UNKNOWN,
                  bool casesensitive = true);
    const_iterator find(string_view name, TypeDesc type = TypeDesc::UNKNOWN,
                        bool casesensitive = true) const;
    const_iterator find(ustring name, TypeDesc type = TypeDesc::UNKNOWN,
                        bool casesensitive = true) const;

    /// Search for the first entry with matching name, etc., and return
    /// a pointer to it, or nullptr if it is not found.
    ParamValue* find_pv(string_view name, TypeDesc type = TypeDesc::UNKNOWN,
                        bool casesensitive = true)
    {
        iterator f = find(name, type, casesensitive);
        return f != end() ? &(*f) : nullptr;
    }
    const ParamValue* find_pv(string_view name,
                              TypeDesc type      = TypeDesc::UNKNOWN,
                              bool casesensitive = true) const
    {
        const_iterator f = find(name, type, casesensitive);
        return f != cend() ? &(*f) : nullptr;
    }

    /// Case insensitive search for an integer, with default if not found.
    /// Automatically will return an int even if the data is really
    /// unsigned, short, or byte, but not float. It will retrive from a
    /// string, but only if the string is entirely a valid int format.
    int get_int(string_view name, int defaultval = 0,
                bool casesensitive = false, bool convert = true) const;

    /// Case insensitive search for a float, with default if not found.
    /// Automatically will return a float even if the data is really double
    /// or half. It will retrive from a string, but only if the string is
    /// entirely a valid float format.
    float get_float(string_view name, float defaultval = 0,
                    bool casesensitive = false, bool convert = true) const;

    /// Simple way to get a string attribute, with default provided.
    /// If the value is another type, it will be turned into a string.
    string_view get_string(string_view name,
                           string_view defaultval = string_view(),
                           bool casesensitive     = false,
                           bool convert           = true) const;
    ustring get_ustring(string_view name,
                        string_view defaultval = string_view(),
                        bool casesensitive = false, bool convert = true) const;

    /// Remove the named parameter, if it is in the list.
    void remove(string_view name, TypeDesc type = TypeDesc::UNKNOWN,
                bool casesensitive = true);

    /// Does the list contain the named attribute?
    bool contains(string_view name, TypeDesc type = TypeDesc::UNKNOWN,
                  bool casesensitive = true) const;

    // Add the param to the list, replacing in-place any existing one with
    // the same name.
    void add_or_replace(const ParamValue& pv, bool casesensitive = true);
    void add_or_replace(ParamValue&& pv, bool casesensitive = true);

    /// Add (or replace) a value in the list.
    void attribute(string_view name, TypeDesc type, int nvalues,
                   const void* value)
    {
        if (!name.empty())
            add_or_replace(ParamValue(name, type, nvalues, value));
    }

    void attribute(string_view name, TypeDesc type, const void* value)
    {
        attribute(name, type, 1, value);
    }

    /// Set directly from string -- parse if type is non-string.
    void attribute(string_view name, TypeDesc type, string_view value)
    {
        if (!name.empty())
            add_or_replace(ParamValue(name, type, value));
    }

    // Shortcuts for single value of common types.
    void attribute(string_view name, int value)
    {
        attribute(name, TypeInt, 1, &value);
    }
    void attribute(string_view name, unsigned int value)
    {
        attribute(name, TypeUInt, 1, &value);
    }
    void attribute(string_view name, float value)
    {
        attribute(name, TypeFloat, 1, &value);
    }
    void attribute(string_view name, string_view value)
    {
        ustring v(value);
        attribute(name, TypeString, 1, &v);
    }

    /// Search list for named item, return its type or TypeUnknnown if not
    /// found.
    TypeDesc getattributetype(string_view name,
                              bool casesensitive = false) const
    {
        auto p = find(name, TypeUnknown, casesensitive);
        return p != cend() ? p->type() : TypeUnknown;
    }

    /// Retrieve from list: If found its data type is reasonably convertible
    /// to `type`, copy/convert the value into val[...] and return true.
    /// Otherwise, return false and don't modify what val points to.
    bool getattribute(string_view name, TypeDesc type, void* value,
                      bool casesensitive = false) const;
    /// Shortcut for retrieving a single string via getattribute.
    bool getattribute(string_view name, std::string& value,
                      bool casesensitive = false) const;

    /// Retrieve from list: If found its data type is reasonably convertible
    /// to `type`, copy/convert the value into val[...] and return true.
    /// Otherwise, return false and don't modify what val points to.
    bool getattribute_indexed(string_view name, int index, TypeDesc type,
                              void* value, bool casesensitive = false) const;
    /// Shortcut for retrieving a single string via getattribute.
    bool getattribute_indexed(string_view name, int index, std::string& value,
                              bool casesensitive = false) const;

    /// Sort alphabetically, optionally case-insensitively, locale-
    /// independently, and with all the "un-namespaced" items appearing
    /// first, followed by items with "prefixed namespaces" (e.g. "z" comes
    /// before "foo:a").
    void sort(bool casesensitive = true);

    /// Merge items from PVL `other` into `*this`. Note how this differs
    /// from `operator=` : assignment completely replaces the list with
    /// the contents of another. But merge() adds the other items without
    /// erasing any items already in this list.
    ///
    /// @param other
    ///     The ParamValueList whose entries will be merged into this one.
    /// @param override
    ///     If true, `other` attributes will replace any identically-named
    ///     attributes already in this list. If false, only attributes whose
    ///     names are not already in this list will be appended.
    void merge(const ParamValueList& other, bool override = false);

    /// Even more radical than clear, free ALL memory associated with the
    /// list itself.
    void free()
    {
        clear();
        shrink_to_fit();
    }

    /// Array indexing by integer will return a reference to the ParamValue
    /// in that position of the list.
    ParamValue& operator[](int index)
    {
        return std::vector<ParamValue>::operator[](index);
    }
    const ParamValue& operator[](int index) const
    {
        return std::vector<ParamValue>::operator[](index);
    }

    /// Array indexing by string will create a "Delegate" that enables a
    /// convenient shorthand for adding and retrieving values from the list:
    ///
    /// 1. Assigning to the delegate adds a ParamValue to the list:
    ///        ParamValueList list;
    ///        list["foo"] = 42;       // adds integer
    ///        list["bar"] = 39.8f;    // adds float
    ///        list["baz"] = "hello";  // adds string
    ///    Be very careful, the attribute's type will be implied by the C++
    ///    type of what you assign.
    ///
    /// 2. The delegate supports a get<T>() that retrieves an item of type T:
    ///         int i = list["foo"].get<int>();
    ///         std::string s = list["baz"].get<std::string>();
    ///
    AttrDelegate<const ParamValueList> operator[](string_view name) const
    {
        return { this, name };
    }
    AttrDelegate<ParamValueList> operator[](string_view name)
    {
        return { this, name };
    }
};



OIIO_NAMESPACE_END
