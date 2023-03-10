// Copyright (c) 2022, NVIDIA CORPORATION. All rights reserved.
//
// NVIDIA CORPORATION and its licensors retain all intellectual property
// and proprietary rights in and to this software, related documentation
// and any modifications thereto. Any use, reproduction, disclosure or
// distribution of this software and related documentation without an express
// license agreement from NVIDIA CORPORATION is strictly prohibited.
//
// --------- Warning: This is a build system generated file. ----------
//

//! @file
//!
//! @brief This file was generated by <i>omni.bind</i>.

#include <omni/core/OmniAttr.h>
#include <omni/core/Interface.h>
#include <omni/core/ResultError.h>

#include <functional>
#include <utility>
#include <type_traits>

#ifndef OMNI_BIND_INCLUDE_INTERFACE_IMPL


/**
 * Interface for providing a CPU affinity mask to the plugin. Instances of this interface can be thought of as an array
 * of \c MaskType values, which allows for setting affinities on machines with more than 64 processors. Each affinity
 * mask this object contains is a bitmask that represents the associated CPUs.
 *
 * On Linux, this object is treated as one large bitset analogous to cpu_set_t. So \c get_affinity_mask(0) represents
 * CPUs 0-63, \c get_affinity_mask(1) represents CPUs 64-127, etc.
 *
 * On Windows, each affinity mask in this object applies to its own Processor Group, so \c get_affinity_mask(0) is for
 * Processor Group 0, \c get_affinity_mask(1) for Processor Group 1, etc.
 */
template <>
class omni::core::Generated<omni::experimental::job::IAffinityMask_abi> : public omni::experimental::job::IAffinityMask_abi
{
public:
    OMNI_PLUGIN_INTERFACE("omni::experimental::job::IAffinityMask")

    /**
     * Gets the affinity mask at \c index.
     *
     * @note \c index must be less than \ref get_mask_count_abi()
     *
     * @param index Index to get affinity mask for.
     *
     * @return The affinity mask at the provided index.
     */
    omni::experimental::job::MaskType get_affinity_mask(size_t index) noexcept;

    /**
     * Gets the affinity \c mask at \c index.
     *
     * @note \c index must be less than \ref get_mask_count_abi()
     *
     * @param index Index to set affinity mask for.
     * @param mask Mask to set.
     */
    void set_affinity_mask(size_t index, omni::experimental::job::MaskType mask) noexcept;

    /**
     * Gets the current number of affinity masks stored by this object.
     *
     * @return The current number of affinity masks stored by this object.
     */
    size_t get_mask_count() noexcept;

    /**
     * Gets the default number of affinity masks stored by this object.
     *
     * @return The default number of affinity masks stored by this object.
     */
    size_t get_default_mask_count() noexcept;

    /**
     * Sets the number of affinity masks stored by this object to \c count.
     *
     * If \c count is greater than the current size, the appended affinity masks will bet set to \c 0. If \c count
     * is less than the current size, then this object will only contain the first \c count elements after this call.
     *
     * @param count Number of affinity masks to set the size to.
     */
    void set_mask_count(size_t count) noexcept;
};

/**
 * Basic interface for launching jobs on a foreign job system.
 */
template <>
class omni::core::Generated<omni::experimental::job::IJob_abi> : public omni::experimental::job::IJob_abi
{
public:
    OMNI_PLUGIN_INTERFACE("omni::experimental::job::IJob")

    /**
     * Adds a new job to be executed.
     *
     * @param job_fn User provided function to be executed by a worker.
     * @param job_data User provided data for the job, the memory must not be released until it no longer needed by the
     * task.
     */
    void enqueue_job(omni::experimental::job::JobFunction job_fn, void* job_data) noexcept;
};

/**
 * Interface for managing the number of workers in the job system.
 */
template <>
class omni::core::Generated<omni::experimental::job::IJobWorker_abi> : public omni::experimental::job::IJobWorker_abi
{
public:
    OMNI_PLUGIN_INTERFACE("omni::experimental::job::IJobWorker")

    /**
     * Returns default number of workers used for creation of a new Job system.
     *
     * @return The default number of workers.
     */
    size_t get_default_worker_count() noexcept;

    /**
     * Returns the number of worker threads in the job system.
     *
     * @returns The number of worker threads.
     */
    size_t get_worker_count() noexcept;

