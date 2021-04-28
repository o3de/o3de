// Modifications copyright Amazon.com, Inc. or its affiliates.

namespace decomp {
// Taken from http://tog.acm.org/GraphicsGems/gemsiv/polar_decomp/Decompose.h

/**** Decompose.h - Basic declarations ****/
#ifndef CRYINCLUDE_CRYCOMMONTOOLS_DECOMPOSE_H
#define CRYINCLUDE_CRYCOMMONTOOLS_DECOMPOSE_H
#pragma once

typedef struct {float x, y, z, w;} Quat; /* Quaternion */
enum QuatPart {X, Y, Z, W};
typedef Quat HVect; /* Homogeneous 3D vector */
typedef float HMatrix[4][4]; /* Right-handed, for column vectors */
typedef struct {
    HVect t;    /* Translation components */
    Quat  q;    /* Essential rotation      */
    Quat  u;    /* Stretch rotation      */
    HVect k;    /* Stretch factors      */
    float f;    /* Sign of determinant      */
} AffineParts;
float polar_decomp(HMatrix M, HMatrix Q, HMatrix S);
HVect spect_decomp(HMatrix S, HMatrix U);
Quat snuggle(Quat q, HVect *k);
void decomp_affine(HMatrix A, AffineParts *parts);
void invert_affine(AffineParts *parts, AffineParts *inverse);

#endif // CRYINCLUDE_CRYCOMMONTOOLS_DECOMPOSE_H

}
