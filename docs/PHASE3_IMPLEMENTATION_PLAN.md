# PHASE 3 IMPLEMENTATION PLAN: ML & Advanced Features

**Project:** Automotive Navigation System - Geocoder Modernization  
**Phase:** 3 (Weeks 9-12)  
**Status:** 📋 PLANNING - Ready for Implementation  
**Dependencies:** Phase 1 ✅ Complete, Phase 2 🔄 (reverted, will re-implement)

---

## 🎯 PHASE 3 OBJECTIVES

### **Core Goals:**
1. **ML-Based Address Similarity** - Deep learning for semantic matching
2. **Reverse Geocoding** - Lat/lon → structured address
3. **Address Autocomplete** - Real-time predictive search

### **Success Metrics:**
- ✅ ML model accuracy: >90% for address matching
- ✅ Reverse geocoding latency: <10ms
- ✅ Autocomplete response time: <50ms
- ✅ Autocomplete relevance: Top-3 accuracy >85%

---

## 📦 WEEK 9: ML-BASED ADDRESS SIMILARITY

### **Feature 1: TensorFlow Lite Integration**

**Files to Create:**
```
hmi/services/geocoding/ml/
├── include/
│   ├── ml_address_matcher.h       # ML-based matching interface
│   ├── tflite_model_loader.h      # TFLite model management
│   └── embedding_generator.h       # Address → vector embeddings
└── src/
    ├── ml_address_matcher.cpp
    ├── tflite_model_loader.cpp
    └── embedding_generator.cpp

models/
├── address_similarity_model.tflite  # Pre-trained model
└── training/
    ├── train_address_model.py       # Training script
    ├── prepare_dataset.py            # Data preprocessing
    └── requirements.txt              # Python dependencies
```

**Key Components:**

**1. TFLite Model Loader (C++)**
```cpp
// hmi/services/geocoding/ml/include/tflite_model_loader.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/model.h"

namespace geocoding {
namespace ml {

class TFLiteModelLoader {
public:
    explicit TFLiteModelLoader(const std::string& model_path);
    ~TFLiteModelLoader();
    
    // Model loading
    bool load();
    bool isLoaded() const { return m_loaded; }
    
    // Inference
    std::vector<float> predict(const std::vector<float>& input);
    
    // Model info
    std::vector<int> getInputShape() const;
    std::vector<int> getOutputShape() const;
    
private:
    std::string m_model_path;
    std::unique_ptr<tflite::FlatBufferModel> m_model;
    std::unique_ptr<tflite::Interpreter> m_interpreter;
    bool m_loaded;
    
    void allocateTensors();
};

}} // namespace geocoding::ml
```

**2. Address Embedding Generator**
```cpp
// hmi/services/geocoding/ml/include/embedding_generator.h
#pragma once
#include "tflite_model_loader.h"
#include "address_components.h"
#include <vector>
#include <string>

namespace geocoding {
namespace ml {

struct AddressEmbedding {
    std::vector<float> vector;  // 128-dim embedding
    float norm;                  // L2 norm for quick comparison
};

class EmbeddingGenerator {
public:
    explicit EmbeddingGenerator(const std::string& model_path);
    
    // Generate embeddings
    AddressEmbedding generate(const AddressComponents& address);
    AddressEmbedding generate(const std::string& freeform_address);
    
    // Similarity computation
    double cosineSimilarity(const AddressEmbedding& a, const AddressEmbedding& b);
    double euclideanDistance(const AddressEmbedding& a, const AddressEmbedding& b);
    
private:
    std::unique_ptr<TFLiteModelLoader> m_model;
    
    // Text preprocessing
    std::vector<float> tokenizeAndEncode(const std::string& text);
    std::vector<int> tokenize(const std::string& text);
    std::vector<float> encodeTokens(const std::vector<int>& tokens);
};

}} // namespace geocoding::ml
```

