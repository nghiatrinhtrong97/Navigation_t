# Spatial Index Query - Visual Flow Diagram

## 🔄 LUỒNG XỬ LÝ HOÀN CHỈNH

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          USER INPUT (Text)                                   │
│                    "123 phố huế, hà nội"                                     │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                    STEP 1: ADDRESS PARSING                                   │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  AddressParser::parse()                                                 │ │
│  │  Input:  "123 phố huế, hà nội"                                         │ │
│  │  Output: AddressComponents {                                            │ │
│  │            house_number: "123"                                          │ │
│  │            street_name: "Phố Huế"                                       │ │
│  │            city: "Hà Nội"                                               │ │
│  │            country: "VN"                                                │ │
│  │          }                                                              │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                 STEP 2: ADDRESS NORMALIZATION                                │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  AddressNormalizer::normalize()                                         │ │
│  │  - Expand abbreviations: "St" → "Street"                               │ │
│  │  - Uppercase: "phố huế" → "PHO HUE"                                    │ │
│  │  - Remove diacritics: "Phố" → "PHO"                                    │ │
│  │  - Standardize format                                                   │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│           ★★★ STEP 3: ESTIMATE LOCATION (TEXT → COORDINATES) ★★★            │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  Chiến lược ước tính:                                                   │ │
│  │                                                                          │ │
│  │  [A] Nếu có Postal Code:                                                │ │
│  │      zipDatabase.lookup("100000") → {21.028°N, 105.850°E}             │ │
│  │                                                                          │ │
│  │  [B] Nếu có City:                                                       │ │
│  │      cityDatabase.lookup("Hà Nội") → {21.028°N, 105.850°E}            │ │
│  │                                                                          │ │
│  │  [C] Nếu có Reference Location (GPS user):                              │ │
│  │      Use current_location: {21.030°N, 105.852°E}                       │ │
│  │                                                                          │ │
│  │  [D] Fallback (Default centroid):                                       │ │
│  │      Country center: {16.0°N, 108.0°E}                                 │ │
│  │                                                                          │ │
│  │  ✅ OUTPUT: estimated_center = {21.028°N, 105.850°E}                   │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│          ★★★ STEP 4: SPATIAL INDEX QUERY (R-TREE) ★★★                       │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  Query: findNearestAddresses(estimated_center, k=10)                   │ │
│  │                                                                          │ │
│  │  Input Coordinates: {21.028°N, 105.850°E}                              │ │
│  │                                                                          │ │
│  │  R-tree Traversal (Log N complexity):                                   │ │
│  │                                                                          │ │
│  │     Root Node                                                           │ │
│  │       │                                                                  │ │
│  │       ├── North Vietnam [21°-24°N] ← Query point inside! Visit this    │ │
│  │       ├── Central [15°-18°N]       ← Too far, skip                      │ │
│  │       └── South [8°-12°N]          ← Too far, skip                      │ │
│  │           │                                                              │ │
│  │           ├── Hà Nội [20.9°-21.1°N] ← Query point inside! Visit        │ │
│  │           ├── Hải Phòng [20.8°-20.9°N] ← Maybe check                   │ │
│  │           └── ...                                                        │ │
│  │               │                                                          │ │
│  │               ├── Hai Bà Trưng District ← Contains query point          │ │
│  │               ├── Hoàn Kiếm District                                    │ │
│  │               └── ...                                                    │ │
│  │                   │                                                      │ │
│  │                   └── Phố Huế Street                                    │ │
│  │                       │                                                  │ │
│  │                       ├── 120 Phố Huế {21.0283, 105.8495} dist=50m     │ │
│  │                       ├── 123 Phố Huế {21.0285, 105.8498} dist=85m ★   │ │
│  │                       ├── 125 Phở Huế {21.0287, 105.8500} dist=120m    │ │
│  │                       └── ...                                            │ │
│  │                                                                          │ │
│  │  ✅ OUTPUT: Top 10 candidates (sorted by distance)                      │ │
│  │     1. "120 Phố Huế, Hai Bà Trưng" (50m)                               │ │
│  │     2. "123 Phố Huế, Hai Bà Trưng" (85m)  ← Best match                 │ │
│  │     3. "125 Phố Huế, Hai Bà Trưng" (120m)                              │ │
│  │     4. "100 Phố Huế, Hai Bà Trưng" (200m)                              │ │
│  │     ...                                                                  │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│               STEP 5: STRING MATCHING & RANKING                              │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  Compare input text with candidates:                                    │ │
│  │                                                                          │ │
│  │  Input (normalized): "123 PHO HUE HA NOI"                               │ │
│  │                                                                          │ │
│  │  Candidate 1: "120 PHO HUE HAI BA TRUNG HA NOI"                        │ │
│  │    String similarity: 0.85                                              │ │
│  │    Distance penalty: 1.0 - (50m / 1000m) = 0.95                        │ │
│  │    Final score: 0.85 * 0.7 + 0.95 * 0.3 = 0.595 + 0.285 = 0.88        │ │
│  │                                                                          │ │
│  │  Candidate 2: "123 PHO HUE HAI BA TRUNG HA NOI" ★                      │ │
│  │    String similarity: 0.95  ← House number matches!                    │ │
│  │    Distance penalty: 1.0 - (85m / 1000m) = 0.915                       │ │
│  │    Final score: 0.95 * 0.7 + 0.915 * 0.3 = 0.665 + 0.275 = 0.94       │ │
│  │                                                                          │ │
│  │  Candidate 3: "125 PHO HUE HAI BA TRUNG HA NOI"                        │ │
│  │    String similarity: 0.90                                              │ │
│  │    Distance penalty: 1.0 - (120m / 1000m) = 0.88                       │ │
│  │    Final score: 0.90 * 0.7 + 0.88 * 0.3 = 0.63 + 0.264 = 0.894        │ │
│  │                                                                          │ │
│  │  ✅ Best Match: Candidate 2 with score 0.94                             │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└──────────────────────────────┬──────────────────────────────────────────────┘
                               │
                               ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                      FINAL RESULT (GeocodingCandidate)                       │
