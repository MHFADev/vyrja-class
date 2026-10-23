# Vyrja Classroom: design direction

Read this before changing any UI. The theme lives in `core/resources/vyrja-theme.qss`,
its tokens in `core/src/VyrjaTheme.cpp`, the brand constants in `core/src/Branding.h`.

**Design read.** A desktop classroom-control app for teachers who are not technical, mostly
Indonesian speaking, in a warm, flat style anchored on the Vyrja Studio mascot logo.
Dials: ENERGY 2 / RHYTHM 1 / MOTION 1. A tool people work in all day stays calm and uniform on
purpose, and the only motion is hover and press feedback.

## Palette (unchanged, taken from the logo)

| Role | Value | Where |
|---|---|---|
| Navy | `#26236B` | chrome (header, sidebar brand), default buttons, headings, text on orange |
| Orange | `#FF6B4A` | the single accent: Quick class, Quick Setup, timer, selection fill, focus |
| Cream | `#FAF9FD` | window ground, logo plaque |

Functional additions, derived from the palette rather than new brand colors:

| Token | Value | Why it exists |
|---|---|---|
| `OrangeStrong` | `#E5522E` | strokes and small indicators on light surfaces: 3.8:1 on white, the brand orange is 2.8:1 |
| `StatusOnline` | `#2E9E5B` | online computers |
| `StatusOffline` | `#8582AA` | offline computers |
| `StatusProblem` | `#E5522E` | computers with a connection or access problem |
| `@CONTROL_BORDER@` | `#8582AC` light, `#7773C4` dark | outline of inputs, buttons, checkboxes: 3:1 so a control is visible as a control |
| `@HEADING@` | navy light, `#CFCDF7` dark | titles; the dark theme primary is only 2.6:1 on its surface |

All three status colors are at least 3:1 on white, on the navy header and on the dark surfaces.

## Decisions and the reason for each

1. **Text on orange is navy, never white.** White on `#FF6B4A` is 2.82:1 (fails AA 4.5:1), navy is 4.86:1. Same for selected text (`HighlightedText`).
2. **Header is flat navy. No gradient, no orange rule under it.** The gradient served no hierarchy and the rule made orange appear in one more place. Orange stays a single accent.
3. **The mark in the header, the wordmark where there is room.** The mark (cat and V) sits on a cream plaque so it reads on navy and on dark. The wordmark shows in the empty state, About and the setup wizard. Use `VyrjaTheme::logo()`, never scale the PNG by hand: it is sharp on high-DPI screens.
4. **State is shape plus color.** Online is a filled dot, offline a ring, problem a square with "!". Computer cards and the header counters use the same shapes, so state survives color blindness and a washed-out projector.
5. **Counters in the header.** "Who is online, who is not" is the first thing a teacher wants to know, so it moved from small print in the status bar to the header. Only the selection count stays in the status bar, next to the grid controls.
6. **Radius scale: 8 controls and chips, 12 groups and cards.** Not every element is a pill; the old 13 to 15px pills made a chip look the same as a button.
7. **Hover is a tone step, not a shadow.** Cards and buttons sit flat. The selected card gets the orange tint and a 3px `OrangeStrong` stroke.
8. **Selected page in Vyrja Setup is a filled row, not a colored left stripe.** The stripe carried no information the fill does not.
9. **Visible focus everywhere.** Inputs, buttons, tool buttons and checkboxes get a 2px stroke on focus; padding is reduced by 1px at the same time so nothing shifts. The navy default button gets an orange ring.
10. **The empty state explains and offers.** Why it is empty, then "Choose a room" and "Search actions". It used to be a paragraph of text.
11. **No hex colors in C++.** Widgets get an `objectName` and the theme styles it, so light and dark stay in sync. The only colors left in C++ are in `ComputerItemDelegate` (custom painting) and they come from `Branding`.

## Assets added by the refresh

- `core/resources/vyrja-mark.png` (217x256) and `vyrja-logo-wide.png` (640x293): the Vyrja Studio logo, downscaled with Lanczos from `branding/vyrja-mark.png` and `branding/vyrja-logo-transparent.png`, now registered in `core.qrc` and used through `VyrjaTheme::logo()`. Regenerate them from `branding/` if the logo changes.
- `core/resources/quick-class.png`: 128px, the mark centered on a cream (`#FAF9FD`) rounded plaque, radius 28. It is the icon of the orange Quick class button; the plain app icon disappeared into the orange.
- `core/resources/check-navy.png`: `check-white.png` recolored to navy `#26236B`, keeping its alpha. A white check on the orange checkbox fill was 2.8:1.

## Rules for the next change

- Pair every status color with a shape.
- Use `@ON_ACCENT@` for text on orange. Check any new foreground/background pair at 4.5:1 (text) or 3:1 (icons, borders, strokes).
- Add a token to both the light and the dark table in `VyrjaTheme.cpp` before using it in the QSS.
- Header rules start with `#HeaderBar #Name`. `#HeaderBar QLabel` is more specific than a plain `#Name`, and would win.
- rgba() alpha in QSS is an integer 0 to 255.
- No gradients, no glow, no shadow at rest. New icons follow `branding/icons/generate_icons.py`.
