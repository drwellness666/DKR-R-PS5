# Upstream DKR-R title graphics

The PS5 title now uses the actual DKR-R project graphics. 

- Icon: assets/ui/Icons/DKR-R-Spinning-Icon.png from the pinned DKR-R source,
  used by its launcher. Upstream credits the application icon to
  [POOTERMAN](https://www.deviantart.com/pooterman).
- Background: the unmodified image linked at the top of the pinned DKR-R README,
  with the Recompiled wordmark on the project sky/cloud background.
- Source paths, URL and SHA-256 values: TITLE_ART_SOURCES.json.

The format pipeline only scales/encodes these inputs: 512x512 PNG icon and two identical
3840x2160 single-level BC7_UNORM DX10 DDS selection/launch backgrounds. No figure, facial expression,
cap, balloon, logo or scenery is repainted or regenerated. Both title-selection and launch use the same upstream header. The game releases
the launch overlay after its first completed presentation. This does not imply a Nintendo/Sony endorsement.

The public integration includes the recipe and provenance. The graphics are
obtained as verified public build inputs; console-ready output stays in the
private local build tree. No extracted ROM model or texture is used by this
recipe. Preserve upstream and artist terms: these graphics are not assigned
CC0 by the port. The project source's MIT licence does not grant Nintendo/Rare
rights. Review asset terms separately before distributing a packaged app/artwork.

Source: [pinned DKR-R README and credits](https://github.com/ThatGuyMcd/DKR-R/blob/c3eda4d368afbc729ed4a71312cce38d27f87c37/README.md).
Formats: [public native template](https://github.com/blackbearreloaded/ps5-native-app-boilerplate/blob/main/docs/PLATFORM_NOTES.md).
Icon, selected-title background and launch-image release have been confirmed on the tested PS5.

See [title presentation](../../docs/DISPLAY.md) for paired images,
Shell registration boundaries and the qualified display path.
