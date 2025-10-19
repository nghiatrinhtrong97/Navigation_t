#pragma once
#include <vector>
#include <functional>
#include <future>
#include <atomic>

// Forward declarations
namespace nav {
namespace geocoding {
    struct EnhancedAddressRequest;
    struct EnhancedGeocodingResult;
}
}

namespace geocoding {
namespace batch {

/**
 * @brief Progress callback for batch geocoding
 */
using ProgressCallback = std::function<void(size_t completed, size_t total)>;

/**
 * @brief Batch geocoding request
 */
struct BatchGeocodingRequest {
    std::vector<nav::geocoding::EnhancedAddressRequest> addresses;
    bool parallel_processing = true;
    size_t max_threads = 4;
    ProgressCallback progress_callback;
};

/**
 * @brief Batch geocoding result
 */
struct BatchGeocodingResult {
    std::vector<nav::geocoding::EnhancedGeocodingResult> results;
    size_t total_count = 0;
    size_t success_count = 0;
    size_t failed_count = 0;
    double elapsed_seconds = 0.0;
    double throughput_per_second = 0.0;
};

/**
 * @brief High-performance batch geocoding engine
 * Supports parallel processing and progress tracking
 */
class BatchGeocoder {
public:
    /**
     * @brief Constructor
     * @param geocode_func Single address geocoding function
     */
    using GeocodingFunction = std::function<nav::geocoding::EnhancedGeocodingResult(const nav::geocoding::EnhancedAddressRequest&)>;
    
    explicit BatchGeocoder(GeocodingFunction geocode_func);

    /**
     * @brief Process batch geocoding request
     * @param request Batch request with addresses
     * @return Batch results with statistics
     */
    BatchGeocodingResult processBatch(const BatchGeocodingRequest& request);

    /**
     * @brief Process batch with manual parallelism control
     * @param addresses List of addresses to geocode
     * @param num_threads Number of worker threads
     * @param progress_callback Optional progress callback
     * @return Batch results
     */
    BatchGeocodingResult processBatchParallel(
        const std::vector<nav::geocoding::EnhancedAddressRequest>& addresses,
        size_t num_threads = 4,
        ProgressCallback progress_callback = nullptr);

    /**
     * @brief Process batch sequentially (single-threaded)
     * @param addresses List of addresses to geocode
     * @param progress_callback Optional progress callback
     * @return Batch results
     */
    BatchGeocodingResult processBatchSequential(
        const std::vector<nav::geocoding::EnhancedAddressRequest>& addresses,
        ProgressCallback progress_callback = nullptr);

    /**
     * @brief Cancel ongoing batch processing
     */
    void cancel();

    /**
     * @brief Check if batch processing is in progress
     * @return True if processing
     */
    bool isProcessing() const;

private:
    GeocodingFunction m_geocode_func;
    std::atomic<bool> m_cancel_requested;
    std::atomic<bool> m_is_processing;

    // Worker thread function
    void workerThread(const std::vector<nav::geocoding::EnhancedAddressRequest>& addresses,
                     std::vector<nav::geocoding::EnhancedGeocodingResult>& results,
                     size_t start_idx,
                     size_t end_idx,
                     std::atomic<size_t>& completed_count,
                     ProgressCallback progress_callback);
};

}} // namespace geocoding::batch
