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
