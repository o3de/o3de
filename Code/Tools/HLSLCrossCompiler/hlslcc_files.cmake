#
# All or portions of this file Copyright (c) Amazon.com, Inc. or its affiliates or
# its licensors.
#
# For complete copyright and license terms please see the LICENSE at the root of this
# distribution (the "License"). All use of this software is governed by the License,
# or, if provided, by the license below or the license accompanying this file. Do not
# remove or modify any license notices. This file is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#

set(FILES
    offline/hash.h
    offline/serializeReflection.h
    offline/timer.h
    offline/compilerStandalone.cpp
    offline/serializeReflection.cpp
    offline/timer.cpp
    offline/cjson/cJSON.h
    offline/cjson/cJSON.c
    src/amazon_changes.c
    src/decode.c
    src/decodeDX9.c
    src/reflect.c
    src/toGLSL.c
    src/toGLSLDeclaration.c
    src/toGLSLInstruction.c
    src/toGLSLOperand.c
    src/hlslccToolkit.c
    src/internal_includes/debug.h
    src/internal_includes/decode.h
    src/internal_includes/hlslcc_malloc.h
    src/internal_includes/hlslcc_malloc.c
    src/internal_includes/languages.h
    src/internal_includes/reflect.h
    src/internal_includes/shaderLimits.h
    src/internal_includes/structs.h
    src/internal_includes/toGLSLDeclaration.h
    src/internal_includes/toGLSLInstruction.h
    src/internal_includes/toGLSLOperand.h
    src/internal_includes/tokens.h
    src/internal_includes/tokensDX9.h
    src/internal_includes/hlslccToolkit.h
    src/cbstring/bsafe.h
    src/cbstring/bstraux.h
    src/cbstring/bstrlib.h
    src/cbstring/bsafe.c
    src/cbstring/bstraux.c
    src/cbstring/bstrlib.c
    include/amazon_changes.h
    include/hlslcc.h
    include/hlslcc.hpp
    include/hlslcc_bin.hpp
    include/pstdint.h
)

set(SKIP_UNITY_BUILD_INCLUSION_FILES
    # 'bsafe.c' tries to forward declar 'strncpy', 'strncat', etc, but they are already declared in other modules. Remove from unity builds conideration
    src/cbstring/bsafe.c
)
