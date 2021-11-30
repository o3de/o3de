/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#include <AzCore/std/sort.h>

#include <SceneAPI/SceneCore/Utilities/Reporting.h>

#include <NumericalMethods/Eigenanalysis.h>
#include <NumericalMethods/Optimization.h>

#include <Source/Pipeline/PrimitiveShapeFitter/AbstractShapeParameterization.h>
#include <Source/Pipeline/PrimitiveShapeFitter/PrimitiveShapeFitter.h>

using namespace NumericalMethods;

namespace PhysX::Pipeline
{
    // Given an abstract shape parameterization that has been initialized to a suitable initial guess (e.g. using the
    // eigendecomposition of the vertex data covariance matrix), this class runs the BFGS solver to find the set of
    // shape paramters that minimize the objective function (which considers net deviation and volume).
    class ShapeFitter
    {
    public:
        // This struct is used as the return type for the optimization routine.
        struct Result
        {
            // The shape configuration.
            MeshAssetData::ShapeConfigurationPair m_shapeConfigurationPair
                = MeshAssetData::ShapeConfigurationPair{nullptr, nullptr};

            // The minimum mean square distance achieved.
            double m_distanceTerm = 0.0;

            // The minimum volume achieved.
            double m_volumeTerm = 0.0;

            // The minimum value of the objective function. This is a weighted sum of the two terms above.
            double m_minimum = 0.0;
        };

        ShapeFitter(
            const AZStd::vector<Vector>& vertices,
            const double volumeTermWeight,
            const double volumeTermNormalizationQuotient,
            const double distanceTermNormalizationQuotient
        )
            : m_vertices(vertices)
            , m_volumeTermWeight(volumeTermWeight)
            , m_volumeTermNormalizationQuotient(volumeTermNormalizationQuotient)
            , m_distanceTermNormalizationQuotient(distanceTermNormalizationQuotient)
        {
            // Sanity checks.
            AZ_Assert(
                volumeTermWeight >= 0.0 && volumeTermWeight < 1.0,
                "FitPrimitiveShape: The weight of the volume term must be in the interval [0.0, 1.0)."
            );

            AZ_Assert(
                !m_vertices.empty(),
                "FitPrimitiveShape: The objective function cannot be invoked with an empty vertex cloud."
            );

            AZ_Assert(
                m_volumeTermNormalizationQuotient > 0.0,
                "FitPrimitiveShape: The objective function cannot be invoked with a non-positive volume "
                "normalization coefficient."
            );

            AZ_Assert(
                m_distanceTermNormalizationQuotient > 0.0,
                "FitPrimitiveShape: The objective function cannot be invoked with a non-positive distance "
                "normalization coefficient."
            );
        }

        // Run the BFGS solver to find the optimal shape parameterization.
        Result ComputeOptimalShapeParameterization(AbstractShapeParameterization::Ptr&& shapeParamsPtr) const
        {
            // Create BFGS function to be optimized along with an initial guess.
            Function objectiveFn(*this, *shapeParamsPtr);
            const AZStd::vector<double> initialGuess = shapeParamsPtr->PackArguments();

            // Run the solver. The shape should already have been initialized with appropriate initial parameters.
            const Optimization::SolverResult solverResult = Optimization::SolverBFGS(objectiveFn, initialGuess);

            // Create and return result.
            Result result;

            if (
                solverResult.m_outcome == Optimization::SolverOutcome::Success ||
                solverResult.m_outcome == Optimization::SolverOutcome::Incomplete
            )
            {
                if (solverResult.m_outcome == Optimization::SolverOutcome::Incomplete)
                {
                    AZ_TracePrintf(
                        AZ::SceneAPI::Utilities::LogWindow,
                        "BFGS solver did not complete (ran %d iterations). Result may be inaccurate.\n",
                        solverResult.m_iterations
                    );
                }

                // We need to evaluate the function one last time to actually retrieve the minimum.
                shapeParamsPtr->UnpackArguments(solverResult.m_xValues);

                // Set result values.
                result.m_shapeConfigurationPair = shapeParamsPtr->GetShapeConfigurationPair();
                result.m_distanceTerm = objectiveFn.GetMeanSquareDistanceTerm();
                result.m_volumeTerm = objectiveFn.GetVolumeTerm();
                result.m_minimum = objectiveFn.GetWeightedTotal(result.m_distanceTerm, result.m_volumeTerm);
            }

            return result;
        }

