// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef LANGUAGES_H
#define LANGUAGES_H

#include "hlslcc.h"

static int InOutSupported(const ShaderLang eLang)
{
    if(eLang == LANG_ES_100 || eLang == LANG_120)
    {
        return 0;
    }
    return 1;
}

static int WriteToFragData(const ShaderLang eLang)
{
    if(eLang == LANG_ES_100 || eLang == LANG_120)
    {
        return 1;
    }
    return 0;
}

static int ShaderBitEncodingSupported(const ShaderLang eLang)
{
    if( eLang != LANG_ES_300 &&
        eLang != LANG_ES_310 &&
        eLang < LANG_330)
    {
        return 0;
    }
    return 1;
}

static int HaveOverloadedTextureFuncs(const ShaderLang eLang)
{
    if(eLang == LANG_ES_100 || eLang == LANG_120)
    {
        return 0;
    }
    return 1;
}

//Only enable for ES.
//Not present in 120, ignored in other desktop languages.
static int HavePrecisionQualifers(const ShaderLang eLang)
{
    if(eLang >= LANG_ES_100 && eLang <= LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

//Only on vertex inputs and pixel outputs.
static int HaveLimitedInOutLocationQualifier(const ShaderLang eLang, unsigned int flags)
{
    (void)flags;

    if(eLang >= LANG_330 || eLang == LANG_ES_300 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveInOutLocationQualifier(const ShaderLang eLang,const struct GlExtensions *extensions, unsigned int flags)
{
    (void)flags;

    if(eLang >= LANG_410 || eLang == LANG_ES_310 || (extensions && ((GlExtensions*)extensions)->ARB_explicit_attrib_location))
    {
        return 1;
    }
    return 0;
}

//layout(binding = X) uniform {uniformA; uniformB;}
//layout(location = X) uniform uniform_name;
static int HaveUniformBindingsAndLocations(const ShaderLang eLang,const struct GlExtensions *extensions, unsigned int flags)
{
    if (flags & HLSLCC_FLAG_DISABLE_EXPLICIT_LOCATIONS)
        return 0;

    if (eLang >= LANG_430 || eLang == LANG_ES_310 ||
        (extensions && ((GlExtensions*)extensions)->ARB_explicit_uniform_location && ((GlExtensions*)extensions)->ARB_shading_language_420pack))
    {
        return 1;
    }
    return 0;
}

static int DualSourceBlendSupported(const ShaderLang eLang)
{
    if(eLang >= LANG_330)
    {
        return 1;
    }
    return 0;
}

static int SubroutinesSupported(const ShaderLang eLang)
{
    if(eLang >= LANG_400)
    {
        return 1;
    }
    return 0;
}

//Before 430, flat/smooth/centroid/noperspective must match
//between fragment and its previous stage.
//HLSL bytecode only tells us the interpolation in pixel shader.
static int PixelInterpDependency(const ShaderLang eLang)
{
    if(eLang < LANG_430)
    {
        return 1;
    }
    return 0;
}

static int HaveUVec(const ShaderLang eLang)
{
    switch(eLang)
    {
    case LANG_ES_100:
    case LANG_120:
        return 0;
    default:
        break;
    }
    return 1;
}

static int HaveGather(const ShaderLang eLang)
{
    if(eLang >= LANG_400 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveGatherNonConstOffset(const ShaderLang eLang)
{
    if(eLang >= LANG_420 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}


static int HaveQueryLod(const ShaderLang eLang)
{
    if(eLang >= LANG_400)
    {
        return 1;
    }
    return 0;
}

static int HaveQueryLevels(const ShaderLang eLang)
{
    if(eLang >= LANG_430)
    {
        return 1;
    }
    return 0;
}


static int HaveAtomicCounter(const ShaderLang eLang)
{
    if(eLang >= LANG_420 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveAtomicMem(const ShaderLang eLang)
{
    if(eLang >= LANG_430)
    {
        return 1;
    }
    return 0;
}

static int HaveCompute(const ShaderLang eLang)
{
    if(eLang >= LANG_430 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveImageLoadStore(const ShaderLang eLang)
{
    if(eLang >= LANG_420 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

#endif
