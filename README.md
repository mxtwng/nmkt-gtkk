# Automatic Towel Drying Rack

A student engineering project exploring a compact towel rack that dries a towel indoors using controlled warm airflow. The goal is to reduce the time a towel stays damp when outdoor drying space is limited.

## Project architecture

The concept has three main parts:

- **Rack and airflow path:** a freestanding frame supports the towel and guides air across it.
- **Sensing and control:** a microcontroller monitors the towel and its environment, then coordinates the drying cycle.
- **Heating and airflow:** a low-voltage fan and heating assembly moves warm air through the rack. The system is intended to stop when drying is complete or a monitored limit is reached.

The current repository contains the project overview and planning document only. Firmware, detailed wiring diagrams, construction drawings, and measured test results will be added as they are ready to share.

## Development status

This is a prototype concept under development. Performance targets are provisional until they have been measured on a physical build. Component choices and the final control behavior may change during testing.

## Repository contents

- `README.md` — public project overview and architecture.
- `firmware/` — planned location for controller code.
- `hardware/` — planned location for public schematics and construction drawings.
- `tests/` — planned location for test procedures and results.

The planned directories will appear when those project files are ready to share.

## Safety

The project is not a finished consumer product. Any prototype involving heat, electricity, or ultraviolet light requires appropriate independent protection and supervised testing before operation.
