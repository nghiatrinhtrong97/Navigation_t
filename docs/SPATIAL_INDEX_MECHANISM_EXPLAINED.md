# Giải Thích Cơ Chế Spatial Index Query

**Tài liệu:** Cách thức hoạt động của R-tree Spatial Index trong Geocoding  
**Ngày:** 19/10/2025

---

## 🎯 VẤN ĐỀ CẦN GIẢI QUYẾT

### Câu hỏi:
> "Từ address được chuẩn hóa tìm ra tile trong spatial kiểu gì?"

### Trả lời ngắn gọn:
**KHÔNG dùng tile! Spatial Index sử dụng R-tree (Rectangle-tree) để tìm địa chỉ gần nhất dựa trên TỌA ĐỘ địa lý (lat/lon), KHÔNG phải từ chuỗi text.**

---

## 🏗️ KIẾN TRÚC SPATIAL INDEX

### 1. **Cấu trúc dữ liệu: R*-tree**

R-tree là cấu trúc cây cân bằng để index dữ liệu không gian (spatial data):

```
R-tree Structure:
┌─────────────────────────────────────────────────────────┐
│                    Root Node                             │
│  Bounding Box: (21.0°N-23.5°N, 105.0°E-106.5°E)        │ ← Toàn bộ Việt Nam (vùng rộng)
└────────────┬────────────────────────────────────────────┘
             │
    ┌────────┴────────┬────────────────────┐
    │                 │                    │
┌───▼─────┐    ┌─────▼──────┐      ┌─────▼──────┐
│ Node A  │    │  Node B    │      │  Node C    │
│ Hà Nội  │    │  TP.HCM    │      │  Đà Nẵng   │        ← Chia theo vùng
│ 21.0°N  │    │  10.7°N    │      │  16.0°N    │
└───┬─────┘    └─────┬──────┘      └─────┬──────┘
    │                │                    │
    │         ┌──────┴──────┐             │
    │         │             │             │
┌───▼───┐  ┌─▼───┐    ┌────▼───┐    ┌───▼────┐
│Addr 1 │  │Addr2│    │ Addr 3 │    │ Addr 4 │          ← Leaf nodes (địa chỉ cụ thể)
│21.028°│  │21.03°│   │ 10.77° │    │ 16.05° │
└───────┘  └─────┘    └────────┘    └────────┘

Mỗi node lưu:
- Bounding Box (minLat, maxLat, minLon, maxLon)
- Con trỏ đến các node con hoặc địa chỉ
```

### 2. **AddressRecord - Dữ liệu trong Index**

```cpp
struct AddressRecord {
    uint64_t id;                      // ID duy nhất: 12345
    Point location;                   // ★ QUAN TRỌNG: {lat: 21.028511, lon: 105.804817}
    std::string formatted_address;    // "123 Phố Huế, Hai Bà Trưng, Hà Nội"
    std::string normalized_address;   // "123 PHO HUE HAI BA TRUNG HA NOI"
    std::string postal_code;          // "100000"
    std::string city;                 // "Hà Nội"
    std::string state;                // "Vietnam"
    
    // Metadata
    std::string data_source;          // "POI_DB" hoặc "TIGER", "OSM"
    GeocodingQuality quality;         // STREET_LEVEL (±10m)
    uint64_t last_updated_ms;         // 1697702400000
};
```

**CHÚ Ý:** 
- `location` (lat/lon) là **KEY** để build R-tree
- `formatted_address` chỉ dùng để **hiển thị kết quả**, KHÔNG dùng để search

---

## 🔍 QUY TRÌNH GEOCODING VỚI SPATIAL INDEX

### **BƯỚC 1: Parse & Normalize Address (Text Processing)**

**Input (User):**
```
"123 phố huế, hà nội"
```

**Output (AddressComponents):**
```cpp
AddressComponents parsed = {
    house_number: "123",
    street_name: "Phố Huế",
    city: "Hà Nội",
    state: "Vietnam",
    postal_code: null,  // Không có trong input
    country: "VN"
};
```

**Code:**
```cpp
// File: poi_service.cpp, Line 337-342
geocoding::AddressParser parser;
geocoding::AddressComponents parsed_components = 
    parser.parse(request.freeform_address, request.country_code);

geocoding::AddressNormalizer normalizer;
geocoding::AddressComponents normalized_components = 
    normalizer.normalize(parsed_components);
```

---

### **BƯỚC 2: Estimate Location (Text → Coordinates)**

