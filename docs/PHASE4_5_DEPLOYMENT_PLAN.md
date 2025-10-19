# PHASE 4 & 5 IMPLEMENTATION PLAN: Cloud Deployment & Production

**Project:** Automotive Navigation System - Geocoder Modernization  
**Phases:** 4-5 (Weeks 13-20)  
**Status:** 📋 PLANNING - Production Deployment  
**Dependencies:** Phase 1-3 Complete

---

## 🎯 OVERVIEW

### **Phase 4 (Weeks 13-16): Cloud Infrastructure**
- Deploy to Google Cloud Platform (GCP)
- Set up GKE (Google Kubernetes Engine)
- Migrate to Cloud Spanner database
- Configure CI/CD pipelines

### **Phase 5 (Weeks 17-20): Data & Monitoring**
- Data ingestion pipelines
- Real-time monitoring & alerting
- Performance optimization
- Production hardening

---

## 📦 PHASE 4: CLOUD DEPLOYMENT

### **WEEK 13: GCP Infrastructure Setup**

**Objective:** Establish cloud infrastructure foundation

**Components:**

**1. Project Structure**
```
deployment/
├── gcp/
│   ├── terraform/
│   │   ├── main.tf              # Main infrastructure
│   │   ├── variables.tf          # Configuration variables
│   │   ├── gke.tf                # Kubernetes cluster
│   │   ├── spanner.tf            # Cloud Spanner database
│   │   ├── networking.tf         # VPC, Load Balancers
│   │   └── iam.tf                # Permissions & service accounts
│   ├── kubernetes/
│   │   ├── namespace.yaml
│   │   ├── geocoding-service/
│   │   │   ├── deployment.yaml
│   │   │   ├── service.yaml
│   │   │   ├── hpa.yaml         # Horizontal Pod Autoscaler
│   │   │   └── configmap.yaml
│   │   ├── reverse-geocoding/
│   │   │   ├── deployment.yaml
│   │   │   └── service.yaml
│   │   └── autocomplete/
│   │       ├── deployment.yaml
│   │       └── service.yaml
│   └── scripts/
│       ├── setup-gcp.sh
│       ├── deploy.sh
│       └── rollback.sh
└── docker/
    ├── Dockerfile.geocoding       # Geocoding service container
    ├── Dockerfile.reverse          # Reverse geocoding container
    ├── Dockerfile.autocomplete     # Autocomplete container
    └── docker-compose.yml          # Local testing
```

**2. Terraform Configuration**

**GKE Cluster (gke.tf)**
```hcl
# deployment/gcp/terraform/gke.tf
resource "google_container_cluster" "geocoding_cluster" {
  name     = "geocoding-prod-cluster"
  location = var.gcp_region
  
  # Node pool configuration
  initial_node_count = 3
  
  node_config {
    machine_type = "n2-standard-4"  # 4 vCPUs, 16GB RAM
    disk_size_gb = 100
    disk_type    = "pd-ssd"
    
    # OAuth scopes
    oauth_scopes = [
      "https://www.googleapis.com/auth/cloud-platform"
    ]
    
    # Labels
    labels = {
      environment = "production"
      service     = "geocoding"
    }
    
    # Taints for dedicated workloads
    taint {
      key    = "workload-type"
      value  = "geocoding"
      effect = "NO_SCHEDULE"
    }
  }
  
  # Autoscaling
  cluster_autoscaling {
    enabled = true
    resource_limits {
      resource_type = "cpu"
      minimum       = 12
      maximum       = 48
    }
    resource_limits {
      resource_type = "memory"
      minimum       = 48
      maximum       = 192
    }
  }
  
  # Monitoring
  monitoring_config {
    enable_components = ["SYSTEM_COMPONENTS", "WORKLOADS"]
  }
  
  # Logging
  logging_config {
    enable_components = ["SYSTEM_COMPONENTS", "WORKLOADS"]
  }
}

# Node pool for geocoding workloads
resource "google_container_node_pool" "geocoding_pool" {
  name       = "geocoding-node-pool"
  cluster    = google_container_cluster.geocoding_cluster.name
  node_count = 3
  
  autoscaling {
    min_node_count = 3
    max_node_count = 10
  }
  
  node_config {
    machine_type = "n2-highmem-4"  # Memory-optimized for ML
    
    labels = {
      workload = "geocoding"
    }
  }
}
```

