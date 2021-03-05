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
#include "PoundPoundParser.h"
#include <AzCore/std/string/regex.h>

class PoundPoundContext::Impl
{
public:
    Impl() : m_ownedLayer(nullptr)
    {}
    ~Impl()
    {
        delete m_ownedLayer;
    }

    void Preprocess(char** buf);
    bool IsEndOfBuffer(char** buf, bool* layerSwitch);
    void SetDefine(const AZStd::string &name, const AZStd::string &value)
    {
        m_macros[name] = value;
    }

private:
    class Layer {
    public:
        Layer* m_ownedNextLayer;
        enum struct IfState {
            noIf,
            activeIf,
            triggeredIf,
            dormantIf,
            activeElse,
            dormantElse
        };
        IfState m_ifState;

        enum struct InterdictionState {
            interdictionActive,
            interdictionPaused
        };
        InterdictionState m_interdictionState;

        char* m_oldBuffer;
        char* m_ownedIncludeBuffer;

        Layer(Layer* layer)
            : m_ownedNextLayer(layer)
            , m_ifState(IfState::noIf)
            , m_interdictionState(InterdictionState::interdictionActive)
            , m_oldBuffer(nullptr)
            , m_ownedIncludeBuffer(nullptr)
        {
        }

        ~Layer()
        {
            delete[] m_ownedIncludeBuffer;
            delete m_ownedNextLayer;
        }
    };

    AZStd::string ConsumeLine(char** buf);
    using Tokens = AZStd::vector<AZStd::string>;
    Tokens TokenizeLine(char** buf, const char* regex);

    void EvaluateIf(const Tokens& tokens);

    void ProcessIf(char** buf);
    void ProcessElif(char** buf);
    void ProcessElse(char** buf);
    void ProcessEndif(char** buf);
    void ProcessDefine(char** buf);
    void ProcessUndef(char** buf);
    void ProcessInclude(char** buf);

    Layer *m_ownedLayer;
    using Macros = AZStd::unordered_map<AZStd::string, AZStd::string>;
    Macros m_macros;
};

void PoundPoundContext::Impl::Preprocess(char** buf)
{
    bool layerSwitch;

    // We should only get called when the buffer is pointing at a ## directive
    do 
    {
        // Handle the if/elif/else/endif directives. These always need to be processed because of scope stacking
        if (!strncmp(*buf, "##if", 4))
        {
            *buf += 4;
            ProcessIf(buf);
        }
        else if (!strncmp(*buf, "##elif", 6))
        {
            *buf += 6;
            ProcessElif(buf);
        }
        else if (!strncmp(*buf, "##else", 6))
        {
            *buf += 6;
            ProcessElse(buf);
        }
        else if (!strncmp(*buf, "##endif", 7))
        {
            *buf += 7;
            ProcessEndif(buf);
        }

        // If the interdiction is active, these ## directives can be ignored
        else if (m_ownedLayer != nullptr && m_ownedLayer->m_interdictionState == Layer::InterdictionState::interdictionActive)
        {
            if (!strncmp(*buf, "##define", 8) || !strncmp(*buf, "##undef", 7) || !strncmp(*buf, "##include_restricted", 20))
            {
                ConsumeLine(buf);
            }
            else
            {
                AZ_Assert(false, "unknown ## directive");
            }
        }

        // Otherwise, process the non-if related tokens
        else
        {
            if (!strncmp(*buf, "##define", 8))
            {
                *buf += 8;
                ProcessDefine(buf);
            }
            else if (!strncmp(*buf, "##undef", 7))
            {
                *buf += 7;
                ProcessUndef(buf);
            }
            else if (!strncmp(*buf, "##include_restricted", 20))
            {
                *buf += 20;
                ProcessInclude(buf);
            }
            else
            {
                AZ_Assert(false, "unknown ## directive");
            }
        }

        // We've processed the token line. If we aren't interdicting the text, let the caller process normally
        if (!m_ownedLayer
            || m_ownedLayer->m_interdictionState == Layer::InterdictionState::interdictionPaused)
        {
            return;
        }

        // We are interdicting, so keep going until we find another ## directive or we run out of buffer
        while (!IsEndOfBuffer(buf, &layerSwitch))
        {
            if (**buf == '#' && (*buf)[1] == '#')
            {
                break;
            }
            ++(*buf);
        }

        // Return if we are at EOB, otherwise start parsing the next ## directive.
    } while (!IsEndOfBuffer(buf, &layerSwitch));
}

