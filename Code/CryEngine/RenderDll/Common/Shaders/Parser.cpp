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

#include "RenderDll_precompiled.h"

const char* kWhiteSpace = " ,";
char* pCurCommand;

void SkipCharacters (char** buf, const char* toSkip)
{
    char theChar;
    char* skip;

    while ((theChar = **buf) != 0)
    {
        if (theChar >= 0x20)
        {
            skip = (char*) toSkip;
            while (*skip)
            {
                if (theChar == *skip)
                {
                    break;
                }
                ++skip;
            }
            if (*skip == 0)
            {
                return;
            }
        }
        ++*buf;
    }
}

void RemoveCR(char* pbuf)
{
    while (*pbuf)
    {
        if (*pbuf == 0xd)
        {
            *pbuf = 0x20;
        }
        pbuf++;
    }
}

FXMacro sStaticMacros;

bool SkipChar(unsigned int ch)
{
    // Return true if ch is any of the first 0x20 ascii characters
    bool res = ch <= 0x20;

    // Also return true if ch is any of the following commented characters
    res |= (ch - 0x21) <  2; // !"
    res |= (ch - 0x26) < 10; // &'()*+,-./
    res |= (ch - 0x3A) <  6; // :;<=>?
    res |= ch == 0x5B; // [
    res |= ch == 0x5D; // ]
    res |= (ch - 0x7B) <  3; // {|}

    return res;
}

// Determine is this preprocessor directive belongs to first pass or second one
bool fxIsFirstPass(char* buf)
{
    char com[1024];
    char tok[256];
    fxFillCR(&buf, com);
    char* s = com;
    while (*s != 0)
    {
        fxFillPr(&s, tok);
        if (tok[0] == '%' && tok[1] == '_')
        {
            return false;
        }
    }
    return true;
}

static void fxAddMacro(const char* Name, const char* Macro, FXMacro& Macros)
{
    SMacroFX pr;

    if (Name[0] == '%')
    {
        pr.m_nMask = shGetHex(&Macro[0]);
#ifdef _DEBUG
        FXMacroItor it = Macros.find(CONST_TEMP_STRING(Name));
        if (it != Macros.end())
        {
            assert(0);
        }
#endif
    }
    else
    {
        pr.m_nMask = 0;
    }
    pr.m_szMacro = Macro ? Macro : "";
    FXMacroItor it = Macros.find(CONST_TEMP_STRING(Name));
    if (it != Macros.end())
    {
        Macros.erase(Name);
    }
    Macros.insert(FXMacroItor::value_type(Name, pr));

    /*it = Macros.find(Name);
    if (it != Macros.end())
    {
      int nnn = 0;
    }*/
}

void fxParserInit(void)
{
#if !defined(NULL_RENDERER) && defined(D3DX_SDK_VERSION)
    // Needed for a workaround for the optimization rule problem of DXSDKJune10's HLSL Compiler (9.29.952.3111)
    // See: http://support.microsoft.com/kb/2448404
    // Causes cracks in tessellated meshes
    char sdkVer[4];
    azitoa(D3DX_SDK_VERSION, sdkVer, 4);
    fxAddMacro("D3DX_SDK_VERSION", sdkVer, sStaticMacros);
#else
    fxAddMacro("D3DX_SDK_VERSION", "0", sStaticMacros);
#endif
}

void fxRegisterEnv([[maybe_unused]] const char* szStr)
{
    // deprecated
}

char* fxFillPr (char** buf, char* dst)
{
    int n = 0;
    char ch;
    while ((ch = **buf) != 0)
    {
        if (!SkipChar(ch))
        {
            break;
        }
        ++*buf;
    }
    char* pStart = *buf;
    while ((ch = **buf) != 0)
    {
        if (SkipChar(ch))
        {
            break;
        }
        dst[n++] = ch;
        ++*buf;
    }
    dst[n] = 0;
    return pStart;
}

char* fxFillPrC (char** buf, char* dst)
{
    int n = 0;
    char ch;
    while ((ch = **buf) != 0)
    {
        if (!SkipChar(ch))
        {
            break;
        }
        ++*buf;
    }
    char* pStart = *buf;
    while ((ch = **buf) != 0)
    {
        if (ch != ',' && SkipChar(ch))
        {
            break;
        }
        dst[n++] = ch;
        ++*buf;
    }
    dst[n] = 0;
    return pStart;
}

