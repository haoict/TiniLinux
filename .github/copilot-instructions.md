# Copilot Instructions for TiniLinux

These notes make AI agents productive quickly in this Buildroot-based distro. Focus on the actual patterns in this repo — not generic advice.

**Big Picture**
- **Buildroot External Tree:** This repo is a Buildroot BR2_EXTERNAL that defines boards, overlays, and custom packages. Build output lives in `output.<board>` created via an out-of-tree build. See [external.desc](external.desc) and [external.mk](external.mk).
- **Boards as Variants:** Each board name maps 1:1 to a defconfig in [configs/](configs). Matching board directories live under [board/](board) for BOOT, rootfs overlays, and board-specific assets.
- **Custom Packages:** All packages reside in [package/](package) and are auto-included by [external.mk](external.mk). A top-level [Config.in](Config.in) exposes package menus grouped by function.
- **Init System + Kernel:** Systemd-based images with board-specific kernels and U-Boot patches defined in each `*_defconfig`. Example: [h700_sway_defconfig](configs/h700_sway_defconfig).

**Repo Layout**
- **Boards:** [board/h700](board/h700), [board/rgb30](board/rgb30), [board/pc_qemu_aarch64_virt](board/pc_qemu_aarch64_virt), plus `_squashfs`, `_sway`, `_consoleonly` variants. Each has `BOOT/`, `rootfs/`, and sometimes `overlay_upper/` (for squashfs root).
- **Configs:** [configs/](configs) holds all `<board>_defconfig` and toolchain-only defconfigs.
- **Packages:** Examples: [package/initramfs](package/initramfs), [package/simple-launcher](package/simple-launcher), [package/mesa3d-no-llvm](package/mesa3d-no-llvm), [package/rk3566-dtbo](package/rk3566-dtbo).
- **Tooling:** [make-board-build.sh](make-board-build.sh) bootstraps an out-of-tree Buildroot output; [Dockerfile](Dockerfile) provides a reproducible build environment.
- **Docs:** Start with [README.md](README.md). QEMU notes in [board/pc_qemu_aarch64_virt/README.md](board/pc_qemu_aarch64_virt/README.md).

**Build Workflow**
- **Bootstrap build dir:**
  - `./make-board-build.sh configs/<board>_defconfig` → creates `output.<board>` and wires `BR2_EXTERNAL`.
- **Configure and build:**
  - `cd output.<board>` → `make menuconfig` (optional) → `make -j$(nproc)`.
- **Image creation:**
  - `make img` invokes [external.mk](external.mk) which selects either [mk-flashable-img-rootless.sh](board/common/mk-flashable-img-rootless.sh) or [mk-flashable-img-squashfs-rootless.sh](board/common/mk-flashable-img-squashfs-rootless.sh) based on presence of `rootfs.squashfs`.
- **Flash to SD:**
  - `make flash` runs [board/common/flash-to-sdcard.sh](board/common/flash-to-sdcard.sh) with the current board.
- **QEMU (virt boards):**
  - `make run-qemu` (headless) or `make run-qemu-gui` (GTK) from `output.<board>`; see the helper targets in [external.mk](external.mk).

**Images and Partitions**
- **Partition metadata:** Per-board sizing is defined in `rootfs/root/partition-info.sh` (e.g., [pc_qemu_aarch64_virt](board/pc_qemu_aarch64_virt/rootfs/root/partition-info.sh)).
- **BOOT content:** `Image`, `initramfs`, device trees, and `extlinux.conf` (e.g., [board/h700/BOOT/extlinux/extlinux.conf](board/h700/BOOT/extlinux/extlinux.conf)).
- **Rootfs:**
  - ext4 flow: `rootfs.tar` extracted into partition by `populatefs-*` binaries.
  - squashfs flow: BOOT includes `rootfs.squashfs`; writeable overlay comes from `overlay_upper/`.

**Custom Package Patterns**
- **Structure:** Each package has `Config.in` and `<name>.mk`. Register in the top-level [Config.in](Config.in) to appear in menuconfig.
- **generic-package:** Packages use Buildroot’s `$(eval $(generic-package))`. Example build/install steps in [simple-launcher.mk](package/simple-launcher/simple-launcher.mk).
- **Defconfig-aware builds:** It’s common to branch behavior on `$(BR2_DEFCONFIG)` substrings to set platform flags, e.g., `PLATFORM=h700` in [simple-launcher.mk](package/simple-launcher/simple-launcher.mk).
- **Initramfs:** Packaged via [package/initramfs/initramfs.mk](package/initramfs/initramfs.mk) which builds BusyBox and emits `images/initramfs` for BOOT.

**Conventions and Gotchas**
- **Defconfig naming:** Board name equals defconfig basename without `_defconfig` and equals the `output.<board>` directory name.
- **Overlays:** `BR2_ROOTFS_OVERLAY` composes common + board overlays (see [h700_sway_defconfig](configs/h700_sway_defconfig)). Place files in `board/<board>/rootfs` (ext4) or `overlay_upper` (squashfs overlay).
- **Phony helpers:** `img`, `flash`, `clean-target`, `run-qemu`, `run-qemu-gui` are defined in [external.mk](external.mk) and run from `output.<board>`.
- **Toolchains:** Toolchain-only defconfigs live in [configs/](configs) (e.g., `toolchain_x86_64_aarch64_defconfig`) for building SDKs without a full image.
- **Docker builds:** See [README.md](README.md) for container usage.

**Examples**
- Add a new package: copy a minimal pair, adjust names, add to [Config.in](Config.in), then rebuild: `make <pkg>-dirclean && make` from `output.<board>`.
- Add a new board: create `configs/<board>_defconfig` + `board/<board>/{BOOT,rootfs}` and optional `overlay_upper` or `ROMS`; update DT/UBoot options in defconfig.

If anything here seems off or incomplete for your workflow, tell me which board/flow you’re targeting and I’ll refine these notes.