**3. ML Address Matcher**
```cpp
// hmi/services/geocoding/ml/include/ml_address_matcher.h
#pragma once
#include "embedding_generator.h"
#include "fuzzy_address_matcher.h"  // Phase 2
#include <memory>
#include <vector>

namespace geocoding {
namespace ml {

struct MLMatchResult {
    double ml_score;           // ML-based similarity (0.0-1.0)
    double fuzzy_score;        // Traditional fuzzy score
    double combined_score;     // Weighted combination
    bool is_semantic_match;    // Detected via ML
};

struct MLMatchConfig {
    double ml_weight = 0.70;      // ML gets 70% weight
    double fuzzy_weight = 0.30;   // Fuzzy gets 30%
    double min_ml_score = 0.80;
    bool use_hybrid = true;       // Combine ML + fuzzy
};

class MLAddressMatcher {
public:
    MLAddressMatcher(const std::string& model_path, 
                     const MLMatchConfig& config = MLMatchConfig());
    
    // Single address matching
    MLMatchResult calculateMatch(const AddressComponents& query,
                                  const AddressComponents& candidate);
    
    // Batch matching with caching
    std::vector<std::pair<AddressComponents, MLMatchResult>> 
        findBestMatches(const AddressComponents& query,
                       const std::vector<AddressComponents>& candidates,
                       size_t max_results = 5);
    
    // Hybrid mode: ML + traditional fuzzy
    void setHybridMode(bool enabled) { m_config.use_hybrid = enabled; }
    
private:
    std::unique_ptr<EmbeddingGenerator> m_embedding_gen;
    std::unique_ptr<FuzzyAddressMatcher> m_fuzzy_matcher;  // Fallback
    MLMatchConfig m_config;
    
    // Embedding cache (query → embedding)
    std::unordered_map<std::string, AddressEmbedding> m_embedding_cache;
};

}} // namespace geocoding::ml
```

**Training Pipeline (Python)**
```python
# models/training/train_address_model.py
import tensorflow as tf
from tensorflow.keras import layers, models
import pandas as pd
import numpy as np

class AddressSimilarityModel:
    def __init__(self, vocab_size=10000, embedding_dim=128):
        self.vocab_size = vocab_size
        self.embedding_dim = embedding_dim
        self.model = self._build_model()
    
    def _build_model(self):
        # Siamese network for address similarity
        input_a = layers.Input(shape=(50,))  # Max 50 tokens
        input_b = layers.Input(shape=(50,))
        
        # Shared embedding layer
        embedding = layers.Embedding(self.vocab_size, self.embedding_dim)
        
        # Shared LSTM encoder
        lstm = layers.Bidirectional(layers.LSTM(64, return_sequences=True))
        attention = layers.GlobalAveragePooling1D()
        
        # Encode both addresses
        encoded_a = attention(lstm(embedding(input_a)))
        encoded_b = attention(lstm(embedding(input_b)))
        
        # Concatenate and classify
        merged = layers.Concatenate()([encoded_a, encoded_b])
        dense1 = layers.Dense(64, activation='relu')(merged)
        dropout = layers.Dropout(0.3)(dense1)
        dense2 = layers.Dense(32, activation='relu')(dropout)
        output = layers.Dense(1, activation='sigmoid')(dense2)  # Similarity score
        
        model = models.Model(inputs=[input_a, input_b], outputs=output)
        model.compile(optimizer='adam', 
                     loss='binary_crossentropy',
                     metrics=['accuracy', 'AUC'])
        return model
    
    def train(self, address_pairs, labels, epochs=20, batch_size=32):
        """
        address_pairs: List of (address1, address2) tuples
        labels: 1 if similar, 0 if not
        """
        X_a = np.array([pair[0] for pair in address_pairs])
        X_b = np.array([pair[1] for pair in address_pairs])
        y = np.array(labels)
        
        history = self.model.fit([X_a, X_b], y,
                                 epochs=epochs,
                                 batch_size=batch_size,
                                 validation_split=0.2,
                                 callbacks=[
                                     tf.keras.callbacks.EarlyStopping(patience=3),
                                     tf.keras.callbacks.ModelCheckpoint('best_model.h5')
                                 ])
        return history
    
    def convert_to_tflite(self, output_path='address_similarity_model.tflite'):
        """Convert Keras model to TFLite for C++ deployment"""
        converter = tf.lite.TFLiteConverter.from_keras_model(self.model)
        converter.optimizations = [tf.lite.Optimize.DEFAULT]
        tflite_model = converter.convert()
        
        with open(output_path, 'wb') as f:
            f.write(tflite_model)
        print(f"Model saved to {output_path}")

# Training script
if __name__ == "__main__":
    # Load training data (address pairs + similarity labels)
    df = pd.read_csv('training_data.csv')
    
    # Preprocess addresses
    from prepare_dataset import preprocess_addresses, tokenize_addresses
    
    addresses_a = preprocess_addresses(df['address_1'].tolist())
    addresses_b = preprocess_addresses(df['address_2'].tolist())
    labels = df['is_similar'].tolist()
    
    # Tokenize
    address_pairs_tokenized = tokenize_addresses(list(zip(addresses_a, addresses_b)))
    
    # Train model
    model = AddressSimilarityModel()
    history = model.train(address_pairs_tokenized, labels, epochs=20)
    
    # Convert to TFLite
    model.convert_to_tflite('../address_similarity_model.tflite')
```

