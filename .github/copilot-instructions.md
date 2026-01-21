# Copilot Instructions for TiniLinux

These notes make AI agents productive quickly in this Buildroot-based distro. Focus on the actual patterns in this repo — not generic advice.

**Big Picture**
- **Buildroot External Tree:** This repo is a Buildroot BR2_EXTERNAL that defines boards, overlays, and custom packages. Build output lives in `output.<board>` created via an out-of-tree build alongside a sibling `buildroot/` directory. See [external.desc](external.desc) and [external.mk](external.mk).
- **Boards as Variants:** Each board name maps 1:1 to a defconfig in [configs/](configs). Matching board directories live under [board/](board) for BOOT, rootfs overlays, and board-specific assets. Board variants (e.g., `h700`, `h700_sway`, `h700_rootrw`, `h700_consoleonly`) share configs via fragments.
- **Custom Packages:** All packages reside in [package/](package) and are auto-included via `include $(wildcard $(BR2_EXTERNAL_TiniLinux_PATH)/package/*/*.mk)` in [external.mk](external.mk). A top-level [Config.in](Config.in) exposes package menus grouped by function: "TiniLinux Common Packages" (btop, gptokeyb2, initramfs, etc.), "TiniLinux Graphic Packages" (mesa3d-no-llvm, retroarch, simple-launcher, etc.), and "TiniLinux RK3566 Packages" (rk3566-dtbo).
- **Init System + Kernel:** Systemd-based images with board-specific kernels (Linux 6.18.2) and U-Boot patches defined in each `*_defconfig`. Example: [h700_sway_defconfig](configs/h700_sway_defconfig).
- **Architecture:** Targets embedded ARM64 devices (Rockchip RK3326/RK3566, Allwinner H700) with GPU acceleration via Panfrost Mesa driver. Also supports QEMU virtual boards (`pc_qemu_aarch64_virt`) and Raspberry Pi 3B (including `pi3b_docker` variant with Docker support).

**Repo Layout**
- **Boards:** [board/h700](board/h700), [board/rgb30](board/rgb30), [board/pc_qemu_aarch64_virt](board/pc_qemu_aarch64_virt), [board/pi3b](board/pi3b) plus `_rootrw`, `_sway`, `_consoleonly`, `_docker` variants. Default configs (h700, rgb30) use squashfs rootfs. Each board dir contains:
  - `BOOT/` - bootloader assets, device trees, extlinux config
  - `rootfs/` - overlay files for ext4 rootrw variants
  - `overlay_upper/` - overlay files for squashfs variants (persistent overlay partition)
  - `ROMS/` - optional RetroArch configs and cores (mainly in `board/common/ROMS`)
- **Configs:** [configs/](configs) holds all `<board>_defconfig` and toolchain-only defconfigs. Most defconfigs use fragments via `BR2_DEFCONFIG_FRAGMENT` to reduce duplication. Example: `h700_defconfig` is just 2 lines referencing fragments and overlay paths.
- **Config Fragments:** [configs/fragments/](configs/fragments) contains reusable config fragments: `common.fragment` (shared by all), `h700.fragment`/`rgb30.fragment`/`pi.fragment` (board-specific), `with-graphics.fragment` (GUI packages), `rootrw.fragment`, `sway.fragment`, `pc_qemu.fragment`.
- **Packages:** Examples: [package/initramfs](package/initramfs), [package/simple-launcher](package/simple-launcher), [package/mesa3d-no-llvm](package/mesa3d-no-llvm), [package/rk3566-dtbo](package/rk3566-dtbo). Each package has `<name>.mk` (Makefile) and `Config.in` (menu entry).
- **Tooling:** [make-board-build.sh](make-board-build.sh) bootstraps an out-of-tree Buildroot output, auto-clones buildroot 2025.11 if needed, and merges fragments; [Dockerfile](Dockerfile) provides Ubuntu 24.04 build container with all deps.
- **CI/CD:** [.github/workflows/build.yaml](.github/workflows/build.yaml) defines manual workflow_dispatch builds with caching for `dl/` (downloads) and `.buildroot-ccache/` (compiled objects). Supports multiple runner types including GitHub-hosted and self-hosted ARM runners.
- **Docs:** Start with [README.md](README.md). Board-specific notes may exist in `board/<board>/README`.

