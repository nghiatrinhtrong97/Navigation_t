# DESIGN DOCUMENT: EMBEDDED NAVIGATION SYSTEM FOR IVI

**Version:** 1.0
**Author:** [Tên của bạn]
**Project:** Instrument Cluster & IVI Navigation Module

## 1. Overview

This document outlines the design and architecture of the core navigation module for the In-Vehicle Infotainment (IVI) system. The system is designed to run on a resource-constrained embedded platform using the QNX Real-Time Operating System.

The primary goals of this module are:
-   To provide fast and memory-efficient routing based on map data.
-   To deliver accurate turn-by-turn guidance.
-   To ensure smooth map rendering performance on embedded GPUs.
-   To maintain positional accuracy even during GPS signal loss.

**Target Hardware:** Embedded SoC (System-on-Chip), e.g., NXP i.MX8 series.
**Target OS:** QNX Neutrino RTOS 7.x

## 2. System Architecture & ECU Interaction

The navigation system is designed as a set of independent, cooperating processes running on the QNX OS. This multi-process architecture enhances system stability; a crash in one service will not bring down the entire system.

### 2.1. High-Level Component Diagram

```plantuml
@startuml
!theme plain
skinparam componentStyle uml2

package "External ECUs / Peripherals" {
  [GPS Receiver] <<Device>> as GPS
  [Vehicle CAN Bus] <<Bus>> as CAN
}

package "IVI System on QNX" {
    [HMI Application (Qt/QML)] <<Process>> as HMI

    package "Navigation Core Services (C++)" {
        [Positioning Service] <<Process>> as PosSvc
        [Map Service] <<Process>> as MapSvc
        [Routing Service] <<Process>> as RouteSvc
        [Guidance Service] <<Process>> as GuideSvc
    }

    [QNX OS Platform] <<OS>>
}

package "Output ECUs" {
    [Instrument Cluster] <<ECU>> as IC
}

' Data Flow
GPS -- [Serial I/O] --> PosSvc : NMEA 0183 sentences
CAN -- [CAN Driver] --> PosSvc : Vehicle Speed, Yaw Rate
PosSvc --> GuideSvc : IPC (Current Position)
MapSvc <--> [Filesystem] : Map Data
RouteSvc --> MapSvc : IPC (Request Map Graph)
GuideSvc --> RouteSvc : IPC (Request Reroute)
HMI -> RouteSvc : IPC (Request Route)
GuideSvc --> HMI : IPC (Guidance Info)
GuideSvc -- [CAN Driver] --> IC : Turn Instructions (CAN Msg)

@enduml
```

### 2.2. Interactions with other ECUs:

-   **Input - GPS Receiver:** The `PositioningService` reads NMEA 0183 standard sentences (like `$GPRMC`, `$GPGGA`) from the GPS module via a serial port (`/dev/ser1`).
-   **Input - CAN Bus:** The `PositioningService` listens on the vehicle's CAN bus for specific message IDs that contain **Vehicle Speed** and **Yaw Rate** (tốc độ quay). This data is critical for Dead Reckoning.
-   **Output - Instrument Cluster (Đồng hồ):** The `GuidanceService` sends simplified instructions (e.g., "turn left icon", "distance to next turn: 200m") to the Instrument Cluster via custom CAN messages. This allows the driver to see navigation cues without looking at the main IVI screen.
-   **Output - IVI Display:** The `HMI Application` is responsible for all complex rendering (map, POIs). It communicates with the backend services via IPC to get the data it needs to display.

## 3. Key Features

This module implements the core logic to support the following features described in the CV:

1.  **Map Data Processing & Routing:**
    -   Parses and loads map data from a custom format into an in-memory graph structure.
    -   Implements the **A\* (A-Star) algorithm** for efficient shortest-path calculation.
2.  **POI & Addressing Services:**
    -   Provides an interface to search for Points of Interest (POIs) and addresses within the loaded map data.
    -   Uses **Map Matching** algorithms to snap imprecise GPS coordinates to the correct road segment, ensuring locational accuracy.
3.  **Performance Optimization:**
    -   **Map Tiling & On-Demand Loading:** The map is pre-processed into tiles. The `MapService` only loads tiles around the current location into memory to keep the RAM footprint low.
    -   **Optimized Rendering Logic:** The backend provides data in a format suitable for rendering with OpenGL ES, using techniques like **Level of Detail (LOD)** (i.e., not drawing small streets when zoomed out).
4.  **Dead Reckoning:**
    -   When GPS signal is lost (e.g., in a tunnel), the `PositioningService` uses the last known position, vehicle speed, and yaw rate from the CAN bus to estimate the current position for a short period.

## 4. Detailed Module Design

### 4.1. `PositioningService`
-   **Responsibility:** To be the single source of truth for the vehicle's position.
-   **Core Logic:**
    -   Continuously reads the serial port for NMEA data from the GPS.
    -   Continuously reads the CAN bus for vehicle speed and yaw rate.
    -   If GPS signal is valid, it applies a Map Matching algorithm to the raw coordinates.
    -   If GPS signal is lost, it switches to Dead Reckoning mode.
    -   Broadcasts the final, corrected position to other services via IPC.

### 4.2. `MapService`
-   **Responsibility:** Manages all access to map data.
-   **Core Logic:**
    -   On startup, it indexes the available map data tiles.
    -   It listens for IPC requests for specific map data (e.g., "give me the graph data for this bounding box" from `RoutingService`).
    -   Implements a cache (e.g., LRU - Least Recently Used) to keep frequently accessed map tiles in memory.

### 4.3. `RoutingService`
-   **Responsibility:** Calculates optimal routes.
-   **Core Logic:**
    -   Waits for an IPC message containing a start and end point.
    -   Requests the relevant map graph data from the `MapService`.
    -   Executes the A* algorithm on the graph.
    -   Returns the calculated route (a sequence of node IDs) via IPC to the requester.

### 4.4. `GuidanceService`
-   **Responsibility:** Provides real-time, turn-by-turn instructions.
-   **Core Logic:**
    -   Receives a route from the `RoutingService`.
    -   Subscribes to position updates from the `PositioningService`.
    -   Continuously compares the vehicle's current position against the route.
    -   Determines the next maneuver (e.g., "turn left at Node #123").
    -   Calculates distances and generates instruction text.
    -   Publishes guidance updates via IPC for the HMI and sends simplified data to the Instrument Cluster via CAN.
    -   If the vehicle deviates significantly from the path, it requests a new route from the `RoutingService`.

## 5. Inter-Process Communication (IPC) on QNX

-   **Mechanism:** We use QNX's native, highly efficient **Message Passing** (`MsgSend`, `MsgReceive`, `MsgReply`). This is a synchronous, server-client model.
-   **Message Format:** A simple, fixed-size C `struct` is used for messages to ensure high performance and avoid dynamic memory allocation in real-time contexts.

```c++
// Example message structure
#define MAX_ROUTE_NODES 1024

enum class MessageType {
    REQUEST_ROUTE,
    ROUTE_RESPONSE,
    CURRENT_POSITION,
    // ... other types
};

struct NavMessage {
    MessageType type;
    union {
        struct {
            Point start;
            Point end;
        } route_request;

        struct {
            int node_count;
            int nodes[MAX_ROUTE_NODES];
        } route_response;

        struct {
            Point current_pos;
            double speed_kmh;
        } position_update;
    };
};
```