**CMakeLists.txt Updates:**
```cmake
# Find TensorFlow Lite
find_package(TensorFlowLite REQUIRED)

# ML Address Matcher library
add_library(ml_address_matcher
    hmi/services/geocoding/ml/src/tflite_model_loader.cpp
    hmi/services/geocoding/ml/src/embedding_generator.cpp
    hmi/services/geocoding/ml/src/ml_address_matcher.cpp
)

target_link_libraries(ml_address_matcher
    TensorFlowLite::TensorFlowLite
    geocoding_fuzzy  # Phase 2 dependency
)
```

**Dependencies:**
```bash
# Install TensorFlow Lite C++ (Windows)
vcpkg install tensorflow-lite:x64-windows

# Python training environment
pip install tensorflow==2.13.0 pandas numpy scikit-learn
```

---

## 📦 WEEK 10-11: REVERSE GEOCODING

### **Feature 2: Reverse Geocoding Engine**

**Files to Create:**
```
hmi/services/geocoding/reverse/
├── include/
│   ├── reverse_geocoder.h         # Main reverse geocoding interface
│   ├── interpolation_engine.h     # Address number interpolation
│   └── nearest_address_finder.h   # Spatial nearest neighbor search
└── src/
    ├── reverse_geocoder.cpp
    ├── interpolation_engine.cpp
    └── nearest_address_finder.cpp
```

**Key Components:**

**1. Reverse Geocoder**
```cpp
// hmi/services/geocoding/reverse/include/reverse_geocoder.h
#pragma once
#include "spatial_index.h"  // Phase 1
#include "address_components.h"
#include <optional>

namespace geocoding {
namespace reverse {

struct ReverseGeocodingRequest {
    double latitude;
    double longitude;
    double max_distance_meters = 100.0;  // Search radius
    bool include_interpolated = true;     // Interpolate street numbers
};

struct ReverseGeocodingResult {
    AddressComponents address;
    double distance_meters;
    double confidence;
    bool is_interpolated;
    std::string match_type;  // "exact", "nearest", "interpolated"
};

class ReverseGeocoder {
public:
    explicit ReverseGeocoder(std::shared_ptr<SpatialIndex> spatial_index);
    
    // Main reverse geocoding
    std::optional<ReverseGeocodingResult> 
        reverseGeocode(const ReverseGeocodingRequest& request);
    
    // Batch reverse geocoding
    std::vector<ReverseGeocodingResult> 
        reverseGeocodeBatch(const std::vector<Point>& locations);
    
    // Get nearest N addresses
    std::vector<ReverseGeocodingResult> 
        getNearestAddresses(double lat, double lon, size_t count = 5);
    
private:
    std::shared_ptr<SpatialIndex> m_spatial_index;
    std::unique_ptr<InterpolationEngine> m_interpolator;
    std::unique_ptr<NearestAddressFinder> m_nearest_finder;
    
    // Street number interpolation
    std::optional<AddressComponents> interpolateStreetNumber(
        const Point& location,
        const std::vector<AddressComponents>& nearby_addresses);
};

}} // namespace geocoding::reverse
```

