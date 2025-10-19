#include "batch_geocoder.h"
#include "enhanced_geocoding.h"  // For type definitions
#include <thread>
#include <chrono>

namespace geocoding {
namespace batch {

BatchGeocoder::BatchGeocoder(GeocodingFunction geocode_func)
    : m_geocode_func(std::move(geocode_func))
    , m_cancel_requested(false)
    , m_is_processing(false) {
}

BatchGeocodingResult BatchGeocoder::processBatch(const BatchGeocodingRequest& request) {
    if (request.parallel_processing) {
        return processBatchParallel(request.addresses, 
                                    request.max_threads,
                                    request.progress_callback);
    } else {
        return processBatchSequential(request.addresses,
                                      request.progress_callback);
    }
}

BatchGeocodingResult BatchGeocoder::processBatchParallel(
    const std::vector<nav::geocoding::EnhancedAddressRequest>& addresses,
    size_t num_threads,
    ProgressCallback progress_callback) {
    
    m_is_processing = true;
    m_cancel_requested = false;
    
    auto start_time = std::chrono::steady_clock::now();
    
    BatchGeocodingResult batch_result;
    batch_result.total_count = addresses.size();
    batch_result.results.resize(addresses.size());
    
    if (addresses.empty()) {
        m_is_processing = false;
        return batch_result;
    }
    
    // Determine actual thread count
    size_t actual_threads = std::min(num_threads, addresses.size());
    actual_threads = std::max<size_t>(1, actual_threads);
    
    // Divide work among threads
    size_t chunk_size = addresses.size() / actual_threads;
    size_t remainder = addresses.size() % actual_threads;
    
    std::vector<std::thread> threads;
    std::atomic<size_t> completed_count(0);
    
    size_t start_idx = 0;
    for (size_t i = 0; i < actual_threads; ++i) {
        size_t end_idx = start_idx + chunk_size + (i < remainder ? 1 : 0);
        
        threads.emplace_back(&BatchGeocoder::workerThread, this,
                            std::cref(addresses),
                            std::ref(batch_result.results),
                            start_idx,
                            end_idx,
                            std::ref(completed_count),
                            progress_callback);
        
        start_idx = end_idx;
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    // Calculate statistics
    for (const auto& result : batch_result.results) {
        if (result.success) {
            ++batch_result.success_count;
        } else {
            ++batch_result.failed_count;
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    batch_result.elapsed_seconds = elapsed.count() / 1000.0;
    
    if (batch_result.elapsed_seconds > 0) {
        batch_result.throughput_per_second = 
            batch_result.total_count / batch_result.elapsed_seconds;
    }
    
    m_is_processing = false;
    return batch_result;
}

BatchGeocodingResult BatchGeocoder::processBatchSequential(
    const std::vector<nav::geocoding::EnhancedAddressRequest>& addresses,
    ProgressCallback progress_callback) {
    
    m_is_processing = true;
    m_cancel_requested = false;
    
    auto start_time = std::chrono::steady_clock::now();
    
    BatchGeocodingResult batch_result;
    batch_result.total_count = addresses.size();
    batch_result.results.reserve(addresses.size());
    
    for (size_t i = 0; i < addresses.size(); ++i) {
        if (m_cancel_requested) {
            break;
        }
        
        // Geocode single address
        nav::geocoding::EnhancedGeocodingResult result = m_geocode_func(addresses[i]);
        batch_result.results.push_back(result);
        
        if (result.success) {
            ++batch_result.success_count;
        } else {
            ++batch_result.failed_count;
        }
        
        // Report progress
        if (progress_callback) {
            progress_callback(i + 1, addresses.size());
        }
    }
    
    auto end_time = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    batch_result.elapsed_seconds = elapsed.count() / 1000.0;
    
    if (batch_result.elapsed_seconds > 0) {
        batch_result.throughput_per_second = 
            batch_result.total_count / batch_result.elapsed_seconds;
    }
    
    m_is_processing = false;
    return batch_result;
}

void BatchGeocoder::cancel() {
    m_cancel_requested = true;
}

bool BatchGeocoder::isProcessing() const {
    return m_is_processing;
}

void BatchGeocoder::workerThread(
    const std::vector<nav::geocoding::EnhancedAddressRequest>& addresses,
    std::vector<nav::geocoding::EnhancedGeocodingResult>& results,
    size_t start_idx,
    size_t end_idx,
    std::atomic<size_t>& completed_count,
    ProgressCallback progress_callback) {
    
    for (size_t i = start_idx; i < end_idx; ++i) {
        if (m_cancel_requested) {
            break;
        }
        
        // Geocode single address
        results[i] = m_geocode_func(addresses[i]);
        
        // Update progress
        size_t completed = ++completed_count;
        if (progress_callback) {
            progress_callback(completed, addresses.size());
        }
    }
}

}} // namespace geocoding::batch
