/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Description : Implementation of TypeInfo classes and functions.

#include <AzCore/base.h>
#include "CryTypeInfo.h"
#include "CryCustomTypes.h"
#include "Cry_Math.h"
#include "CrySizer.h"
#include "CryEndian.h"
#include "TypeInfo_impl.h"

// Traits
#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(CryTypeInfo_cpp)
#elif defined(LINUX) || defined(APPLE)
#define CRYTYPEINFO_CPP_TRAIT_DEFINE_LTOA_S 1
#endif

#if CRYTYPEINFO_CPP_TRAIT_DEFINE_LTOA_S

char* _ltoa_s(long value, char* string, size_t size, int32 radix)
{
    if (10 == radix)
    {
        sprintf_s(string, size, "%ld", value);
    }
    else
    {
        sprintf_s(string, size, "%lx", value);
    }
    return(string);
}


char* _i64toa_s(int64 value, char* string, size_t size, int32 radix)
{
    if (10 == radix)
    {
        sprintf_s(string, size, "%llu", (unsigned long long)value);
    }
    else
    {
        sprintf_s(string, size, "%llx", (unsigned long long)value);
    }
    return(string);
}

char* _ultoa_s(uint32 value, char* string, size_t size, int32 radix)
{
    if (10 == radix)
    {
        sprintf_s(string, size, "%.d", value);
    }
    else
    {
        sprintf_s(string, size, "%.x", value);
    }
    return(string);
}

#endif //LINUX || MAC

//////////////////////////////////////////////////////////////////////
// Case-insensitive comparison helpers.

class StrNoCase
{
public:
    inline StrNoCase(cstr str)
        : m_Str(str) {}
    inline bool operator == (cstr str) const        { return azstricmp(m_Str, str) == 0; }
    inline bool operator != (cstr str) const        { return azstricmp(m_Str, str) != 0; }
private:
    cstr    m_Str;
};

// Default Endian swap function, forwards to TypeInfo.
void SwapEndian(const CTypeInfo& Info, [[maybe_unused]] size_t nSizeCheck, void* data, size_t nCount, bool bWriting)
{
    assert(nSizeCheck == Info.Size);
    Info.SwapEndian(data, nCount, bWriting);
}

//////////////////////////////////////////////////////////////////////
// Basic TypeInfo implementations.

// Basic type infos.

DEFINE_TYPE_INFO(void, CTypeInfo, ("void", 0, 0))

TYPE_INFO_BASIC(bool)
TYPE_INFO_BASIC(char)
TYPE_INFO_BASIC(wchar_t)

TYPE_INFO_INT(signed char)
TYPE_INFO_INT(unsigned char)
TYPE_INFO_INT(short)
TYPE_INFO_INT(unsigned short)
TYPE_INFO_INT(int)
TYPE_INFO_INT(unsigned int)
TYPE_INFO_INT(long)
TYPE_INFO_INT(unsigned long)
TYPE_INFO_INT(int64)
TYPE_INFO_INT(uint64)

TYPE_INFO_BASIC(float)
TYPE_INFO_BASIC(double)

TYPE_INFO_BASIC(string)


const CTypeInfo&PtrTypeInfo()
{
    static CTypeInfo Info(TYPE_INFO_NAME(void*), sizeof(void*), alignof(void*));
    return Info;
}

//////////////////////////////////////////////////////////////////////
// Basic type info implementations.

// String conversion functions needed by TypeInfo.

// bool
string ToString(bool const& val)
{
    static string sTrue = "true", sFalse = "false";
    return val ? sTrue : sFalse;
}

bool FromString(bool& val, cstr s)
{
    if (!strcmp(s, "0") || !azstricmp(s, "false"))
    {
        val = false;
        return true;
    }
    if (!strcmp(s, "1") || !azstricmp(s, "true"))
    {
        val = true;
        return true;
    }
    return false;
}

// int64
string ToString(int64 const& val)
{
    char buffer[64];
    _i64toa_s(val, buffer, sizeof(buffer), 10);
    return buffer;
}
// uint64
string ToString(uint64 const& val)
{
    char buffer[64];
    sprintf_s(buffer, "%" PRIu64, val);
    return buffer;
}



// long
string ToString(long const& val)
{
    char buffer[64];
    _ltoa_s(val, buffer, sizeof(buffer), 10);
    return buffer;
}

// ulong
string ToString(unsigned long const& val)
{
    char buffer[64];
    _ultoa_s(val, buffer, sizeof(buffer), 10);
    return buffer;
}