**Build Workflow**
- **Prerequisites:** Build environment requires `build-essential cmake mtools libncurses-dev dosfstools parted`. The buildroot repo (2025.11 branch) is auto-cloned by [make-board-build.sh](make-board-build.sh) if not present as a sibling `../buildroot/`.
- **Directory structure:** Expected layout is `TiniLinux/` (this repo) and `buildroot/` (auto-cloned) as siblings, with build outputs in `TiniLinux/output.<board>` or `buildroot/output.<board>` (for Docker builds).
- **Bootstrap build dir:**
  - `./make-board-build.sh configs/<board>_defconfig` → creates `output.<board>`, merges fragments if used, and wires `BR2_EXTERNAL`. Pass `docker` as second arg to adjust paths for containerized builds.
- **Configure and build:**
  - `cd output.<board>` → `make menuconfig` (optional) → `make -j$(nproc)`.
- **Save config changes:**
  - `make savefconf` → saves minimal config while preserving `BR2_DEFCONFIG_FRAGMENT` structure. Use this instead of `make savedefconfig` for fragment-based configs. Implemented in [save-fragment-defconfig.sh](board/common/save-fragment-defconfig.sh).
- **Image creation:**
  - `make img` invokes [external.mk](external.mk) which selects either [mk-flashable-img-rootrw-rootless.sh](board/common/mk-flashable-img-rootrw-rootless.sh) or [mk-flashable-img-squashfs-rootless.sh](board/common/mk-flashable-img-squashfs-rootless.sh) based on presence of `rootfs.squashfs`.
- **Flash to SD:**
  - `make flash` runs [board/common/flash-to-sdcard.sh](board/common/flash-to-sdcard.sh) with the current board.
- **QEMU (virt boards):**
  - `make runqemu` (headless) or `make runqemugui` (GTK) from `output.<board>`; see the helper targets in [external.mk](external.mk). For rootrw variants, use `make runqemurootrw`.
- **Rebuild after changes:**
  - Package changes: `make <pkg>-dirclean && make` to force rebuild from scratch.
  - Kernel changes: `make linux-rebuild && make` or `make linux-dirclean && make`.
  - Target cleanup: `make cleantarget` removes staged files and forces target reinstall.

**Images and Partitions**
- **Partition metadata:** Per-board sizing is defined in `rootfs/root/partition-info.sh` (e.g., [pc_qemu_aarch64_virt](board/pc_qemu_aarch64_virt/rootfs/root/partition-info.sh)) with variables like `DISK_SIZE`, `BOOT_SIZE`, `OVERLAY_SIZE`.
- **BOOT content:** `Image`, `initramfs`, device trees, and `extlinux.conf` (e.g., [board/h700/BOOT/extlinux/extlinux.conf](board/h700/BOOT/extlinux/extlinux.conf)). Kernel boot args specify rootfs type.
- **U-Boot offsets:** rgb30 uses 32KiB offset (`seek=64`), h700 uses 8KiB offset (`seek=8`). See [mk-flashable-img-squashfs-rootless.sh](board/common/mk-flashable-img-squashfs-rootless.sh) for dd commands.
- **Rootfs:**
  - squashfs flow (default): BOOT includes `rootfs.squashfs`; writeable overlay comes from `overlay_upper/`. Kernel args: `bootpart=/dev/vda1 squashfsimg=rootfs.squashfs overlayfs=/dev/vda2`.
  - ext4 flow (rootrw variants): `rootfs.tar` extracted into partition by `populatefs-*` binaries. Kernel args: `root=/dev/vda2`.

**Custom Package Patterns**
- **Structure:** Each package has `Config.in` and `<name>.mk`. Register in the top-level [Config.in](Config.in) to appear in menuconfig.
- **generic-package:** Packages use Buildroot’s `$(eval $(generic-package))`. Example build/install steps in [simple-launcher.mk](package/simple-launcher/simple-launcher.mk).
- **Defconfig-aware builds:** It’s common to branch behavior on `$(BR2_DEFCONFIG)` substrings to set platform flags, e.g., `PLATFORM=h700` in [simple-launcher.mk](package/simple-launcher/simple-launcher.mk).
- **Initramfs:** Packaged via [package/initramfs/initramfs.mk](package/initramfs/initramfs.mk) which builds BusyBox and emits `images/initramfs` for BOOT.