bool PoundPoundContext::Impl::IsEndOfBuffer(char** buf, bool* layerSwitch)
{
    // Standard non-variant EOB cases
    if (*buf == nullptr)
    {
        return true;
    }
    if (**buf != 0)
    {
        return false;
    }

    // At this point we have hit a zero in the buffer. If there are no layers active, then we are truly
    // at the end of parseable text
    if (m_ownedLayer == nullptr)
    {
        return true;
    }

    // Otherwise, we must be at the end of a ##include file. Indicate a layer switch (because the caller may need to
    // reparse past comments/whitespace), then restore the previous layer and delete this one.
    AZ_Assert(m_ownedLayer->m_ownedIncludeBuffer != nullptr, "#include buffer is missing");
    AZ_Assert(m_ownedLayer->m_ifState == Layer::IfState::noIf, "Reached end of #include buffer while #if was still in scope");
    *layerSwitch = true;
    auto next = m_ownedLayer->m_ownedNextLayer;
    *buf = m_ownedLayer->m_oldBuffer;
    m_ownedLayer->m_ownedNextLayer = nullptr;
    delete m_ownedLayer;
    m_ownedLayer = next;

    // We don't know whether the new layer is at EOB, so we now have to check it.
    return IsEndOfBuffer(buf, layerSwitch);
}

AZStd::string PoundPoundContext::Impl::ConsumeLine(char** buf)
{
    // Copy the remainder of the line into a scratch buffer, consuming the text including the EOL in the buffer
    auto start = *buf;
    char ch;
    while ((ch = **buf) != 0)
    {
        if (ch == '\n')
        {
            break;
        }
        ++(*buf);
    }
    auto lineLen = *buf - start;
    auto lineBuf = new char[lineLen + 1];
    azstrncpy(lineBuf, lineLen + 1, start, lineLen);
    // Make sure to have a null-terminated buffer
    lineBuf[lineLen] = '\0';

    if (**buf == '\n')
    {
        ++(*buf);
    }

    AZStd::string result(lineBuf);
    delete[] lineBuf;
    return result;
}

// Use regex to parse text into tokens
PoundPoundContext::Impl::Tokens PoundPoundContext::Impl::TokenizeLine(char** buf, const char* pattern)
{
    AZStd::vector<AZStd::string> tokens;

    auto lineBuf = ConsumeLine(buf);
    AZStd::cmatch match;
    if (AZStd::regex_search(lineBuf.c_str(), match, AZStd::regex(pattern)))
    {
        // First pattern is always the entire string
        for (unsigned i = 1; i < match.size(); ++i)
        {
            if (match[i].matched)
            {
                tokens.push_back(match[i].str().c_str());
            }
        }
    }

    return tokens;
}

// Handles evaluating the conditional we support. ##if NAME will simply test for the existance of the NAME macro, pausing the
// interdiction if it is true. ## NAME == VALUE will convert either both sides to values (if they are macros) and then compare,
// pausing interdiction if the two sides equate. Everything is done as strings.
void PoundPoundContext::Impl::EvaluateIf(const Tokens& tokens)
{
    auto tokenCount = tokens.size();
    switch (tokenCount)
    {
    case 1:
    {
        auto finder = m_macros.find(tokens[0]);
        if (finder != m_macros.end())
        {
            m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionPaused;
            m_ownedLayer->m_ifState = Layer::IfState::triggeredIf;
        }
    }
    break;

    case 3:
    {
        AZ_Assert(tokens[1] == "==", "Unknown operator %s in ##if/##elif", tokens[1].c_str());

        // Expand both sides of the file as needed
        auto finder = m_macros.find(tokens[0]);
        AZStd::string value1 = (finder == m_macros.end()) ? tokens[0] : finder->second;
        finder = m_macros.find(tokens[2]);
        AZStd::string value2 = (finder == m_macros.end()) ? tokens[2] : finder->second;

        // start reading again if true
        if (value1 == value2)
        {
            m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionPaused;
            m_ownedLayer->m_ifState = Layer::IfState::triggeredIf;
        }
    }
    break;

    default:
        AZ_Assert(false, "Malformed ##if/##elif condition");
        break;
    }
}

