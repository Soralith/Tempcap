# tempcap

**Control your Intel CPU's thermal throttle limit from the command line.**

`tempcap` reads and writes the **TCC (Thermal Control Circuit) offset** on modern Intel
processors, the same setting that Windows OEM tools like Armoury Crate or Lenovo Vantage
control via their "CPU temperature limit" profiles.

No config files. No daemons. One binary.

## Requirements

- Intel CPU — **12th gen (Alder Lake) or newer** recommended; older generations may not expose the sysfs interface
- Linux kernel with the Intel ISA / Innovation Engine driver (mainline since ~5.18)
- Root access to *write* the limit (`sudo`)

## Install

```sh
git clone https://github.com/Soralith/Tempcap.git
cd tempcap
make
sudo make install
```

## Usage

```
tempcap                    Show current offset and effective max temp
tempcap set 15             Throttle at 85°C (offset = Tjmax - 85)
tempcap set-temp 85        Same — but you type your desired temp directly
tempcap reset              Remove the cap, use full Tjmax (100°C)
tempcap --help             Show help
tempcap --version          Show.. version, of course.
```

### Examples

| Command | Result |
|---|---|
| `tempcap` | `TCC offset: 21°C → Effective max: 79°C` |
| `sudo tempcap set-temp 85` | Throttle starts at 85°C |
| `sudo tempcap reset` | Full 100°C available |

## How it works

`tempcap` writes to the sysfs file
`/sys/devices/pci0000:00/0000:00:04.0/tcc_offset_degree_celsius`
which controls the TCC offset on Intel SoCs. The offset is subtracted
from Tjmax (typically 100°C) to get the actual temperature at which
the CPU starts throttling.

This is the same register that Windows OEM software writes, and the
value **persists across reboots** until you change it or the OEM
software reapplies it.