**Cloud Spanner (spanner.tf)**
```hcl
# deployment/gcp/terraform/spanner.tf
resource "google_spanner_instance" "geocoding_instance" {
  name             = "geocoding-prod"
  config           = "regional-${var.gcp_region}"
  display_name     = "Geocoding Production Database"
  num_nodes        = 3  # For high availability
  processing_units = 1000
  
  labels = {
    environment = "production"
    service     = "geocoding"
  }
}

resource "google_spanner_database" "geocoding_db" {
  instance = google_spanner_instance.geocoding_instance.name
  name     = "geocoding"
  
  ddl = [
    <<-EOT
    CREATE TABLE Addresses (
      address_id STRING(36) NOT NULL,
      street_number STRING(20),
      street_name STRING(200),
      city STRING(100),
      state STRING(50),
      zip_code STRING(20),
      country STRING(50),
      latitude FLOAT64,
      longitude FLOAT64,
      confidence FLOAT64,
      embedding ARRAY<FLOAT64>,  -- ML embeddings
      created_at TIMESTAMP NOT NULL OPTIONS (allow_commit_timestamp=true),
      updated_at TIMESTAMP NOT NULL OPTIONS (allow_commit_timestamp=true)
    ) PRIMARY KEY (address_id),
    ROW DELETION POLICY (OLDER_THAN(updated_at, INTERVAL 365 DAY))
    EOT
    ,
    <<-EOT
    CREATE INDEX idx_location ON Addresses(latitude, longitude)
    EOT
    ,
    <<-EOT
    CREATE INDEX idx_city_state ON Addresses(city, state)
    EOT
    ,
    <<-EOT
    CREATE TABLE GeocodeCache (
      cache_key STRING(256) NOT NULL,
      result_json STRING(MAX),
      hit_count INT64,
      created_at TIMESTAMP NOT NULL OPTIONS (allow_commit_timestamp=true),
      expires_at TIMESTAMP NOT NULL
    ) PRIMARY KEY (cache_key),
    ROW DELETION POLICY (OLDER_THAN(expires_at, INTERVAL 0 DAY))
    EOT
  ]
}
```

**Load Balancer & Networking (networking.tf)**
```hcl
# deployment/gcp/terraform/networking.tf
resource "google_compute_global_address" "geocoding_lb_ip" {
  name = "geocoding-lb-ip"
}

resource "google_compute_managed_ssl_certificate" "geocoding_ssl" {
  name = "geocoding-ssl-cert"
  
  managed {
    domains = ["geocoding.yourdomain.com"]
  }
}

resource "google_compute_backend_service" "geocoding_backend" {
  name        = "geocoding-backend"
  protocol    = "HTTP"
  timeout_sec = 30
  
  backend {
    group = google_container_cluster.geocoding_cluster.instance_group_urls[0]
  }
  
  health_checks = [google_compute_health_check.geocoding_health.id]
  
  # CDN configuration
  enable_cdn = true
  cdn_policy {
    cache_mode = "CACHE_ALL_STATIC"
    default_ttl = 3600
  }
}

resource "google_compute_health_check" "geocoding_health" {
  name               = "geocoding-health-check"
  check_interval_sec = 10
  timeout_sec        = 5
  
  http_health_check {
    port         = 8080
    request_path = "/health"
  }
}
```

**3. Kubernetes Manifests**

**Geocoding Service Deployment**
```yaml
# deployment/gcp/kubernetes/geocoding-service/deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: geocoding-service
  namespace: geocoding-prod
spec:
  replicas: 3
  selector:
    matchLabels:
      app: geocoding-service
  template:
    metadata:
      labels:
        app: geocoding-service
    spec:
      containers:
      - name: geocoding
        image: gcr.io/your-project/geocoding-service:latest
        ports:
        - containerPort: 8080
          name: http
        env:
        - name: SPANNER_INSTANCE
          value: "geocoding-prod"
        - name: SPANNER_DATABASE
          value: "geocoding"
        - name: CACHE_TTL_SECONDS
          value: "86400"
        - name: ML_MODEL_PATH
          value: "/models/address_similarity_model.tflite"
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
        volumeMounts:
        - name: ml-models
          mountPath: /models
          readOnly: true
      volumes:
      - name: ml-models
        configMap:
          name: ml-models
---
apiVersion: v1
kind: Service
metadata:
  name: geocoding-service
  namespace: geocoding-prod
spec:
  type: LoadBalancer
  ports:
  - port: 80
    targetPort: 8080
    protocol: TCP
  selector:
    app: geocoding-service
```