**2. Address Interpolation Engine**
```cpp
// hmi/services/geocoding/reverse/include/interpolation_engine.h
#pragma once
#include "address_components.h"
#include <vector>

namespace geocoding {
namespace reverse {

struct StreetSegment {
    Point start_point;
    Point end_point;
    int start_number;
    int end_number;
    std::string street_name;
    bool is_odd_side;  // Odd vs even numbering
};

class InterpolationEngine {
public:
    // Interpolate street number from location on segment
    int interpolateNumber(const Point& location, const StreetSegment& segment);
    
    // Find best segment for interpolation
    std::optional<StreetSegment> findBestSegment(
        const Point& location,
        const std::vector<AddressComponents>& nearby_addresses);
    
    // Calculate position along segment (0.0 - 1.0)
    double calculatePositionOnSegment(const Point& point, 
                                       const StreetSegment& segment);
    
private:
    // Geometry helpers
    double pointToSegmentDistance(const Point& point, 
                                  const Point& seg_start, 
                                  const Point& seg_end);
    Point projectPointOntoSegment(const Point& point,
                                  const Point& seg_start,
                                  const Point& seg_end);
};

}} // namespace geocoding::reverse
```

**Usage Example:**
```cpp
// Reverse geocoding usage
ReverseGeocoder reverse_geocoder(spatial_index);

ReverseGeocodingRequest request;
request.latitude = 21.0285;
request.longitude = 105.8542;
request.max_distance_meters = 50.0;

auto result = reverse_geocoder.reverseGeocode(request);
if (result) {
    std::cout << "Address: " << result->address.street_name << std::endl;
    std::cout << "Number: " << result->address.street_number << std::endl;
    std::cout << "Distance: " << result->distance_meters << "m" << std::endl;
    std::cout << "Interpolated: " << result->is_interpolated << std::endl;
}
```

---

## 📦 WEEK 12: ADDRESS AUTOCOMPLETE

### **Feature 3: Real-time Address Autocomplete**

**Files to Create:**
```
hmi/services/geocoding/autocomplete/
├── include/
│   ├── autocomplete_service.h     # Main autocomplete interface
│   ├── prefix_trie.h              # Trie data structure
│   └── ranking_engine.h           # Result ranking
└── src/
    ├── autocomplete_service.cpp
    ├── prefix_trie.cpp
    └── ranking_engine.cpp
```

**Key Components:**

**1. Prefix Trie for Fast Lookups**
```cpp
// hmi/services/geocoding/autocomplete/include/prefix_trie.h
#pragma once
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace geocoding {
namespace autocomplete {

struct TrieNode {
    std::unordered_map<char, std::unique_ptr<TrieNode>> children;
    bool is_end_of_word = false;
    std::vector<std::string> suggestions;  // Top suggestions at this node
    int frequency = 0;  // Usage frequency for ranking
};

class PrefixTrie {
public:
    PrefixTrie();
    
    // Insert address
    void insert(const std::string& address, int frequency = 1);
    
    // Search with prefix
    std::vector<std::string> search(const std::string& prefix, size_t max_results = 10);
    
    // Fuzzy search (with typos)
    std::vector<std::string> fuzzySearch(const std::string& prefix, 
                                         int max_edit_distance = 2,
                                         size_t max_results = 10);
    
    // Update frequency (for popularity ranking)
    void incrementFrequency(const std::string& address);
    
    // Statistics
    size_t size() const { return m_size; }
    size_t memory_bytes() const;
    
private:
    std::unique_ptr<TrieNode> m_root;
    size_t m_size;
    
    // Helper methods
    void collectSuggestions(TrieNode* node, 
                           const std::string& prefix,
                           std::vector<std::string>& results,
                           size_t max_results);
};

}} // namespace geocoding::autocomplete
```

