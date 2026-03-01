# VoxAI 3DEngine

A proprietary, AI-first 3D game engine built via a hard fork and decapitation of the Godot source code. Designed to be exclusively puppeteered by VoxAI_IDE.

## Overview

Rather than building a 3D rendering and physics pipeline from scratch or relying on an unoptimized external engine, this project hard-forks the open-source Godot engine. The fork will be heavily stripped of its native editor and unnecessary modules, creating an ultra-lean, headless client. VoxAI_IDE will act as the master brain, passing real-time generation and manipulation commands to the engine's core C++ loop via a dedicated local network protocol.

See `VOXAI_ARCHITECTURE.md` for the full technical specification and project outline.

## Building (Phase 1)
Compiled using `scons` under Windows x86_64.
```bash
scons p=windows target=editor -j8
```