char* fxFillNumber (char** buf, char* dst)
{
    int n = 0;
    char ch;
    while ((ch = **buf) != 0)
    {
        if (!SkipChar(ch))
        {
            break;
        }
        ++*buf;
    }
    char* pStart = *buf;
    while ((ch = **buf) != 0)
    {
        if (ch != '.' && SkipChar(ch))
        {
            break;
        }
        dst[n++] = ch;
        ++*buf;
    }
    dst[n] = 0;
    return pStart;
}

int shFill(char** buf, char* dst, int nSize)
{
    int n = 0;
    SkipCharacters (buf, kWhiteSpace);

    while (**buf > 0x20)
    {
        dst[n++] = **buf;
        ++*buf;

        if (nSize > 0 && n == nSize)
        {
            break;
        }
    }

    dst[n] = 0;
    return n;
}
int fxFill(char** buf, char* dst, int nSize)
{
    int n = 0;
    SkipCharacters (buf, kWhiteSpace);

    while (**buf != ';')
    {
        if (**buf == 0)
        {
            break;
        }
        dst[n++] = **buf;
        ++*buf;

        if (nSize > 0 && n == nSize)
        {
            dst[n - 1] = 0;
            return 1;
        }
    }

    PREFAST_ASSUME(*buf);
    dst[n] = 0;
    if (**buf == ';')
    {
        ++*buf;
    }
    return n;
}

int fxFillCR(char** buf, char* dst)
{
    int n = 0;
    SkipCharacters (buf, kWhiteSpace);
    while (**buf != 0xa)
    {
        if (**buf == 0)
        {
            break;
        }
        dst[n++] = **buf;
        ++*buf;
    }
    dst[n] = 0;
    return n;
}

//================================================================================

bool shGetBool(char* buf)
{
    if (!buf)
    {
        return false;
    }

    if (!_strnicmp(buf, "yes", 3))
    {
        return true;
    }

    if (!_strnicmp(buf, "true", 4))
    {
        return true;
    }

    if (!_strnicmp(buf, "on", 2))
    {
        return true;
    }

    if (!strncmp(buf, "1", 1))
    {
        return true;
    }

    return false;
}

float shGetFloat(const char* buf)
{
    if (!buf)
    {
        return 0;
    }
    float f = 0;

#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%f", &f);
    assert(res);

    return f;
}

void shGetFloat(const char* buf, float* v1, float* v2)
{
    if (!buf)
    {
        return;
    }
    float f = 0, f1 = 0;

    int n = azsscanf(buf, "%f %f", &f, &f1);
    if (n == 1)
    {
        *v1 = f;
        *v2 = f;
    }
    else
    {
        *v1 = f;
        *v2 = f1;
    }
}

int shGetInt(const char* buf)
{
    if (!buf)
    {
        return 0;
    }
    int i = 0;

    if (buf[0] == '0' && buf[1] == 'x')
    {
#ifndef NDEBUG
        int res =
#endif
            azsscanf(&buf[2], "%x", &i);
        assert(res);
    }
    else
    {
#ifndef NDEBUG
        int res =
#endif
            azsscanf(buf, "%i", &i);
        assert(res);
    }

    return i;
}

int shGetHex(const char* buf)
{
    if (!buf)
    {
        return 0;
    }
    int i = 0;
#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%x", &i);
    assert(res);

    return i;
}

uint64 shGetHex64(const char* buf)
{
    if (!buf)
    {
        return 0;
    }
#if defined(__GNUC__)
    unsigned long long i = 0;
#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%llx", &i);
    assert(res);
    return (uint64)i;
#else
    uint64 i = 0;
#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%I64x", &i);
    assert(res);
    return i;
#endif
}

void shGetVector(char* buf, Vec3& v)
{
    if (!buf)
    {
        return;
    }
#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%f %f %f", &v[0], &v[1], &v[2]);
    assert(res);
}

void shGetVector(char* buf, float v[3])
{
    if (!buf)
    {
        return;
    }
#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%f %f %f", &v[0], &v[1], &v[2]);
    assert(res);
}