    private:
        // The actual objective function that is optimized.
        class Function
            : public Optimization::Function
        {
        public:
            Function(const ShapeFitter& fitter, AbstractShapeParameterization& shapeParams)
                : m_fitter(fitter)
                , m_shapeParams(shapeParams)
            {
            }

            AZ::u32 GetDimension() const
            {
                return m_shapeParams.GetDegreesOfFreedom();
            }

            double GetMeanSquareDistanceTerm() const
            {
                double meanSquareDistance = 0.0;

                for (const auto& vertex : m_fitter.m_vertices)
                {
                    meanSquareDistance += m_shapeParams.SquaredDistanceToShape(vertex);
                }

                return meanSquareDistance / m_fitter.m_vertices.size()
                    / m_fitter.m_distanceTermNormalizationQuotient;
            }

            double GetVolumeTerm() const
            {
                return m_shapeParams.GetVolume() / m_fitter.m_volumeTermNormalizationQuotient;
            }

            double GetWeightedTotal(const double meanSquareDistanceTerm, const double volumeTerm) const
            {
                return meanSquareDistanceTerm * (1.0 - m_fitter.m_volumeTermWeight)
                    + volumeTerm * m_fitter.m_volumeTermWeight;
            }

            AZ::Outcome<double, Optimization::FunctionOutcome> ExecuteImpl(const AZStd::vector<double>& x) const
            {
                // Update the shape parameters from the given arguments.
                m_shapeParams.UnpackArguments(x);

                // Compute the value of the objective function. The function consists of two terms:
                //  - The mean square distance of the vertex cloud from the shape.
                //  - The normalized volume of the shape.
                //
                // The first term prefers shapes for which the input vertices are close to the shape surface. The second
                // term prefers shapes with small volumes, so as to shrink the shape around the vertex cloud as much as
                // possible. This is a requirement when the data cloud is sparse so that many possible solutions exist.

                if (m_fitter.m_volumeTermWeight > 0.0)
                {
                    return AZ::Success(GetWeightedTotal(GetMeanSquareDistanceTerm(), GetVolumeTerm()));
                }
                else
                {
                    return AZ::Success(GetMeanSquareDistanceTerm());
                }
            }

        private:
            const ShapeFitter& m_fitter;
            AbstractShapeParameterization& m_shapeParams;
        };

        const AZStd::vector<Vector>& m_vertices;
        const double m_volumeTermWeight;
        const double m_volumeTermNormalizationQuotient;
        const double m_distanceTermNormalizationQuotient;
    };

    static double ComputeHalfRangeOfProjectedData(const AZStd::vector<Vector>& vertices, const Vector& axis)
    {
        double minProjection = 0.0;
        double maxProjection = 0.0;

        for (const auto& vertex : vertices)
        {
            double projection = Dot(vertex, axis);

            if (projection > maxProjection)
            {
                maxProjection = projection;
            }
            else if (projection < minProjection)
            {
                minProjection = projection;
            }
        }

        return (maxProjection - minProjection) * 0.5;
    }

    template<typename T>
    void AddPrimitiveShapeCandidate(
        AZStd::vector<AZStd::pair<AZStd::string, ShapeFitter::Result>>& candidates,
        const AZStd::string& primitiveName,
        const ShapeFitter& fitter,
        const Eigenanalysis::SolverResult<Eigenanalysis::Real, 3>& eigensolverResult,
        const Vector& mean,
        const Vector& halfRanges
    )
    {
        AZ_TracePrintf(
            AZ::SceneAPI::Utilities::LogWindow,
            "Attempting to fit %s primitive.\n",
            primitiveName.c_str()
        );

        ShapeFitter::Result fittingResult = fitter.ComputeOptimalShapeParameterization(
            CreateAbstractShape<T>(
                mean,
                eigensolverResult.m_eigenpairs[0].m_vector,
                eigensolverResult.m_eigenpairs[1].m_vector,
                eigensolverResult.m_eigenpairs[2].m_vector,
                halfRanges
            )
        );

        if (fittingResult.m_shapeConfigurationPair.second)
        {
            AZ_TracePrintf(
                AZ::SceneAPI::Utilities::LogWindow,
                "Achieved minimal objective function value %g with (distance term, volume term) = (%g, %g).\n",
                fittingResult.m_minimum,
                fittingResult.m_distanceTerm,
                fittingResult.m_volumeTerm
            );

            candidates.emplace_back(primitiveName, AZStd::move(fittingResult));
        }
        else
        {
            AZ_TracePrintf(
                AZ::SceneAPI::Utilities::WarningWindow,
                "No suitable shape parameterization returned.\n"
            );
        }
    }

