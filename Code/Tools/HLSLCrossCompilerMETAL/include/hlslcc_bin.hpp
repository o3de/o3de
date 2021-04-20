// Modifications copyright Amazon.com, Inc. or its affiliates
// Modifications copyright Crytek GmbH

#define FOURCC(a, b, c, d) ((uint32_t)(uint8_t)(a) | ((uint32_t)(uint8_t)(b) << 8) | ((uint32_t)(uint8_t)(c) << 16) | ((uint32_t)(uint8_t)(d) << 24))

enum
{
    DXBC_BASE_ALIGNMENT = 4,
    FOURCC_DXBC = FOURCC('D', 'X', 'B', 'C'),
    FOURCC_RDEF = FOURCC('R', 'D', 'E', 'F'),
    FOURCC_ISGN = FOURCC('I', 'S', 'G', 'N'),
    FOURCC_OSGN = FOURCC('O', 'S', 'G', 'N'),
    FOURCC_PCSG = FOURCC('P', 'C', 'S', 'G'),
    FOURCC_SHDR = FOURCC('S', 'H', 'D', 'R'),
    FOURCC_SHEX = FOURCC('S', 'H', 'E', 'X'),
    FOURCC_GLSL = FOURCC('G', 'L', 'S', 'L'),
    FOURCC_ISG1 = FOURCC('I', 'S', 'G', '1'), // When lower precision float/int/uint is used
    FOURCC_OSG1 = FOURCC('O', 'S', 'G', '1'), // When lower precision float/int/uint is used
};

#undef FOURCC

template <typename T>
inline T DXBCSwapBytes(const T& kValue)
{
    return kValue;
}

#if defined(__BIG_ENDIAN__) || SYSTEM_IS_BIG_ENDIAN

inline uint16_t DXBCSwapBytes(const uint16_t& uValue)
{
    return
        (((uValue) >> 8) & 0xFF) |
        (((uValue) << 8) & 0xFF);
}

inline uint32_t DXBCSwapBytes(const uint32_t& uValue)
{
    return
        (((uValue) >> 24) & 0x000000FF) |
        (((uValue) >> 8)  & 0x0000FF00) |
        (((uValue) << 8)  & 0x00FF0000) |
        (((uValue) << 24) & 0xFF000000);
}

#endif //defined(__BIG_ENDIAN__) || SYSTEM_IS_BIG_ENDIAN

template <typename Element>
struct SDXBCBufferBase
{
    Element* m_pBegin;
    Element* m_pEnd;
    Element* m_pIter;

    SDXBCBufferBase(Element* pBegin, Element* pEnd)
        : m_pBegin(pBegin)
        , m_pEnd(pEnd)
        , m_pIter(pBegin)
    {
    }

    bool SeekRel(int32_t iOffset)
    {
        Element* pIterAfter(m_pIter + iOffset);
        if (pIterAfter > m_pEnd)
        {
            return false;
        }

        m_pIter = pIterAfter;
        return true;
    }

    bool SeekAbs(uint32_t uPosition)
    {
        Element* pIterAfter(m_pBegin + uPosition);
        if (pIterAfter > m_pEnd)
        {
            return false;
        }

        m_pIter = pIterAfter;
        return true;
    }
};

struct SDXBCInputBuffer
    : SDXBCBufferBase<const uint8_t>
{
    SDXBCInputBuffer(const uint8_t* pBegin, const uint8_t* pEnd)
        : SDXBCBufferBase(pBegin, pEnd)
    {
    }

    bool Read(void* pElements, size_t uSize)
    {
        const uint8_t* pIterAfter(m_pIter + uSize);
        if (pIterAfter > m_pEnd)
        {
            return false;
        }

        memcpy(pElements, m_pIter, uSize);

        m_pIter = pIterAfter;
        return true;
    }
};

struct SDXBCOutputBuffer
    : SDXBCBufferBase<uint8_t>
{
    SDXBCOutputBuffer(uint8_t* pBegin,  uint8_t* pEnd)
        : SDXBCBufferBase(pBegin, pEnd)
    {
    }

    bool Write(const void* pElements, size_t uSize)
    {
        uint8_t* pIterAfter(m_pIter + uSize);
        if (pIterAfter > m_pEnd)
        {
            return false;
        }

        memcpy(m_pIter, pElements, uSize);

        m_pIter = pIterAfter;
        return true;
    }
};

template <typename S, typename External, typename Internal>
inline bool DXBCReadAs(S& kStream, External& kValue)
{
    Internal kInternal;
    bool bResult(kStream.Read(&kInternal, sizeof(Internal)));
    kValue = static_cast<External>(DXBCSwapBytes(kInternal));
    return bResult;
}

