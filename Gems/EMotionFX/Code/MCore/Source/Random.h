/*
 * Copyright (c) Contributors to the Open 3D Engine Project. For complete copyright and license terms please see the LICENSE at the root of this distribution.
 * 
 * SPDX-License-Identifier: Apache-2.0 OR MIT
 *
 */

#pragma once

#include <AzCore/std/containers/vector.h>
#include "StandardHeaders.h"
#include <AzCore/std/containers/vector.h>
#include <AzCore/Math/Vector3.h>


namespace MCore
{
    /**
    * A very simple and fast LCG random number generator, useful if you need a fast pseudo-random sequence and
    * don't really care about the quality of the sequence.
    * This is similar to the SimpleLcgRandom class in AZ with a few modification.
    */
    class MCORE_API LcgRandom
    {
    public:

        LcgRandom(AZ::u64 seed = 1234) { SetSeed(seed); }

        void SetSeed(AZ::u64 seed)
        {
            m_seed = seed;
        }

        AZ::u64 GetSeed() const { return m_seed; }

        unsigned int GetRandom();

        //Gets a random float in the range [0,1)
        float GetRandomFloat();

    private:
        AZ::u64 m_seed;
    };

    /**
     * A random number generation class.
     * This class can generate both random numbers and vectors using several different algorithms.
     */
    class MCORE_API Random
    {
    public:
        /**
         * Generate a uniform random float in range of [0..1].
         * @result A uniform random floating point number in range of [0..1].
         */
        static MCORE_INLINE float RandF()                                                           { return rand() / (float)RAND_MAX; }

        /**
         * Generate a uniform random float in a range of a given minimum and maximum.
         * @param minVal The minimum value of the range.
         * @param maxVal The maximum value of the range.
         * @result A uniform random floating point number in range of [min..max].
         */
        static MCORE_INLINE float RandF(float minVal, float maxVal)                                 { return minVal + (maxVal - minVal) * rand() / (float)RAND_MAX; }

        /**
        * Generate a uniform random float in a range of a given minimum and maximum.
        * @param minVal The minimum value of the range.
        * @param maxVal The maximum value of the range.
        * @result A uniform random floating point number in range of [min..max].
        */
        static MCORE_INLINE float RandF(float minVal, float maxVal, unsigned int seed) { AZ_UNUSED(seed); return minVal + (maxVal - minVal) * rand() / (float)RAND_MAX; }

        /**
        * Generate a uniform random float in a range of a given minimum and maximum.
        * @param minVal The minimum value of the range.
        * @param maxVal The maximum value of the range.
        * @result A uniform random floating point number in range of [min..max].
        */
        static MCORE_INLINE float RandF(float minVal, float maxVal, LcgRandom& rand) { return minVal + (maxVal - minVal) * rand.GetRandomFloat(); }

        /**
         * Generates a uniform random normalized direction vector, using floats.
         * @result A uniform random direction vector with a length of 1.
         */
        static AZ::Vector3 RandDirVecF();

        /**
         * Generates a uniform random vector where (x,y,z) are in range of [-1..+1] using floats.
         * @result A uniform random vector in range of [-1..+1].
         */
        static MCORE_INLINE AZ::Vector3 RandomVecF()                                                 { return AZ::Vector3(RandF(-1, 1), RandF(-1, 1), RandF(-1, 1)); }

        /**
         * Generates a uniform random direction vector from a given direction and cone angle.
         * This means that if you specify pi as cone angle, with a direction vector of (0,1,0) you will get
         * a random vector on the upper hemisphere above the XZ plane.
         * @param dir The base direction.
         * @param coneAngle The maximum spread angle in radians, which together with the base direction creates a cone, in which a random vector is generated and returned.
         * @result A random vector in the cone defined by the direction and cone angle.
         */
        static AZ::Vector3 RandomDirVec(const AZ::Vector3& dir, float coneAngle);

