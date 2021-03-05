// Copyright 2008-present Contributors to the OpenImageIO project.
// SPDX-License-Identifier: BSD-3-Clause
// https://github.com/OpenImageIO/oiio/blob/master/LICENSE.md


#pragma once

#include <OpenImageIO/export.h>
#include <OpenImageIO/oiioversion.h>
#include <OpenImageIO/span.h>
#include <OpenImageIO/string_view.h>

OIIO_NAMESPACE_BEGIN


struct TypeDesc;
class ImageSpec;



/// A `DeepData` holds the contents of an image of ``deep'' pixels (multiple
/// depth samples per pixel).

class OIIO_API DeepData {
public:
    /// Construct an empty DeepData.
    DeepData();

    /// Construct and init from an ImageSpec.
    DeepData(const ImageSpec& spec);

    /// Copy constructor
    DeepData(const DeepData& d);

    ~DeepData();

    /// Copy assignment
    const DeepData& operator=(const DeepData& d);

    /// Reset the `DeepData` to be equivalent to its empty initial state.
    void clear();
    /// In addition to performing the tasks of `clear()`, also ensure that
    /// all allocated memory has been truly freed.
    void free();

    /// Initialize the `DeepData` with the specified number of pixels,
    /// channels, channel types, and channel names, and allocate memory for
    /// all the data.
    void init(int64_t npix, int nchan, cspan<TypeDesc> channeltypes,
              cspan<std::string> channelnames);

    /// Initialize the `DeepData` based on the `ImageSpec`'s total number of
    /// pixels, number and types of channels. At this stage, all pixels are
    /// assumed to have 0 samples, and no sample data is allocated.
    void init(const ImageSpec& spec);

    /// Is the DeepData initialized?
    bool initialized() const;

    /// Has the DeepData fully allocated? If no, it is still very
    /// inexpensive to call set_capacity().
    bool allocated() const;

    /// Retrieve the total number of pixels.
    int64_t pixels() const;
    /// Retrieve the number of channels.
    int channels() const;

    // Retrieve the index of the Z channel.
    int Z_channel() const;
    // Retrieve the index of the Zback channel, which will return the
    // Z channel if no Zback exists.
    int Zback_channel() const;
    // Retrieve the index of the A (alpha) channel.
    int A_channel() const;
    // Retrieve the index of the AR channel. If it does not exist, the A
    // channel (which always exists) will be returned.
    int AR_channel() const;
    // Retrieve the index of the AG channel. If it does not exist, the A
    // channel (which always exists) will be returned.
    int AG_channel() const;
    // Retrieve the index of the AB channel. If it does not exist, the A
    // channel (which always exists) will be returned.
    int AB_channel() const;

    /// Return the name of channel c.
    string_view channelname(int c) const;

    /// Retrieve the data type of channel `c`.
    TypeDesc channeltype(int c) const;
    /// Return the size (in bytes) of one sample dadum of channel `c`.
    size_t channelsize(int c) const;
    /// Return the size (in bytes) for all channels of one sample.
    size_t samplesize() const;

    /// Retrieve the number of samples for the given pixel index.
    int samples(int64_t pixel) const;

    /// Set the number of samples for the given pixel. This must be called
    /// after init().
    void set_samples(int64_t pixel, int samps);

    /// Set the number of samples for all pixels. The samples.size() is
    /// required to match pixels().
    void set_all_samples(cspan<unsigned int> samples);

    /// Set the capacity of samples for the given pixel. This must be called
    /// after init().
    void set_capacity(int64_t pixel, int samps);

    /// Retrieve the capacity (number of allocated samples) for the given
    /// pixel index.
    int capacity(int64_t pixel) const;

    /// Insert `n` samples of the specified pixel, betinning at the sample
    /// position index. After insertion, the new samples will have
    /// uninitialized values.
    void insert_samples(int64_t pixel, int samplepos, int n = 1);

    /// Erase `n` samples of the specified pixel, betinning at the sample
    /// position index.
    void erase_samples(int64_t pixel, int samplepos, int n = 1);

    /// Retrieve the value of the given pixel, channel, and sample index,
    /// cast to a `float`.
    float deep_value(int64_t pixel, int channel, int sample) const;
    /// Retrieve the value of the given pixel, channel, and sample index,
    /// cast to a `uint32`.
    uint32_t deep_value_uint(int64_t pixel, int channel, int sample) const;

    /// Set the value of the given pixel, channel, and sample index, for
    /// floating-point channels.
    void set_deep_value(int64_t pixel, int channel, int sample, float value);

    /// Set the value of the given pixel, channel, and sample index, for
    /// integer channels.
    void set_deep_value(int64_t pixel, int channel, int sample, uint32_t value);

    /// Retrieve the pointer to a given pixel/channel/sample, or NULL if
    /// there are no samples for that pixel. Use with care, and note that
    /// calls to insert_samples and erase_samples can invalidate pointers
    /// returend by prior calls to data_ptr.
    void* data_ptr(int64_t pixel, int channel, int sample);
    const void* data_ptr(int64_t pixel, int channel, int sample) const;

    cspan<TypeDesc> all_channeltypes() const;
    cspan<unsigned int> all_samples() const;
    cspan<char> all_data() const;

    /// Fill in the vector with pointers to the start of the first
    /// channel for each pixel.
    void get_pointers(std::vector<void*>& pointers) const;

    /// Copy a deep sample from `src` to this `DeepData`. They must have the
    /// same channel layout. Return `true` if ok, `false` if the operation
    /// could not be performed.
    bool copy_deep_sample(int64_t pixel, int sample, const DeepData& src,
                          int64_t srcpixel, int srcsample);

    /// Copy an entire deep pixel from `src` to this `DeepData`, completely
    /// eplacing any pixel data for that pixel. They must have the same
    /// channel ayout. Return `true` if ok, `false` if the operation could
    /// not be erformed.
    bool copy_deep_pixel(int64_t pixel, const DeepData& src, int64_t srcpixel);

    /// Split all samples of that pixel at the given depth zsplit. Samples
    /// that span z (i.e. z < zsplit < zback) will be split into two samples
    /// with depth ranges [z,zsplit] and [zsplit,zback] with appropriate
    /// changes to their color and alpha values. Samples not spanning zsplit
    /// will remain intact. This operation will have no effect if there are
    /// not Z and Zback channels present. Return true if any splits
    /// occurred, false if the pixel was not modified.
    bool split(int64_t pixel, float depth);

    /// Sort the samples of the pixel by their `Z` depth.
    void sort(int64_t pixel);

    /// Merge any adjacent samples in the pixel that exactly overlap in z
    /// range. This is only useful if the pixel has previously been split at
    /// all sample starts and ends, and sorted by Z.  Note that this may
    /// change the number of samples in the pixel.
    void merge_overlaps(int64_t pixel);

    /// Merge the samples of `src`'s pixel into this `DeepData`'s pixel.
    /// Return `true` if ok, `false` if the operation could not be
    /// performed.
    void merge_deep_pixels(int64_t pixel, const DeepData& src, int srcpixel);

    /// Return the z depth at which the pixel reaches full opacity.
    float opaque_z(int64_t pixel) const;

    /// Remvove any samples hidden behind opaque samples.
    void occlusion_cull(int64_t pixel);

private:
    class Impl;
    Impl* m_impl;  // holds all the nontrivial stuff
    int64_t m_npixels;
    int m_nchannels;
};


OIIO_NAMESPACE_END