**Conventions and Gotchas**
- **Defconfig naming:** Board name equals defconfig basename without `_defconfig` and equals the `output.<board>` directory name.
- **Overlays:** `BR2_ROOTFS_OVERLAY` composes common + board overlays (see [h700_sway_defconfig](configs/h700_sway_defconfig)). Place files in `board/<board>/rootfs` for ext4 rootrw variants or `overlay_upper` for squashfs variants (default).
- **Phony helpers:** `img`, `flash`, `clean-target`, `savefconf`, `runqemu`, `runqemugui` are defined in [external.mk](external.mk) and run from `output.<board>`.
- **Toolchains:** Toolchain-only defconfigs live in [configs/](configs) (e.g., `toolchain_aarch64_defconfig`, `toolchain_x86_64_defconfig`) for building reusable cross-compilation SDKs without a full image.
- **Docker builds:** [Dockerfile](Dockerfile) creates an Ubuntu 24.04 container with user `ubuntu:ubuntu`, all build deps, and mounts for TiniLinux source and buildroot cache. Use `./make-board-build.sh configs/<board>_defconfig docker` to adjust paths for containerized builds. See [README.md](README.md) for full workflow.

**Config Fragments System**
- **Fragment-based defconfigs:** Most defconfigs use `BR2_DEFCONFIG_FRAGMENT` to reference multiple fragment files, dramatically reducing duplication.
- **How it works:** [make-board-build.sh](make-board-build.sh) merges fragments at build setup time. [save-fragment-defconfig.sh](save-fragment-defconfig.sh) preserves fragment structure when saving.
- **Fragment hierarchy:** Common settings → board-specific → variant-specific (graphics/squashfs/sway). Example: h700_sway uses `common.fragment + h700.fragment + with-graphics.fragment + sway.fragment`.
- **Adding packages:** Edit the appropriate fragment (usually `common.fragment` or `with-graphics.fragment`) or add unique settings to board defconfig, then rebuild. Use `make savefconf` to save changes correctly.
- **Creating fragments:** Define in `configs/fragments/`, reference via `BR2_DEFCONFIG_FRAGMENT="$(BR2_EXTERNAL_TiniLinux_PATH)/configs/fragments/name.fragment"` in defconfig.

**Examples**
- **Add a new package:** Create `package/<name>/<name>.mk` and `package/<name>/Config.in`, register in top-level [Config.in](Config.in), rebuild: `cd output.<board> && make <pkg>-dirclean && make`.
- **Add a new board:** Create `configs/<board>_defconfig` (ideally using fragments) + `board/<board>/{BOOT,rootfs}` and optional `overlay_upper` or `ROMS`; set DT/U-Boot/kernel options in defconfig or fragments.
- **Modify package build:** Edit `package/<name>/<name>.mk`, then `cd output.<board> && make <pkg>-dirclean && make` to force rebuild from scratch.
- **Quick rootfs iteration:** For squashfs builds, modify `board/<board>/overlay_upper/`, rebuild: `make cleantarget && make && make img`. For rootrw, modify `board/<board>/rootfs/`.

**Testing and Debugging**
- **QEMU testing:** Use `pc_qemu_aarch64_virt` or `pc_qemu_aarch64_virt_consoleonly` configs for rapid kernel/userspace testing without hardware. Build, then `cd output.pc_qemu_aarch64_virt && ZIP=0 make img && make runqemu` (headless) or `make runqemugui` (with virtio-gpu). For rootrw variants, use `make runqemurootrw`.
- **Kernel/initramfs debugging:** Kernel args in `board/<board>/BOOT/extlinux/extlinux.conf` control boot behavior. Add `console=ttyAMA0` (QEMU/serial) or `console=tty0` (physical display). Initramfs source in [package/initramfs](package/initramfs).
- **Build failures:** Check `output.<board>/build/<pkg>-<ver>/` for logs. For persistent failures, `make <pkg>-dirclean` and retry. For target install issues, `make cleantarget` forces reinstall of all target packages.
- **CI builds:** GitHub Actions workflow supports manual dispatch with board selection. Uses Actions cache for `dl/` and `.buildroot-ccache/` to speed up repeated builds. See [.github/workflows/build.yaml](.github/workflows/build.yaml) for cache key patterns.

If anything here seems off or incomplete for your workflow, tell me which board/flow you're targeting and I'll refine these notes.
