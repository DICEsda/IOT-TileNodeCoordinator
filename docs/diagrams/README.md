# Smart Tile System - Architecture Diagrams

This folder contains PlantUML diagrams documenting the system architecture and interactions.

## Sequence Diagrams

1. `pairing_sequence.puml` - Shows the pairing process between a node and the coordinator
   - Button press initiation
   - Join request/accept flow
   - Credential exchange
   - State transitions

2. `presence_detection.puml` - Illustrates the presence detection and light control flow
   - mmWave sensor detection
   - Zone mapping
   - Light control with thermal consideration
   - MQTT status updates

3. `thermal_management.puml` - Details the thermal management process
   - Temperature monitoring
   - Derating decisions
   - Status reporting
   - Coordinator thermal management

## Class Diagram

`class_diagram.puml` - Shows the overall system architecture
   - Coordinator components
   - Node components
   - Shared components
   - Relationships and dependencies

## Viewing the Diagrams

These diagrams are written in PlantUML format. To view them:

1. Use VS Code with the PlantUML extension
2. Use the online PlantUML server at http://www.plantuml.com/plantuml
3. Use the PlantUML CLI tool

## Diagram Conventions

- Solid lines indicate direct relationships
- Dashed lines indicate dependencies
- Colors are used to group related components
- Activation bars show the flow of control
- Notes provide additional context where needed