        /**
         * Generates a uniform random direction vector inside a patch on a cone.
         * This can be used for stratisfied sampling, where you divide for example a hemisphere into equally sized sections (grid cells/patches) and
         * you generate a random sample inside the given grid cells.
         * @param dir The base direction.
         * @param coneAngle The cone angle, in radians, where pi is a hemisphere.
         * @param gridSizeX The number of grid cells for x (phi).
         * @param gridSizeY The number of grid cells for y (theta).
         * @param xGridPos The x (phi) grid number to create a sample in.
         * @param yGridPos The y (theta) grid number to create a sample in.
         * @result A random vector on the defined cone, which is split up into <gridSizeX * gridSizeY> grid cells, and where the random vector is a random vector
         * within grid cell number (xGridPos, yGridPos].
         */
        static AZ::Vector3 RandomDirVec(const AZ::Vector3& dir, float coneAngle, uint32 gridSizeX, uint32 gridSizeY, uint32 xGridPos, uint32 yGridPos);

        /**
         * Generates a uniform random vector inside a given area on a cone.
         * @param dir The base direction.
         * @param startPhi The start Phi value of the area to create a random sample in.
         * @param endPhi The end Phi value of the area to create a random sample in.
         * @param startTheta The start theta value of the area to create a random sample in.
         * @param endTheta The end theta value of the area to create a random sample in.
         * @param midPoint If set to true, the generated random direction will be made exactly in the center of the defined area, so not really random. If set to true,
         *                 a random sample will be taken inside the specified area.
         * @result A random vector pointing towards a random (or the center) position in the specified area on the cone.
         */
        static AZ::Vector3 RandomDirVec(const AZ::Vector3& dir, float startPhi, float endPhi, float startTheta, float endTheta, bool midPoint = false); // returns a random direction vector, using <dir> as basis vector with a cone of <coneAngle>, sampled between the given phi and theta values. If <midPoint> is true the centerpoint of the given region will be returned

        /**
         * Convert an u, v [0..1] to a uniform (so area corrected) direction vector on a specified cone.
         * @param dir The base direction.
         * @param coneAngle The angle of the cone in radians, so pi would be a hemisphere.
         * @param u The value for phi, in range of [0..1].
         * @param v The value for theta, in range of [0..1].
         * @result The direction vector.
         */
        static AZ::Vector3 UVToVector(const AZ::Vector3& dir, float coneAngle, float u, float v);   // convert uv [0..1] to a vector on the sphere or hemisphere

        /**
         * Generate a given number of stratisfied uniform random samples on a cone.
         * @param dir The base direction.
         * @param coneAngle The angle of the cone in radians, where pi would be a hemisphere.
         * @param numVectors The number of direction vectors to generate. This might not be the number of vectors returned by this method!
         * @result The array containing the random vectors. The length of the array will be equal to (Sqrt(numVectors) * Sqrt(numVectors)).
         */
        static AZStd::vector<AZ::Vector3> RandomDirVectorsStratisfied(const AZ::Vector3& dir, float coneAngle, uint32 numVectors);

        /**
         * Generate a given amount of uniform direction vectors using Hammersley sets.
         * @param dir The base direction.
         * @param coneAngle The cone angle in radians, where pi would be a hemisphere.
         * @param numVectors The number of vectors to generate.
         * @result An array containing the generated vectors.
         */
        static AZStd::vector<AZ::Vector3> RandomDirVectorsHammersley(const AZ::Vector3& dir, float coneAngle, uint32 numVectors);

        /**
         * Generate a given amount of uniform direction vectors using Hammersley sets.
         * @param dir The base direction.
         * @param coneAngle The cone angle in radians, where pi would be a hemisphere.
         * @param numVectors The number of vectors to generate.
         * @param base The base number of the sequence (values of 2 or 3 are nice (must be ap rime value))
         * @result An array containing the generated vectors.
         */
        static AZStd::vector<AZ::Vector3> RandomDirVectorsHammersley2(const AZ::Vector3& dir, float coneAngle, uint32 numVectors, uint32 base);

