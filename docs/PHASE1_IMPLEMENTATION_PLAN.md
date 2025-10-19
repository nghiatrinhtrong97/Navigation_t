# Phase 1 Implementation Plan: Foundation Enhancement (Weeks 1-4)

**Project:** Geocoder Modernization - Address Data Model & Spatial Indexing  

## 🏗️ HIGH-LEVEL ARCHITECTURE

### System Overview
```
┌─────────────────────────────────────────────────────────────┐
│                   Geocoding Service API                      │
│  (NavigationMainWindow → POIService::geocodeAddress)        │
└───────────────────────┬─────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│                  Enhanced POI Service                        │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  1. Address Parser (libpostal)                      │    │
│  │     - Parse freeform text → structured components   │    │
│  │     - Extract: street, city, state, zip, etc.       │    │
│  └─────────────────────────────────────────────────────┘    │
│                        │                                     │
│                        ▼                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  2. Address Normalizer                              │    │
│  │     - Expand abbreviations (St → Street)            │    │
│  │     - Normalize case, punctuation                   │    │
│  │     - Standardize directionals (North, N, etc.)     │    │
│  └─────────────────────────────────────────────────────┘    │
│                        │                                     │
│                        ▼                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  3. Spatial Index Query (R-tree)                    │    │
│  │     - Input: Normalized address → Bounding Box      │    │
│  │     - Query: Find candidates within 1km radius      │    │
│  │     - Output: Top-K address candidates              │    │
│  └─────────────────────────────────────────────────────┘    │
│                        │                                     │
│                        ▼                                     │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  4. String Matching & Ranking                       │    │
│  │     - Compare normalized input vs candidates        │    │
│  │     - Score by: edit distance, component match      │    │
│  │     - Return best match with confidence score       │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
                        │
                        ▼
┌─────────────────────────────────────────────────────────────┐
│              EnhancedGeocodingResult                         │
│  {lat, lon, confidence, quality, formatted_address, ...}     │
└─────────────────────────────────────────────────────────────