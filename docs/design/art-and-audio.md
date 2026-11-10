# Art and audio direction

The look and sound of the game. `presentation.md` says what must be on screen; this document says what it should look and sound like. The brief left the style open, with no need for a retro look. This direction is a proposal until the product owner picks one.

## The direction: road signs and loading docks

The game looks like the working world of freight: highway signs, dock markings, shipping labels and painted steel. It's flat, bold and built to be read at a glance, because the whole game is about reading odds quickly.

- **Shapes:** flat color and thick outlines, like signage. No gradients standing in for depth, and no pixel art.
- **Palette:**
  - **Ground:** concrete grey.
  - **Frame:** highway-sign green, for everything that frames the game.
  - **Signal yellow:** kept for the things that matter most: the quota line, warnings and inspections.
  - **Cargo:** four colors, each with its own shape (circle, triangle, square, diamond), so color is never the only signal.
- **Type:** a condensed highway-style face for numbers and headings, and a plain companion for text. Numbers are the stars: big, tabular and always in the same place.
- **Wheels:** painted steel discs with stencilled wedges and a fixed pointer. A wedge that would spoil the load gets a cross and fades.
- **The truck:** drawn side-on, with cargo stacking on the bed as it lands. The quota is a painted fill line on the truck's side. The truck sinks on its springs as the load grows.
- **Items:** enamel pins and dashboard trinkets on the cab's sun visor, in the order they apply.
- **The shop:** a truck stop. Offers sit on a counter, and the bag of wedges is a parts tray.
- **Inspections:** an official form stamped across the round's header.

The browser prototype (`prototypes/web/`) already uses this palette, type and shape language in a bare form.

## Motion

- **The spin:** the wheel slows over about a second, and the last few ticks carry the tension. The wedge under the pointer lights as it passes.
- **A delivery:** the tailgate drops, and each scoring step stamps its number onto a manifest, faster as the total grows.
- **A spoiled load:** the cargo slides off the bed. It's quick and it isn't softened.
- **Everything else stays still.** Motion marks a spin, a save, a spoil or a payout, and nothing else.

## Sound

- **The wheel:** a ratchet tick that slows with the wheel.
- **Risk:** a low engine note rises with each wheel's chance to spoil, so the player hears the danger before reading it.
- **The quota:** a rising tone as the load nears it, and a bell when it's reached.
- **A save:** a ratchet strap snapping tight.
- **A spoil:** a crash, then a beat of silence.
- **A delivery:** an air-brake hiss, then a cash-register run that speeds up with the payout.
- **Music:** road music in layers: a steady rhythm in the shop, with a layer added for each cargo in the truck, so the track fills as the load does. A full truck brings the whole band in.

## Two alternatives considered

| Direction | For it | Against it |
| --- | --- | --- |
| **Carnival midway:** a striped game-show wheel, bulbs and prize tickets | Fits the "wheel of fortune" name directly; loud and cheerful | It's the look of CloverPit and many slot roguelikes, and it leans toward casino feel, which the haul has moved away from |
| **Tabletop:** wooden wheels, chunky tokens and a felt table | Matches the board games the rules come from (Flip 7, Quacks, Azul); warm | Harder to make odds read at a glance, and the truck and quota have no natural place on a table |

The freight direction wins because the rules now have a subject of their own: loading a truck. A look built on that subject makes the rules easier to read, and no other game in the genre has it.