template<class T>
bool ClampedIntFromString(T& val, const char* s)
{
    bool signbit = *s == '-';
    s += signbit;
    if (signbit && !TIntTraits<T>::bSIGNED)
    {
        // Negative number on unsigned.
        val = T(0);
        return true;
    }

    uint digit = (uint8) * s - '0';
    if (digit > 9)
    {
        // No digits.
        return false;
    }

    // Extract digits until overflow.
    T v = static_cast<T>(digit);
    while ((digit = (uint8) * ++s - '0') <= 9)
    {
        T vnew = static_cast<T>(v * 10 + digit);
        if (vnew < v)
        {
            // Overflow.
            if (signbit)
            {
                val = TIntTraits<T>::nMIN;
            }
            else
            {
                val = TIntTraits<T>::nMAX;
            }
            return true;
        }
        v = vnew;
    }

    val = signbit ? ~v + 1 : v;
    return true;
}

bool FromString(int64& val, const char* s)                      { return ClampedIntFromString(val, s); }
bool FromString(uint64& val, const char* s)                     { return ClampedIntFromString(val, s); }

bool FromString(long& val, const char* s)                           { return ClampedIntFromString(val, s); }
bool FromString(unsigned long& val, const char* s)      { return ClampedIntFromString(val, s); }

string ToString(int const& val)                                             { return ToString(long(val)); }
bool FromString(int& val, const char* s)                            { return ClampedIntFromString(val, s); }

string ToString(unsigned int const& val)                            { return ToString((unsigned long)(val)); }
bool FromString(unsigned int& val, const char* s)           { return ClampedIntFromString(val, s); }

string ToString(short const& val)                                           { return ToString(long(val)); }
bool FromString(short& val, const char* s)                      {   return ClampedIntFromString(val, s); }

string ToString(unsigned short const& val)                      { return ToString((unsigned long)(val)); }
bool FromString(unsigned short& val, const char* s)     {   return ClampedIntFromString(val, s); }

string ToString(char const& val)                                            { return ToString(long(val)); }
bool FromString(char& val, const char* s)                           {   return ClampedIntFromString(val, s); }

string ToString(wchar_t const& val)                                     { return ToString(long(val)); }
bool FromString(wchar_t& val, const char* s)                    {   return ClampedIntFromString(val, s); }

string ToString(signed char const& val)                             { return ToString(long(val)); }
bool FromString(signed char& val, const char* s)            {   return ClampedIntFromString(val, s); }

string ToString(unsigned char const& val)                           { return ToString((unsigned long)(val)); }
bool FromString(unsigned char& val, const char* s)      {   return ClampedIntFromString(val, s); }

string ToString(const AZ::Uuid& val)
{
    return val.ToString<string>();
}

bool FromString(AZ::Uuid& val, const char* s)
{
    val = AZ::Uuid(s);
    return true;
}

float NumToFromString(float val, int digits, bool floating, char buffer[], int buf_size)
{
    assert(buf_size >= 32);
    if (floating)
    {
        if (val >= powf(10.f, float(digits)))
        {
            sprintf_s(buffer, buf_size, "%.0f", val);
        }
        else
        {
            sprintf_s(buffer, buf_size, "%.*g", digits, val);
        }
    }
    else
    {
        sprintf_s(buffer, buf_size, "%.*f", digits, float(val));
    }
#if !defined(NDEBUG)
    int readCount =
#endif
        azsscanf(buffer, "%g", &val);
    assert(readCount == 1);
    return val;
}

// double
string ToString(double const& val)
{
    char buffer[64];
    sprintf_s(buffer, "%.16g", val);
    return buffer;
}
bool FromString(double& val, const char* s)
{
    return azsscanf(s, "%lg", &val) == 1;
}

// float
string ToString(float const& val)
{
    char buffer[64];
    for (int digits = 7; digits < 10; digits++)
    {
        if (NumToFromString(val, digits, true, buffer, 64) == val)
        {
            break;
        }
    }
    return buffer;
}

bool FromString(float& val, const char* s)
{
    return azsscanf(s, "%g", &val) == 1;
}



// string override.
template <>
void TTypeInfo<string>::GetMemoryUsage(ICrySizer* pSizer, void const* data) const
{
    // CRAIG: just a temp hack to try and get things working
#if !defined(LINUX) && !defined(APPLE)
    pSizer->AddString(*(string*)data);
#endif
}

#if defined(TEST_TYPEINFO) && defined(_DEBUG)

