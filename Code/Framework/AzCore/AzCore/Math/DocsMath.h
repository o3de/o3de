/*
 * Copyright (c) Contributors to the Open 3D Engine Project.
 * For complete copyright and license terms please see the LICENSE at the root of this distribution.
 *
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

/**
 * \page AZBaseMath AZCore Math Overview
 *
 * \section Naming Method names
 * Functions which return a new copy of the object are prefixed with 'Get', functions which operate on the object
 * in-place will omit the prefix. For example, Matrix3x3::GetTranspose() will return a new Matrix3x3 and leaves the
 * original unchanged, whereas Matrix3x3::Transpose will transpose the matrix in-place.
 *
 * \section Constructors Constructors
 * We use the named constructor idiom instead of a regular constructor wherever possible. These are the static 'Create'
 * methods found in all math types. Using named constructors avoids ambiguity, and makes the code more explicit by
 * preventing the compiler from performing conversions 'behind your back'. (Declaring constructors as explicit goes
 * some of the way towards this goal, but it still is not clear what the code 'Transform x = Transform(y)' is actually
 * doing).
 *
 * \section MultiplicationOrder Multiplication order, handedness, row vs column vectors and storage
 * The standard way to transform a vector is to post-multiply the matrix by the column vector,
 * i.e. column_vector = matrix * column_vector.
 * This choice affects other math functions too, e.g. matrix-matrix multiplication order and the
 * quaternion-vector multiplication formula. Matrices should be multiplied as follows:
 *    - objectToCamera = worldToCamera * objectToWorld
 *
 * Positive rotation direction is defined using the right-hand-rule. Nearly all functions will work
 * with either left-handed or right-handed coordinates, but the few functions which don't (e.g.
 * perspective matrix generation) use right-handed coordinates.
 *
 * A little note about pre-multiplying vs. post-multiplying by vectors. There are two ways we can
 * multiply a vector and a matrix:
 *    - row_vector = row_vector * matrix
 *    - column_vector = matrix * column_vector
 * If we use column vector multiplication, then hardware with a fast dot product will need the
 * matrix stored in row-major format. Hardware which uses multiply-add instructions will prefer
 * the matrix to be stored in column-major format. We choose to use column vector multiplication
 * consistently, and allow different internal storage formats for different platforms.
 *
 * \section Estimates Estimates
 * Many architectures support estimate instructions, which can provide a low-cost low precision result for the requested operation.
 * We use the suffix 'Estimate' to indicate when a raw estimate instruction is used.
 *
 * A function without a suffix may use an approximation, but will have refine the estimate to provide decent accuracy.
 *
 * \section Casting Casting math types to floats
 * \li Rule #1: Don't do it.
 * \li Rule #2: Seriously, don't do it. Didn't you read rule 1?
 *
 * Use the provided functions to convert math types to and from floats. E.g. Vector3::CreateFromFloat3,
 * Matrix4x4::CreateFromRowMajorFloat16. There are several reasons casting directly is a bad idea:
 * \li The math types do not guarantee the internal format of the vector types. A Vector4 may be stored in XYZW order
 *     today, but could be changed to WZYX tomorrow if it gave a performance benefit. The order is also not consistent
 *     between platforms, even the size of the math types may vary.
 * \li Matrix types can be stored in either row-major or column-major format, this depends on the platform, and is
 *     transparent to the user, provided they do not cast directly to float.
 * \li Type based alias analysis on newer compilers will likely break your casts anyway, unless you are very careful.
 *     Strict aliasing is enabled by default on the some compilers, and maybe other platforms in the future.
 *
 * A corollary to this is that the math types should not be stored in data directly. Store floats in the data and
 * convert on load.
 */
