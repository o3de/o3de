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
        // helper interface to component entity cameras
        class ISceneCamera
        {
        public:
            virtual ~ISceneCamera() = default;

            virtual const AZ::Vector3 GetWorldPosition() const = 0;
            virtual const AZ::Quaternion GetWorldRotation() const = 0;

            virtual float GetFoV() const = 0;
            virtual float GetNearZ() const = 0;

            // Setting methods are supposed to check for needed changes.
            virtual void SetWorldPosition(const AZ::Vector3& worldPosition) = 0;
            // Keeps existing world position
            virtual void SetWorldRotation(const AZ::Quaternion& worldRotation) = 0;
            virtual void SetFovAndNearZ(float degreesFoV, float nearZ) = 0;

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
        // With a valid camera selection key, apply camera properties at ec.time,
        // interpolating with a next key camera properties, if available
        void ApplyCameraKey(int currKeyIdx, ISelectKey& currKey, const SAnimContext& ec);
        // Initialize camera properties, if the key is valid (camera EntityId is valid), so that these can be restored after changes during interpolation.
        // @return True, if the key is re-initialized.
        bool InitializeCameraProperties(ISelectKey& key) const;
        // Restore cameras' properties, if available, because these could be changed while playing the sequence having CSelectTrack.
        void RestoreCameraProperties(ISelectKey& key) const;

        bool OverrideCameraIdNeeded(); // Returns true if a previously active camera state is overridden by a camera properties set by ICVar in CMovieSystem.
        bool RestoreOverriddenCameraIdNeeded(); // Returns true if a previously active camera state is restored after an override.

        void ApplyEventKey(IEventKey& key, SAnimContext& ec);
        void ApplyConsoleKey(IConsoleKey& key, SAnimContext& ec);
        void ApplyAudioKey(char const* const sTriggerName, bool const bPlay = true) override;
        void ApplySequenceKey(IAnimTrack* pTrack, int nPrevKey, int nCurrKey, ISequenceKey& key, SAnimContext& ec);

        void ApplyGotoKey(CGotoTrack* poGotoTrack, SAnimContext& ec);

        void InitializeTrackDefaultValue(IAnimTrack* pTrack, const CAnimParamType& paramType, AnimValueType remapValueType = AnimValueType::Unknown) override;

        // Cached parameters of node at given time.
        float m_time = 0.0f;

        CSelectTrack* m_CurrentSelectTrack;

        AZ::EntityId m_OverrideCamId; // A Camera Component EntityId to override camera selections, if set by ICVar in CMovieSystem.
        ISelectKey m_overriddenCameraProperties; // In Play Game mode, properties of an overridden active camera.

        //! Last animated key numbers in tracks.
        int m_lastEventKey;
        int m_lastConsoleKey;
        int m_lastSequenceKey;
        int m_nLastGotoKey;
        int m_lastCaptureKey;
        bool m_bLastCapturingEnded;
        int m_captureFrameCount;

        AZStd::vector<SSoundInfo> m_SoundInfo;

        AZ::TimeUs m_simulationTickOverrideBackup = AZ::Time::ZeroTimeUs;
        float m_timeScaleBackup = 1.0f;
    };

} // namespace Maestro