void shGetVector4(char* buf, Vec4& v)
{
    if (!buf)
    {
        return;
    }
#ifndef NDEBUG
    int res =
#endif
        azsscanf(buf, "%f %f %f %f", &v.x, &v.y, &v.z, &v.w);
    assert(res);
}

static struct SColAsc
{
    const char* nam;
    ColorF col;

    SColAsc(const char* name, const ColorF& c)
    {
        nam = name;
        col = c;
    }
} sCols[] =
{
    SColAsc("Aquamarine", Col_Aquamarine),
    SColAsc("Black", Col_Black),
    SColAsc("Blue", Col_Blue),
    SColAsc("BlueViolet", Col_BlueViolet),
    SColAsc("Brown", Col_Brown),
    SColAsc("CadetBlue", Col_CadetBlue),
    SColAsc("Coral", Col_Coral),
    SColAsc("CornflowerBlue", Col_CornflowerBlue),
    SColAsc("Cyan", Col_Cyan),
    SColAsc("DarkGray", Col_DarkGray),
    SColAsc("DarkGrey", Col_DarkGrey),
    SColAsc("DarkGreen", Col_DarkGreen),
    SColAsc("DarkOliveGreen", Col_DarkOliveGreen),
    SColAsc("DarkOrchid", Col_DarkOrchid),
    SColAsc("DarkSlateBlue", Col_DarkSlateBlue),
    SColAsc("DarkSlateGray", Col_DarkSlateGray),
    SColAsc("DarkSlateGrey", Col_DarkSlateGrey),
    SColAsc("DarkTurquoise", Col_DarkTurquoise),
    SColAsc("DarkWood", Col_DarkWood),
    SColAsc("DeepPink", Col_DeepPink),
    SColAsc("DimGray", Col_DimGray),
    SColAsc("DimGrey", Col_DimGrey),
    SColAsc("FireBrick", Col_FireBrick),
    SColAsc("ForestGreen", Col_ForestGreen),
    SColAsc("Gold", Col_Gold),
    SColAsc("Goldenrod", Col_Goldenrod),
    SColAsc("Gray", Col_Gray),
    SColAsc("Grey", Col_Grey),
    SColAsc("Green", Col_Green),
    SColAsc("GreenYellow", Col_GreenYellow),
    SColAsc("IndianRed", Col_IndianRed),
    SColAsc("Khaki", Col_Khaki),
    SColAsc("LightBlue", Col_LightBlue),
    SColAsc("LightGray", Col_LightGray),
    SColAsc("LightGrey", Col_LightGrey),
    SColAsc("LightSteelBlue", Col_LightSteelBlue),
    SColAsc("LightWood", Col_LightWood),
    SColAsc("Lime", Col_Lime),
    SColAsc("LimeGreen", Col_LimeGreen),
    SColAsc("Magenta", Col_Magenta),
    SColAsc("Maroon", Col_Maroon),
    SColAsc("MedianWood", Col_MedianWood),
    SColAsc("MediumAquamarine", Col_MediumAquamarine),
    SColAsc("MediumBlue", Col_MediumBlue),
    SColAsc("MediumForestGreen", Col_MediumForestGreen),
    SColAsc("MediumGoldenrod", Col_MediumGoldenrod),
    SColAsc("MediumOrchid", Col_MediumOrchid),
    SColAsc("MediumSeaGreen", Col_MediumSeaGreen),
    SColAsc("MediumSlateBlue", Col_MediumSlateBlue),
    SColAsc("MediumSpringGreen", Col_MediumSpringGreen),
    SColAsc("MediumTurquoise", Col_MediumTurquoise),
    SColAsc("MediumVioletRed", Col_MediumVioletRed),
    SColAsc("MidnightBlue", Col_MidnightBlue),
    SColAsc("Navy", Col_Navy),
    SColAsc("NavyBlue", Col_NavyBlue),
    SColAsc("Orange", Col_Orange),
    SColAsc("OrangeRed", Col_OrangeRed),
    SColAsc("Orchid", Col_Orchid),
    SColAsc("PaleGreen", Col_PaleGreen),
    SColAsc("Pink", Col_Pink),
    SColAsc("Plum", Col_Plum),
    SColAsc("Red", Col_Red),
    SColAsc("Salmon", Col_Salmon),
    SColAsc("SeaGreen", Col_SeaGreen),
    SColAsc("Sienna", Col_Sienna),
    SColAsc("SkyBlue", Col_SkyBlue),
    SColAsc("SlateBlue", Col_SlateBlue),
    SColAsc("SpringGreen", Col_SpringGreen),
    SColAsc("SteelBlue", Col_SteelBlue),
    SColAsc("Tan", Col_Tan),
    SColAsc("Thistle", Col_Thistle),
    SColAsc("Turquoise", Col_Turquoise),
    SColAsc("Violet", Col_Violet),
    SColAsc("VioletRed", Col_VioletRed),
    SColAsc("Wheat", Col_Wheat),
    SColAsc("White", Col_White),
    SColAsc("Yellow", Col_Yellow),
    SColAsc("YellowGreen", Col_YellowGreen),

    SColAsc(NULL, ColorF(1.0f, 1.0f, 1.0f))
};

