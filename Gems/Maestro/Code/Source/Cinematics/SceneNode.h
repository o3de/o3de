/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/Time/ITime.h>

#include "AnimNode.h"
#include "SoundTrack.h"
#include "SelectTrack.h"

namespace Maestro
{

    class CGotoTrack;

    class CAnimSceneNode : public CAnimNode
    {
    public:
        AZ_CLASS_ALLOCATOR(CAnimSceneNode, AZ::SystemAllocator);
        AZ_RTTI(CAnimSceneNode, "{659BB221-38D3-43C0-BEE4-7EAB49C8CB33}", CAnimNode);

        ///////////////////////////////////////////////////////////////////////////////////////////////
        // helper interface for a uniform interface to legacy and component entity cameras
        class ISceneCamera
        {
        public:
            virtual ~ISceneCamera() = default;

            virtual const Vec3& GetPosition() const = 0;
            virtual const Quat& GetRotation() const = 0;
            virtual void SetPosition(const Vec3& localPosition) = 0;
            virtual void SetRotation(const Quat& localRotation) = 0;

            virtual float GetFoV() const = 0;
            virtual float GetNearZ() const = 0;

            // includes check for changes
            virtual void SetNearZAndFOVIfChanged(float fov, float nearZ) = 0;
            virtual void TransformPositionFromLocalToWorldSpace(Vec3& position) = 0;
            virtual void TransformPositionFromWorldToLocalSpace(Vec3& position) = 0;
            virtual void TransformRotationFromLocalToWorldSpace(Quat& rotation) = 0;
            // keeps existing world position
            virtual void SetWorldRotation(const Quat& rotation) = 0;

            // returns true if the camera has a parent
            virtual bool HasParent() const = 0;

        protected:
            ISceneCamera() {}
        };

        CAnimSceneNode(const int id);
        CAnimSceneNode();
        ~CAnimSceneNode();
        static void Initialize();

        //////////////////////////////////////////////////////////////////////////
        // Overrides from CAnimNode
        //////////////////////////////////////////////////////////////////////////
        void Animate(SAnimContext& ec) override;
        void CreateDefaultTracks() override;

        void Serialize(XmlNodeRef& xmlNode, bool bLoading, bool bLoadEmptyTracks) override;

        void Activate(bool bActivate) override;

        // overridden from IAnimNode/CAnimNode
        void OnStart() override;
        void OnReset() override;
        void OnPause() override;
        void OnStop() override;
        void OnLoop() override;

        //////////////////////////////////////////////////////////////////////////
        unsigned int GetParamCount() const override;
        CAnimParamType GetParamType(unsigned int nIndex) const override;

        void PrecacheStatic(float startTime) override;
        void PrecacheDynamic(float time) override;

        static void Reflect(AZ::ReflectContext* context);

        // Utility function to find the sequence associated with an ISequenceKey
        static IAnimSequence* GetSequenceFromSequenceKey(const ISequenceKey& sequenceKey);

    protected:
        bool GetParamInfoFromType(const CAnimParamType& paramId, SParamInfo& info) const override;

        void ResetSounds() override;
        void ReleaseSounds(); // Stops audio

    private:
        void ApplyCameraKey(ISelectKey& key, SAnimContext& ec);
        void ApplyEventKey(IEventKey& key, SAnimContext& ec);
        void ApplyConsoleKey(IConsoleKey& key, SAnimContext& ec);
        void ApplyAudioKey(char const* const sTriggerName, bool const bPlay = true) override;
        void ApplySequenceKey(IAnimTrack* pTrack, int nPrevKey, int nCurrKey, ISequenceKey& key, SAnimContext& ec);

        void ApplyGotoKey(CGotoTrack* poGotoTrack, SAnimContext& ec);

        // fill retInterpolatedCameraParams with interpolated camera data. If firstCameraId is a valid AZ::EntityId, it is used.
        // should be non-null. Preference will be given to firstCamera if they are both non-null
        void InterpolateCameras(
            SCameraParams& retInterpolatedCameraParams, ISceneCamera* firstCamera, ISelectKey& firstKey, ISelectKey& secondKey, float time);

        void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType) override;

        // Cached parameters of node at given time.
        float m_time = 0.0f;

        CSelectTrack* m_CurrentSelectTrack;
        int m_CurrentSelectTrackKeyNumber;
        IAnimNode* m_pCamNodeOnHoldForInterp;
        float m_lastPrecachePoint;

        //! Last animated key in track.
        int m_lastCameraKey;
        int m_lastEventKey;
        int m_lastConsoleKey;
        int m_lastSequenceKey;
        int m_nLastGotoKey;
        int m_lastCaptureKey;
        bool m_bLastCapturingEnded;
        int m_captureFrameCount;

        struct InterpolatingCameraStartState
        {
            Vec3 m_interpolatedCamFirstPos;
            Quat m_interpolatedCamFirstRot;
            float m_FoV;
            float m_nearZ;
        };

        using keyIdx = int;

        // each camera key with a blend time > 0 needs a stashed initial xform for interpolation
        AZStd::map<keyIdx, InterpolatingCameraStartState> m_InterpolatingCameraStartStates;

        AZStd::vector<SSoundInfo> m_SoundInfo;

        AZ::TimeUs m_simulationTickOverrideBackup = AZ::Time::ZeroTimeUs;
        float m_timeScaleBackup = 1.0f;
    };

} // namespace Maestro
