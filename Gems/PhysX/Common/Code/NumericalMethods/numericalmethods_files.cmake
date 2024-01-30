#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#
#

set(FILES
    Include/NumericalMethods/Optimization.h
    Include/NumericalMethods/Eigenanalysis.h
    Include/NumericalMethods/DoublePrecisionMath/Quaternion.h
    Source/Optimization/Constants.h
    Source/Optimization/LineSearch.cpp
    Source/Optimization/LineSearch.h
    Source/Optimization/SolverBFGS.cpp
    Source/Optimization/SolverBFGS.h
    Source/Optimization/Utilities.cpp
    Source/Optimization/Utilities.h
    Source/Eigenanalysis/Solver3x3.cpp
    Source/Eigenanalysis/Solver3x3.h
    Source/Eigenanalysis/EigenanalysisUtilities.cpp
    Source/Eigenanalysis/Utilities.h
    Source/NumericalMethods.cpp
    Source/LinearAlgebra.cpp
    Source/LinearAlgebra.h
    Source/DoublePrecisionMath/Quaternion.cpp
)