#include <ctype.h>

void shGetColor(const char* buf, ColorF& v)
{
    char name[64];
    int n;

    if (!buf)
    {
        v = Col_White;
        return;
    }
    if (buf[0] == '{')
    {
        buf++;
    }
    if (isalpha((unsigned char)buf[0]))
    {
        n = 0;
        float scal = 1;
        azstrcpy(name, AZ_ARRAY_SIZE(name), buf);
        char nm[64];
        if (strchr(buf, '*'))
        {
            while (buf[n] != '*')
            {
                if (buf[n] == 0x20)
                {
                    break;
                }
                nm[n] = buf[n];
                n++;
            }
            nm[n] = 0;
            if (buf[n] == 0x20)
            {
                while (buf[n] != '*')
                {
                    n++;
                }
            }
            n++;
            while (buf[n] == 0x20)
            {
                n++;
            }
            scal = shGetFloat(&buf[n]);
            azstrcpy(name, AZ_ARRAY_SIZE(name), nm);
        }
        n = 0;
        while (sCols[n].nam)
        {
            if (!azstricmp(sCols[n].nam, name))
            {
                v = sCols[n].col;
                if (scal != 1)
                {
                    v.ScaleCol(scal);
                }
                return;
            }
            n++;
        }
    }
    n = 0;
    while (true)
    {
        if (n == 4)
        {
            break;
        }
        char par[64];
        par[0] = 0;
        fxFillNumber((char**)&buf, par);
        if (!par[0])
        {
            break;
        }
        v[n++] = (float)atof(par);
    }

    //v.Clamp();
}

void shGetColor(char* buf, float v[4])
{
    char name[64];

    if (!buf)
    {
        v[0] = 1.0f;
        v[1] = 1.0f;
        v[2] = 1.0f;
        v[3] = 1.0f;
        return;
    }
    if (isalpha((unsigned char)buf[0]))
    {
        int n = 0;
        float scal = 1;
        azstrcpy(name, AZ_ARRAY_SIZE(name), buf);
        char nm[64];
        if (strchr(buf, '*'))
        {
            while (buf[n] != '*')
            {
                if (buf[n] == 0x20)
                {
                    break;
                }
                nm[n] = buf[n];
                n++;
            }
            nm[n] = 0;
            if (buf[n] == 0x20)
            {
                while (buf[n] != '*')
                {
                    n++;
                }
            }
            n++;
            while (buf[n] == 0x20)
            {
                n++;
            }
            scal = shGetFloat(&buf[n]);
            azstrcpy(name, AZ_ARRAY_SIZE(name), nm);
        }
        n = 0;
        while (sCols[n].nam)
        {
            if (!azstricmp(sCols[n].nam, name))
            {
                v[0] = sCols[n].col[0];
                v[1] = sCols[n].col[1];
                v[2] = sCols[n].col[2];
                v[3] = sCols[n].col[3];
                if (scal != 1)
                {
                    v[0] *= scal;
                    v[1] *= scal;
                    v[2] *= scal;
                }
                return;
            }
            n++;
        }
    }
    int n = azsscanf(buf, "%f %f %f %f", &v[0], &v[1], &v[2], &v[3]);
    switch (n)
    {
    case 0:
        v[0] = v[1] = v[2] = v[3] = 1.0f;
        break;

    case 1:
        v[1] = v[2] = v[3] = 1.0f;
        break;

    case 2:
        v[2] = v[3] = 1.0f;
        break;

    case 3:
        v[3] = 1.0f;
        break;
    }
    //v.Clamp();
}