void PoundPoundContext::Impl::ProcessIf(char** buf)
{
    auto oldLayer = m_ownedLayer;
    m_ownedLayer = new Layer(oldLayer);

    // Initialize the layer state based on parent state
    m_ownedLayer->m_ifState = Layer::IfState::activeIf;
    bool parseIf = true;
    if (oldLayer != nullptr)
    {
        // Look at the parent's if state (if the parent is an #include file, we know it is active so no
        // need to test for weird combos there) and make sure to start disabled if the parent is disabled.
        // There are possible modes of nesting here that haven't been handled...
        switch (oldLayer->m_ifState)
        {
        case Layer::IfState::activeIf:
        case Layer::IfState::triggeredIf:
            AZ_Assert(false, "unimplemented ##if state");
            break;

        case Layer::IfState::dormantIf:
        case Layer::IfState::dormantElse:
            m_ownedLayer->m_ifState = Layer::IfState::dormantIf;
            parseIf = false;
            break;
        }
    }

    // Mark the layer as interdicted until we know it isn't
    m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionActive;
    auto tokens = TokenizeLine(buf, "\\s*(\\w+)\\s*(==)?\\s*(\\w+)?\\s*$");
    if (parseIf)
    {
        EvaluateIf(tokens);
    }
}

void PoundPoundContext::Impl::ProcessElif(char** buf)
{
    AZ_Assert(m_ownedLayer != nullptr, "missing ##if state");
    switch (m_ownedLayer->m_ifState)
    {
    case Layer::IfState::dormantIf:
    case Layer::IfState::triggeredIf:
        ConsumeLine(buf);
        m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionActive;
        break;

    case Layer::IfState::activeIf:
    {
        m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionActive;
        auto tokens = TokenizeLine(buf, "\\s*(\\w+)\\s*(==)?\\s*(\\w+)?\\s*$");
        EvaluateIf(tokens);
        break;
    }

    case Layer::IfState::dormantElse:
    case Layer::IfState::activeElse:
    case Layer::IfState::noIf:
        AZ_Assert(false, "##elif not permitted here");
        break;
    }
}

void PoundPoundContext::Impl::ProcessElse(char** buf)
{
    ConsumeLine(buf);

    AZ_Assert(m_ownedLayer != nullptr, "missing ##if state");
    switch (m_ownedLayer->m_ifState)
    {
    case Layer::IfState::activeIf:
        m_ownedLayer->m_ifState = Layer::IfState::activeElse;
        m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionPaused;
        break;

    case Layer::IfState::dormantIf:
    case Layer::IfState::triggeredIf:
        m_ownedLayer->m_ifState = Layer::IfState::dormantElse;
        m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionActive;
        break;

    case Layer::IfState::noIf:
    case Layer::IfState::activeElse:
    case Layer::IfState::dormantElse:
        AZ_Assert(false, "##else not permitted here");
        break;
    }
}

void PoundPoundContext::Impl::ProcessEndif(char** buf)
{
    ConsumeLine(buf);

    AZ_Assert(m_ownedLayer != nullptr, "missing ##if state");
    AZ_Assert(m_ownedLayer != nullptr, "##else not permitted");
    AZ_Assert(m_ownedLayer->m_ifState != Layer::IfState::noIf, "##else not permitted");

    auto next = m_ownedLayer->m_ownedNextLayer;
    m_ownedLayer->m_ownedNextLayer = nullptr;
    delete m_ownedLayer;
    m_ownedLayer = next;
}

void PoundPoundContext::Impl::ProcessDefine(char** buf)
{
    auto tokens = TokenizeLine(buf, "\\s*(\\w+)\\s+(\\w+)?\\s*$");
    auto count = tokens.size();
    AZ_Assert(count == 1 || count == 2, "malformed ##define");

    auto finder = m_macros.find(tokens[0]);
    AZ_Assert(finder == m_macros.end(), "duplicate ##define %s", tokens[0].c_str());

    m_macros[tokens[0]] = (count == 2) ? tokens[1] : "1";
}