struct STypeInfoTest
{
    STypeInfoTest()
    {
        TestType(string("well"));

        TestType(true);

        TestType(int8(-0x12));
        TestType(uint8(0x87));
        TestType(int16(-0x1234));
        TestType(uint16(0x8765));
        TestType(int32(-0x12345678));
        TestType(uint32(0x87654321));
        TestType(int64(-0x123456789ABCDEF0LL));
        TestType(uint64(0xFEDCBA9876543210LL));

        TestType(float(1234.5678));
        TestType(float(12345678));
        TestType(float(12345678e-20));
        TestType(float(12345678e20));

        TestType(double(987654321.0123456789));
        TestType(double(9876543210123456789.0));
        TestType(double(9876543210123456789e-40));
        TestType(double(9876543210123456789e40));
    }
};
static STypeInfoTest _TypeInfoTest;

#endif //TEST_TYPEINFO


//////////////////////////////////////////////////////////////////////
// CTypeInfo implementation

//---------------------------------------------------------------------------
// Endian helper functions.

void CTypeInfo::SwapEndian(void* pData, size_t nCount, [[maybe_unused]] bool bWriting) const
{
    switch (Size)
    {
    case 1:
        break;
    case 2:
        ::SwapEndianBase((uint16*)pData, nCount);
        break;
    case 4:
        ::SwapEndianBase((uint32*)pData, nCount);
        break;
    case 8:
        ::SwapEndianBase((uint64*)pData, nCount);
        break;
    default:
        assert(0);
    }
}

// If attr name is found, return pointer to start of value text; else 0.
static cstr FindAttr(cstr attrs, cstr name)
{
    size_t name_len = strlen(name);
    while (attrs)
    {
        attrs = strchr(attrs, '<');
        if (!attrs)
        {
            return 0;
        }

        attrs++;
        size_t attr_len = strcspn(attrs, "=>");
        if (attr_len == name_len && _strnicmp(attrs, name, name_len) == 0)
        {
            attrs += attr_len;
            if (*attrs == '=')
            {
                ++attrs;
            }
            return attrs;
        }
        attrs += attr_len;
        if (*attrs == '=')
        {
            attrs = strchr(attrs + 1, '>');
        }
    }
    return 0;
}

bool CTypeInfo::CVarInfo::GetAttr(cstr name) const
{
    return FindAttr(Attrs, name) != 0;
}

bool CTypeInfo::CVarInfo::GetAttr(cstr name, string& val) const
{
    cstr valstr = FindAttr(Attrs, name);
    if (!valstr)
    {
        return false;
    }

    // Find attr delimiter.
    cstr end = strchr(valstr, '>');
    if (!end)
    {
        return false;
    }

    // Strip quotes.
    if (*valstr == '"')
    {
        valstr++;
        if (end - valstr > 1 && end[-1] == '"')
        {
            end--;
        }
    }
    val = string(valstr, end - valstr);
    return true;
}

bool CTypeInfo::CVarInfo::GetAttr(cstr name, float& val) const
{
    cstr valstr = FindAttr(Attrs, name);
    if (!valstr)
    {
        return false;
    }
    val = (float)atof(valstr);
    return true;
}

cstr CTypeInfo::CVarInfo::GetComment() const
{
    cstr send = strrchr(Attrs, '>');
    if (send)
    {
        do
        {
            ++send;
        } while (*send == ' ');
        return send;
    }
    else
    {
        return Attrs;
    }
}

//////////////////////////////////////////////////////////////////////
// CStructInfo implementation

inline cstr DisplayName(cstr name)
{
    // Skip prefixes in Name.
    cstr dname = name;
    while (islower((unsigned char)*dname) || *dname == '_')
    {
        dname++;
    }
    if (isupper((unsigned char)*dname))
    {
        return dname;
    }
    else
    {
        return name;
    }
}