//=========================================================================================

char* GetAssignmentText (char** buf)
{
    SkipCharacters (buf, kWhiteSpace);
    char* result = *buf;

    char theChar;

    PREFAST_SUPPRESS_WARNING(28182)
    while ((theChar = **buf) != 0)
    {
        if (theChar == '[')
        {
            while ((theChar = **buf) != ']')
            {
                if (theChar == 0 || theChar == ';')
                {
                    break;
                }
                ++*buf;
            }
            continue;
        }
        if (theChar <= 0x20 || theChar == ';')
        {
            break;
        }
        ++*buf;
    }

    PREFAST_ASSUME(*buf);
    **buf = 0;
    if (theChar)
    {
        ++*buf;
    }
    return result;
}

char* GetSubText (char** buf, char open, char close)
{
    if (**buf == 0 || **buf != open)
    {
        return 0;
    }
    ++*buf;
    char* result = *buf;

    char theChar;
    long skip = 1;

    if (open == close)
    {
        open = 0;
    }
    while ((theChar = **buf) != 0)
    {
        if (theChar == open)
        {
            ++skip;
        }
        if (theChar == close)
        {
            if (--skip == 0)
            {
                **buf = 0;
                ++*buf;
                break;
            }
        }
        ++*buf;
    }
    return result;
}

_inline static int IsComment(char** buf)
{
    if (!(*buf))
    {
        return 0;
    }

    if ((*buf)[0] == '/' && (*buf)[1] == '/')
    {
        return 2;
    }

    if ((*buf)[0] == '/' && (*buf)[1] == '*')
    {
        return 3;
    }

    return 0;
}

void SkipComments(char** buf, bool bSkipWhiteSpace)
{
    int n;
    static int m;

    n = IsComment(buf);
    while (n)
    {
        switch (n)
        {
        case 2:
            // skip comment lines.
            *buf = strchr (*buf, '\n');
            if (*buf && bSkipWhiteSpace)
            {
                SkipCharacters (buf, kWhiteSpace);
            }
            break;

        case 3:
            // skip comment blocks.
            m = 0;
            do
            {
                *buf = strchr (*buf, '*');
                if (!(*buf))
                {
                    break;
                }
                if ((*buf)[-1] == '/')
                {
                    *buf += 1;
                    m++;
                }
                else
                if ((*buf)[1] == '/')
                {
                    *buf += 2;
                    m--;
                }
                else
                {
                    *buf += 1;
                }
            } while (m);
            if (!(*buf))
            {
                iLog->Log ("Warning: Comment lines aren't closed\n");
                break;
            }
            if (bSkipWhiteSpace)
            {
                SkipCharacters (buf, kWhiteSpace);
            }
            break;
        }
        n = IsComment(buf);
    }
}

void fxSkipTillCR(char** buf)
{
    char ch;
    while ((ch = **buf) != 0)
    {
        if (ch == 0xa)
        {
            break;
        }
        ++*buf;
    }
}

