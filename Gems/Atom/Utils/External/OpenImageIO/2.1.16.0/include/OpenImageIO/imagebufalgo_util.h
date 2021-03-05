// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md

// clang-format off

#pragma once

#include <functional>

#include <OpenImageIO/parallel.h>
#include <OpenImageIO/imagebufalgo.h>



OIIO_NAMESPACE_BEGIN

using std::bind;
using std::ref;
using std::cref;
using namespace std::placeholders;
using std::placeholders::_1;


namespace ImageBufAlgo {



/// Helper template for generalized multithreading for image processing
/// functions.  Some function/functor f is applied to every pixel the
/// region of interest roi, dividing the region into multiple threads if
/// threads != 1.  Note that threads == 0 indicates that the number of
/// threads should be as set by the global OIIO "threads" attribute.
///
/// The optional splitdir determines along which axis the split will be
/// made. The default is Split_Y (vertical splits), which generally seems
/// the fastest (due to cache layout issues?), but perhaps there are
/// algorithms where it's better to split in X, Z, or along the longest
/// axis.
///
/// Most image operations will require additional arguments, including
/// additional input and output images or other parameters.  The
/// parallel_image template can still be used by employing the
/// std::bind. For example, suppose you have an image operation defined as:
///     void my_image_op (ImageBuf &out, const ImageBuf &in,
///                       float scale, ROI roi);
/// Then you can parallelize it as follows:
///     ImageBuf R, A;   // result, input
///     ROI roi = get_roi (R.spec());
///     parallel_image (bind(my_image_op,ref(R), cref(A),3.14,_1), roi);
inline void
parallel_image (ROI roi, parallel_image_options opt,
                std::function<void(ROI)> f)
{
    opt.resolve ();
    // Try not to assign a thread less than 16k pixels, or it's not worth
    // the thread startup/teardown cost.
    opt.maxthreads = std::min (opt.maxthreads, 1 + int(roi.npixels() / opt.minitems));
    if (opt.singlethread()) {
        // Just one thread, or a small image region, or if recursive use of
        // parallel_image is disallowed: use this thread only
        f (roi);
        return;
    }

    // If splitdir was not explicit, find the longest edge.
    SplitDir splitdir = opt.splitdir;
    if (splitdir == Split_Biggest)
        splitdir = roi.width() > roi.height() ? Split_X : Split_Y;

    int64_t xchunk = 0, ychunk = 0;
    if (splitdir == Split_Y) {
        xchunk = roi.width();
        // ychunk = std::max (64, minitems/xchunk);
    } else if (splitdir == Split_X) {
        ychunk = roi.height();
        // ychunk = std::max (64, minitems/xchunk);
    } else if (splitdir == Split_Tile) {
        int64_t n = std::min<imagesize_t>(opt.minitems, roi.npixels());
        xchunk = ychunk = std::max (1, int(std::sqrt(n))/4);
    } else {
        xchunk = ychunk = std::max (int64_t(1), int64_t(std::sqrt(opt.maxthreads))/2);
    }

    auto task = [&](int id, int64_t xbegin, int64_t xend,
                    int64_t ybegin, int64_t yend) {
        f (ROI (xbegin, xend, ybegin, yend, roi.zbegin, roi.zend,
                roi.chbegin, roi.chend));
    };
    parallel_for_chunked_2D (roi.xbegin, roi.xend, xchunk,
                             roi.ybegin, roi.yend, ychunk, task, opt);
}


inline void
parallel_image (ROI roi, std::function<void(ROI)> f)
{
    parallel_image (roi, parallel_image_options(), f);
}



// DEPRECATED(1.8) -- eventually enable the OIIO_DEPRECATION
template <class Func>
// OIIO_DEPRECATED("switch to new parallel_image (1.8)")
void
parallel_image (Func f, ROI roi, int nthreads=0, SplitDir splitdir=Split_Y)
{
    parallel_image (roi, parallel_image_options (nthreads, splitdir), f);
}



/// Common preparation for IBA functions: Given an ROI (which may or may not
/// be the default ROI::All()), destination image (which may or may not yet
/// be allocated), and optional input images, adjust roi if necessary and
/// allocate pixels for dst if necessary.  If dst is already initialized, it
/// will keep its "full" (aka display) window, otherwise its full/display
/// window will be set to the union of A's and B's full/display windows.  If
/// dst is uninitialized and  force_spec is not NULL, use *force_spec as
/// dst's new spec rather than using A's.  Also, if A or B inputs are
/// specified but not initialized or broken, it's an error so return false.
/// If all is ok, return true.  Some additional checks and behaviors may be
/// specified by the 'prepflags', which is a bit field defined by
/// IBAprep_flags.
bool OIIO_API IBAprep (ROI &roi, ImageBuf *dst, const ImageBuf *A=NULL,
                       const ImageBuf *B=NULL, const ImageBuf *C=NULL,
                       ImageSpec *force_spec=NULL, int prepflags=0);
inline bool IBAprep (ROI &roi, ImageBuf *dst, const ImageBuf *A,
                     const ImageBuf *B, ImageSpec *force_spec,
                     int prepflags=0) {
    return IBAprep (roi, dst, A, B, NULL, force_spec, prepflags);
}
inline bool IBAprep (ROI &roi, ImageBuf *dst,
                     const ImageBuf *A, const ImageBuf *B, int prepflags) {
    return IBAprep (roi, dst, A, B, NULL, NULL, prepflags);
}
inline bool IBAprep (ROI &roi, ImageBuf *dst,
                     const ImageBuf *A, int prepflags) {
    return IBAprep (roi, dst, A, NULL, NULL, NULL, prepflags);
}

enum IBAprep_flags {
    IBAprep_DEFAULT = 0,
    IBAprep_REQUIRE_ALPHA = 1<<0,
    IBAprep_REQUIRE_Z = 1<<1,
    IBAprep_REQUIRE_SAME_NCHANNELS = 1<<2,
    IBAprep_NO_COPY_ROI_FULL = 1<<3,    // Don't copy the src's roi_full
    IBAprep_NO_SUPPORT_VOLUME = 1<<4,   // Don't know how to do volumes
    IBAprep_NO_COPY_METADATA = 1<<8,    // N.B. default copies all metadata
    IBAprep_COPY_ALL_METADATA = 1<<9,   // Even unsafe things
    IBAprep_CLAMP_MUTUAL_NCHANNELS = 1<<10, // Clamp roi.chend to min of inputs
    IBAprep_SUPPORT_DEEP = 1<<11,       // Operation allows deep images
    IBAprep_DEEP_MIXED = 1<<12,         // Allow deep & non-deep combinations
    IBAprep_DST_FLOAT_PIXELS = 1<<13,   // If dst is uninit, make it float
    IBAprep_MINIMIZE_NCHANNELS = 1<<14, // Multi-inputs get min(nchannels)
    IBAprep_REQUIRE_MATCHING_CHANNELS = 1<<15, // Channel names must match
    IBAprep_MERGE_METADATA = 1 << 16,   // Merge all inputs' metadata
};



/// Given data types a and b, return a type that is a best guess for one
/// that can handle both without any loss of range or precision.
TypeDesc::BASETYPE OIIO_API type_merge (TypeDesc::BASETYPE a, TypeDesc::BASETYPE b);

inline TypeDesc::BASETYPE
type_merge (TypeDesc::BASETYPE a, TypeDesc::BASETYPE b, TypeDesc::BASETYPE c)
{
    return type_merge (type_merge(a,b), c);
}

inline TypeDesc type_merge (TypeDesc a, TypeDesc b) {
    return type_merge (TypeDesc::BASETYPE(a.basetype), TypeDesc::BASETYPE(b.basetype));
}

inline TypeDesc type_merge (TypeDesc a, TypeDesc b, TypeDesc c)
{
    return type_merge (type_merge(a,b), c);
}



// Macro to call a type-specialzed version func<type>(R,...)
#define OIIO_DISPATCH_TYPES(ret,name,func,type,R,...)                   \
    switch (type.basetype) {                                            \
    case TypeDesc::FLOAT :                                              \
        ret = func<float> (R, __VA_ARGS__); break;                      \
    case TypeDesc::UINT8 :                                              \
        ret = func<unsigned char> (R, __VA_ARGS__); break;              \
    case TypeDesc::HALF  :                                              \
        ret = func<half> (R, __VA_ARGS__); break;                       \
    case TypeDesc::UINT16:                                              \
        ret = func<unsigned short> (R, __VA_ARGS__); break;             \
    case TypeDesc::INT8  :                                              \
        ret = func<char> (R, __VA_ARGS__); break;                       \
    case TypeDesc::INT16 :                                              \
        ret = func<short> (R, __VA_ARGS__); break;                      \
    case TypeDesc::UINT  :                                              \
        ret = func<unsigned int> (R, __VA_ARGS__); break;               \
    case TypeDesc::INT   :                                              \
        ret = func<int> (R, __VA_ARGS__); break;                        \
    case TypeDesc::DOUBLE:                                              \
        ret = func<double> (R, __VA_ARGS__); break;                     \
    default:                                                            \
        (R).errorf("%s: Unsupported pixel data format '%s'", name, type); \
        ret = false;                                                    \
    }

// Helper, do not call from the outside world.
#define OIIO_DISPATCH_TYPES2_HELP(ret,name,func,Rtype,Atype,R,...)      \
    switch (Atype.basetype) {                                           \
    case TypeDesc::FLOAT :                                              \
        ret = func<Rtype,float> (R, __VA_ARGS__); break;                \
    case TypeDesc::UINT8 :                                              \
        ret = func<Rtype,unsigned char> (R, __VA_ARGS__); break;        \
    case TypeDesc::HALF  :                                              \
        ret = func<Rtype,half> (R, __VA_ARGS__); break;                 \
    case TypeDesc::UINT16:                                              \
        ret = func<Rtype,unsigned short> (R, __VA_ARGS__); break;       \
    case TypeDesc::INT8 :                                               \
        ret = func<Rtype,char> (R, __VA_ARGS__); break;                 \
    case TypeDesc::INT16 :                                              \
        ret = func<Rtype,short> (R, __VA_ARGS__); break;                \
    case TypeDesc::UINT :                                               \
        ret = func<Rtype,unsigned int> (R, __VA_ARGS__); break;         \
    case TypeDesc::INT :                                                \
        ret = func<Rtype,int> (R, __VA_ARGS__); break;                  \
    case TypeDesc::DOUBLE :                                             \
        ret = func<Rtype,double> (R, __VA_ARGS__); break;               \
    default:                                                            \
        (R).errorf("%s: Unsupported pixel data format '%s'", name, Atype); \
        ret = false;                                                    \
    }

// Macro to call a type-specialzed version func<Rtype,Atype>(R,...).
#define OIIO_DISPATCH_TYPES2(ret,name,func,Rtype,Atype,R,...)           \
    switch (Rtype.basetype) {                                           \
    case TypeDesc::FLOAT :                                              \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,float,Atype,R,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::UINT8 :                                              \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,unsigned char,Atype,R,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::HALF  :                                              \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,half,Atype,R,__VA_ARGS__);  \
        break;                                                          \
    case TypeDesc::UINT16:                                              \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,unsigned short,Atype,R,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::INT8:                                                \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,char,Atype,R,__VA_ARGS__);  \
        break;                                                          \
    case TypeDesc::INT16:                                               \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,short,Atype,R,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::UINT:                                                \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,unsigned int,Atype,R,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::INT:                                                 \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,int,Atype,R,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::DOUBLE:                                              \
        OIIO_DISPATCH_TYPES2_HELP(ret,name,func,double,Atype,R,__VA_ARGS__);\
        break;                                                          \
    default:                                                            \
        (R).errorf("%s: Unsupported pixel data format '%s'", name, Rtype); \
        ret = false;                                                    \
    }