template <typename S, typename Internal>
inline bool DXBCWriteAs(S& kStream, Internal kValue)
{
    Internal kInternal(DXBCSwapBytes(kValue));
    return kStream.Write(&kInternal, sizeof(Internal));
}

template <typename S, typename T>
bool DXBCReadUint8 (S& kStream, T& kValue) { return DXBCReadAs<S, T, uint8_t >(kStream, kValue); }
template <typename S, typename T>
bool DXBCReadUint16(S& kStream, T& kValue) { return DXBCReadAs<S, T, uint16_t>(kStream, kValue); }
template <typename S, typename T>
bool DXBCReadUint32(S& kStream, T& kValue) { return DXBCReadAs<S, T, uint32_t>(kStream, kValue); }

template <typename S>
bool DXBCWriteUint8 (S& kStream, uint8_t kValue)  { return DXBCWriteAs<S, uint8_t >(kStream, kValue); }
template <typename S>
bool DXBCWriteUint16(S& kStream, uint16_t kValue) { return DXBCWriteAs<S, uint16_t>(kStream, kValue); }
template <typename S>
bool DXBCWriteUint32(S& kStream, uint32_t kValue) { return DXBCWriteAs<S, uint32_t>(kStream, kValue); }

template <typename O, typename I>
bool DXBCCopy(O& kOutput, I& kInput, size_t uSize)
{
    char acBuffer[1024];
    while (uSize > 0)
    {
        size_t uToCopy(std::min<size_t>(uSize, sizeof(acBuffer)));
        if (!kInput.Read(acBuffer, uToCopy) ||
            !kOutput.Write(acBuffer, uToCopy))
        {
            return false;
        }
        uSize -= uToCopy;
    }
    return true;
}

enum
{
    DXBC_SIZE_POSITION = 6 * 4,
    DXBC_HEADER_SIZE = 7 * 4,
    DXBC_CHUNK_HEADER_SIZE = 2 * 4,
    DXBC_MAX_NUM_CHUNKS_IN = 128,
    DXBC_MAX_NUM_CHUNKS_OUT = 8,
    DXBC_OUT_CHUNKS_INDEX_SIZE = (1 + 1 + DXBC_MAX_NUM_CHUNKS_OUT) * 4,
    DXBC_OUT_FIXED_SIZE = DXBC_HEADER_SIZE + DXBC_OUT_CHUNKS_INDEX_SIZE,
};

inline void DXBCSizeGLSLChunk(uint32_t& uGLSLChunkSize, uint32_t& uNumSamplers, uint32_t& uGLSLSourceSize, const Shader* pShader)
{
    enum
    {
        GLSL_HEADER_SIZE =  4 * 8,  // {uint32 uNumSamplers; uint32 uNumImports; uint32 uNumExports; uint32 uInputHash;uint32 uResources; uint32 ui32Thread_x; uint32 ui32Thread_y; uint32 ui32Thread_z}
        GLSL_SAMPLER_SIZE = 4 * 2,  // {uint32 uTexture; uint32 uSampler;}
        GLSL_SYMBOL_SIZE =  4 * 3,  // {uint32 uType; uint32 uID; uint32 uValue}
        //extend for metal compute UAV type
        GLSL_UAV_RESOURCES_AREA = 4 * 2, //{uint32 uResource; uint32 eBindArea}
    };

    // Only texture registers that are used are written
    uNumSamplers = 0;
    for (uint32_t uTexture = 0; uTexture < MAX_RESOURCE_BINDINGS; ++uTexture)
    {
        if (pShader->reflection.aui32SamplerMap[uTexture] != MAX_RESOURCE_BINDINGS)
        {
            ++uNumSamplers;
        }
    }

    //uint32_t uNumSymbols(
    //  pShader->reflection.ui32NumImports +
    //  pShader->reflection.ui32NumExports);
    uint32_t uNumSymbols(0); // always 0
    uint32_t uNumResources(pShader->reflection.ui32NumResourceBindings);

    uint32_t uGLSLInfoSize(
        DXBC_CHUNK_HEADER_SIZE +
        GLSL_HEADER_SIZE +
        uNumSamplers * GLSL_SAMPLER_SIZE +
        uNumSymbols  * GLSL_SYMBOL_SIZE +
        uNumResources * GLSL_UAV_RESOURCES_AREA
        );
    uGLSLSourceSize = (uint32_t)strlen(pShader->sourceCode) + 1;
    uGLSLChunkSize = uGLSLInfoSize + uGLSLSourceSize;
    uGLSLChunkSize += DXBC_BASE_ALIGNMENT - 1 - (uGLSLChunkSize - 1) % DXBC_BASE_ALIGNMENT;
}

