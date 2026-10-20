# Title artwork

The PS5 build uses the original graphics from the pinned DKR-R project:

- **Icon:** `assets/ui/Icons/DKR-R-Spinning-Icon.png`, credited upstream to
  [POOTERMAN](https://www.deviantart.com/pooterman).
- **Background:** the unmodified cloud/wordmark header linked in its README.
- **Source paths and hashes:** `TITLE_ART_SOURCES.json` at the repository root.

The recipe scales/encodes a 512×512 PNG icon and identical 3840×2160 BC7 DDS
selection/launch images. It does not repaint characters or use extracted ROM
models/textures. The launch overlay clears after the first game presentation.
Icon, selected-title background and launch handoff were confirmed on the test PS5.

The maintainer confirmed permission for the icon/background in this PS5 port,
its installer and updates on 18 October 2026.
[Scope and hashes](../../licenses/CONTENT-PERMISSIONS.json).

The repository includes the recipe and provenance; graphics are fetched as
verified local build inputs. Upstream/artist terms continue to apply. These
images are not assigned CC0 by the port, and a project-code licence does not
grant rights to Nintendo/Rare artwork. Review the actual packaged contents and
preserve attribution before distributing an application.

[Upstream README/credits](https://github.com/ThatGuyMcd/DKR-R/blob/0368f51d769af4d89671a212e3ea8ac8078ce41b/README.md)
· [Display](../../docs/DISPLAY.md) · [Licences](../../docs/LICENSES.md)