// Macro to call a type-specialzed version func<type>(R,...) for
// the most common types, will auto-convert the rest to float.
#define OIIO_DISPATCH_COMMON_TYPES(ret,name,func,type,R,...)            \
    switch (type.basetype) {                                            \
    case TypeDesc::FLOAT :                                              \
        ret = func<float> (R, __VA_ARGS__); break;                      \
    case TypeDesc::UINT8 :                                              \
        ret = func<unsigned char> (R, __VA_ARGS__); break;              \
    case TypeDesc::HALF  :                                              \
        ret = func<half> (R, __VA_ARGS__); break;                       \
    case TypeDesc::UINT16:                                              \
        ret = func<unsigned short> (R, __VA_ARGS__); break;             \
    default: {                                                          \
        /* other types: punt and convert to float, then copy back */    \
        ImageBuf Rtmp;                                                  \
        if ((R).initialized())                                          \
            Rtmp.copy (R, TypeDesc::FLOAT);                             \
        ret = func<float> (Rtmp, __VA_ARGS__);                          \
        if (ret)                                                        \
            (R).copy (Rtmp);                                            \
        else                                                            \
            (R).errorf("%s", Rtmp.geterror());                          \
        }                                                               \
    }

// Helper, do not call from the outside world.
#define OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,Rtype,Atype,R,A,...) \
    switch (Atype.basetype) {                                           \
    case TypeDesc::FLOAT :                                              \
        ret = func<Rtype,float> (R, A, __VA_ARGS__); break;             \
    case TypeDesc::UINT8 :                                              \
        ret = func<Rtype,unsigned char> (R, A, __VA_ARGS__); break;     \
    case TypeDesc::HALF  :                                              \
        ret = func<Rtype,half> (R, A, __VA_ARGS__); break;              \
    case TypeDesc::UINT16:                                              \
        ret = func<Rtype,unsigned short> (R, A, __VA_ARGS__); break;    \
    default: {                                                          \
        /* other types: punt and convert to float, then copy back */    \
        ImageBuf Atmp;                                                  \
        Atmp.copy (A, TypeDesc::FLOAT);                                 \
        ret = func<Rtype,float> (R, Atmp, __VA_ARGS__);                 \
        }                                                               \
    }