void PoundPoundContext::Impl::ProcessUndef(char** buf)
{
    auto tokens = TokenizeLine(buf, "\\s*(\\w+)\\s*$");
    AZ_Assert(tokens.size() == 1, "malformed ##undef");

    auto finder = m_macros.find(tokens[0]);
    if (finder != m_macros.end())
    {
        m_macros.erase(finder);
    }
}

#if defined(AZ_PLATFORM_WINDOWS)
#   define AZ_PATH_SEPARATOR_TOKEN  "\\"
#else
#   define AZ_PATH_SEPARATOR_TOKEN  "/"
#endif

void PoundPoundContext::Impl::ProcessInclude(char** buf)
{
    auto tokens = TokenizeLine(buf, "\\s*\\(\\s*(\\w+)\\s*,\\s*(\\w+)\\s*\\)\\s*$");

    auto oldLayer = m_ownedLayer;
    m_ownedLayer = new Layer(m_ownedLayer);
    m_ownedLayer->m_interdictionState = Layer::InterdictionState::interdictionPaused;

    // Look for our root file and platform tokens
    AZ_Assert(tokens.size() == 2, "Malformed ##include_restricted");
    auto platform = m_macros.find(tokens[1]);
    AZ_Assert(platform != m_macros.end(), "Can't expand %s", tokens[1].c_str());

    // Compute and open the filename
    auto restrictedFile = platform->second + AZ_PATH_SEPARATOR_TOKEN + tokens[0] + "_" + platform->second + ".cfr";
    char nameFile[256];
    sprintf_s(nameFile, "%sCryFX/%s", gRenDev->m_cEF.m_ShadersPath.c_str(), restrictedFile.c_str());
    AZ::IO::HandleType includeFileHandle = gEnv->pCryPak->FOpen(nameFile, "rb");
    AZ_Assert(includeFileHandle != AZ::IO::InvalidHandle, "Couldn't open file %s", nameFile);

    // Read the file into a new buffer which we will substitute back into the stream.
    gEnv->pCryPak->FSeek(includeFileHandle, 0, SEEK_END);
    auto readSize = gEnv->pCryPak->FTell(includeFileHandle);
    m_ownedLayer->m_ownedIncludeBuffer = new char[readSize + 1];
    m_ownedLayer->m_ownedIncludeBuffer[readSize] = 0;
    gEnv->pCryPak->FSeek(includeFileHandle, 0, SEEK_SET);
    gEnv->pCryPak->FRead(m_ownedLayer->m_ownedIncludeBuffer, readSize, includeFileHandle);
    gEnv->pCryPak->FClose(includeFileHandle);

    // Remember the old buffer pointer, then insert our new pointer
    m_ownedLayer->m_oldBuffer = *buf;
    *buf = m_ownedLayer->m_ownedIncludeBuffer;

    // Prepare the file just as the calling file had been prepared
    RemoveCR(m_ownedLayer->m_ownedIncludeBuffer);
}


PoundPoundContext::PoundPoundContext([[maybe_unused]] const AZStd::string& shadersFilter)
    : m_impl(new Impl)
{
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM) || defined(AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS)
#define AZ_RESTRICTED_PLATFORM_EXPANSION(CodeName, CODENAME, codename, PrivateName, PRIVATENAME, privatename, PublicName, PUBLICNAME, publicname, PublicAuxName1, PublicAuxName2, PublicAuxName3)\
        if (shadersFilter == #PrivateName)                              \
        {                                                               \
            m_impl->SetDefine("AZ_RESTRICTED_PLATFORM", #privatename);  \
        }
#if defined(AZ_EXPAND_FOR_RESTRICTED_PLATFORM)
    AZ_EXPAND_FOR_RESTRICTED_PLATFORM
#else
    AZ_TOOLS_EXPAND_FOR_RESTRICTED_PLATFORMS
#endif
#undef AZ_RESTRICTED_PLATFORM_EXPANSION
#endif
}

PoundPoundContext::~PoundPoundContext()
{
    delete m_impl;
}

void PoundPoundContext::PreprocessLines(char** buf)
{
    m_impl->Preprocess(buf);
}

bool PoundPoundContext::IsEndOfBuffer(char** buf, bool* layerSwitch)
{
    return m_impl->IsEndOfBuffer(buf, layerSwitch);
}