CStructInfo::CStructInfo(cstr name, size_t size, size_t align, Array<CVarInfo> vars, Array<CTypeInfo const*> templates)
    : CTypeInfo(name, size, align)
    , Vars(vars)
    , TemplateTypes(templates)
    , HasBitfields(false)
{
    // Process and validate offsets and sizes.
    if (Vars.size() > 0)
    {
        size = 0;
        int bitoffset = 0;

        for (int i = 0; i < Vars.size(); i++)
        {
            CStructInfo::CVarInfo& var = Vars[i];

            // Convert name.
            var.Name = DisplayName(var.Name);

            if (var.bBitfield)
            {
                HasBitfields = true;
                if (bitoffset > 0)
                {
                    // Continuing bitfield.
                    var.Offset = Vars[i - 1].Offset;
                    var.BitWordWidth = Vars[i - 1].BitWordWidth;

                    if (bitoffset + var.ArrayDim > var.GetSize() * 8)
                    {
                        // Overflows word, start on next one.
                        bitoffset = 0;
                        size += var.GetSize();
                    }
                }

                if (bitoffset == 0)
                {
                    var.Offset = check_cast<uint32>(size);

                    // Detect real word size of bitfield, from offset of next field.
                    size_t next_offset = Size;
                    for (int j = i + 1; j < Vars.size(); j++)
                    {
                        if (!Vars[j].bBitfield)
                        {
                            next_offset = Vars[j].Offset;
                            break;
                        }
                    }
                    assert(next_offset > size);
                    size_t wordsize = min(next_offset - size, var.Type.Size);
                    size = next_offset;
                    switch (wordsize)
                    {
                    case 1:
                        var.BitWordWidth = 0;
                        break;
                    case 2:
                        var.BitWordWidth = 1;
                        break;
                    case 4:
                        var.BitWordWidth = 2;
                        break;
                    case 8:
                        var.BitWordWidth = 3;
                        break;
                    default:
                        assert(0);
                    }
                }

                assert(var.ArrayDim <= var.GetSize() * 8);
                var.BitOffset = bitoffset;
                bitoffset += var.ArrayDim;
            }
            else
            {
                bitoffset = 0;
                if (var.Offset >= size)
                {
                    size = var.Offset + var.GetSize();
                }
            }
        }
        assert(Align(size, Alignment) == Align(Size, Alignment));
    }
}

size_t EndianDescSize(cstr desc)
{
    // Iterate the endian descriptor.
    size_t nSize = 0;
    for (; *desc; desc++)
    {
        size_t count = *desc & 0x3F;
        size_t nType = *(uint8*)desc >> 6;
        nSize += count << nType;
    }
    return nSize;
}

size_t CStructInfo::AddEndianDesc(cstr desc, size_t dim, size_t elem_size)
{
    if (dim == 0)
    {
        return 0;
    }

    size_t endian_size = EndianDescSize(desc);
    size_t total_size = elem_size * (dim - 1) + endian_size;

    if (desc[1] || (endian_size < elem_size && dim > 1))
    {
        // Composite endian descriptor, replicate it.
        assert(endian_size <= elem_size);
        assert(elem_size - endian_size < 0x40);
        while (dim-- > 0)
        {
            EndianDesc += (cstr)desc;
            if (dim > 1 && endian_size < elem_size)
            {
                EndianDesc += char(0x40 | (elem_size - endian_size));
            }
        }
    }
    else
    {
        // Single endian component. Replicate using count field.
        size_t subdim = desc[0] & 0x3F;
        dim *= subdim;
        if (!EndianDesc.empty())
        {
            // Combine with previous component if possible.
            char prevdesc = *(EndianDesc.end() - 1);
            if ((prevdesc & ~0x3F) == (desc[0] & ~0x3F))
            {
                // Combine with previous char.
                size_t maxdim = min(dim, size_t(0x3F - (prevdesc & 0x3F)));
                prevdesc += check_cast<char>(maxdim);
                EndianDesc.erase(EndianDesc.length() - 1, 1);
                EndianDesc.append(1, prevdesc);
                dim -= maxdim;
            }
        }
        for (; dim > 0x3F; dim -= 0x3F)
        {
            EndianDesc += desc[0] | 0x3F;
        }
        if (dim > 0)
        {
            EndianDesc += check_cast<char>((desc[0] & ~0x3F) | dim);
        }
    }

    return total_size;
}

bool CStructInfo::FromValue(void* data, const void* value, const CTypeInfo& typeVal) const
{
    if (IsCompatibleType(typeVal))
    {
        bool bOK = true;
        int i = 0;
        for AllSubVars(pToVar, typeVal)
        {
            if (!Vars[i].Type.FromValue(Vars[i].GetAddress(data), pToVar->GetAddress(value), pToVar->Type))
            {
                bOK = false;
            }
            i++;
        }
        return bOK;
    }

    // Check all subclasses.
    for (int i = 0; i < Vars.size() && Vars[i].IsBaseClass(); i++)
    {
        if (Vars[i].Type.FromValue(Vars[i].GetAddress(data), value, typeVal))
        {
            return true;
        }
    }
    return false;
}

