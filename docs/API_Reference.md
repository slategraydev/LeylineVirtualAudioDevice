# IOCTL API Reference

The Leyline Virtual Audio Device exposes a Control Device Object (CDO) at `\\.\LeylineAudio`.

## `IOCTL_LEYLINE_GET_STATUS`
- **Direction**: Output
- **Buffer**: `ULONG`
- **Description**: Returns `0x1337BEEF` if the driver is loaded and the CDO is responding.

## `IOCTL_LEYLINE_MAP_BUFFER`
- **Direction**: Output
- **Buffer**: `PVOID` (Pointer to mapped user-mode memory)
- **Description**: Maps the internal ring buffer into user space. Validates buffer size size equals `sizeof(PVOID)`.

## `IOCTL_LEYLINE_MAP_PARAMS`
- **Direction**: Output
- **Buffer**: `PVOID`
- **Description**: Maps the `LeylineSharedParameters` structure to user space.

## `IOCTL_LEYLINE_CREATE_CABLE`
- **Direction**: Input/Output
- **Buffer**: None
- **Description**: Dynamically spawns an independent generic Render/Capture subdevice pair on the fly without a GUI.