bool fxCheckMacroses(char** str, int nPass)
{
    char tmpBuf[1024];
    byte bRes[64];
    byte bOr[64];
    int nLevel = 0;
    int i;
    while (true)
    {
        SkipCharacters(str, kWhiteSpace);
        if (**str == '(')
        {
            ++*str;
            int n = 0;
            int nD = 0;
            while (true)
            {
                if (**str == '(')
                {
                    n++;
                }
                else
                if (**str == ')')
                {
                    if (!n)
                    {
                        tmpBuf[nD] = 0;
                        ++*str;
                        break;
                    }
                    n--;
                }
                else
                if (**str == 0)
                {
                    return false;
                }
                tmpBuf[nD++] = **str;
                ++*str;
            }
            char* s = &tmpBuf[0];
            bRes[nLevel] = fxCheckMacroses(&s, nPass);
            nLevel++;
            bOr[nLevel] = 255;
        }
        else
        {
            int n = 0;
            while (true)
            {
                if (**str == '|' || **str == '&' || **str == 0)
                {
                    break;
                }
                if (**str <= 0x20)
                {
                    break;
                }
                tmpBuf[n++] = **str;
                ++*str;
            }
            tmpBuf[n] = 0;
            if (tmpBuf[0] != 0)
            {
                char* s = tmpBuf;
                bool bNeg = false;
                if (s[0] == '!')
                {
                    bNeg = true;
                    s++;
                }
                const SMacroBinFX* pFound;
                if (isdigit((unsigned)s[0]))
                {
                    if ((s[0] == '0' && s[1] == 'x') || s[0] != 0)
                    {
                        pFound = (SMacroBinFX*)1;
                    }
                    else
                    {
                        pFound = NULL;
                    }
                }
                else
                {
                    bool bKey = false;
                    uint32 nTok = CParserBin::NextToken(s, tmpBuf, bKey);
                    if (nTok == eT_unknown)
                    {
                        nTok = CParserBin::GetCRC32(tmpBuf);
                    }
                    pFound = CParserBin::FindMacro(nTok, CParserBin::GetStaticMacroses());
                }
                bRes[nLevel] = (pFound) ? true : false;
                if (bNeg)
                {
                    bRes[nLevel] = !bRes[nLevel];
                }
                nLevel++;
                bOr[nLevel] = 255;
            }
            else
            {
                assert(0);
            }
        }
        SkipCharacters(str, kWhiteSpace);
        if (**str == 0)
        {
            break;
        }
        char* s = *str;
        if (s[0] == '|' && s[1] == '|')
        {
            bOr[nLevel] = true;
            *str = s + 2;
        }
        else
        if (s[0] == '&' && s[1] == '&')
        {
            bOr[nLevel] = false;
            *str = s + 2;
        }
        else
        {
            assert(0);
        }
    }
    byte Res = false;
    for (i = 0; i < nLevel; i++)
    {
        if (!i)
        {
            Res = bRes[i];
        }
        else
        {
            assert(bOr[i] != 255);
            if (bOr[i])
            {
                Res = Res | bRes[i];
            }
            else
            {
                Res = Res & bRes[i];
            }
        }
    }
    return Res != 0;
}

bool fxIgnorePreprBlock(char** buf)
{
    int nLevel = 0;
    bool bEnded = false;
    SkipCharacters (buf, kWhiteSpace);
    SkipComments(buf, true);
    PREFAST_SUPPRESS_WARNING(6011)
    while (**buf != 0)
    {
        char ch = **buf;
        while (ch && SkipChar((unsigned)ch))
        {
            while ((ch = **buf) != 0)
            {
                if (ch == '/' && IsComment(buf))
                {
                    break;
                }
                if (!SkipChar(ch))
                {
                    break;
                }
                ++*buf;
            }
            SkipComments(buf, true);
            ch = **buf;
        }
        char* posS = *buf;
        char* st = posS;
        PREFAST_ASSUME(posS);
        if (*posS == '#')
        {
            posS++;
            if (SkipChar(posS[0]))
            {
                while ((ch = *posS) != 0)
                {
                    if (!SkipChar(ch))
                    {
                        break;
                    }
                    posS++;
                }
            }
            if (posS[0] == 'i' && posS[1] == 'f')
            {
                nLevel++;
                *buf = posS + 2;
                continue;
            }
            if (!strncmp(posS, "endif", 5))
            {
                if (!nLevel)
                {
                    *buf = st;
                    bEnded = true;
                    break;
                }
                nLevel--;
                *buf = posS + 4;
            }
            else
            if (!strncmp(posS, "else", 4) || !strncmp(posS, "elif", 4))
            {
                if (!nLevel)
                {
                    *buf = st;
                    break;
                }
                *buf = posS + 4;
            }
        }
        ch = **buf;
        while (ch)
        {
            if (ch == '/' && IsComment(buf))
            {
                break;
            }
            if (SkipChar((unsigned)ch))
            {
                break;
            }
            ++*buf;
            ch = **buf;
        }
    }
    PREFAST_ASSUME(*buf);
    if (!**buf)
    {
        assert(0);
        Warning("Couldn't find #endif directive for associated #ifdef");
        return false;
    }

    return bEnded;
}

