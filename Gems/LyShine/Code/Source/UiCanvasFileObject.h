/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */
#pragma once

#include <AzCore/RTTI/TypeInfo.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <LyShine/Bus/Tools/UiSystemToolsBus.h>
#include <AzCore/Script/ScriptAsset.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
//! Wrapper class for canvas file. This allows us to make changes to what the top
//! level objects are in the canvas file and do some conversion
class UiCanvasFileObject
    : public UiSystemToolsInterface::CanvasAssetHandle
{
public:
    ~UiCanvasFileObject() override { }
    AZ_CLASS_ALLOCATOR(UiCanvasFileObject, AZ::SystemAllocator);
    AZ_RTTI(UiCanvasFileObject, "{1F02632F-F113-49B1-85AD-8CD0FA78B8AA}");

    // Load canvas from stream with an optional asset filter. No asset references are ignored by default
    static UiCanvasFileObject* LoadCanvasFromStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());
    static void SaveCanvasToStream(AZ::IO::GenericStream& stream, UiCanvasFileObject* canvasFileObject);

    static AZ::Entity* LoadCanvasEntitiesFromStream(AZ::IO::GenericStream& stream, AZ::Entity*& rootSliceEntity);

    static void Reflect(AZ::ReflectContext* context);

public: // data

    AZ::Entity* m_canvasEntity = nullptr;
    AZ::Entity* m_rootSliceEntity = nullptr;

private: // static methods

    static UiCanvasFileObject* LoadCanvasEntitiesFromOldFormatFile(const char* buffer, size_t bufferSize, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());
    static UiCanvasFileObject* LoadCanvasFromNewFormatStream(AZ::IO::GenericStream& stream, const AZ::ObjectStream::FilterDescriptor& filterDesc = AZ::ObjectStream::FilterDescriptor());

    static AZ::SerializeContext::DataElementNode* FindRootElementInCanvasEntity(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& canvasEntityNode);

    static bool CreateRootSliceNodeAndCopyInEntities(
        AZ::SerializeContext& context,
        AZ::SerializeContext::DataElementNode& canvasFileObjectNode,
        AZStd::vector<AZ::SerializeContext::DataElementNode>& copiedEntities);

    static bool VersionConverter(AZ::SerializeContext& context, AZ::SerializeContext::DataElementNode& classElement);
};
