# Leyline Internal Architecture

The Leyline Virtual Audio Device uses Microsoft's PortCls (Port Class) framework. 
It defines two miniports:
1. **CMiniportWaveRT**: Handles audio streaming, allocating the `LoopbackMdl` ring buffer, and exposes physical channels.
2. **CMiniportTopology**: Exposes the volume, mute, and peak meters interfaces to Windows Audio.

## Loopback DPC
A `KTIMER` fires every 1ms at `DISPATCH_LEVEL`. The `LoopbackDpcRoutine` copies samples from the Render streams into the Capture streams through a master `LoopbackMdl` ring buffer while applying Volume and Mute properties.

## Hardware Position Registers
For zero-latency position reporting, the DPC updates memory-mapped variables sent to WASAPI via `GetPositionRegister` and `GetClockRegister`.