bool CStructInfo::ToValue(const void* data, void* value, const CTypeInfo& typeVal) const
{
    if (IsCompatibleType(typeVal))
    {
        bool bOK = true;
        int i = 0;
        for AllSubVars(pToVar, typeVal)
        {
            if (!Vars[i].Type.ToValue(Vars[i].GetAddress(data), pToVar->GetAddress(value), pToVar->Type))
            {
                bOK = false;
            }
            i++;
        }
        return bOK;
    }

    // Check all subclasses.
    for (int i = 0; i < Vars.size() && Vars[i].IsBaseClass(); i++)
    {
        if (Vars[i].Type.ToValue(Vars[i].GetAddress(data), value, typeVal))
        {
            return true;
        }
    }
    return false;
}

// Parse structs as comma-separated values.

/*                                  ,               1,      ,2      1,2

        Top                                         1           ,2      1,2             ; strip trail commas
        Child   Named                           1           (,2)    (1,2)           ; strip trail commas, paren if internal commas
                    Nameless    ,               1,      ,2      1,2             ;
*/

static void StripCommas(string& str)
{
    size_t nLast = str.size();
    while (nLast > 0 && str[nLast - 1] == ',')
    {
        nLast--;
    }
    str.resize(nLast);
}

string CStructInfo::ToString(const void* data, FToString flags, const void* def_data) const
{
    string str;                     // Return str.

    for (int i = 0; i < Vars.size(); i++)
    {
        // Handling of empty values: Skip trailing empty values.
        // If there are intermediate empty values, replace them with non-empty ones.
        const CVarInfo& var = Vars[i];

        if (!var.IsInline())
        {
            // Named sub var or struct.
            if (!flags.NamedFields && i > 0)
            {
                str += ",";
            }

            string substr = var.ToString(data, FToString(flags).Sub(0), def_data);

            if (flags.SkipDefault && substr.empty())
            {
                continue;
            }

            if (flags.NamedFields)
            {
                if (*str)
                {
                    str += ",";
                }
                if (*var.Name)
                {
                    str += var.Name;
                    str += "=";
                }
            }
            if (substr.find(',') != string::npos || substr.find('=') != string::npos)
            {
                // Encase nested composite types in parens.
                str += "(";
                str += substr;
                str += ")";
            }
            else
            {
                str += substr;
            }
        }
        else
        {
            // Nameless base struct. Treat children as inline.
            str += var.ToString(data, FToString(flags).Sub(1), def_data);
        }
    }

    if (flags.SkipDefault && !flags.Sub)
    {
        StripCommas(str);
    }
    return str;
}

// Retrieve and return one subelement from src, advancing the pointer.
// Copy to tempstr if necessary.

typedef CryStackStringT<char, 256> CTempStr;

void ParseElement(cstr& src, cstr& varname, cstr& val, CTempStr& tempstr)
{
    varname = val = 0;

    while (*src == ' ')
    {
        src++;
    }
    if (!*src)
    {
        return;
    }

    // Find end of var assignment.
    int nest = 0;
    cstr eq = 0;

    cstr end;
    for (end = src; *end; end++)
    {
        if (*end == '(')
        {
            nest++;
        }
        else if (*end == ')')
        {
            nest--;
        }
        else if (nest == 0)
        {
            if (*end == '=' && !eq)
            {
                eq = end;
            }
            else if (*end == ',')
            {
                break;
            }
        }
    }

    // Advance src past element.
    val = src;
    src = end;

    if (*src == ',')
    {
        src++;
    }

    if (eq)
    {
        varname = val;
        val = eq + 1;
    }

    PREFAST_ASSUME(val);
    if (*val == '(' && end[-1] == ')')
    {
        // Remove parens.
        val++;
        end--;
    }

    if (eq)
    {
        if (*end)
        {
            // Must copy sub string to temp.
            val = (cstr)tempstr + (val - varname);
            eq = (cstr)tempstr + (eq - varname);
            varname = tempstr.assign(varname, end);
            non_const(*eq) = 0;
        }
        else
        {
            // Copy just varname to temp, return val in place.
            varname = tempstr.assign(varname, eq);
        }
    }
    else if (*end)
    {
        // Must copy sub string to temp.
        val = tempstr.assign(val, end);
    }

    // Else can return val without copying.
}