**Horizontal Pod Autoscaler**
```yaml
# deployment/gcp/kubernetes/geocoding-service/hpa.yaml
apiVersion: autoscaling/v2
kind: HorizontalPodAutoscaler
metadata:
  name: geocoding-service-hpa
  namespace: geocoding-prod
spec:
  scaleTargetRef:
    apiVersion: apps/v1
    kind: Deployment
    name: geocoding-service
  minReplicas: 3
  maxReplicas: 20
  metrics:
  - type: Resource
    resource:
      name: cpu
      target:
        type: Utilization
        averageUtilization: 70
  - type: Resource
    resource:
      name: memory
      target:
        type: Utilization
        averageUtilization: 80
  behavior:
    scaleDown:
      stabilizationWindowSeconds: 300
      policies:
      - type: Percent
        value: 50
        periodSeconds: 60
    scaleUp:
      stabilizationWindowSeconds: 0
      policies:
      - type: Percent
        value: 100
        periodSeconds: 30
```

**4. Docker Containers**

**Geocoding Service Dockerfile**
```dockerfile
# deployment/docker/Dockerfile.geocoding
FROM ubuntu:22.04 AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential cmake git \
    qt6-base-dev libqt6network6 \
    libtensorflow-lite-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
WORKDIR /app
COPY . .

# Build
RUN mkdir build && cd build && \
    cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_GUI=OFF && \
    cmake --build . --config Release --parallel

# Runtime image
FROM ubuntu:22.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libqt6core6 libqt6network6 libtensorflow-lite2 \
    && rm -rf /var/lib/apt/lists/*

# Copy binaries
COPY --from=builder /app/build/hmi/Release/geocoding_service /usr/local/bin/

# Copy ML models
COPY models/*.tflite /models/

# Expose port
EXPOSE 8080

# Health check
HEALTHCHECK --interval=30s --timeout=10s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

# Run service
CMD ["/usr/local/bin/geocoding_service", "--port=8080"]
```

---

### **WEEK 14-15: Data Migration to Cloud Spanner**

**Objective:** Migrate address data to Cloud Spanner

**1. Data Migration Script**
```cpp
// tools/spanner_migration.cpp
#include "google/cloud/spanner/client.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace spanner = google::cloud::spanner;

class SpannerMigration {
public:
    SpannerMigration(const std::string& project_id,
                     const std::string& instance_id,
                     const std::string& database_id)
        : m_client(spanner::MakeConnection(
            spanner::Database(project_id, instance_id, database_id))) {}
    
    bool migrateAddresses(const std::string& json_file) {
        std::ifstream file(json_file);
        nlohmann::json addresses = nlohmann::json::parse(file);
        
        std::vector<spanner::Mutation> mutations;
        
        for (const auto& addr : addresses) {
            mutations.push_back(
                spanner::InsertOrUpdateMutationBuilder(
                    "Addresses",
                    {"address_id", "street_number", "street_name", "city", 
                     "state", "zip_code", "country", "latitude", "longitude"}
                ).EmplaceRow(
                    addr["id"].get<std::string>(),
                    addr["street_number"].get<std::string>(),
                    addr["street_name"].get<std::string>(),
                    addr["city"].get<std::string>(),
                    addr["state"].get<std::string>(),
                    addr["zip_code"].get<std::string>(),
                    addr["country"].get<std::string>(),
                    addr["latitude"].get<double>(),
                    addr["longitude"].get<double>()
                ).Build()
            );
            
            // Batch commit every 10,000 records
            if (mutations.size() >= 10000) {
                auto commit_result = m_client.Commit(std::move(mutations));
                if (!commit_result.ok()) {
                    std::cerr << "Commit failed: " << commit_result.status() << std::endl;
                    return false;
                }
                mutations.clear();
            }
        }
        
        // Commit remaining
        if (!mutations.empty()) {
            auto commit_result = m_client.Commit(std::move(mutations));
            return commit_result.ok();
        }
        
        return true;
    }
    
private:
    spanner::Client m_client;
};
```