**2. Autocomplete Service**
```cpp
// hmi/services/geocoding/autocomplete/include/autocomplete_service.h
#pragma once
#include "prefix_trie.h"
#include "ranking_engine.h"
#include "geocoding_cache.h"  // Phase 2
#include <QObject>

namespace geocoding {
namespace autocomplete {

struct AutocompleteRequest {
    std::string prefix;
    size_t max_results = 10;
    bool fuzzy_matching = true;
    int max_edit_distance = 2;
    Point user_location;  // For location-based ranking
    bool use_location_bias = false;
};

struct AutocompleteSuggestion {
    std::string text;
    double score;
    std::string match_type;  // "prefix", "fuzzy", "semantic"
    double distance_km;  // From user location
};

class AutocompleteService : public QObject {
    Q_OBJECT
public:
    explicit AutocompleteService(QObject* parent = nullptr);
    
    // Initialize with address database
    bool initialize(const std::vector<AddressComponents>& addresses);
    
    // Get autocomplete suggestions
    std::vector<AutocompleteSuggestion> 
        getSuggestions(const AutocompleteRequest& request);
    
    // Update index (add new addresses)
    void addAddress(const AddressComponents& address, int frequency = 1);
    
    // Statistics
    size_t getIndexSize() const;
    double getAverageResponseTime() const;
    
signals:
    void indexUpdated(size_t new_size);
    
private:
    std::unique_ptr<PrefixTrie> m_trie;
    std::unique_ptr<RankingEngine> m_ranker;
    std::unique_ptr<InMemoryGeocodingCache> m_cache;
    
    // Performance tracking
    std::atomic<double> m_avg_response_time_ms;
    std::atomic<size_t> m_query_count;
};

}} // namespace geocoding::autocomplete
```

**3. Ranking Engine**
```cpp
// hmi/services/geocoding/autocomplete/include/ranking_engine.h
#pragma once
#include "address_components.h"
#include <vector>

namespace geocoding {
namespace autocomplete {

struct RankingFeatures {
    double prefix_match_score;    // How well prefix matches
    double popularity_score;       // Usage frequency
    double location_score;         // Distance from user
    double recency_score;          // Recently used addresses
};

class RankingEngine {
public:
    // Rank suggestions by relevance
    std::vector<std::pair<std::string, double>> 
        rankSuggestions(const std::vector<std::string>& suggestions,
                       const std::string& query,
                       const Point& user_location);
    
    // Feature extraction
    RankingFeatures extractFeatures(const std::string& suggestion,
                                   const std::string& query,
                                   const Point& user_location);
    
    // Combined score calculation
    double calculateScore(const RankingFeatures& features);
    
private:
    // Weights for ranking
    double m_prefix_weight = 0.40;
    double m_popularity_weight = 0.30;
    double m_location_weight = 0.20;
    double m_recency_weight = 0.10;
};

}} // namespace geocoding::autocomplete
```

**Usage Example:**
```cpp
// Autocomplete usage
AutocompleteService autocomplete;
autocomplete.initialize(all_addresses);

AutocompleteRequest request;
request.prefix = "123 Main";
request.max_results = 10;
request.fuzzy_matching = true;
request.user_location = Point(21.0285, 105.8542);
request.use_location_bias = true;

auto suggestions = autocomplete.getSuggestions(request);
for (const auto& suggestion : suggestions) {
    std::cout << suggestion.text 
              << " (score: " << suggestion.score << ")" 
              << std::endl;
}
```

---

## 🧪 TESTING STRATEGY

### **Unit Tests**
```
tests/
├── test_ml_address_matcher.cpp     # ML matching accuracy
├── test_reverse_geocoder.cpp       # Reverse geocoding accuracy
├── test_autocomplete.cpp           # Autocomplete relevance
└── test_performance.cpp            # Latency benchmarks
```

### **Test Cases:**