│  ┌────────────────────────────────────────────────────────────────────────┐ │
│  │  {                                                                       │ │
│  │    formatted_address: "123 Phố Huế, Hai Bà Trưng, Hà Nội",            │ │
│  │    latitude: 21.028511,                                                 │ │
│  │    longitude: 105.849817,                                               │ │
│  │    confidence_score: 0.94,                                              │ │
│  │    match_type: "spatial_proximity",                                     │ │
│  │    geocoding_method: "spatial_index_knn",                               │ │
│  │    accuracy_meters: 10.0,                                               │ │
│  │    quality: STREET_LEVEL                                                │ │
│  │  }                                                                       │ │
│  └────────────────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 🗺️ R-TREE STRUCTURE VISUALIZATION

### Initial State: Empty R-tree
```
┌─────────────────────────┐
│     R-tree Root         │
│      (empty)            │
└─────────────────────────┘
```

### After Building Index (100,000 addresses):

```
                     Depth 0: Root Node
        ┌─────────────────────────────────────────────┐
        │   Vietnam Bounding Box                      │
        │   [8.0°N - 24.0°N, 102.0°E - 110.0°E]      │
        │   Contains: 100,000 addresses               │
        └────────────┬────────────────────────────────┘
                     │
        ┏━━━━━━━━━━━━┻━━━━━━━━━━━━━━━━━━━━━━━━┓
        ┃                                       ┃
   Depth 1: Regional Nodes                     ┃
┌───────▼──────────┐  ┌──────────────┐  ┌─────▼─────────┐
│ North Vietnam    │  │  Central     │  │ South Vietnam │
│ [20°-24°N]       │  │  [15°-19°N]  │  │ [8°-12°N]     │
│ 40,000 addresses │  │  20,000 addr │  │ 40,000 addr   │
└────────┬─────────┘  └──────────────┘  └───────────────┘
         │
    Depth 2: City Nodes
    ┌────┴─────┬──────────────┐
    ▼          ▼              ▼
┌────────┐ ┌────────┐    ┌────────┐
│ Hà Nội │ │Hải Phòng│    │Nam Định│
│20,000  │ │ 10,000  │    │ 10,000 │
│addresses│ │addresses│    │addresses│
└───┬────┘ └─────────┘    └─────────┘
    │
Depth 3: District Nodes
    ┌─────┴─────┬─────────────┬──────────────┐
    ▼           ▼             ▼              ▼
┌────────┐ ┌────────┐  ┌────────┐    ┌────────┐
│Ba Đình │ │Hoàn Kiếm│ │Hai Bà  │    │Đống Đa │
│3,000   │ │ 4,000   │ │Trưng   │    │ 5,000  │
│        │ │         │ │4,000   │    │        │
└────────┘ └─────────┘ └───┬────┘    └────────┘
                           │
                    Depth 4: Street Nodes
                    ┌──────┴────┬─────────────┐
                    ▼           ▼             ▼
                ┌──────────┐ ┌──────┐   ┌────────┐
                │Phố Huế   │ │Trần  │   │Lê Duẩn │
                │150 addr  │ │Nhân  │   │200 addr│
                │          │ │Tông  │   │        │
                └────┬─────┘ │100   │   └────────┘
                     │        └──────┘
              Depth 5: Leaf (Actual Addresses)
                     │
        ┌────────────┼────────────┬──────────────┐
        ▼            ▼            ▼              ▼
    ┌───────┐   ┌────────┐   ┌────────┐    ┌────────┐
    │120 Phố│   │123 Phố │   │125 Phố │    │130 Phố │
    │Huế    │   │Huế ★   │   │Huế     │    │Huế     │
    │21.0283│   │21.0285 │   │21.0287 │    │21.0290 │
    │105.850│   │105.850 │   │105.850 │    │105.851 │
    └───────┘   └────────┘   └────────┘    └────────┘
```