**2. Spanner Client Integration**
```cpp
// hmi/services/geocoding/database/spanner_client.h
#pragma once
#include "google/cloud/spanner/client.h"
#include "address_components.h"
#include <vector>

namespace geocoding {
namespace database {

class SpannerClient {
public:
    SpannerClient(const std::string& project_id,
                  const std::string& instance_id,
                  const std::string& database_id);
    
    // Query addresses by location
    std::vector<AddressComponents> queryNearby(double lat, double lon, double radius_km);
    
    // Insert/update address
    bool upsertAddress(const AddressComponents& address);
    
    // Batch operations
    bool batchUpsert(const std::vector<AddressComponents>& addresses);
    
    // Cache operations
    bool cacheResult(const std::string& key, const std::string& result_json, int ttl_seconds);
    std::optional<std::string> getCachedResult(const std::string& key);
    
private:
    google::cloud::spanner::Client m_client;
    
    // Convert between AddressComponents and Spanner rows
    google::cloud::spanner::Row toRow(const AddressComponents& address);
    AddressComponents fromRow(const google::cloud::spanner::Row& row);
};

}} // namespace geocoding::database
```

---

### **WEEK 16: CI/CD Pipeline**

**Objective:** Automated build, test, and deployment

**1. GitHub Actions Workflow**
```yaml
# .github/workflows/deploy-gcp.yml
name: Deploy to GCP

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

env:
  GCP_PROJECT_ID: ${{ secrets.GCP_PROJECT_ID }}
  GKE_CLUSTER: geocoding-prod-cluster
  GKE_ZONE: us-central1
  IMAGE: geocoding-service

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    steps:
    - name: Checkout code
      uses: actions/checkout@v3
    
    - name: Setup Qt
      run: |
        sudo apt-get install -y qt6-base-dev
    
    - name: Build
      run: |
        mkdir build && cd build
        cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON
        cmake --build . --parallel
    
    - name: Run tests
      run: |
        cd build
        ctest --output-on-failure
  
  deploy:
    needs: build-and-test
    runs-on: ubuntu-latest
    if: github.ref == 'refs/heads/main'
    steps:
    - name: Checkout code
      uses: actions/checkout@v3
    
    - name: Authenticate to Google Cloud
      uses: google-github-actions/auth@v1
      with:
        credentials_json: ${{ secrets.GCP_SA_KEY }}
    
    - name: Set up Cloud SDK
      uses: google-github-actions/setup-gcloud@v1
    
    - name: Configure Docker
      run: gcloud auth configure-docker
    
    - name: Build Docker image
      run: |
        docker build -t gcr.io/$GCP_PROJECT_ID/$IMAGE:$GITHUB_SHA \
          -f deployment/docker/Dockerfile.geocoding .
    
    - name: Push to GCR
      run: |
        docker push gcr.io/$GCP_PROJECT_ID/$IMAGE:$GITHUB_SHA
    
    - name: Deploy to GKE
      run: |
        gcloud container clusters get-credentials $GKE_CLUSTER --zone $GKE_ZONE
        kubectl set image deployment/geocoding-service \
          geocoding=gcr.io/$GCP_PROJECT_ID/$IMAGE:$GITHUB_SHA \
          -n geocoding-prod
        kubectl rollout status deployment/geocoding-service -n geocoding-prod
```

---

## 📦 PHASE 5: MONITORING & PRODUCTION

### **WEEK 17-18: Monitoring & Alerting**

**1. Prometheus Metrics**
```cpp
// hmi/services/geocoding/metrics/prometheus_exporter.h
#pragma once
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

namespace geocoding {
namespace metrics {

class PrometheusExporter {
public:
    static PrometheusExporter& instance();
    
    // Counters
    void incrementGeocodeRequests() { m_geocode_requests->Increment(); }
    void incrementGeocodeErrors() { m_geocode_errors->Increment(); }
    void incrementCacheHits() { m_cache_hits->Increment(); }
    void incrementCacheMisses() { m_cache_misses->Increment(); }
    
    // Gauges
    void setActiveConnections(double count) { m_active_connections->Set(count); }
    void setCacheSize(double size) { m_cache_size->Set(size); }
    
    // Histograms
    void observeLatency(double ms) { m_latency->Observe(ms); }
    
    // Expose metrics endpoint
    std::string serializeMetrics();
    
private:
    PrometheusExporter();
    
    std::shared_ptr<prometheus::Registry> m_registry;
    prometheus::Counter* m_geocode_requests;
    prometheus::Counter* m_geocode_errors;
    prometheus::Counter* m_cache_hits;
    prometheus::Counter* m_cache_misses;
    prometheus::Gauge* m_active_connections;
    prometheus::Gauge* m_cache_size;
    prometheus::Histogram* m_latency;
};

}} // namespace geocoding::metrics
```