**⚠️ ĐÂY LÀ BƯỚC QUAN TRỌNG NHẤT!**

Vì R-tree chỉ làm việc với **TỌA ĐỘ** (lat/lon), ta phải **ƯỚC TÍNH** tọa độ từ text!

#### **Chiến lược ước tính:**

**A. Nếu có Postal Code (Mã bưu chính):**
```cpp
// Line 496-499
if (normalized_address.postal_code.has_value()) {
    // Tra cứu centroid (tâm vùng) từ database ZIP code
    estimated_center = zipCodeDatabase.lookup("100000");
    // → Kết quả: {lat: 21.0285, lon: 105.8542} (trung tâm Hà Nội)
}
```

**B. Nếu có City/State:**
```cpp
// Tra cứu tọa độ trung tâm thành phố
if (normalized_address.city.has_value()) {
    estimated_center = cityDatabase.lookup("Hà Nội");
    // → Kết quả: {lat: 21.0285, lon: 105.8542}
}
```

**C. Nếu User cung cấp Reference Location:**
```cpp
// Line 501-503
if (request.reference_location.has_value()) {
    estimated_center = *request.reference_location;
    // Ví dụ: GPS hiện tại của user {lat: 21.030, lon: 105.850}
}
```

**D. Fallback - Dùng centroid quốc gia:**
```cpp
// Default cho Việt Nam
estimated_center = {lat: 16.0, lon: 108.0};
```

---

### **BƯỚC 3: Query Spatial Index (Geographic Search)**

#### **3.1. Query by k-Nearest Neighbors (k-NN)**

**Mục đích:** Tìm k địa chỉ **GẦN NHẤT** với tọa độ ước tính.

**Code:**
```cpp
// Line 505-508
auto candidates = m_spatialIndex->findNearestAddresses(
    estimated_center,    // {lat: 21.028, lon: 105.850}
    request.max_results  // k = 10
);
```

**Cơ chế R-tree k-NN (trong spatial_index.cpp, Line 235-263):**

```cpp
// 1. Dùng Boost.Geometry R-tree built-in k-NN query
BoostPoint query_point = toBoostPoint(estimated_center);
std::vector<RTreeValue> nearest;

m_impl->rtree->query(
    bgi::nearest(query_point, k),  // ← Tìm k nearest neighbors
    std::back_inserter(nearest)
);

// 2. Tính khoảng cách chính xác (Haversine formula)
std::vector<std::pair<AddressRecord, double>> scored;
for (const auto& value : nearest) {
    const AddressRecord& addr = value.second;
    double distance = calculateDistance(estimated_center, addr.location);
    scored.emplace_back(addr, distance);
}

// 3. Sắp xếp theo khoảng cách tăng dần
std::sort(scored.begin(), scored.end(),
    [](const auto& a, const auto& b) { return a.second < b.second; });
```

**Ví dụ kết quả:**
```
estimated_center: {lat: 21.028, lon: 105.850}

Candidates (top 10 nearest):
1. "120 Phố Huế, Hai Bà Trưng" → distance = 50m
2. "123 Phố Huế, Hai Bà Trưng" → distance = 85m  ← ★ Match!
3. "125 Phố Huế, Hai Bà Trưng" → distance = 120m
4. "100 Phố Huế, Hai Bà Trưng" → distance = 200m
5. "150 Trần Nhân Tông, Hai Bà Trưng" → distance = 300m
...
```

#### **3.2. Hoặc Query by Radius (Tìm trong bán kính)**

**Code:**
```cpp
auto candidates = m_spatialIndex->queryRadius(
    estimated_center,  // {lat: 21.028, lon: 105.850}
    1000.0            // radius = 1km (1000 meters)
);
```

**Cơ chế trong spatial_index.cpp (Line 172-214):**

```cpp
// 1. Convert radius to degrees (approximate)
double radius_degrees = radius_meters / 111000.0;  // 1 degree ≈ 111km

// 2. Create bounding box for initial filter
BoundingBox bbox;
bbox.minLat = center.latitude - radius_degrees;
bbox.maxLat = center.latitude + radius_degrees;
bbox.minLon = center.longitude - radius_degrees;
bbox.maxLon = center.longitude + radius_degrees;

// 3. Query R-tree for candidates in bounding box
auto candidates = queryRegion(bbox);

// 4. Filter by exact geodetic distance (Haversine)
for (const auto& addr : candidates) {
    double distance = calculateDistance(center, addr.location);
    if (distance <= radius_meters) {
        results.push_back(addr);
    }
}
```