---

## 🎯 QUERY EXAMPLE: "123 Phố Huế, Hà Nội"

### Step-by-Step Tree Traversal:

```
Query Point: {21.0285°N, 105.8498°E} (estimated from "Hà Nội")

┌─ Step 1: Start at Root ─────────────────────────────────────┐
│                                                              │
│  Check children bounding boxes:                              │
│  • North Vietnam [20°-24°N] ← Contains query point ✓         │
│  • Central [15°-19°N]       ← Too far south ✗                │
│  • South [8°-12°N]          ← Too far south ✗                │
│                                                              │
│  Decision: Visit North Vietnam node                          │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─ Step 2: North Vietnam Node ────────────────────────────────┐
│                                                              │
│  Check children:                                             │
│  • Hà Nội [20.9°-21.1°N]     ← Contains query ✓             │
│  • Hải Phòng [20.8°-20.9°N]  ← Nearby, check ✓              │
│  • Nam Định [20.4°-20.6°N]   ← Too far ✗                    │
│                                                              │
│  Decision: Visit Hà Nội first (contains point)               │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─ Step 3: Hà Nội Districts ───────────────────────────────────┐
│                                                              │
│  Calculate distance to each district bounding box:           │
│  • Hai Bà Trưng [21.02°-21.04°N] ← distance = 0m ✓          │
│  • Hoàn Kiếm [21.01°-21.03°N]    ← distance = 500m ✓        │
│  • Ba Đình [21.03°-21.05°N]      ← distance = 1km ✓         │
│  • Đống Đa [21.00°-21.02°N]      ← distance = 1.5km         │
│                                                              │
│  Decision: Visit Hai Bà Trưng (closest)                      │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─ Step 4: Hai Bà Trưng Streets ───────────────────────────────┐
│                                                              │
│  Calculate distance to street bounding boxes:                │
│  • Phố Huế [21.027°-21.030°N]      ← distance = 50m ✓       │
│  • Trần Nhân Tông [21.025°-21.028°] ← distance = 300m ✓     │
│  • Lê Duẩn [21.020°-21.025°N]       ← distance = 800m       │
│                                                              │
│  Decision: Visit Phở Huế (closest)                           │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─ Step 5: Addresses on Phố Huế (LEAF NODES) ─────────────────┐
│                                                              │
│  Calculate exact distances (Haversine formula):              │
│                                                              │
│  1. "120 Phở Huế" {21.02830, 105.84950}                     │
│     distance = 50.2 meters                                   │
│                                                              │
│  2. "123 Phố Huế" {21.02851, 105.84982} ★                   │
│     distance = 85.7 meters                                   │
│                                                              │
│  3. "125 Phố Huế" {21.02870, 105.85000}                     │
│     distance = 120.3 meters                                  │
│                                                              │
│  4. "130 Phở Huế" {21.02900, 105.85100}                     │
│     distance = 180.5 meters                                  │
│                                                              │
│  Continue searching other nearby streets...                  │
│  (Need k=10 results, currently have 4)                       │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─ Step 6: Backtrack to get more candidates ──────────────────┐
│                                                              │
│  Visit Trần Nhân Tông street (next closest):                │
│  • "50 Trần Nhân Tông" - distance = 300m                    │
│  • "100 Trần Nhân Tông" - distance = 350m                   │
│  ...                                                         │
│                                                              │
│  Visit Hoàn Kiếm district (if needed):                       │
│  • Addresses 500m+ away                                      │
│                                                              │
│  Stop when k=10 candidates found                             │
└──────────────────────────────────────────────────────────────┘
                            │
                            ▼
┌─ Final Results: Top 10 Candidates ───────────────────────────┐
│                                                              │
│  1. "120 Phở Huế, Hai Bà Trưng, Hà Nội" (50m)              │
│  2. "123 Phở Huế, Hai Bà Trưng, Hà Nội" (85m) ★ BEST       │
│  3. "125 Phở Huế, Hai Bà Trưng, Hà Nội" (120m)             │
│  4. "130 Phở Huế, Hai Bà Trưng, Hà Nội" (180m)             │
│  5. "50 Trần Nhân Tông, Hai Bà Trưng, Hà Nội" (300m)       │
│  6. "100 Trần Nhân Tông, Hai Bà Trưng, Hà Nội" (350m)      │
│  7. "15 Phố Huế, Hai Bà Trưng, Hà Nội" (400m)              │
│  8. "200 Phở Huế, Hai Bà Trưng, Hà Nội" (450m)             │
│  9. "10 Lê Duẩn, Hoàn Kiếm, Hà Nội" (800m)                 │
│  10. "50 Hàng Bài, Hoàn Kiếm, Hà Nội" (900m)               │
│                                                              │
│  Nodes Visited: ~7 out of 100,000 addresses                 │
│  Query Time: ~5-10ms (vs 50-100ms linear search)            │
└──────────────────────────────────────────────────────────────┘
```

---

## 📊 PERFORMANCE COMPARISON

### Scenario: Find "123 Phố Huế, Hà Nội" in 100,000 addresses

| Method | Algorithm | Nodes/Records Visited | Time | Scalability |
|--------|-----------|----------------------|------|-------------|
| **Linear Search** | Brute force | 100,000 | 50-100ms | O(n) - Poor |
| **R-tree Spatial Index** | Tree traversal | ~50-100 | 5-10ms | O(log n) - Excellent |

### With 1,000,000 addresses:

| Method | Nodes Visited | Time | Difference |
|--------|---------------|------|-----------|
| Linear | 1,000,000 | 500-1000ms | 10x worse |
| R-tree | ~60-150 | 8-15ms | Only 1.5x slower! |

**Key Insight:** R-tree scales logarithmically, linear search scales linearly!

---

**Document Version:** 1.0  
**Last Updated:** October 19, 2025