// Macro to call a type-specialzed version func<Rtype,Atype>(R,A,...) for
// the most common types, and even for uncommon types when src and dst types
// are identical. It will auto-convert remaining rare cases to float.
#define OIIO_DISPATCH_COMMON_TYPES2(ret,name,func,Rtype,Atype,R,A,...)  \
    if (Rtype == Atype) {                                               \
        /* All data types, when Rtype == Atype */                       \
        switch (Atype.basetype) {                                       \
        case TypeDesc::FLOAT :                                          \
            ret = func<float,float> (R, A, __VA_ARGS__); break;         \
        case TypeDesc::UINT8 :                                          \
            ret = func<uint8_t,uint8_t> (R, A, __VA_ARGS__); break;     \
        case TypeDesc::UINT16:                                          \
            ret = func<uint16_t,uint16_t> (R, A, __VA_ARGS__); break;   \
        case TypeDesc::HALF  :                                          \
            ret = func<half,half> (R, A, __VA_ARGS__); break;           \
        case TypeDesc::INT8 :                                           \
            ret = func<char,char> (R, A, __VA_ARGS__); break;           \
        case TypeDesc::INT16 :                                          \
            ret = func<short,short> (R, A, __VA_ARGS__); break;         \
        case TypeDesc::UINT :                                           \
            ret = func<uint32_t,uint32_t> (R, A, __VA_ARGS__); break;   \
        case TypeDesc::INT :                                            \
            ret = func<int,int> (R, A, __VA_ARGS__); break;             \
        case TypeDesc::DOUBLE :                                         \
            ret = func<double,double> (R, A, __VA_ARGS__); break;       \
        default:                                                        \
            (R).errorf("%s: Unsupported pixel data format '%s'", name, Atype); \
            ret = false;                                                \
        }                                                               \
    } else {                                                            \
        /* When Rtype != Atype, handle the common pairs */              \
        switch (Rtype.basetype) {                                       \
        case TypeDesc::FLOAT :                                          \
            OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,float,Atype,R,A,__VA_ARGS__); \
            break;                                                      \
        case TypeDesc::UINT8 :                                          \
            OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,uint8_t,Atype,R,A,__VA_ARGS__); \
            break;                                                      \
        case TypeDesc::HALF  :                                          \
            OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,half,Atype,R,A,__VA_ARGS__); \
            break;                                                      \
        case TypeDesc::UINT16:                                          \
            OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,uint16_t,Atype,R,A,__VA_ARGS__); \
            break;                                                      \
        default: {                                                      \
            /* other combinationd: convert to float, then copy back */  \
            ImageBuf Rtmp;                                              \
            if ((R).initialized())                                      \
                Rtmp.copy (R, TypeDesc::FLOAT);                         \
            OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,float,Atype,Rtmp,A,__VA_ARGS__); \
            if (ret)                                                    \
                (R).copy (Rtmp);                                        \
            else                                                        \
                (R).errorf("%s", Rtmp.geterror());                      \
            }                                                           \
        }                                                               \
    }


