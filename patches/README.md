# Device patches for sm7225-common

Framework patches needed by the sm7225 devices (e.g. gts7xllite / SM-T736B).
They live outside the forked repos so that anyone forking this tree can apply
them on top of a clean `repo sync` of any LineageOS 21.0 based ROM.

## Applying

After `repo sync`, from anywhere in the source tree:

```bash
device/samsung/sm7225-common/patches/apply.sh
```

The script maps `patches/<repo path>/*.patch` onto the matching repository
under the Android root and applies each patch with `git am` in filename order,
preserving authorship and commit messages.

## Contents

| Patch | Target | What it does |
|-------|--------|--------------|
| `frameworks/base/0001-sm7225-disable-sparkle-noise-in-ripple-effects.patch` | `frameworks/base` | Removes the sparkle/turbulence noise overlay from ripple animations which renders as white grain on the SDE display stack. |
| `frameworks/base/0002-keyguard-don-t-arm-delayed-keyguard-while-dreaming-w.patch` | `frameworks/base` | Stops the DELAYED_KEYGUARD alarm while dreaming when no lockscreen is configured; previously each screensaver start woke the display ~5s later via `UNLOCK_DREAMING`. |

## Binary blob patches (separate mechanism)

`patch_*.py` files are **not** applied by `apply.sh`. They are binary patches
for extracted vendor blobs, invoked by `blob_fixup()` in `extract-files.sh`
during `extract-files.sh` runs. `patch_bqh_vendor.py` is kept for
documentation only — the blob it patched was replaced by an AOSP source
build (`libstagefright_bufferqueue_helper_vendor`).

## Reverting

```bash
cd frameworks/base && git reset --hard m/lineage-21.0   # or the remote branch you synced
```