bool CStructInfo::FromString(void* data, cstr str, FFromString flags) const
{
    if (!flags.SkipEmpty)
    {
        // Initialise all to default
        for (int i = 0; i < Vars.size(); i++)
        {
            Vars[i].FromString(data, "");
        }
    }

    CTempStr tempstr;
    const CVarInfo* pVar = 0;
    int nErrors = 0;

    while (*str)
    {
        cstr varname, val;
        ParseElement(str, varname, val, tempstr);

        if (varname)
        {
            pVar = FindSubVar(varname);
        }
        else
        {
            pVar = NextSubVar(pVar, true);
        }
        if (pVar)
        {
            if (*val || !flags.SkipEmpty)
            {
                nErrors += !pVar->FromString(data, val, flags);
            }
        }
        else
        {
            nErrors++;
        }
    }

    return !nErrors;
}

bool CStructInfo::ValueEqual(const void* data, const void* def_data) const
{
    for (int i = 0; i < Vars.size(); i++)
    {
        const CVarInfo& var = Vars[i];
        if (!var.Type.ValueEqual((char*)data + var.Offset, (def_data ? (char*)def_data + var.Offset : 0)))
        {
            return false;
        }
    }
    return true;
}

void CStructInfo::SwapEndian(void* data, size_t nCount, bool bWriting) const
{
    non_const(*this).MakeEndianDesc();

    if (EndianDesc.length() == 1 && !HasBitfields && EndianDescSize(EndianDesc) == Size)
    {
        // Optimised array swap.
        size_t nElems = (EndianDesc[0u] & 0x3F) * nCount;
        switch (EndianDesc[0u] & 0xC0)
        {
        case 0:             // Skip bytes
            break;
        case 0x40:      // Swap 2 bytes
            ::SwapEndianBase((uint16*)data, nElems);
            break;
        case 0x80:      // Swap 4 bytes
            ::SwapEndianBase((uint32*)data, nElems);
            break;
        case 0xC0:      // Swap 8 bytes
            ::SwapEndianBase((uint64*)data, nElems);
            break;
        }
        return;
    }

    for (; nCount-- > 0; data = (char*)data + Size)
    {
        // First swap bits.
        // Iterate the endian descriptor.
        void* step = data;
        for (cstr desc = EndianDesc; *desc; desc++)
        {
            size_t nElems = *desc & 0x3F;
            switch (*desc & 0xC0)
            {
            case 0:             // Skip bytes
                step = (uint8*)step + nElems;
                break;
            case 0x40:      // Swap 2 bytes
                ::SwapEndianBase((uint16*)step, nElems);
                step = (uint16*)step + nElems;
                break;
            case 0x80:      // Swap 4 bytes
                ::SwapEndianBase((uint32*)step, nElems);
                step = (uint32*)step + nElems;
                break;
            case 0xC0:      // Swap 8 bytes
                ::SwapEndianBase((uint64*)step, nElems);
                step = (uint64*)step + nElems;
                break;
            }
        }

        // Then bitfields if needed.
        if (HasBitfields)
        {
            uint64 uOrigBits = 0, uNewBits = 0;
            for (int i = 0; i < Vars.size(); i++)
            {
                CVarInfo const& var = Vars[i];
                if (var.bBitfield)
                {
                    // Reverse location of all bitfields in word.
                    size_t nWordBits = var.GetElemSize() * 8;
                    assert(nWordBits <= 64);
                    if (var.BitOffset == 0)
                    {
                        // Initialise bitfield swapping.
                        var.Type.ToValue(var.GetAddress(data), uOrigBits);
                        uNewBits = 0;
                    }
                    size_t nSrcOffset = (GetPlatformEndian() == eLittleEndian) == bWriting ? var.BitOffset : nWordBits - var.GetBits() - var.BitOffset;
                    size_t nDstOffset = nWordBits - var.GetBits() - nSrcOffset;

                    uint64 uFieldVal = uOrigBits >> nSrcOffset;
                    uFieldVal &= ((1 << var.GetBits()) - 1);
                    uNewBits |= uFieldVal << nDstOffset;
                    var.Type.FromValue(var.GetAddress(data), uNewBits);
                }
            }
        }
    }
}