    MeshAssetData::ShapeConfigurationPair FitPrimitiveShape(
        [[maybe_unused]] const AZStd::string& meshName,
        const AZStd::vector<Vec3>& vertices,
        const double volumeTermWeight,
        const PrimitiveShapeTarget targetShape
    )
    {
        AZ_TracePrintf(
            AZ::SceneAPI::Utilities::LogWindow,
            "Attempting to fit primitive shape to mesh %s.\n",
            meshName.c_str()
        );

        if (!vertices.empty())
        {
            if (volumeTermWeight >= 0.0 && volumeTermWeight < 1.0)
            {
                const AZ::u32 numberOfVertices = static_cast<AZ::u32>(vertices.size());

                // Convert vertices and compute the mean of the vertex cloud.
                AZStd::vector<Vector> verticesConverted(numberOfVertices, Vector{{ 0.0, 0.0, 0.0 }});
                Vector mean{{ 0.0, 0.0, 0.0 }};

                AZStd::transform(
                    vertices.begin(),
                    vertices.end(),
                    verticesConverted.begin(),
                    [&mean] (const Vec3& vertex) {
                        Vector converted{{ vertex[0], vertex[1], vertex[2] }};
                        mean = mean + converted;
                        return converted;
                    }
                );

                mean = mean / numberOfVertices;

                // Shift the entire cloud by its mean so that vertices are centered around the origin.
                AZStd::vector<Vector> verticesCentered(numberOfVertices, Vector{{ 0.0, 0.0, 0.0 }});

                AZStd::transform(
                    verticesConverted.begin(),
                    verticesConverted.end(),
                    verticesCentered.begin(),
                    [&mean] (const Vector& vertex) {
                        return vertex - mean;
                    }
                );

                // Compute the 3x3 covariance matrix.
                Eigenanalysis::SquareMatrix<Eigenanalysis::Real, 3> covariances{};

                for (const auto& vertex : verticesCentered)
                {
                    for (int i = 0; i < 3; ++i)
                    {
                        for (int j = i; j < 3; ++j)
                        {
                            covariances[i][j] += vertex[i] * vertex[j];
                        }
                    }
                }

                covariances[0][0] = covariances[0][0] / numberOfVertices;
                covariances[1][1] = covariances[1][1] / numberOfVertices;
                covariances[2][2] = covariances[2][2] / numberOfVertices;

                covariances[1][0] = covariances[0][1] = covariances[0][1] / numberOfVertices;
                covariances[2][0] = covariances[0][2] = covariances[0][2] / numberOfVertices;
                covariances[2][1] = covariances[1][2] = covariances[1][2] / numberOfVertices;

                // Find the eigenvectors of the covariance matrix.
                Eigenanalysis::SolverResult<Eigenanalysis::Real, 3> eigensolverResult
                    = Eigenanalysis::Solver3x3RealSymmetric(covariances);

                if (eigensolverResult.m_outcome == Eigenanalysis::SolverOutcome::Success)
                {
                    // Sanity check.
                    AZ_Assert(
                        eigensolverResult.m_eigenpairs.size() == 3,
                        "FitPrimitiveShape: Require exactly three basis vectors. Given: %d",
                        eigensolverResult.m_eigenpairs.size()
                    );

                    // Sort eigenvalues from largest to smallest.
                    AZStd::sort(
                        eigensolverResult.m_eigenpairs.begin(),
                        eigensolverResult.m_eigenpairs.end(),
                        [] (
                            const Eigenanalysis::Eigenpair<Eigenanalysis::Real, 3>& lhs,
                            const Eigenanalysis::Eigenpair<Eigenanalysis::Real, 3>& rhs
                        ) {
                            return lhs.m_value > rhs.m_value;
                        }
                    );

                    // Ensure that we still have a right-handed system after sorting.
                    eigensolverResult.m_eigenpairs[2].m_vector = Cross(
                        eigensolverResult.m_eigenpairs[0].m_vector,
                        eigensolverResult.m_eigenpairs[1].m_vector
                    );

                    // Compute the half-ranges of the data along each of the principal axes.
                    Vector halfRanges = {{
                        ComputeHalfRangeOfProjectedData(verticesCentered, eigensolverResult.m_eigenpairs[0].m_vector),
                        ComputeHalfRangeOfProjectedData(verticesCentered, eigensolverResult.m_eigenpairs[1].m_vector),
                        ComputeHalfRangeOfProjectedData(verticesCentered, eigensolverResult.m_eigenpairs[2].m_vector)
                    }};

                    if (
                        !IsAbsoluteValueWithinEpsilon(halfRanges[0]) &&
                        !IsAbsoluteValueWithinEpsilon(halfRanges[1]) &&
                        !IsAbsoluteValueWithinEpsilon(halfRanges[2])
                    )
                    {
                        // Create optimizer, fit appropriate shape and return result.
                        const double volumeTermNormalizationQuotient = halfRanges[0] * halfRanges[1] * halfRanges[2];
                        const double distanceTermNormalizationQuotient = Norm(halfRanges);

                        ShapeFitter fitter(
                            verticesConverted,
                            volumeTermWeight,
                            volumeTermNormalizationQuotient,
                            distanceTermNormalizationQuotient
                        );

                        AZStd::vector<AZStd::pair<AZStd::string, ShapeFitter::Result>> candidates;

#define ADD_CANDIDATE(WHICH) \
    AddPrimitiveShapeCandidate<WHICH ## Parameterization>( \
        candidates, \
        #WHICH, \
        fitter, \
        eigensolverResult, \
        mean, \
        halfRanges \
    )

                        // Fit the appropriate shape and return result.
                        switch (targetShape)
                        {
                        case PrimitiveShapeTarget::BestFit:
                            ADD_CANDIDATE(Sphere);
                            ADD_CANDIDATE(Box);
                            ADD_CANDIDATE(Capsule);
                            break;

                        case PrimitiveShapeTarget::Sphere:
                            ADD_CANDIDATE(Sphere);
                            break;

                        case PrimitiveShapeTarget::Box:
                            ADD_CANDIDATE(Box);
                            break;

                        case PrimitiveShapeTarget::Capsule:
                            ADD_CANDIDATE(Capsule);
                            break;
                        }

#undef ADD_CANDIDATE

                        if (!candidates.empty())
                        {
                            const auto& bestFit = *(AZStd::minmax_element(
                                candidates.begin(),
                                candidates.end(),
                                [] (auto lhs, auto rhs) {
                                    return lhs.second.m_minimum < rhs.second.m_minimum;
                                }
                            ).first);

                            AZ_TracePrintf(
                                AZ::SceneAPI::Utilities::SuccessWindow,
                                "Successfully fitted %s primitive to mesh %s.\n",
                                bestFit.first.c_str(),
                                meshName.c_str()
                            );

                            return bestFit.second.m_shapeConfigurationPair;
                        }
                        else
                        {
                            AZ_TracePrintf(
                                AZ::SceneAPI::Utilities::ErrorWindow,
                                "Failed to fit a primitive to mesh %s. No suitable parameterization could be found.\n",
                                meshName.c_str()
                            );
                        }
                    }
                    else
                    {
                        AZ_TracePrintf(
                            AZ::SceneAPI::Utilities::ErrorWindow,
                            "Failed to fit a primitive to mesh %s. Vertices are not sufficiently well distributed.\n",
                            meshName.c_str()
                        );
                    }
                }
                else
                {
                    AZ_TracePrintf(
                        AZ::SceneAPI::Utilities::ErrorWindow,
                        "Failed to fit a primitive to mesh %s. Eigensolver terminated unsuccessfully.\n",
                        meshName.c_str()
                    );
                }
            }
            else
            {
                AZ_TracePrintf(
                    AZ::SceneAPI::Utilities::ErrorWindow,
                    "Failed to fit a primitive to mesh %s. "
                    "The weight of the volume term must be in the interval [0.0, 1.0).\n",
                    meshName.c_str()
                );
            }
        }
        else
        {
            AZ_TracePrintf(
                AZ::SceneAPI::Utilities::ErrorWindow,
                "Failed to fit a primitive to mesh %s. Mesh contains no vertices.\n",
                meshName.c_str()
            );
        }

        return MeshAssetData::ShapeConfigurationPair(nullptr, nullptr);
    }
} // PhysX::Pipeline