// Macro to call a type-specialzed version func<Rtype,Atype>(R,A,...) for
// the most common types. It will auto-convert other cases to/from float.
// This is the case for when we don't actually write to the read-only R image.
#define OIIO_DISPATCH_COMMON_TYPES2_CONST(ret,name,func,Rtype,Atype,R,A,...)  \
    switch (Rtype.basetype) {                                           \
    case TypeDesc::FLOAT :                                              \
        OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,float,Atype,R,A,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::UINT8 :                                              \
        OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,unsigned char,Atype,R,A,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::HALF  :                                              \
        OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,half,Atype,R,A,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::UINT16:                                              \
        OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,unsigned short,Atype,R,A,__VA_ARGS__); \
        break;                                                          \
    default: {                                                          \
        /* other types: punt and convert to float, then copy back */    \
        ImageBuf Rtmp;                                                  \
        if ((R).initialized())                                          \
            Rtmp.copy (R, TypeDesc::FLOAT);                             \
        OIIO_DISPATCH_COMMON_TYPES2_HELP(ret,name,func,float,Atype,Rtmp,A,__VA_ARGS__); \
    } }


// Helper, do not call from the outside world.
#define OIIO_DISPATCH_COMMON_TYPES3_HELP2(ret,name,func,Rtype,Atype,Btype,R,A,B,...) \
    switch (Rtype.basetype) {                                           \
    case TypeDesc::FLOAT :                                              \
        ret = func<float,Atype,Btype> (R,A,B,__VA_ARGS__); break;       \
    case TypeDesc::UINT8 :                                              \
        ret = func<unsigned char,Atype,Btype> (R,A,B,__VA_ARGS__); break;  \
    case TypeDesc::HALF  :                                              \
        ret = func<half,Atype,Btype> (R,A,B,__VA_ARGS__); break;        \
    case TypeDesc::UINT16:                                              \
        ret = func<unsigned short,Atype,Btype> (R,A,B,__VA_ARGS__); break;  \
    default: {                                                          \
        /* other types: punt and convert to float, then copy back */    \
        ImageBuf Rtmp;                                                  \
        if ((R).initialized())                                          \
            Rtmp.copy (R, TypeDesc::FLOAT);                             \
        ret = func<float,Atype,Btype> (R,A,B,__VA_ARGS__);              \
        if (ret)                                                        \
            (R).copy (Rtmp);                                            \
        else                                                            \
            (R).errorf("%s", Rtmp.geterror());                          \
        }                                                               \
    }

