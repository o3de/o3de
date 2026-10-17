/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzToolsFramework/AzToolsFrameworkAPI.h>

#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace AzToolsFramework
{
    namespace EntityPresets
    {
        //! A component a preset needs, and the gem that supplies it.
        //!
        //! The gem name is carried because it is the part the user can act on: knowing that
        //! "PhysX Heightfield Collider" is missing does not tell you what to switch on, and the
        //! component itself cannot say where it would have come from when it is not there.
        struct RequiredComponent
        {
            AZStd::string m_componentName;
            AZStd::string m_gemName;
        };

        //! Those of @p required whose component is not registered in this editor.
        //!
        //! Availability is asked of the component registry rather than of the settings registry's
        //! gem list on purpose: a gem being enabled is not the same as its components being
        //! registered and usable, and it is the latter that decides whether a preset can build.
        AZTF_API AZStd::vector<RequiredComponent> MissingComponents(
            const AZStd::vector<RequiredComponent>& required);

        //! Tell the user the preset cannot run, and what is missing. Modal, and informational -
        //! there is no choice to offer, because nothing can be built.
        //!
        //! Call *before* opening an undo batch, so a refusal leaves nothing behind.
        AZTF_API void ReportMissingRequirements(
            const AZStd::string& presetName, const AZStd::vector<RequiredComponent>& missing);

        //! Ask whether to build a reduced version of the preset.
        //!
        //! For the case where the missing pieces are a layer on top rather than a prerequisite, so
        //! there is a real choice between something useful and nothing at all.
        //!
        //! @param reduction what the user gets instead, in their words - "without vegetation".
        //! @return false if the user cancelled, in which case nothing should be created.
        AZTF_API bool ConfirmReducedSetup(
            const AZStd::string& presetName,
            const AZStd::string& reduction,
            const AZStd::vector<RequiredComponent>& missing);
    } // namespace EntityPresets
} // namespace AzToolsFramework
