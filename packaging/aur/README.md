# AUR PKGBUILDs — sakura-lslc

This directory contains the Arch Linux [PKGBUILD](https://wiki.archlinux.org/title/PKGBUILD)
files used to publish `sakura-lslc` to the
[Arch User Repository (AUR)](https://aur.archlinux.org/).

## Packages

| Subdirectory       | AUR package        | Source                                |
|--------------------|--------------------|---------------------------------------|
| `sakura-lslc/`     | `sakura-lslc`      | Latest tagged release (`v$pkgver`)    |
| `sakura-lslc-git/` | `sakura-lslc-git`  | Tip of `main` (`git+https://...`)     |

End users install via any AUR helper, e.g.:

```sh
yay -S sakura-lslc        # stable, follows releases
yay -S sakura-lslc-git    # rolling, follows main
```

The two packages `provides`/`conflicts` each other, so only one can be
installed at a time.

## Release pipeline

When CI tags a new release, it:

1. Bumps `pkgver` and refreshes `sha256sums` in `sakura-lslc/PKGBUILD`
   (replacing the placeholder `SKIP`) via `updpkgsums`.
2. Regenerates `.SRCINFO` with `makepkg --printsrcinfo > .SRCINFO`.
3. Pushes the updated `PKGBUILD` + `.SRCINFO` to the AUR git remote:

   ```sh
   git remote add aur ssh://aur@aur.archlinux.org/sakura-lslc.git
   git push aur master
   ```

The `-git` flavor is push-published on the same cadence but does not embed
a fixed `pkgver`; its `pkgver()` function resolves the version at build
time from `git describe --long --tags`.

## Local sanity check

```sh
cd sakura-lslc          # or sakura-lslc-git
makepkg --printsrcinfo  # validate metadata
makepkg -si             # build + install locally
```
