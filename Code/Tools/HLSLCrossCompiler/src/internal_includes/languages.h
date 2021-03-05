// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#ifndef LANGUAGES_H
#define LANGUAGES_H

#include "hlslcc.h"

static int InOutSupported(const GLLang eLang)
{
    if(eLang == LANG_ES_100 || eLang == LANG_120)
    {
        return 0;
    }
    return 1;
}

static int WriteToFragData(const GLLang eLang)
{
    if(eLang == LANG_ES_100 || eLang == LANG_120)
    {
        return 1;
    }
    return 0;
}

static int ShaderBitEncodingSupported(const GLLang eLang)
{
    if( eLang != LANG_ES_300 &&
        eLang != LANG_ES_310 &&
        eLang < LANG_330)
    {
        return 0;
    }
    return 1;
}

static int HaveOverloadedTextureFuncs(const GLLang eLang)
{
    if(eLang == LANG_ES_100 || eLang == LANG_120)
    {
        return 0;
    }
    return 1;
}

//Only enable for ES.
//Not present in 120, ignored in other desktop languages.
static int HavePrecisionQualifers(const GLLang eLang)
{
    if(eLang >= LANG_ES_100 && eLang <= LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

//Only on vertex inputs and pixel outputs.
static int HaveLimitedInOutLocationQualifier(const GLLang eLang)
{
    if(eLang >= LANG_330 || eLang == LANG_ES_300 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveInOutLocationQualifier(const GLLang eLang,const struct GlExtensions *extensions)
{
    if(eLang >= LANG_410 || eLang == LANG_ES_310 || (extensions && ((GlExtensions*)extensions)->ARB_explicit_attrib_location))
    {
        return 1;
    }
    return 0;
}

//layout(binding = X) uniform {uniformA; uniformB;}
//layout(location = X) uniform uniform_name;
static int HaveUniformBindingsAndLocations(const GLLang eLang,const struct GlExtensions *extensions)
{
    if(eLang >= LANG_430 || eLang == LANG_ES_310 || (extensions && ((GlExtensions*)extensions)->ARB_explicit_uniform_location))
    {
        return 1;
    }
    return 0;
}

static int DualSourceBlendSupported(const GLLang eLang)
{
    if(eLang >= LANG_330)
    {
        return 1;
    }
    return 0;
}

static int SubroutinesSupported(const GLLang eLang)
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
static int PixelInterpDependency(const GLLang eLang)
{
    if(eLang < LANG_430)
    {
        return 1;
    }
    return 0;
}

static int HaveUVec(const GLLang eLang)
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

static int HaveGather(const GLLang eLang)
{
    if(eLang >= LANG_400 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveGatherNonConstOffset(const GLLang eLang)
{
    if(eLang >= LANG_420 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}


static int HaveQueryLod(const GLLang eLang)
{
    if(eLang >= LANG_400)
    {
        return 1;
    }
    return 0;
}

static int HaveQueryLevels(const GLLang eLang)
{
    if(eLang >= LANG_430)
    {
        return 1;
    }
    return 0;
}


static int HaveAtomicCounter(const GLLang eLang)
{
    if(eLang >= LANG_420 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveAtomicMem(const GLLang eLang)
{
    if(eLang >= LANG_430)
    {
        return 1;
    }
    return 0;
}

static int HaveCompute(const GLLang eLang)
{
    if(eLang >= LANG_430 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int HaveImageLoadStore(const GLLang eLang)
{
    if(eLang >= LANG_420 || eLang == LANG_ES_310)
    {
        return 1;
    }
    return 0;
}

static int EmulateDepthClamp(const GLLang eLang)
{
    if (eLang >= LANG_ES_300 && eLang < LANG_120) //Requires gl_FragDepth available in fragment shader
    {
        return 1;
    }
    return 0;
}

static int HaveNoperspectiveInterpolation(const GLLang eLang)
{
    if (eLang >= LANG_330)
    {
        return 1;
    }
    return 0;
}

static int EarlyDepthTestSupported(const GLLang eLang)
{
    if ((eLang > LANG_410) || (eLang == LANG_ES_310))
    {
        return 1;
    }
    return 0;
}

static int StorageBlockBindingSupported(const GLLang eLang)
{
    if (eLang >= LANG_430)
    {
        return 1;
    }
    return 0;
}


#endif
