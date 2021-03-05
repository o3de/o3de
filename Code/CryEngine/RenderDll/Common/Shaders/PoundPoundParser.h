#pragma once

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

// This is the interface object that handles the ## directive processing in the shader files. It
// pre-processes the text stream in a C-like manner, removing chunks of text from the input stream
// so that they never enter the token stream.
//
// supported directives:
//   ##if - works like C #if, currently supports only the following tests:
//          ##if token - enables the branch if token is defined
//          ##if token1 == token2 - will first try to expand the two tokens, then will string compare the result.
//                                  If a token isn't expanded, it will be used as the string to compare.
//   ##elif - just like C #if, same caveats as ##if
//   ##else - just like C #else
//   ##endif - just like C #endif
//   ##define - just like C define, but doesn't support parenthesis so it is just the token
//   ##undef - just like C undef
//   ##include_restricted(rootfile, macro) -
//       This will build a filename using macro and rootfile and then process it. For example, given
//       ##include_restricted(shader_cfx, AZ_RESTRICTED_PLATFORM) and AZ_RESTRICTED_PLATFORM set to "banana",
//       it will open and process a file called "banana/shader_cfx_banana.cfr".

class PoundPoundContext
{
public:
    explicit PoundPoundContext(const AZStd::string& shadersFilter);
    PoundPoundContext(const PoundPoundContext& other) = delete;
    void operator=(const PoundPoundContext& other) = delete;
    ~PoundPoundContext();

    // Call this function when encountering ## in the input stream. It will consume all text starting with the ##
    // until it reaches a state where regular token parsing might be enabled again
    void PreprocessLines(char** buf);

    // Callers need to use this function to test for the end of the buffer because we handle switching from an
    // include file back to the #including file inside this function. The layerSwitch bool is needed so that the
    // caller can know that they need to possibly start scanning for comments/whitespace again due to the change
    // in which buffer is being scanned
    bool IsEndOfBuffer(char** buf, bool* layerSwitch);

private:
    class Impl;
    Impl *m_impl;
};