**Hình ảnh minh họa:**
```
        Bounding Box (initial filter)
        ┌─────────────────────────┐
        │                         │
        │      ○  ○               │ ← Candidates in box
        │    ○   ●   ○            │   (fast R-tree query)
        │      ○  ○  ○            │
        │                         │
        └─────────────────────────┘
                 │
                 ▼
        Apply exact distance filter
                 │
                 ▼
           Results within 1km
              ○  ●  ○
               (sorted)
```

---

### **BƯỚC 4: String Matching & Ranking (Text Comparison)**

**Sau khi có candidates từ spatial index, so sánh string:**

```cpp
// Simplified example
std::string input_normalized = "123 PHO HUE";
double best_score = 0.0;
AddressRecord best_match;

for (const auto& candidate : candidates) {
    // Compare normalized strings
    double score = calculateStringSimilarity(
        input_normalized, 
        candidate.normalized_address
    );
    
    // Adjust score by distance (closer = better)
    double distance_penalty = 1.0 - (distance / 1000.0);
    double final_score = score * 0.7 + distance_penalty * 0.3;
    
    if (final_score > best_score) {
        best_score = final_score;
        best_match = candidate;
    }
}
```

---

## 📊 SO SÁNH: LINEAR SEARCH vs SPATIAL INDEX

### **Scenario: Tìm địa chỉ trong database 100,000 POIs**

#### **1. Linear Search (Không dùng Spatial Index):**

```cpp
// Cách cũ: Brute-force
std::vector<POI> results;
for (const auto& poi : m_poiDatabase) {  // 100,000 iterations
    if (poi.address.contains("Phố Huế")) {
        double distance = calculateDistance(reference, poi.location);
        results.push_back({poi, distance});
    }
}
std::sort(results.begin(), results.end());
```

**Performance:**
- **Time complexity:** O(n) = 100,000 comparisons
- **Time:** ~50-100ms cho mỗi query
- **Scalability:** Tệ (1M addresses → 500ms+)

---

#### **2. Spatial Index (R-tree):**

```cpp
// Cách mới: Spatial query
auto candidates = m_spatialIndex->findNearestAddresses(
    estimated_center, 10  // Chỉ cần 10 candidates
);
// → Only ~50-100 nodes visited in tree (not 100,000!)
```

**Performance:**
- **Time complexity:** O(log n) = ~15-20 node visits
- **Time:** ~5-10ms cho mỗi query
- **Scalability:** Tốt (1M addresses → still ~10-15ms)

---

## 🔧 CHI TIẾT KỸ THUẬT R-TREE

### **1. Build Index (spatial_index.cpp, Line 95-128)**

```cpp
bool SpatialIndex::buildIndex(const std::vector<AddressRecord>& addresses) {
    // 1. Convert AddressRecord → RTreeValue (Boost type)
    std::vector<RTreeValue> values;
    for (const auto& addr : addresses) {
        BoostPoint point(addr.location.longitude, addr.location.latitude);
        values.emplace_back(point, addr);  // (geometry, payload)
    }
    
    // 2. Bulk load R-tree (faster than incremental insert)
    m_impl->rtree = std::make_unique<RTree>(values.begin(), values.end());
    // → Automatically builds balanced tree
    
    return true;
}
```

**Tree Structure After Build:**
```
Depth 0 (Root):    [Vietnam: 8°N-24°N, 102°E-110°E]
                            |
        ┌───────────────────┴────────────────────┐
        |                   |                    |
Depth 1:  [North Vietnam]  [Central]      [South Vietnam]
        21°-24°N         15°-18°N           8°-12°N
            |                |                  |
        ┌───┴───┐        ┌───┴───┐         ┌───┴───┐
Depth 2: [Hanoi] [HaiPhong] [DaNang] [Hue] [HCM] [CanTho]
            |
        ┌───┴────┐
Depth 3: [District 1] [District 2] ...
            |
        ┌───┴────┐
Depth 4: [Street 1] [Street 2] ...
            |
Leaf:    [Address 1] [Address 2] [Address 3] ...
```

### **2. Query Algorithm (Boost.Geometry bgi::nearest)**

