/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/Component/ComponentBus.h>
#include <WhiteBox/WhiteBoxToolApi.h>

namespace WhiteBox
{
    //! Enumerates the different type of transform sub-modes available.
    enum class TransformType
    {
        Translation,
        Rotation,
        Scale
    };

    //! Request bus for White Box ComponentMode operations while in 'transform' mode.
    class EditorWhiteBoxTransformModeRequests : public AZ::EntityComponentBus
    {
    public:
        //! Change the TransformType for the WhiteBox Transform sub-mode.
        virtual void ChangeTransformType(TransformType subModeType) = 0;

        // ------------------------------------------------------------------ //
        // Blender-style numeric input dispatch methods                       //
        // ------------------------------------------------------------------ //
        virtual void NumericBeginMove()           = 0;
        virtual void NumericBeginRotate()         = 0;
        virtual void NumericBeginScale()          = 0;
        virtual void NumericSetAxisX()            = 0;
        virtual void NumericSetAxisY()            = 0;
        virtual void NumericSetAxisZ()            = 0;
        virtual void NumericConfirm()             = 0;
        virtual void NumericCancel()              = 0;
        virtual void NumericBackspace()           = 0;
        virtual void NumericDecimal()             = 0;
        virtual void NumericNegate()              = 0; //!< '-' operator
        virtual void NumericAppendDigit(char d)   = 0;
        virtual void NumericAppendOperatorPlus()  = 0; //!< '+' binary add
        virtual void NumericAppendOperatorMult()  = 0; //!< '*' multiply
        virtual void NumericAppendOperatorDiv()   = 0; //!< '/' divide

    protected:
        ~EditorWhiteBoxTransformModeRequests() = default;
    };

    using EditorWhiteBoxTransformModeRequestBus = AZ::EBus<EditorWhiteBoxTransformModeRequests>;
} // namespace WhiteBox
