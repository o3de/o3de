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

#pragma once

#include <Common/RendererDefs.h>

#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <StlUtils.h>
#include "Common/DevBuffer.h"

#include "XRenderD3D9/DeviceManager/DeviceManager.h"

#include <VertexFormats.h>

#include "Common/CommonRender.h"
#include <IRenderAuxGeom.h>
#include "Common/Shaders/ShaderComponents.h"
#include "Common/Shaders/Shader.h"
#include "Common/Shaders/CShader.h"
#include "Common/RenderMesh.h"
#include "Common/RenderPipeline.h"
#include "Common/RenderThread.h"

#include "Common/Renderer.h"
#include "Common/Textures/Texture.h"

#include "Common/OcclQuery.h"

#include "Common/PostProcess/PostProcess.h"

// All handled render elements (except common ones included in "RendElement.h")
#include "Common/RendElements/CREBeam.h"
#include "Common/RendElements/CREClientPoly.h"
#include "Common/RendElements/CRELensOptics.h"
#include "Common/RendElements/CREHDRProcess.h"
#include "Common/RendElements/CRECloud.h"
#include "Common/RendElements/CREDeferredShading.h"
#include "Common/RendElements/CREMeshImpl.h"
