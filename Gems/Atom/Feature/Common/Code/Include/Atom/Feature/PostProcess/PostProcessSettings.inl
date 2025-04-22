/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */


// Specifies sub-settings of PostProcessSettings like Depth of Field
// Used to auto-generate a large part of PostProcessSettings.h and .cpp
//
// To add new post process sub-settings to PostProcessSettings simply add an entry here using the format
//
// POST_PROCESS_MEMBER(ClassName, MemberName)
//
// Where ClassName is the name of your post process sub-settings class and MemberName is the name you want to give the 
// member of type ClassName in PostProcessSettings (be sure to include the appropriate header in PostProcessSettings.h)
//
// Note: Any class you add here must define a Simulate and ApplySettingsTo function

POST_PROCESS_MEMBER(BloomSettings, m_bloomSettings)
POST_PROCESS_MEMBER(DepthOfFieldSettings, m_depthOfFieldSettings)
POST_PROCESS_MEMBER(ExposureControlSettings, m_exposureControlSettings)
POST_PROCESS_MEMBER(SsaoSettings, m_ssaoSettings)
POST_PROCESS_MEMBER(LookModificationSettings, m_lookModificationSettings)
POST_PROCESS_MEMBER(DeferredFogSettings, m_deferredFogSettings)
POST_PROCESS_MEMBER(HDRColorGradingSettings, m_hdrColorGradingSettings)
POST_PROCESS_MEMBER(ChromaticAberrationSettings, m_ChromaticAberrationSettings)
POST_PROCESS_MEMBER(PaniniProjectionSettings, m_PaniniProjectionSettings)
POST_PROCESS_MEMBER(FilmGrainSettings, m_FilmGrainSettings)
POST_PROCESS_MEMBER(WhiteBalanceSettings, m_whiteBalanceSettings)
POST_PROCESS_MEMBER(VignetteSettings, m_VignetteSettings)