**2. Cloud Monitoring Alerts**
```yaml
# deployment/gcp/monitoring/alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: geocoding-alerts
  namespace: geocoding-prod
spec:
  groups:
  - name: geocoding
    interval: 30s
    rules:
    - alert: HighErrorRate
      expr: rate(geocode_errors_total[5m]) > 0.05
      for: 5m
      annotations:
        summary: "High error rate in geocoding service"
        description: "Error rate is {{ $value }} errors/sec"
    
    - alert: HighLatency
      expr: histogram_quantile(0.95, rate(geocode_latency_seconds_bucket[5m])) > 0.1
      for: 5m
      annotations:
        summary: "High P95 latency"
        description: "P95 latency is {{ $value }}s"
    
    - alert: LowCacheHitRate
      expr: rate(cache_hits_total[5m]) / (rate(cache_hits_total[5m]) + rate(cache_misses_total[5m])) < 0.7
      for: 10m
      annotations:
        summary: "Cache hit rate below 70%"
```

### **WEEK 19: Load Testing**

**1. K6 Load Test Script**
```javascript
// tests/load/geocoding_load_test.js
import http from 'k6/http';
import { check, sleep } from 'k6';

export const options = {
  stages: [
    { duration: '2m', target: 100 },   // Ramp up to 100 users
    { duration: '5m', target: 100 },   // Stay at 100 for 5 minutes
    { duration: '2m', target: 1000 },  // Spike to 1000 users
    { duration: '5m', target: 1000 },  // Stay at 1000 for 5 minutes
    { duration: '2m', target: 0 },     // Ramp down to 0
  ],
  thresholds: {
    http_req_duration: ['p(95)<100'],   // 95% of requests < 100ms
    http_req_failed: ['rate<0.01'],     // Error rate < 1%
  },
};

const BASE_URL = 'https://geocoding.yourdomain.com';

export default function () {
  // Test geocoding
  const geocodeRes = http.post(`${BASE_URL}/api/geocode`, JSON.stringify({
    freeform_address: '123 Main St, Springfield, IL'
  }), {
    headers: { 'Content-Type': 'application/json' },
  });
  
  check(geocodeRes, {
    'geocode status is 200': (r) => r.status === 200,
    'geocode has result': (r) => JSON.parse(r.body).success === true,
  });
  
  // Test autocomplete
  const autocompleteRes = http.get(`${BASE_URL}/api/autocomplete?q=123+Main`);
  check(autocompleteRes, {
    'autocomplete status is 200': (r) => r.status === 200,
    'autocomplete has suggestions': (r) => JSON.parse(r.body).suggestions.length > 0,
  });
  
  sleep(1);
}
```

### **WEEK 20: Production Hardening**

**1. Security Hardening**
- Enable Cloud Armor for DDoS protection
- Set up VPC Service Controls
- Implement rate limiting
- Add API key authentication
- Enable audit logging

**2. Performance Optimization**
- Enable GCP CDN for static content
- Configure connection pooling
- Optimize Spanner indexes
- Implement request batching

---

## 📊 SUCCESS METRICS

| Metric | Target | Measurement |
|--------|--------|-------------|
| Uptime | >99.9% | Cloud Monitoring |
| P95 Latency | <100ms | Prometheus |
| Error Rate | <0.1% | Prometheus |
| Cache Hit Rate | >80% | Prometheus |
| Throughput | >10K QPS | Load testing |
| Cost per 1M requests | <$50 | GCP Billing |

---

## 💰 ESTIMATED COSTS (Monthly)

| Service | Configuration | Cost |
|---------|--------------|------|
| GKE Cluster | 3 nodes (n2-standard-4) | ~$450 |
| Cloud Spanner | 1000 PU | ~$900 |
| Cloud Storage | 1TB | ~$20 |
| Load Balancer | Global | ~$18 |
| Egress | 1TB | ~$120 |
| **Total** | | **~$1,508/month** |

---

**Document Version:** 1.0  
**Last Updated:** October 19, 2025  
**Status:** 📋 **READY FOR PRODUCTION DEPLOYMENT**
