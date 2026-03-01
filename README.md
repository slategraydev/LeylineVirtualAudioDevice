# Leyline Audio Driver (C++)

A high-performance virtual audio driver for Windows, re-implemented in C++ from the original Rust codebase. Work in progress.

## Project Layout

```
LeylineAudioDriverCpp/
├── driver/                   # Kernel-mode driver (C++17, WDM)
│   ├── include/
│   │   ├── leyline_common.h    # Shared types: RingBuffer, SharedParameters, IOCTL codes
│   │   ├── leyline_guids.h     # All KS / PortCls GUIDs and constant IDs
│   │   ├── leyline_descriptors.h # KS descriptor table declarations
│   │   └── leyline_miniport.h  # Miniport class declarations + DeviceExtension
│   ├── src/
│   │   ├── driver.cpp          # DriverEntry, DriverUnload
│   │   ├── adapter.cpp         # AddDevice, StartDevice, IRP dispatch, CDO
│   │   ├── wavert.cpp          # CMiniportWaveRT, CMiniportWaveRTStream
│   │   ├── topology.cpp        # CMiniportTopology
│   │   └── descriptors.cpp     # All KS descriptor tables & property handlers
│   ├── leyline.inx             # INF template (identical to Rust project)
│   └── sources                 # eWDK NMAKE build file
├── scripts/
│   ├── LaunchBuildEnv.ps1      # eWDK environment initializer
│   ├── Install.ps1             # Build → deploy → verify pipeline
│   └── Uninstall.ps1           # VM uninstall wrapper
├── test/
│   └── EndpointTester/         # C# tool to enumerate audio endpoints
├── package/                    # Staged build artifacts (gitignored)
├── Makefile                    # GNU Make task aliases
└── README.md
```

## Architecture

The driver exposes four PortCls subdevices:

| Name              | Class     | Role                                |
|-------------------|-----------|-------------------------------------|
| `WaveRender`      | WaveRT    | Renders audio from client apps      |
| `WaveCapture`     | WaveRT    | Captures audio to client apps       |
| `TopologyRender`  | Topology  | Volume/mute nodes for render path   |
| `TopologyCapture` | Topology  | Bridging for the capture path       |

Physical connections mirror the Rust project:
- `WaveRender (pin 1)` → `TopologyRender (pin 0)`
- `TopologyCapture (pin 1)` → `WaveCapture (pin 1)`

## Building

### Prerequisites

- **eWDK** (Enterprise WDK) at `D:\eWDK_28000` or set `LEYLINE_EWDK_ROOT`.
- Windows 10 / 11 SDK `10.0.28000.0` or set `LEYLINE_SDK_VERSION`.
- A Hyper-V VM named `LeylineTestVM` with snapshot `LeylineSnapshot`.

### Quick Start

```powershell
# Full build + deploy to VM
.\scripts\Install.ps1

# Build only (no VM)
.\scripts\Install.ps1 -fast

# Clean rebuild
.\scripts\Install.ps1 -clean

# Uninstall from VM
.\scripts\Uninstall.ps1

# Check endpoints on the VM after install
cd test\EndpointTester && dotnet run
```

### Environment Variables

| Variable              | Default            | Description                        |
|-----------------------|--------------------|------------------------------------|
| `LEYLINE_EWDK_ROOT`   | `D:\eWDK_28000`   | Root of the eWDK installation      |
| `LEYLINE_SDK_VERSION` | `10.0.28000.0`    | WDK / SDK version string           |
| `LEYLINE_VM_NAME`     | `TestVM`   | Hyper-V VM name                    |
| `LEYLINE_VM_SNAPSHOT` | `Leyline` | Snapshot to revert before testing  |
| `LEYLINE_VM_USER`     | `USER`            | VM administrator username          |
| `LEYLINE_VM_PASS`     | *(required)*      | VM administrator password          |
| `LEYLINE_CERT_PASS`   | *(required)*      | Code-signing certificate password  |

## Key Design Decisions vs. Rust

- **COM via PortCls C++ helpers**: `CUnknown` / `DECLARE_STD_UNKNOWN()` handle ref-counting,
  eliminating the manual vtable boilerplate needed in Rust's `no_std` environment.
- **`/kernel` flag**: Suppresses C++ features incompatible with kernel mode (exceptions, RTTI).
- **Pool tags**: Every `new` call uses a four-byte pool tag (e.g., `'LLWS'`) for leak tracking.
- **Descriptor tables**: Statically initialized in `.rdata`, same as the Rust `#[link_section]` approach.
- **No CRT**: Builds against `wdm.lib` only; `RtlCopyMemory` replaces `memcpy`.