**ML Matching Tests:**
```cpp
TEST_F(MLAddressMatcherTest, SemanticSimilarity) {
    // Test: "123 Main Street" vs "123 Main St"
    // Expected: High ML score (>0.9)
}

TEST_F(MLAddressMatcherTest, CrossLanguageSimilarity) {
    // Test: "Nguyen Hue" vs "阮惠" (Vietnamese vs Chinese)
    // Expected: High ML score if model trained on multi-lingual data
}

TEST_F(MLAddressMatcherTest, AbbreviationHandling) {
    // Test: "123 E Main St Apt 4B" vs "123 East Main Street Apartment 4B"
    // Expected: High ML score (>0.85)
}
```

**Reverse Geocoding Tests:**
```cpp
TEST_F(ReverseGeocoderTest, ExactAddressMatch) {
    // Test: Reverse geocode known address coordinates
    // Expected: Exact match with distance <5m
}

TEST_F(ReverseGeocoderTest, StreetNumberInterpolation) {
    // Test: Point between 100 and 200 Main St
    // Expected: Interpolated address (e.g., "150 Main St")
}

TEST_F(ReverseGeocoderTest, PerformanceLatency) {
    // Test: 1000 reverse geocoding requests
    // Expected: Average latency <10ms
}
```

**Autocomplete Tests:**
```cpp
TEST_F(AutocompleteTest, PrefixMatching) {
    // Test: Prefix "123 Mai"
    // Expected: "123 Main St" in top 3 results
}

TEST_F(AutocompleteTest, FuzzyMatching) {
    // Test: Typo "123 Mian" (should match "Main")
    // Expected: "123 Main St" in results
}

TEST_F(AutocompleteTest, LocationBiasRanking) {
    // Test: User in Hanoi, query "Main St"
    // Expected: Hanoi "Main St" addresses ranked higher
}

TEST_F(AutocompleteTest, ResponseTime) {
    // Test: 1000 autocomplete queries
    // Expected: P95 latency <50ms
}
```

---

## 📊 PERFORMANCE TARGETS

| Feature | Metric | Target | Measurement |
|---------|--------|--------|-------------|
| ML Matching | Accuracy | >90% | Precision@1 on test set |
| ML Matching | Latency | <20ms | Single address pair |
| Reverse Geocoding | Accuracy | >95% | Exact + interpolated |
| Reverse Geocoding | Latency | <10ms | Single location |
| Autocomplete | Relevance | >85% | Top-3 accuracy |
| Autocomplete | Latency | <50ms | 10 suggestions |
| Autocomplete | Throughput | >1000 QPS | Concurrent requests |

---

## 🔄 INTEGRATION WITH EXISTING CODE

### **POIService Integration:**
```cpp
// hmi/services/poi/src/poi_service.cpp

#include "ml_address_matcher.h"
#include "reverse_geocoder.h"
#include "autocomplete_service.h"

// In POIService constructor
POIService::POIService() {
    // Phase 3 integrations
    m_ml_matcher = std::make_unique<ml::MLAddressMatcher>("models/address_similarity_model.tflite");
    m_reverse_geocoder = std::make_unique<reverse::ReverseGeocoder>(m_spatial_index);
    m_autocomplete = std::make_unique<autocomplete::AutocompleteService>();
}

// Enhanced geocoding with ML
EnhancedGeocodingResult POIService::geocodeAddressEnhanced(const EnhancedAddressRequest& request) {
    // 1. Try exact match (Phase 1)
    auto result = exactMatch(request);
    if (result.success) return result;
    
    // 2. Try fuzzy match (Phase 2)
    auto fuzzy_result = m_fuzzy_matcher->findBestMatches(...);
    if (!fuzzy_result.empty() && fuzzy_result[0].second.overall_score > 0.8) {
        return convertToResult(fuzzy_result[0]);
    }
    
    // 3. Try ML-based semantic match (Phase 3)
    auto ml_result = m_ml_matcher->findBestMatches(...);
    if (!ml_result.empty() && ml_result[0].second.ml_score > 0.85) {
        result.success = true;
        result.primary_result = convertToCandidate(ml_result[0].first);
        result.primary_result.match_type = "ml_semantic_match";
        return result;
    }
    
    return result;  // No match found
}

// Reverse geocoding endpoint
AddressComponents POIService::reverseGeocode(double lat, double lon) {
    reverse::ReverseGeocodingRequest request;
    request.latitude = lat;
    request.longitude = lon;
    request.max_distance_meters = 100.0;
    
    auto result = m_reverse_geocoder->reverseGeocode(request);
    if (result) {
        return result->address;
    }
    return AddressComponents();  // Empty if not found
}

// Autocomplete endpoint
std::vector<std::string> POIService::getAutocompleteSuggestions(const std::string& prefix) {
    autocomplete::AutocompleteRequest request;
    request.prefix = prefix;
    request.max_results = 10;
    request.fuzzy_matching = true;
    
    auto suggestions = m_autocomplete->getSuggestions(request);
    
    std::vector<std::string> results;
    for (const auto& suggestion : suggestions) {
        results.push_back(suggestion.text);
    }
    return results;
}
```