void CStructInfo::MakeEndianDesc()
{
    if (!EndianDesc.empty())
    {
        return;
    }

    size_t last_offset = 0;
    for (int i = 0; i < Vars.size(); i++)
    {
        CVarInfo const& var = Vars[i];
        bool bUnionAlias = var.bBitfield ? var.BitOffset > 0 : var.Offset < last_offset;
        if (!bUnionAlias)
        {
            // Add endian desc for member.
            cstr subdesc = 0;
            if (var.Type.HasSubVars())
            {
                // Struct-computed endian desc.
                CStructInfo const& infoSub = static_cast<CStructInfo const&>(var.Type);
                non_const(infoSub).MakeEndianDesc();
                subdesc = infoSub.EndianDesc;
                if (!*subdesc)
                {
                    // No swapping.
                    continue;
                }
            }
            else
            {
                // Basic type.
                switch (var.GetElemSize())
                {
                case 0:
                case 1:
                    continue;                                           // No swapping needed.
                case 2:
                    subdesc = "\x41";
                    break;
                case 4:
                    subdesc = "\x81";
                    break;
                case 8:
                    subdesc = "\xC1";
                    break;
                default:
                    assert(0);
                }
            }

            // Apply any padding to current offset.
            assert(last_offset <= var.Offset);
            if (last_offset < var.Offset)
            {
                last_offset += AddEndianDesc("\x01", var.Offset - last_offset, 1);
            }
            last_offset += AddEndianDesc(subdesc, var.GetDim(), var.GetElemSize());
        }
    }
}

void CStructInfo::GetMemoryUsage(ICrySizer* pSizer, void const* data) const
{
    for (int i = 0; i < Vars.size(); i++)
    {
        Vars[i].Type.GetMemoryUsage(pSizer, (char*)data + Vars[i].Offset);
    }
}

const CTypeInfo::CVarInfo* CStructInfo::NextSubVar(const CVarInfo* pPrev, bool bRecurseBase) const
{
    if (pPrev >= Vars.begin() && pPrev < Vars.end())
    {
        // pPrev is within this struct's vars
        if (++pPrev < Vars.end())
        {
            return pPrev;
        }
        return 0;
    }

    if (Vars.empty())
    {
        return 0;
    }

    if (bRecurseBase)
    {
        // Recurse into inline base structs
        const CVarInfo* pBase = Vars.begin();
        if (pBase->IsInline())
        {
            if (const CVarInfo* pNext = pBase->Type.NextSubVar(pPrev, true))
            {
                return pNext;
            }
            if (++pBase < Vars.end())
            {
                return pBase;
            }
            return 0;
        }
    }

    if (!pPrev)
    {
        // Return first var
        return Vars.begin();
    }

    return 0;
}

const CTypeInfo::CVarInfo* CStructInfo::FindSubVar(cstr name) const
{
    static int s_nLast = 0;
    int nSize = Vars.size();
    if (s_nLast >= nSize)
    {
        s_nLast = 0;
    }

    for (int i = s_nLast, iEnd = s_nLast + nSize; i < iEnd; i++)
    {
        int v = i >= nSize ? i - nSize : i;
        const CVarInfo& var = Vars[v];
        if (var.Type.Size > 0 && StrNoCase(var.GetName()) == name)
        {
            s_nLast = v;
            return &var;
        }
        if (var.IsBaseClass())
        {
            if (const CVarInfo* pSubVar = var.Type.FindSubVar(name))
            {
                return pSubVar;
            }
        }
    }
    return 0;
}

bool CStructInfo::IsCompatibleType(CTypeInfo const& Info) const
{
    if (this == &Info)
    {
        return true;
    }

    if (!TemplateTypes.empty() && Info.IsTemplate() && strcmp(Name, Info.Name) == 0)
    {
        CTypeInfo const* const* pA = NextTemplateType(0);
        CTypeInfo const* const* pB = Info.NextTemplateType(0);

        while (pA && pB && (*pA)->IsType(**pB))
        {
            pA = Info.NextTemplateType(pA);
            pB = Info.NextTemplateType(pB);
        }
        return !pA && !pB;
    }

    return false;
}

bool CStructInfo::IsType(CTypeInfo const& Info) const
{
    if (IsCompatibleType(Info))
    {
        return true;
    }

    // Check all subclasses.
    for (int i = 0; i < Vars.size() && Vars[i].IsBaseClass(); i++)
    {
        if (Vars[i].Type.IsType(Info))
        {
            return true;
        }
    }
    return false;
}

//////////////////////////////////////////////////////////////////////
// CSimpleEnumDef implementation

cstr ParseNextEnum(char*& rs)
{
    char* s = rs;
    while (isspace(*s))
    {
        s++;
    }
    if (!*s)
    {
        return 0;
    }

    cstr se = s;
    while (isalnum(*s) || *s == '_')
    {
        s++;
    }

    if (*s == ',')
    {
        *s++ = 0;
    }
    else if (*s)
    {
        *s++ = 0;
        while (*s && *s != ',')
        {
            s++;
        }
        if (*s)
        {
            s++;
        }
    }
    rs = s;
    return se;
}

