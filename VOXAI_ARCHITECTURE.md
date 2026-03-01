# VoxAI Headless Engine Architecture
Author: DJ BATTYVOX

## Project Scope
A proprietary, AI-first 3D game engine built via a hard fork and decapitation of the Godot source code, designed to be exclusively puppeteered by VoxAI_IDE.

## 1. Executive Summary
The goal of this project is to create a seamless "prompt-to-game" development environment. Rather than building a 3D rendering and physics pipeline from scratch or relying on an unoptimized external engine, this project will hard-fork the open-source Godot engine. The fork will be heavily stripped of its native editor and unnecessary modules, creating an ultra-lean, headless client. VoxAI_IDE will act as the master brain, passing real-time generation and manipulation commands to the engine's core C++ loop via a dedicated local network protocol.

## 2. Core Architecture: The "3rd Arm" Model
The system relies on a strict Client-Server architecture running locally.

- **The Brain (VoxAI_IDE)**: Handles all LLM inference, user prompt parsing, code generation, and asset logic. It sits entirely outside the engine's memory space.
- **The Puppet (Custom Engine)**: A deeply gutted version of Godot's C++ core. It handles purely rendering, physics math, and runtime execution.
- **The Bridge (RPC/TCP Hook)**: A hardcoded networking listener injected directly into the engine's `Main::iteration()` loop, awaiting JSON payloads from the IDE.

## 3. Hardware & Resource Optimization Target
Running agentic text-to-game generation locally requires rigorous memory management. Because the system utilizes an RX 6600 with 8GB of VRAM, standard heavy-duty engines (like Unreal 5) or even a fully loaded Godot Editor would cause severe bottlenecking when run alongside local LLM inference.
By purging the engine's native UI and compiling a custom, stripped-down `.exe`, the engine's VRAM and system RAM footprint will be minimized, reserving maximum hardware overhead for the AI models.

## 4. Implementation Phases

### Phase 1: The Severing (Repository Setup)
- Clone the upstream Godot C++ repository.
- Delete the `.git` directory and initialize a new, independent repository.
- Establish the baseline build environment using the `scons` build system.

### Phase 2: The Great Purge (Stripping the Core)
- **Editor Removal**: Systematically excise the `editor/` directory from the source code to enforce a headless, IDE-driven workflow.
- **Module Disabling**: Utilize the `custom.py` build configuration to disable unnecessary engine modules (e.g., VisualScript, 2D physics engines if strictly making 3D, unused audio parsers).
- **Goal**: Achieve a compilation time and binary size significantly smaller than the standard engine.

### Phase 3: The C++ Brain Injection
- Navigate to `main/main.cpp` (the engine's heartbeat).
- Write and inject a low-level C++ TCP/WebSocket listener that initializes the moment the engine boots.
- Define the command structure (e.g., how the engine receives a command to parse a newly generated GDScript file or GLSL shader).

### Phase 4: The Agentic Pipeline (VoxAI_IDE Integration)
- Standardize the JSON payload format sent from VoxAI_IDE to the custom engine.
- Implement specific commands:
  - **SPAWN**: Instructs the engine's C++ core to instantiate a primitive or mesh.
  - **COMPILE_ATTACH**: Passes a generated script file path to the engine to be compiled and attached to an active node in memory.
  - **MODIFY**: Targets specific memory addresses or Node IDs to alter properties like material colors, transforms, or shaders in real-time. Drawing on methodologies used when analyzing and mapping out legacy engine asset pipelines, the IDE will keep a precise index of all spawned objects to manipulate them instantly.

## 5. Risk Management & Maintenance
- **Upstream Isolation**: By cutting ties with the main Godot branch, future rendering API updates (Vulkan/OpenGL) or OS-level windowing changes will need to be maintained internally.
- **Mitigation Strategy**: Keep modifications isolated to the networking hook and the `editor/` purge so that core rendering math remains stable and untouched unless absolutely necessary.
