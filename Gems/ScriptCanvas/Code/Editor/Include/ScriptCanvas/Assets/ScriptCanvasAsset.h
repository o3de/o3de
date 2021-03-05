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

#include <AzCore/Asset/AssetCommon.h>
#include <ScriptCanvas/Asset/AssetDescription.h>
#include <ScriptCanvas/Assets/ScriptCanvasAssetBus.h>
#include <ScriptCanvas/Assets/ScriptCanvasBaseAssetData.h>

#include <ScriptCanvas/Asset/ScriptCanvasAssetBase.h>

namespace AZ
{
    class Entity;
}

namespace ScriptCanvas
{
    class Graph;
}

namespace ScriptCanvasEditor
{
    class Graph;
    class ScriptCanvasAsset;

    class ScriptCanvasAssetDescription : public ScriptCanvas::AssetDescription
    {
    public:

        AZ_TYPE_INFO(ScriptCanvasAssetDescription, "{3678E33E-521B-4CAC-9DC1-42566AC71249}");

        ScriptCanvasAssetDescription()
            : ScriptCanvas::AssetDescription(
                azrtti_typeid<ScriptCanvasAsset>(),
                "Script Canvas",
                "Script Canvas Graph Asset",
                "@devassets@/scriptcanvas",
                ".scriptcanvas",
                "Script Canvas",
                "Untitled-%i",
                "Script Canvas Files (*.scriptcanvas)",
                "Script Canvas",
                "Script Canvas",
                "Editor/Icons/ScriptCanvas/Viewport/ScriptCanvas.png",
                AZ::Color(0.321f, 0.302f, 0.164f, 1.0f),
                true
            )
        {}
    };

    class ScriptCanvasAsset
        : public ScriptCanvas::ScriptCanvasAssetBase
    {

    public:
        AZ_RTTI(ScriptCanvasAsset, "{FA10C3DA-0717-4B72-8944-CD67D13DFA2B}", ScriptCanvas::ScriptCanvasAssetBase);
        AZ_CLASS_ALLOCATOR(ScriptCanvasAsset, AZ::SystemAllocator, 0);

        ScriptCanvasAsset(const AZ::Data::AssetId& assetId = AZ::Data::AssetId(AZ::Uuid::CreateRandom()),
            AZ::Data::AssetData::AssetStatus status = AZ::Data::AssetData::AssetStatus::NotLoaded)
            : ScriptCanvas::ScriptCanvasAssetBase(assetId, status)
        {
            m_data = aznew ScriptCanvas::ScriptCanvasData();
        }
        ~ScriptCanvasAsset() override
        {
        }

        ScriptCanvas::AssetDescription GetAssetDescription() const override
        {
            return ScriptCanvasAssetDescription();
        }
       
        ScriptCanvas::Graph* GetScriptCanvasGraph() const;
        using Description = ScriptCanvasAssetDescription;

        ScriptCanvas::ScriptCanvasData& GetScriptCanvasData();
        const ScriptCanvas::ScriptCanvasData& GetScriptCanvasData() const;

    };
}