inline uint32_t DXBCSizeOutputChunk(uint32_t uCode, uint32_t uSizeIn)
{
    uint32_t uSizeOut;
    switch (uCode)
    {
    case FOURCC_RDEF:
    case FOURCC_ISGN:
    case FOURCC_OSGN:
    case FOURCC_PCSG:
    case FOURCC_OSG1:
    case FOURCC_ISG1:
        // Preserve entire chunk
        uSizeOut = uSizeIn;
        break;
    case FOURCC_SHDR:
    case FOURCC_SHEX:
        // Only keep the shader version
        uSizeOut = uSizeIn <  4u ? uSizeIn : 4u;
        break;
    default:
        // Discard the chunk
        uSizeOut = 0;
        break;
    }

    return uSizeOut + DXBC_BASE_ALIGNMENT - 1 - (uSizeOut - 1) % DXBC_BASE_ALIGNMENT;
}

template <typename I>
size_t DXBCGetCombinedSize(I& kDXBCInput, const Shader* pShader)
{
    uint32_t uNumChunksIn;
    if (!kDXBCInput.SeekAbs(DXBC_HEADER_SIZE) ||
        !DXBCReadUint32(kDXBCInput, uNumChunksIn))
    {
        return 0;
    }

    uint32_t auChunkOffsetsIn[DXBC_MAX_NUM_CHUNKS_IN];
    for (uint32_t uChunk = 0; uChunk < uNumChunksIn; ++uChunk)
    {
        if (!DXBCReadUint32(kDXBCInput, auChunkOffsetsIn[uChunk]))
        {
            return 0;
        }
    }

    uint32_t uNumChunksOut(0);
    uint32_t uOutSize(DXBC_OUT_FIXED_SIZE);
    for (uint32_t uChunk = 0; uChunk < uNumChunksIn && uNumChunksOut < DXBC_MAX_NUM_CHUNKS_OUT; ++uChunk)
    {
        uint32_t uChunkCode, uChunkSizeIn;
        if (!kDXBCInput.SeekAbs(auChunkOffsetsIn[uChunk]) ||
            !DXBCReadUint32(kDXBCInput, uChunkCode) ||
            !DXBCReadUint32(kDXBCInput, uChunkSizeIn))
        {
            return 0;
        }

        uint32_t uChunkSizeOut(DXBCSizeOutputChunk(uChunkCode, uChunkSizeIn));
        if (uChunkSizeOut > 0)
        {
            uOutSize += DXBC_CHUNK_HEADER_SIZE + uChunkSizeOut;
        }
    }

    uint32_t uNumSamplers, uGLSLSourceSize, uGLSLChunkSize;
    DXBCSizeGLSLChunk(uGLSLChunkSize, uNumSamplers, uGLSLSourceSize, pShader);
    uOutSize += uGLSLChunkSize;

    return uOutSize;
}

