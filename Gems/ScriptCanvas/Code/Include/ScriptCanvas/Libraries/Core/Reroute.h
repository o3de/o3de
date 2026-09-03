/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <ScriptCanvas/Core/Node.h>

namespace ScriptCanvas::Nodes::Core
{
    //! A translation-transparent connection routing node.
    class Reroute
        : public Node
    {
    public:
        AZ_COMPONENT(Reroute, "{B6D19D0B-FF6C-4D2C-A719-1D91528F574E}", Node);

        enum class Mode : AZ::u8
        {
            Data,
            Execution
        };

        Reroute();

        static void Reflect(AZ::ReflectContext* reflection);

        void ConfigureMode(Mode mode);
        Mode GetMode() const;

        AZ::Outcome<DependencyReport, void> GetDependencies() const override;
        bool IsConnectionTransparentForTranslation() const override;
        bool IsNoOp() const override;

    protected:
        ConstSlotsOutcome GetSlotsInExecutionThreadByTypeImpl(
            const Slot&, CombinedSlotType targetSlotType, const Slot*) const override;
        void OnInit() override;

    private:
        void ConfigureSlotsForMode();

        Mode m_mode = Mode::Data;
    };
} // namespace ScriptCanvas::Nodes::Core
