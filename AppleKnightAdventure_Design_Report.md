# APPLE KNIGHT ADVENTURE

## Software Architecture and Design Report

**Course:** Software Design Patterns - CS202  
**Class group:** Group 55  
**University:** University of Science - VNUHCM  
**Faculty:** Faculty of Information Technology  
**Prepared:** Ho Chi Minh City, August 2026

**Analysis basis:** The complete current project source tree was reviewed, including the 2D Adventure client, application shell, Map Builder, rendering and UI stack, Survival3D mode, persistence layer, and separately built Survival3D HTTP backend. The report is organized around the implemented design rather than repository history.  
**Verification basis:** Static dependency and call-flow tracing across 185 project C++ header/source files, CMake reconfiguration, and successful builds of both `AppleKnightAdventure.exe` and `AegisRiftServer.exe`.

## Group Information

| Student ID | Full name |
|---|---|
| 25125074 | Nguyễn Anh Kiệt |
| 25125037 | Nguyễn Trọng Tiến |

## Table of Contents

1. Executive Summary
2. Project Scope and Source Coverage
3. Architectural Drivers and Design Principles
4. System Architecture and Runtime Composition
5. Adventure Domain Design
6. Gameplay Systems and Collaboration
7. Map Builder and Object Creation Design
8. Rendering, Animation, and UI Architecture
9. Survival3D Architecture
10. Persistence and Online Service Design
11. Applied Design Patterns
12. Design Reasoning and Consequences
13. End-to-End Runtime Flows
14. Build and Reproduction
15. Conclusion
16. Source Evidence Index

<!-- pdf-body -->