int shGetObject (char** buf, STokenDesc* tokens, char** name, char** data)
{
start:
    if (!*buf)
    {
        return 0;
    }
    SkipCharacters (buf, kWhiteSpace);
    SkipComments(buf, true);

    if (!(*buf) || !**buf)
    {
        return -2;
    }

    char* b = *buf;
    if (b[0] == '#')
    {
        char nam[1024];
        bool bPrepr = false;
        if (!strncmp(b, "#if", 3))
        {
            bPrepr = true;
            fxFillPr(buf, nam);
            fxFillCR(buf, nam);
            char* s = &nam[0];
            bool bRes = fxCheckMacroses(&s, 0);
            if (b[2] == 'n')
            {
                bRes = !bRes;
            }
            if (!bRes)
            {
                sfxIFDef.AddElem(false);
                fxIgnorePreprBlock(buf);
            }
            else
            {
                sfxIFDef.AddElem(false);
            }
        }
        else
        if (!strncmp(b, "#else", 5))
        {
            fxFillPr(buf, nam);
            bPrepr = true;
            int nLevel = sfxIFDef.Num() - 1;
            if (nLevel < 0)
            {
                assert(0);
                Warning("#else without #ifdef");
                return false;
            }
            if (sfxIFDef[nLevel] == true)
            {
                bool bEnded = fxIgnorePreprBlock(buf);
                if (!bEnded)
                {
                    assert(0);
                    Warning("#else or #elif after #else");
                    return -1;
                }
            }
        }
        else
        if (!strncmp(b, "#elif", 5))
        {
            fxFillPr(buf, nam);
            bPrepr = true;
            int nLevel = sfxIFDef.Num() - 1;
            if (nLevel < 0)
            {
                assert(0);
                Warning("#elif without #ifdef");
                return -1;
            }
            if (sfxIFDef[nLevel] == true)
            {
                fxIgnorePreprBlock(buf);
            }
            else
            {
                fxFillCR(buf, nam);
                char* s = &nam[0];
                bool bRes = fxCheckMacroses(&s, 0);
                if (!bRes)
                {
                    fxIgnorePreprBlock(buf);
                }
                else
                {
                    sfxIFDef[nLevel] = true;
                }
            }
        }
        else
        if (!strncmp(b, "#endif", 6))
        {
            fxFillPr(buf, nam);
            bPrepr = true;
            int nLevel = sfxIFDef.Num() - 1;
            if (nLevel < 0)
            {
                assert(0);
                Warning("#endif without #ifdef");
                return -1;
            }
            sfxIFDef.Remove(nLevel);
        }
        if (bPrepr)
        {
            goto start;
        }
    }

    STokenDesc* ptokens = tokens;
    while (tokens->id != 0)
    {
        if (!_strnicmp(tokens->token, *buf, strlen(tokens->token)))
        {
            pCurCommand = *buf;
            break;
        }
        ++tokens;
    }
    if (tokens->id == 0)
    {
        char* p = strchr (*buf, '\n');

        char pp[1024];
        if (p)
        {
            cry_strcpy(pp, *buf, (size_t)(p - *buf));
            *buf = p;
        }
        else
        {
            cry_strcpy(pp, *buf);
        }

        iLog->Log ("Warning: Found token '%s' which was not one of the list (Skipping).\n", pp);
        while (ptokens->id != 0)
        {
            iLog->Log ("    %s\n", ptokens->token);
            ptokens++;
        }
        return 0;
    }
    *buf += strlen (tokens->token);
    SkipCharacters (buf, kWhiteSpace);

    *name = GetSubText (buf, 0x27, 0x27);
    SkipCharacters (buf, kWhiteSpace);

    if (**buf == '=')
    {
        ++*buf;
        *data = GetAssignmentText (buf);
    }
    else
    {
        *data = GetSubText (buf, '(', ')');
        if (!*data)
        {
            *data = GetSubText (buf, '{', '}');
        }
    }


    return tokens->id;
}
