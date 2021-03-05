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

#include <platform.h>

#if defined(AZ_RESTRICTED_PLATFORM)
    #include AZ_RESTRICTED_FILE(Gem_h)
#endif

#if defined(LINUX) || defined(APPLE) || defined(ANDROID)
    #define OPENGL 1
#endif

#include <Common/RendererDefs.h>

#include <Cry_Math.h>
#include <Cry_XOptimise.h>
#include <Cry_Math.h>
#include <Cry_Geo.h>
#include <CryArray.h>
#include <CryHeaders.h>
#include <CrySizer.h>
#include <CryArray.h>

#include <IProcess.h>
#include <ITimer.h>
#include <ISystem.h>
#include <ILog.h>
#include <IConsole.h>
#include <IRenderer.h>
#include <IRenderAuxGeom.h>
#include <IEntityRenderState.h>
#include <I3DEngine.h>
#include <IStreamEngine.h>

#include <CryArray2d.h>
#include <PoolAllocator.h>
#include <Cry3DEngineBase.h>
#include <cvars.h>
#include <Material.h>
#include <3dEngine.h>
#include <ObjMan.h>
#include <StlUtils.h>

#include <Common/CommonRender.h>
#include <Common/Shaders/ShaderComponents.h>
#include <Common/Shaders/Shader.h>
#include <Common/Shaders/CShader.h>
#include <Common/RenderMesh.h>
#include <Common/RenderPipeline.h>
#include <Common/RenderThread.h>
#include <Common/Renderer.h>
#include <Common/Textures/Texture.h>
#include <Common/Shaders/Parser.h>

#include <RenderDll/Common/OcclQuery.h>
#include <RenderDll/Common/DeferredRenderUtils.h>
#include <RenderDll/Common/Textures/TextureManager.h>
#include <RenderDll/Common/FrameProfiler.h>
#include <RenderDll/XRenderD3D9/DeviceManager/DeviceManagerInline.h>
#include <RenderDll/XRenderD3D9/DriverD3D.h>
#include <RenderDll/XRenderD3D9/DeviceManager/TempDynBuffer.h>

#include <AzCore/PlatformDef.h>
#include <AzCore/Asset/AssetCommon.h>
#include <AzCore/Asset/AssetManager.h>
#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Asset/AssetTypeInfoBus.h>
#include <AzCore/Component/Component.h>
#include <AzCore/Component/ComponentBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/IO/GenericStreams.h>
#include <AzCore/IO/FileIO.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/RTTI/BehaviorContext.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Serialization/EditContext.h>
#include <AzCore/XML/rapidxml.h>

#include <AzFramework/Asset/SimpleAsset.h>
#include <AzFramework/Asset/AssetCatalogBus.h>
#include <AzFramework/Entity/EntityDebugDisplayBus.h>

#include <LmbrCentral/Rendering/RenderNodeBus.h>