        /**
         * Generate a given amount of uniform direction vectors using Halton sequences.
         * @param dir The base direction.
         * @param coneAngle The cone angle in radians, where pi would be a hemisphere.
         * @param numVectors The number of vectors to generate.
         * @param p2 The base of the halton sequence (must be a prime value).
         * @result An array containing the generated vectors.
         */
        static AZStd::vector<AZ::Vector3> RandomDirVectorsHalton(const AZ::Vector3& dir, float coneAngle, size_t numVectors, uint32 p2 = 3);

        /**
         * Generate a given amount of uniform direction vectors using hammersley sets.
         * @param dir The base direction.
         * @param coneAngle The cone angle in radians, where pi would be a hemisphere.
         * @param numVectors The number of vectors to generate.
         * @param baseA The first base value (must be a prime value).
         * @param baseB The second base value (must be a prime value).
         * @result An array containing the generated vectors.
         */
        static AZStd::vector<AZ::Vector3> RandomDirVectorsHalton2(const AZ::Vector3& dir, float coneAngle, size_t numVectors, uint32 baseA = 2, uint32 baseB = 3);

        /**
         * Generates a set of Halton numbers, which are pseudo random numbers.
         * @param result The array to store the resulting numbers in (this needs to be twice as large as the 'num' parameter, since it are two dimensions!)
         * @param num The number of pointSETS to generate (one set is 2 points).
         * @param p2 The base number of the set, which must be a prime value.
         */
        static void PlaneHalton(float* result, uint32 num, uint32 p2 = 3);

        /**
         * Generates a set of Halton numbers, which are pseudo random numbers.
         * @param result The array to store the resulting numbers in (this needs to be twice as large as the 'num' parameter, since it are two dimensions!)
         * @param num The number of pointSETS to generate (one set is 2 points).
         * @param baseA The first base number of the set, which must be a prime value.
         * @param baseB The second base number of the set, which must be a prime value.
         */
        static void PlaneHalton2(float* result, uint32 num, uint32 baseA = 2, uint32 baseB = 3);

        /**
         * Generates a set of Hammersley numbers, which are pseudo random numbers.
         * @param result The array to store the resulting numbers in (this needs to be twice as large as the 'num' parameter, since it are two dimensions!)
         * @param num The number of pointSETS to generate (one set is 2 points).
         */
        static void PlaneHammersley(float* result, uint32 num);

        /**
         * Generates a set of Hammersley numbers, which are pseudo random numbers.
         * @param result The array to store the resulting numbers in (this needs to be twice as large as the 'num' parameter, since it are two dimensions!)
         * @param num The number of pointSETS to generate (one set is 2 points).
         * @param base The base number of the set, which must be a prime value (3 would be a good value).
         */
        static void PlaneHammersley2(float* result, uint32 num, uint32 base);
    };



    /**
     * The Halton Sequence class.
     * Halton sequences are pseudo random numbers, actually known as Quasi Random Numbers.
     * They are often used in Monte Carlo sampling methods, because they are very good for that, especially for higher dimensions.
     * The generated numbers are always the same. You can however add some real randomness to it to let the random numbers you get
     * start at a given random offset (instance).
     * The values generated try to keep an as large as possible distance between eachother, to prevent groups of points very close to eachother at some spots.
     * This is one of the reasons why they are so cool. They work basically the same as stratisfied sampling your values, but with the nice feature that you do not have
     * to know upfront how many random numbers you will need. So you can keep asking for the next value in the set.
     * The values returned by the sequence are in range of [0..1].
     * If for example you want to generate random points in a 2D box, you can create a two dimensional HaltonSequence. Let me demonstrate with some example code:
     * <pre>
     *
     * // create a two dimensional Halton sequence
     * HaltonSequence seq(2);
     *
     * // generate 100 random points inside the 1x1 unit 2D box.
     * for (uint32 i=0; i<100; i++)
     * {
     *    float x = seq[0];
     *    float y = seq[1];
     *    seq.Next();
     * }
     *
     * </pre>
     * You now might want to ask, why not just use the Random::RandF() function for this?
     * Well, the answer is that the distribution of the samples is much more evenly with the Halton sequences as with just normal random numbers using Random::RandF().
     * When not using Random::RandF() you can get areas where there are more points as in other areas, so no even distribution. This can lead to variance in estimates using random values.
     * HaltonSequences converge much faster, so result in a more accurate estimate.
     */
    class MCORE_API HaltonSequence
    {
    public:
        /**
         * The default constructor.
         * Be sure to call the Init(...) method before using any other methods of this class.
         */
        HaltonSequence();

