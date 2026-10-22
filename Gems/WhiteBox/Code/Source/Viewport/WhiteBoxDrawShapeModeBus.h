/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>

namespace WhiteBox
{
    //! Request bus for the White Box draw-shape sub-mode's Blender-style numeric
    //! depth entry. Keyboard actions (registered against the draw sub-mode's
    //! action context) dispatch to the active DrawShapeMode through this bus.
    class EditorWhiteBoxDrawShapeModeRequests : public AZ::EntityComponentBus
    {
    public:
        virtual void NumericAppendDigit(char digit) = 0;
        virtual void NumericAppendDecimal()         = 0;
        virtual void NumericNegate()                = 0; //!< '-' (leading = negative, binary = subtract)
        virtual void NumericAppendOperatorPlus()    = 0; //!< '+'
        virtual void NumericAppendOperatorMult()    = 0; //!< '*'
        virtual void NumericAppendOperatorDiv()     = 0; //!< '/'
        virtual void NumericBackspace()             = 0;
        virtual void NumericConfirm()               = 0; //!< Enter: apply depth + commit
        virtual void NumericCancel()                = 0; //!< Escape: cancel numeric (then draw)

    protected:
        ~EditorWhiteBoxDrawShapeModeRequests() = default;
    };

    using EditorWhiteBoxDrawShapeModeRequestBus = AZ::EBus<EditorWhiteBoxDrawShapeModeRequests>;
} // namespace WhiteBox