**Pseudocode:**
```python
def find_k_nearest(query_point, k):
    priority_queue = []
    visited_nodes = []
    
    # 1. Start from root
    current = root_node
    
    # 2. Traverse tree (depth-first with backtracking)
    while len(results) < k or priority_queue.not_empty():
        if current.is_leaf():
            # Calculate distance to actual addresses
            for addr in current.addresses:
                dist = haversine(query_point, addr.location)
                priority_queue.push((dist, addr))
        else:
            # Calculate distance to child bounding boxes
            for child in current.children:
                min_dist = distance_to_bbox(query_point, child.bbox)
                priority_queue.push((min_dist, child))
        
        # 3. Pop next closest node/address
        current = priority_queue.pop()
    
    # 4. Return top k
    return priority_queue.top(k)
```

**Ví dụ trace:**
```
Query: Find 3 nearest to {21.028°N, 105.850°E}

Step 1: Start at Root
  → Visit child bounding boxes
  → North Vietnam (distance = 0km) ← Contains query point
  → Central (distance = 500km)
  → South (distance = 1200km)
  → Priority: [North(0), Central(500), South(1200)]

Step 2: Visit North Vietnam
  → Visit child boxes
  → Hanoi (distance = 0km)
  → Hai Phong (distance = 80km)
  → Priority: [Hanoi(0), HaiPhong(80), Central(500), ...]

Step 3: Visit Hanoi districts
  → Hai Bà Trưng (distance = 0km)
  → Hoàn Kiếm (distance = 2km)
  → Priority: [HaiBaTrung(0), HoanKiem(2), HaiPhong(80), ...]

Step 4: Visit Hai Bà Trưng streets
  → Phố Huế (distance = 0km)
  → Trần Nhân Tông (distance = 0.3km)
  → Priority: [PhoHue(0), TranNhanTong(0.3), HoanKiem(2), ...]

Step 5: Visit addresses on Phố Huế
  → 120 Phố Huế (distance = 50m)
  → 123 Phố Huế (distance = 85m) ← TARGET!
  → 125 Phố Huế (distance = 120m)

Step 6: Return top 3
  → Results: [120(50m), 123(85m), 125(120m)]
```

**Nodes visited:** ~5-7 (not 100,000!)

---

## 🎯 TÓM TẮT CƠ CHẾ

### **Câu hỏi ban đầu:**
> "Từ address được chuẩn hóa tìm ra tile trong spatial kiểu gì?"

### **Trả lời chi tiết:**

1. **KHÔNG dùng "tile"** (như Google Maps tiles, OSM tiles)
   - R-tree dùng **bounding boxes** (hình chữ nhật), không phải tiles

2. **KHÔNG tìm trực tiếp từ text**
   - R-tree chỉ làm việc với **tọa độ** (lat/lon)
   - Text chỉ dùng để **ước tính tọa độ** ban đầu

3. **Quy trình thực tế:**
   ```
   Text "123 Phố Huế, Hà Nội"
       ↓ (Parse & Normalize)
   Components {street: "Phố Huế", city: "Hà Nội"}
       ↓ (Estimate Location)
   Coordinates {lat: 21.028, lon: 105.850}
       ↓ (Query R-tree)
   k-Nearest Neighbors [addr1, addr2, addr3, ...]
       ↓ (String Matching)
   Best Match: "123 Phố Huế" (score: 0.95)
   ```

4. **Key insight:**
   - **Input:** Text (string)
   - **Query:** Coordinates (lat/lon) ← **Conversion happens here!**
   - **Output:** Addresses with locations
   - **Ranking:** String similarity + distance

---

## 📚 TÀI LIỆU THAM KHẢO

### **Files trong project:**
- `hmi/services/geocoding/include/spatial_index.h` - Interface
- `hmi/services/geocoding/src/spatial_index.cpp` - Implementation
- `hmi/services/poi/src/poi_service.cpp` (Line 491-545) - Usage

### **Libraries:**
- **Boost.Geometry R-tree:** https://www.boost.org/doc/libs/1_83_0/libs/geometry/doc/html/geometry/reference/spatial_indexes/boost__geometry__index__rtree.html
- **R-tree paper:** Guttman, A. (1984). "R-trees: A Dynamic Index Structure for Spatial Searching"

### **Algorithms:**
- **Haversine formula:** Calculate great-circle distance between two lat/lon points
- **k-NN (k-Nearest Neighbors):** Find k closest points in spatial data
- **Bounding box:** Rectangular region defined by (minLat, maxLat, minLon, maxLon)

---

**Tác giả:** GitHub Copilot  
**Ngày:** 19/10/2025  
**Version:** 1.0
