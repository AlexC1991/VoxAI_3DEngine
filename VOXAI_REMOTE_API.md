# VoxAI 3D Engine Remote Control API Specification
**Version:** 0.2
**Status:** Draft
**Author:** DJ BATTYVOX

## 1. Overview
The VoxAI Remote Control API defines the low-level TCP protocol used by VoxAI_IDE to control and orchestrate the VoxAI 3D Engine runtime. 

The engine acts as a deterministic execution layer. All orchestration, reasoning, and generation occurs externally in VoxAI_IDE.

## 2. Connectivity

| Field | Value |
| :--- | :--- |
| **Transport** | TCP |
| **Default Port** | 5555 |
| **Encoding** | UTF-8 |
| **Message Format** | JSON |
| **Packet Delimiter** | `\n` (newline) |

## 3. Authentication
Every request MUST include the master key.
- **Key**: `VOXAI_MASTER_BRAIN`

## 4. Message Envelopes
### 4.1 Request / Response Envelope
**Request:**
```json
{
  "id": 1,
  "key": "VOXAI_MASTER_BRAIN",
  "cmd": "COMMAND_NAME",
  "...": "args"
}
```
**Response:**
```json
{
  "id": 1,
  "cmd": "COMMAND_NAME",
  "status": "OK | ERROR",
  "code": 200,
  "data": {},
  "error": null
}
```

### 4.2 Asynchronous Event Envelope (Push)
```json
{
  "type": "EVENT",
  "event_class": "RUNTIME_ERROR",
  "data": { "message": "..." }
}
```

## 5. Command Set

### 5.1 `PING`
Verifies engine connectivity.

### 5.2 `LOAD_SCENE`
Loads and attaches a scene resource. Supports `replace` or `add` modes.
```json
{ "cmd": "LOAD_SCENE", "path": "res://...", "name": "ActiveLevel", "mode": "replace" }
```

### 5.3 `SPAWN_NODE`
Creates a new node and attaches it to a parent.
```json
{ "cmd": "SPAWN_NODE", "type": "Node3D", "parent": "/root", "name": "Bot" }
```

### 5.4 `FREE_NODE`
Destroys a node safely.

### 5.5 `INJECT_SCRIPT`
Compiles and attaches **Base64 encoded** GDScript code.
```json
{ "cmd": "INJECT_SCRIPT", "target": "/root/Node", "code": "[BASE64_CODE]" }
```

### 5.6 `CALL_METHOD`
Executes a function on a node's script.
```json
{ "cmd": "CALL_METHOD", "target": "/root/Node", "method": "move", "args": [1, 2] }
```

### 5.7 `SET_PROPS` / `GET_STATE`
Direct property manipulation and state queries.

### 5.8 `GET_TREE`
Returns a JSON map of the active scene graph.

### 5.9 `QUIT`

## 6. Threading Model
- **Networking Thread**: Accepts connections, parses JSON, and pushes commands to a **Mutex Queue**.
- **Main Thread**: Processes up to 50 commands (or 8ms) per frame during the frame loop. All Scene Tree mutations occur here.

## 7. Error Codes
- `200`: Success
- `400`: Malformed JSON
- `401`: Unauthorized
- `404`: Not Found
- `500`: Internal Error (e.g. Script Compile Failure)