    /**
     * Sets the number of workers in the job system.
     *
     * This function may stop all current threads and reset any previously set thread affinity.
     *
     * @param count The new number of workers to set in the system. A value of 0 means to use the default value returned
     * by getDefaultWorkerCount()
     */
    void set_worker_count(size_t count) noexcept;
};

/**
 * Interface for setting CPU affinity for the job system.
 */
template <>
class omni::core::Generated<omni::experimental::job::IJobAffinity_abi> : public omni::experimental::job::IJobAffinity_abi
{
public:
    OMNI_PLUGIN_INTERFACE("omni::experimental::job::IJobAffinity")

    /**
     * Gets the current affinity of a worker.
     *
     * @param worker_id The worker to set the affinity of. If this id is larger than the current number of workers,
     * a \c nullptr will be returned.
     *
     * @return The current affinity being used by the worker. The returned value may be \c nullptr if the worker's
     * affinity could not be determined.
     */
    omni::core::ObjectPtr<omni::experimental::job::IAffinityMask> get_affinity(size_t worker_id) noexcept;

    /**
     * Attempts to set the affinity for the specified worker.
     *
     * @note  On Windows each thread can only belong to a single Processor Group, so the CPU Affinity will only be set
     * to the first non-zero entry. That is to say, if both \c mask->get_affinity_mask(0) and
     * \c mask->get_affinity_mask(1) both have bits sets, only the CPUs in \c mask->get_affinity_mask(0) will be set for
     * the affinity.
     *
     * @param worker_id The worker to set the affinity of. If this id is larger than the current number of workers,
     * false will be returned.
     * @param mask The affinity values to set.
     *
     * @return true if the affinity was successfully set, false otherwise.
     */
    bool set_affinity(size_t worker_id, omni::core::ObjectParam<omni::experimental::job::IAffinityMask> mask) noexcept;
};

#endif

#ifndef OMNI_BIND_INCLUDE_INTERFACE_DECL

inline omni::experimental::job::MaskType omni::core::Generated<omni::experimental::job::IAffinityMask_abi>::get_affinity_mask(
    size_t index) noexcept
{
    return get_affinity_mask_abi(index);
}

inline void omni::core::Generated<omni::experimental::job::IAffinityMask_abi>::set_affinity_mask(
    size_t index, omni::experimental::job::MaskType mask) noexcept
{
    set_affinity_mask_abi(index, mask);
}

inline size_t omni::core::Generated<omni::experimental::job::IAffinityMask_abi>::get_mask_count() noexcept
{
    return get_mask_count_abi();
}

inline size_t omni::core::Generated<omni::experimental::job::IAffinityMask_abi>::get_default_mask_count() noexcept
{
    return get_default_mask_count_abi();
}

inline void omni::core::Generated<omni::experimental::job::IAffinityMask_abi>::set_mask_count(size_t count) noexcept
{
    set_mask_count_abi(count);
}

inline void omni::core::Generated<omni::experimental::job::IJob_abi>::enqueue_job(
    omni::experimental::job::JobFunction job_fn, void* job_data) noexcept
{
    enqueue_job_abi(job_fn, job_data);
}

inline size_t omni::core::Generated<omni::experimental::job::IJobWorker_abi>::get_default_worker_count() noexcept
{
    return get_default_worker_count_abi();
}

inline size_t omni::core::Generated<omni::experimental::job::IJobWorker_abi>::get_worker_count() noexcept
{
    return get_worker_count_abi();
}

inline void omni::core::Generated<omni::experimental::job::IJobWorker_abi>::set_worker_count(size_t count) noexcept
{
    set_worker_count_abi(count);
}

inline omni::core::ObjectPtr<omni::experimental::job::IAffinityMask> omni::core::Generated<
    omni::experimental::job::IJobAffinity_abi>::get_affinity(size_t worker_id) noexcept
{
    return omni::core::steal(get_affinity_abi(worker_id));
}

inline bool omni::core::Generated<omni::experimental::job::IJobAffinity_abi>::set_affinity(
    size_t worker_id, omni::core::ObjectParam<omni::experimental::job::IAffinityMask> mask) noexcept
{
    return set_affinity_abi(worker_id, mask.get());
}

#endif

#undef OMNI_BIND_INCLUDE_INTERFACE_DECL
#undef OMNI_BIND_INCLUDE_INTERFACE_IMPL