        /**
         * Extended constructor.
         * This constructor automatically calls the Init method.
         * Please keep in mind that MCore needs to be initialized before doing this, so if you have a global
         * HaltonSequence object that uses this constructor, this will cause a crash.
         * In that case, use the Init method with the default constructor that takes no parameters.
         * @param dimensions The number of dimensions for the sequence.
         * @param offset The offset to start in the sequence.
         * @param primes The list of prime values. Keep this nullptr when you want to generate them automatically.
         */
        HaltonSequence(uint32 dimensions, uint32 offset = 0, uint32* primes = nullptr);

        /**
         * Destructor.
         */
        ~HaltonSequence();

        /**
         * Initialize the sequence.
         * You can also call the Init method multiple times with different parameters to change its settings.
         * @param dimensions The number of dimensions for the sequence.
         * @param offset The offset to start in the sequence.
         * @param primes The list of prime values. Keep this nullptr when you want to generate them automatically.
         */
        void Init(uint32 dimensions, uint32 offset = 0, uint32* primes = nullptr);

        /**
         * Release all allocated memory.
         * This is automatically called by the destructor.
         */
        void Release();

        /**
         * Returns the number of dimensions in the sequence.
         * @result The number of dimensions.
         */
        uint32 GetNumDimensions() const                                                                 { return mDimensions; }

        /**
         * Get the memory usage in bytes by this sequence.
         * @result The memory usage in bytes by this sequence.
         */
        uint32 GetMemoryUsage() const                                                                   { return mMemory; }

        /**
         * Get the current vector number.
         * @result The vector number.
         */
        uint32 GetVectorNumber() const                                                                  { return (mN - mN0); }

        /**
         * Get the value of current dimension, and go to the next dimension.
         * @result The value of the current dimension, and step to the next dimension.
         */
        double GetNextDimension()                                                                       { MCORE_ASSERT(mNextDim != mDimensions); return mX[mNextDim++]; }

        /**
         * Reset the dimension stepping (by GetNextDimension()) and go to the first dimension again.
         */
        void ResetNextDimension()                                                                       { mNextDim = 0; }

        /**
         * Restart the sequence.
         */
        void Restart()                                                                                  { SetInstance(mN0); }

        /**
         * Get the next values in the sequence. So update the dimension values.
         */
        void Next()                                                                                     { ++(*this); }

        /**
         * Set the instance in the sequence. So the startpoint/offset in the sequence, where 0 would jump to the beginning again.
         * @param instance The instance value, so the startpoint in the sequence.
         */
        void SetInstance(uint32 instance);

        /**
         * Get the next values in the sequence. So update the dimension values.
         */
        void operator++();

        /**
         * Get a value for a given dimension. (*sequence[0]) would be the value of the first dimension and (*sequence[1]) would be the value of the second dimension, etc.
         */
        double operator[](uint32 j) const                                                           { MCORE_ASSERT(j < mDimensions); return mX[j]; }


    private:
        uint32      mDimensions;    /**< The number of dimensions to generate random numbers for. */
        uint32      mNextDim;       /**< The next dimension. */
        uint32      mMemory;        /**< */
        uint32      mN;             /**< */
        uint32      mN0;            /**< */
        double*     mX;             /**< */
        double*     mRadical;       /**< */
        uint32*     mBase;          /**< Prime numbers. */
        bool        mOwnBase;       /**< Specifies whether we use our own primes or user specified. */

        /**
         * Generates the first n number of primes.
         * @param n The number of primes to generate.
         * @result The array with allocated prime values.
         */
        uint32* FirstPrimes(uint32 n) const;
    };
}   // namespace MCore