template <typename I, typename O>
bool DXBCCombineWithGLSL(I& kInput, O& kOutput, const Shader* pShader)
{
    uint32_t uNumChunksIn;
    if (!DXBCCopy(kOutput, kInput, DXBC_HEADER_SIZE) ||
        !DXBCReadUint32(kInput, uNumChunksIn)  ||
        uNumChunksIn > DXBC_MAX_NUM_CHUNKS_IN)
    {
        return false;
    }

    uint32_t auChunkOffsetsIn[DXBC_MAX_NUM_CHUNKS_IN];
    for (uint32_t uChunk = 0; uChunk < uNumChunksIn; ++uChunk)
    {
        if (!DXBCReadUint32(kInput, auChunkOffsetsIn[uChunk]))
        {
            return false;
        }
    }

    uint32_t auZeroChunkIndex[DXBC_OUT_CHUNKS_INDEX_SIZE] = {0};
    if (!kOutput.Write(auZeroChunkIndex, DXBC_OUT_CHUNKS_INDEX_SIZE))
    {
        return false;
    }

    // Copy required input chunks just after the chunk index
    uint32_t uOutSize(DXBC_OUT_FIXED_SIZE);
    uint32_t uNumChunksOut(0);
    uint32_t auChunkOffsetsOut[DXBC_MAX_NUM_CHUNKS_OUT];
    for (uint32_t uChunk = 0; uChunk < uNumChunksIn; ++uChunk)
    {
        uint32_t uChunkCode, uChunkSizeIn;
        if (!kInput.SeekAbs(auChunkOffsetsIn[uChunk]) ||
            !DXBCReadUint32(kInput, uChunkCode) ||
            !DXBCReadUint32(kInput, uChunkSizeIn))
        {
            return false;
        }

        // Filter only input chunks of the specified types
        uint32_t uChunkSizeOut(DXBCSizeOutputChunk(uChunkCode, uChunkSizeIn));
        if (uChunkSizeOut > 0)
        {
            if (uNumChunksOut >= DXBC_MAX_NUM_CHUNKS_OUT)
            {
                return false;
            }

            if (!DXBCWriteUint32(kOutput, uChunkCode) ||
                !DXBCWriteUint32(kOutput, uChunkSizeOut) ||
                !DXBCCopy(kOutput, kInput, uChunkSizeOut))
            {
                return false;
            }

            auChunkOffsetsOut[uNumChunksOut] = uOutSize;
            ++uNumChunksOut;
            uOutSize += DXBC_CHUNK_HEADER_SIZE + uChunkSizeOut;
        }
    }
    // Write GLSL chunk
    uint32_t uGLSLChunkOffset(uOutSize);
    uint32_t uGLSLChunkSize, uNumSamplers, uGLSLSourceSize;
    DXBCSizeGLSLChunk(uGLSLChunkSize, uNumSamplers, uGLSLSourceSize, pShader);
    if (!DXBCWriteUint32(kOutput, (uint32_t)FOURCC_GLSL) ||
        !DXBCWriteUint32(kOutput, uGLSLChunkSize) ||
        !DXBCWriteUint32(kOutput, uNumSamplers) ||
        !DXBCWriteUint32(kOutput, 0) ||
        !DXBCWriteUint32(kOutput, 0) ||
        !DXBCWriteUint32(kOutput, 0) ||
        /*!DXBCWriteUint32(kOutput, pShader->reflection.ui32NumImports) ||
        !DXBCWriteUint32(kOutput, pShader->reflection.ui32NumExports) ||
        !DXBCWriteUint32(kOutput, pShader->reflection.ui32InputHash)*/
        !DXBCWriteUint32(kOutput, pShader->reflection.ui32NumResourceBindings) ||
        !DXBCWriteUint32(kOutput, pShader->reflection.ui32Thread_x) ||
        !DXBCWriteUint32(kOutput, pShader->reflection.ui32Thread_y) ||
        !DXBCWriteUint32(kOutput, pShader->reflection.ui32Thread_z))
    {
        return false;
    }
    for (uint32_t uTexture = 0; uTexture < MAX_RESOURCE_BINDINGS; ++uTexture)
    {
        uint32_t uSampler(pShader->reflection.aui32SamplerMap[uTexture]);
        if (uSampler != MAX_RESOURCE_BINDINGS)
        {
            if (!DXBCWriteUint32(kOutput, uTexture) ||
                !DXBCWriteUint32(kOutput, uSampler))
            {
                return false;
            }
        }
    }
    //for (uint32_t uSymbol = 0; uSymbol < pShader->reflection.ui32NumImports; ++uSymbol)
    //{
    //  if (!DXBCWriteUint32(kOutput, pShader->reflection.psImports[uSymbol].eType) ||
    //      !DXBCWriteUint32(kOutput, pShader->reflection.psImports[uSymbol].ui32ID) ||
    //      !DXBCWriteUint32(kOutput, pShader->reflection.psImports[uSymbol].ui32Value))
    //      return false;
    //}
    //for (uint32_t uSymbol = 0; uSymbol < pShader->reflection.ui32NumExports; ++uSymbol)
    //{
    //  if (!DXBCWriteUint32(kOutput, pShader->reflection.psExports[uSymbol].eType) ||
    //      !DXBCWriteUint32(kOutput, pShader->reflection.psExports[uSymbol].ui32ID) ||
    //      !DXBCWriteUint32(kOutput, pShader->reflection.psExports[uSymbol].ui32Value))
    //      return false;
    //}
    for (uint32_t uResource = 0; uResource < pShader->reflection.ui32NumResourceBindings; ++uResource)
    {
        ResourceBinding* rb = pShader->reflection.psResourceBindings + uResource;
        if (uResource != MAX_RESOURCE_BINDINGS)
        {
            if (!DXBCWriteUint32(kOutput, uResource) ||
                !DXBCWriteUint32(kOutput, rb->eBindArea))
            {
                return false;
            }
        }
    }

    if (!kOutput.Write(pShader->sourceCode, uGLSLSourceSize))
    {
        return false;
    }
    uOutSize += uGLSLChunkSize;

    // Write total size and chunk index
    if (!kOutput.SeekAbs(DXBC_SIZE_POSITION) ||
        !DXBCWriteUint32(kOutput, uOutSize) ||
        !kOutput.SeekAbs(DXBC_HEADER_SIZE) ||
        !DXBCWriteUint32(kOutput, uNumChunksOut + 1))
    {
        return false;
    }
    for (uint32_t uChunk = 0; uChunk < uNumChunksOut; ++uChunk)
    {
        if (!DXBCWriteUint32(kOutput, auChunkOffsetsOut[uChunk]))
        {
            return false;
        }
    }
    DXBCWriteUint32(kOutput, uGLSLChunkOffset);

    return true;
}