---

## 📚 DEPENDENCIES

### **C++ Libraries:**
```cmake
# TensorFlow Lite
find_package(TensorFlowLite REQUIRED)

# Optional: ONNX Runtime (alternative to TFLite)
find_package(onnxruntime REQUIRED)
```

### **Python Training Environment:**
```txt
# requirements.txt
tensorflow==2.13.0
tensorflow-lite==2.13.0
pandas==2.0.3
numpy==1.24.3
scikit-learn==1.3.0
matplotlib==3.7.2
```

### **Installation:**
```bash
# Windows (vcpkg)
vcpkg install tensorflow-lite:x64-windows

# Python
pip install -r models/training/requirements.txt

# Model training
cd models/training
python train_address_model.py

# Convert to TFLite
python -c "from train_address_model import AddressSimilarityModel; \
           model = AddressSimilarityModel(); \
           model.convert_to_tflite('../address_similarity_model.tflite')"
```

---

## 🎓 LEARNING RESOURCES

- **TensorFlow Lite C++ Guide:** https://www.tensorflow.org/lite/guide/inference
- **Siamese Networks for Text:** https://www.tensorflow.org/text/tutorials/nmt_with_attention
- **Reverse Geocoding Algorithms:** Mapbox Blog
- **Trie Data Structures:** https://en.wikipedia.org/wiki/Trie
- **Location-based Ranking:** HERE Autocomplete API Documentation

---

## ✅ PHASE 3 COMPLETION CHECKLIST

- [ ] **Week 9: ML Address Similarity**
  - [ ] TensorFlow Lite C++ integration
  - [ ] Embedding generator implementation
  - [ ] ML address matcher with hybrid mode
  - [ ] Training pipeline setup
  - [ ] Model training on address dataset
  - [ ] TFLite model conversion
  - [ ] Unit tests (>90% accuracy)

- [ ] **Week 10-11: Reverse Geocoding**
  - [ ] Reverse geocoder core implementation
  - [ ] Street number interpolation engine
  - [ ] Nearest address finder with spatial index
  - [ ] Batch reverse geocoding support
  - [ ] Unit tests (>95% accuracy, <10ms latency)

- [ ] **Week 12: Address Autocomplete**
  - [ ] Prefix trie implementation
  - [ ] Fuzzy search support
  - [ ] Ranking engine with location bias
  - [ ] Autocomplete service with caching
  - [ ] UI integration (search box)
  - [ ] Performance tests (>1000 QPS, <50ms P95)

- [ ] **Integration & Documentation**
  - [ ] POIService integration
  - [ ] API documentation
  - [ ] Performance benchmarks
  - [ ] User guide

---

**Document Version:** 1.0  
**Last Updated:** October 19, 2025  
**Status:** 📋 **READY FOR IMPLEMENTATION**  
**Estimated Effort:** 4 weeks (1 engineer)
