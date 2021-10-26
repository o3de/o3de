/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/Component.h>
#include <Include/SandboxAPI.h>
#include "EditorTrackViewEventsBus.h"

namespace AzToolsFramework
{
    //! A legacy component to reflect scriptable commands for the Editor
    class TrackViewFuncsHandler
        : public AZ::Component
    {
    public:
        AZ_COMPONENT(TrackViewFuncsHandler, "{5315678D-2951-4CF6-A9DC-CE21CD23C9C9}")

        SANDBOX_API static void Reflect(AZ::ReflectContext* context);

        // AZ::Component ...
        void Activate() override {}
        void Deactivate() override {}
    };

    //! Component to access the TrackView
    AZ_PUSH_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    AZ_PUSH_DISABLE_DLL_EXPORT_MEMBER_WARNING
    class SANDBOX_API TrackViewComponent final
        : public AZ::Component
        , public EditorLayerTrackViewRequestBus::Handler
    {
    AZ_POP_DISABLE_DLL_EXPORT_MEMBER_WARNING
    AZ_POP_DISABLE_DLL_EXPORT_BASECLASS_WARNING
    public:
        AZ_COMPONENT(TrackViewComponent, "{3CF943CC-6F10-4B19-88FC-CFB697558FFD}")

        TrackViewComponent() = default;
        ~TrackViewComponent() override = default;

        static void Reflect(AZ::ReflectContext* context);

        // Component...
        void Activate() override;
        void Deactivate() override;

        int GetNumSequences() override;

        void NewSequence(const char* name, int sequenceType) override;

        void PlaySequence() override;

        void StopSequence() override;

        void SetSequenceTime(float time) override;

        void AddSelectedEntities() override;

        void AddLayerNode() override;

        void AddTrack(const char* paramName, const char* nodeName, const char* parentDirectorName) override;

        void DeleteTrack(const char* paramName, uint32 index, const char* nodeName, const char* parentDirectorName) override;

        int GetNumTrackKeys(const char* paramName, int trackIndex, const char* nodeName, const char* parentDirectorName) override;

        void SetRecording(bool bRecording) override;

        void DeleteSequence(const char* name) override;

        void SetCurrentSequence(const char* name) override;

        AZStd::string GetSequenceName(unsigned int index) override;

        TRange<float> GetSequenceTimeRange(const char* name) override;

        void AddNode(const char* nodeTypeString, const char* nodeName) override;

        void DeleteNode(AZStd::string_view nodeName, AZStd::string_view parentDirectorName) override;

        int GetNumNodes(AZStd::string_view parentDirectorName) override;

        AZStd::string GetNodeName(int index, AZStd::string_view parentDirectorName) override;

        AZStd::any GetKeyValue(const char* paramName, int trackIndex, int keyIndex, const char* nodeName, const char* parentDirectorName) override;

        AZStd::any GetInterpolatedValue(const char* paramName, int trackIndex, float time, const char* nodeName, const char* parentDirectorName) override;

        void SetSequenceTimeRange(const char* name, float start, float end) override;

    };

} // namespace AzToolsFramework
