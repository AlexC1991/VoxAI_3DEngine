# 🌌 VoxAI 3D Engine
**The Definitive Agentic Rendering Layer**

[![Version](https://img.shields.io/badge/version-0.2-blueviolet.svg)](https://github.com/AlexC1991/VoxAI_3DEngine)
[![Status](https://img.shields.io/badge/status-Technical--POC-orange.svg)](https://github.com/AlexC1991/VoxAI_3DEngine)
[![License](https://img.shields.io/badge/license-MIT-green.svg)](LICENSE)

---

## 🚀 Overview
**VoxAI 3D Engine** is a high-performance, real-time rendering and physics execution layer. It is a "decapitated" hard-fork of the Godot Engine, stripped of its native editor and UI to serve as a pure, headless-capable rendering slave exclusively puppeteered by an external AI orchestrator (VoxAI_IDE).

The engine operates on a local network protocol, allowing an agentic "Brain" to generate scenes, inject logic, and manipulate world state in real-time via a low-latency TCP API.

---

## 🛠 Features (v0.2 API)
- **Agentic Puppeteering**: Fully controlled via JSON-over-TCP on port 5555.
- **Dynamic Node Spawning**: Instantiate any Godot class (Node3D, MeshInstance, Camera, etc.) remotely.
- **Live Script Injection**: Push Base64-encoded GDScript directly into active nodes at runtime.
- **Property Mutation**: Real-time manipulation of positions, materials, and internal variables.
- **Deterministic State Retrieval**: Query the engine for node transforms, hierarchy, and custom script states.
- **Optimized Rendering**: Stripped-down core focusing only on the active 3D viewport.

---

## 🏗 Architecture
The VoxAI system splits **Reasoning** from **Execution**:

```mermaid
graph TD
    A[VoxAI_IDE (The Brain)] -- JSON Commands --> B[VoxAI 3D Engine (Rendering Layer)]
    B -- State Feedback --> A
    B -- Visual Feedback --> C[User Viewport]
    
    subgraph "Execution Layer"
        B[VoxAI 3D Engine]
        D[main_loop]
        E[voxai_brain module]
        B --> D
        D --> E
    end
```

---

## 🚦 Getting Started

### Prerequisites
- **Python 3.10+** (for orchestration scripts)
- **Windows x64** (Native target)

### Building from Source
The engine is built using the SCons build system.
```powershell
scons p=windows target=template_debug -j8
```

### Running the Proof-of-Concept
Navigate to the `VoxAI_POC_Dist` folder (or use the provided scripts) and run the following:
1. Start the engine binary:
   ```powershell
   ./bin/godot.windows.template_debug.x86_64.exe
   ```
2. Launch the orchestration brain:
   ```powershell
   python samples/maze_orchestration.py
   ```

---

## 📜 API Documentation
For the full command set and transmission protocol, please see [VOXAI_REMOTE_API.md](VOXAI_REMOTE_API.md).

---

## 👨‍💻 Author
**DJ BATTYVOX**  
*Agentic 3D Virtualization Lead*

---
*Built on the shoulders of the Godot community. Stripped and re-powered for the Agentic Era.*
