/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

// These macros can be used to declare common shader inputs for this feature. 
// Use the COMMON_SRG_INPUTS_* macro in your material SRG definition, and use the COMMON_OPTIONS_* macro at the global scope in your shader. Then you can pass these variables to the Get*Input() function below.
// You can optionally provide a prefix for the set of inputs which corresponds to a prefix string supplied by the .materialtype file. This is common for multi-layered material types.

#define COMMON_SRG_INPUTS_VERTEX_COLOR(prefix) \
float     prefix##m_vertexColorFactor;

#define COMMON_OPTIONS_VERTEX_COLOR(prefix) \
option bool prefix##o_useVertexColor; \
option TextureBlendMode prefix##o_vertexColorBlendMode = TextureBlendMode::Multiply;