void CSimpleEnumDef::Init(Array<cstr> names, char* enum_str)
{
    asNames = names;

    // Split copy of enums string into elements, store in array.
    int i = 0;
    while (cstr se = ParseNextEnum(enum_str))
    {
        asNames[i++] = se;
    }
}

//////////////////////////////////////////////////////////////////////
// CEnumDef implementation

void CEnumDef::Init(Array<SElem> elems, char* enum_str)
{
    Elems = elems;
    MinValue = 0;
    bRegular = true;
    nPrefixLength = 0;

    if (enum_str)
    {
        // Parse enum names from str
        int i = 0;
        while (cstr se = ParseNextEnum(enum_str))
        {
            Elems[i++].Name = se;
        }
    }

    // Analyse names and values.
    if (!elems.empty())
    {
        cstr sPrefix = "";
        MinValue =  Elems[0].Value;
        for (int i = 0; i < elems.size(); i++)
        {
            if (Elems[i].Value != i + Elems[0].Value)
            {
                bRegular = false;
            }
            MinValue = min(MinValue, Elems[i].Value);

            // Find common prefix.
            cstr sElem = Elems[i].Name;
            if (*sElem)
            {
                if (!*sPrefix)
                {
                    sPrefix = sElem;
                    nPrefixLength = static_cast<uint>(strlen(sPrefix));
                }
                else
                {
                    uint p = 0;
                    while (p < nPrefixLength && sElem[p] == sPrefix[p])
                    {
                        p++;
                    }
                    nPrefixLength = p;
                }
            }
        }

        // Ensure prefix is on underscore boundary.
        while (nPrefixLength > 0 && sPrefix[nPrefixLength - 1] != '_')
        {
            nPrefixLength--;
        }
    }
}

bool CEnumDef::MatchName(uint i, cstr str) const
{
    cstr name = Elems[i].Name;
    if (*name && nPrefixLength)
    {
        if (azstricmp(name + nPrefixLength, str) == 0)
        {
            return true;
        }
    }
    if (*name == '_')
    {
        name++;
    }
    return azstricmp(name, str) == 0;
}

cstr CEnumDef::ToName(TValue value) const
{
    // Find matching element.
    if (bRegular)
    {
        TValue index = value - MinValue;
        if (index >= 0 && index < Elems.size())
        {
            return Name((uint)index);
        }
    }
    else
    {
        for (int i = 0; i < Elems.size(); i++)
        {
            if (Elems[i].Value == value)
            {
                return Name(i);
            }
        }
    }

    // No match
    return 0;
}

LegacyDynArray<CEnumDef::SElem>* CEnumDef::SInit::s_pElems = 0;

void CEnumDefUuid::Init(Array<SElem> elems, char* enum_str)
{
    Elems = elems;
    bRegular = false;
    nPrefixLength = 0;

    if (enum_str)
    {
        // Parse enum names from str
        int i = 0;
        while (cstr se = ParseNextEnum(enum_str))
        {
            Elems[i++].Name = se;
        }
    }

    // Analyse names and values.
    if (!elems.empty())
    {
        cstr sPrefix = "";
        for (int i = 0; i < elems.size(); i++)
        {
            // Find common prefix.
            cstr sElem = Elems[i].Name;
            if (*sElem)
            {
                if (!*sPrefix)
                {
                    sPrefix = sElem;
                    nPrefixLength = static_cast<uint>(strlen(sPrefix));
                }
                else
                {
                    uint p = 0;
                    while (p < nPrefixLength && sElem[p] == sPrefix[p])
                    {
                        p++;
                    }
                    nPrefixLength = p;
                }
            }
        }

        // Ensure prefix is on underscore boundary.
        while (nPrefixLength > 0 && sPrefix[nPrefixLength - 1] != '_')
        {
            nPrefixLength--;
        }
    }
}

bool CEnumDefUuid::MatchName(uint i, cstr str) const
{
    cstr name = Elems[i].Name;
    if (*name && nPrefixLength)
    {
        if (azstricmp(name + nPrefixLength, str) == 0)
        {
            return true;
        }
    }
    if (*name == '_')
    {
        name++;
    }
    return azstricmp(name, str) == 0;
}

cstr CEnumDefUuid::ToName(AZ::Uuid value) const
{
    // Find matching element.
    for (int i = 0; i < Elems.size(); ++i)
    {
        if (Elems[i].Value == value)
        {
            return Elems[i].Name;
        }
    }
    // No match
    return 0;
}