// Helper, do not call from the outside world.
#define OIIO_DISPATCH_COMMON_TYPES3_HELP(ret,name,func,Rtype,Atype,Btype,R,A,B,...) \
    switch (Btype.basetype) {                                           \
    case TypeDesc::FLOAT :                                              \
        OIIO_DISPATCH_COMMON_TYPES3_HELP2(ret,name,func,Rtype,Atype,float,R,A,B,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::UINT8 :                                              \
        OIIO_DISPATCH_COMMON_TYPES3_HELP2(ret,name,func,Rtype,Atype,unsigned char,R,A,B,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::HALF :                                               \
        OIIO_DISPATCH_COMMON_TYPES3_HELP2(ret,name,func,Rtype,Atype,half,R,A,B,__VA_ARGS__); \
        break;                                                          \
    case TypeDesc::UINT16 :                                             \
        OIIO_DISPATCH_COMMON_TYPES3_HELP2(ret,name,func,Rtype,Atype,unsigned short,R,A,B,__VA_ARGS__); \
        break;                                                          \
    default: {                                                          \
        /* other types: punt and convert to float */                    \
        ImageBuf Btmp;                                                  \
        Btmp.copy (B, TypeDesc::FLOAT);                                 \
        OIIO_DISPATCH_COMMON_TYPES3_HELP2(ret,name,func,Rtype,Atype,float,R,A,Btmp,__VA_ARGS__); \
        }                                                               \
    }

// Macro to call a type-specialzed version func<Rtype,Atype,Btype>(R,A,B,...)
// the most common types, and for all types when all three images have
// the same type. Remaining rare cases will auto-convert to float.
#define OIIO_DISPATCH_COMMON_TYPES3(ret,name,func,Rtype,Atype,Btype,R,A,B,...)  \
    if (Rtype == Atype && Rtype == Btype) {                             \
        /* All data types, when Rtype == Atype */                       \
        switch (Atype.basetype) {                                       \
        case TypeDesc::FLOAT :                                          \
            ret = func<float,float,float> (R, A, B, __VA_ARGS__); break;\
        case TypeDesc::UINT8 :                                          \
            ret = func<uint8_t,uint8_t,uint8_t> (R, A, B, __VA_ARGS__); break; \
        case TypeDesc::UINT16:                                          \
            ret = func<uint16_t,uint16_t,uint16_t> (R, A, B, __VA_ARGS__); break; \
        case TypeDesc::HALF  :                                          \
            ret = func<half,half,half> (R, A, B, __VA_ARGS__); break;   \
        case TypeDesc::INT8 :                                           \
            ret = func<char,char,char> (R, A, B, __VA_ARGS__); break;   \
        case TypeDesc::INT16 :                                          \
            ret = func<int16_t,int16_t,int16_t> (R, A, B, __VA_ARGS__); break; \
        case TypeDesc::UINT :                                           \
            ret = func<uint32_t,uint32_t,uint32_t> (R, A, B, __VA_ARGS__); break; \
        case TypeDesc::INT :                                            \
            ret = func<int,int,int> (R, A, B, __VA_ARGS__); break;      \
        case TypeDesc::DOUBLE :                                         \
            ret = func<double,double,double> (R, A, B, __VA_ARGS__); break; \
        default:                                                        \
            (R).errorf("%s: Unsupported pixel data format '%s'", name, Atype); \
            ret = false;                                                \
        }                                                               \
    } else {                                                            \
        switch (Atype.basetype) {                                       \
        case TypeDesc::FLOAT :                                          \
            OIIO_DISPATCH_COMMON_TYPES3_HELP(ret,name,func,Rtype,float,Btype,R,A,B,__VA_ARGS__); \
            break;                                                      \
        case TypeDesc::UINT8 :                                          \
            OIIO_DISPATCH_COMMON_TYPES3_HELP(ret,name,func,Rtype,unsigned char,Btype,R,A,B,__VA_ARGS__); \
            break;                                                      \
        case TypeDesc::HALF  :                                          \
            OIIO_DISPATCH_COMMON_TYPES3_HELP(ret,name,func,Rtype,half,Btype,R,A,B,__VA_ARGS__); \
            break;                                                      \
        case TypeDesc::UINT16:                                          \
            OIIO_DISPATCH_COMMON_TYPES3_HELP(ret,name,func,Rtype,unsigned short,Btype,R,A,B,__VA_ARGS__); \
            break;                                                      \
        default:                                                        \
            /* other types: punt and convert to float */                \
            ImageBuf Atmp;                                              \
            Atmp.copy (A, TypeDesc::FLOAT);                             \
            OIIO_DISPATCH_COMMON_TYPES3_HELP(ret,name,func,Rtype,float,Btype,R,Atmp,B,__VA_ARGS__); \
        }                                                               \
    }


// Utility: for span av, if it had fewer elements than len, alloca a new
// copy that's the right length. Use the `missing` value for missing entries
// (one or more supplied, but not all), and `zdef` default to use if there
// were no entries at all. This is used in many IBA functions that take
// constant per-channel values.
#define IBA_FIX_PERCHAN_LEN(av,len,missing,zdef)                        \
    if (av.size() < len) {                                              \
        int nc = len;                                                   \
        float *vals = OIIO_ALLOCA(float, nc);                           \
        for (int i = 0;  i < nc;  ++i)                                  \
            vals[i] = i < av.size() ? av[i] : (i ? vals[i-1] : zdef);   \
        av = cspan<float>(vals, nc);                                    \
    }

// Default IBA_FIX_PERCHAN_LEN, with zdef=0.0 and missing = the last value
// that was supplied.
#define IBA_FIX_PERCHAN_LEN_DEF(av,len)                                 \
    IBA_FIX_PERCHAN_LEN (av, len, 0.0f, av.size() ? av.back() : 0.0f);


}  // end namespace ImageBufAlgo


OIIO_NAMESPACE_END
