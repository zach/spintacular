# PROTOTYPE: the haul in a browser

A playable, throwaway version of the current design for playtests. It follows the rules in `docs/design/` and the layout rules in `docs/design/presentation.md`: every wedge face up, each wheel's chance to spoil the load, crossed-out wedges that would spoil it, the quota line on the truck, and payouts shown step by step.

```bash
python3 prototypes/web/build.py
```

Builds `spintacular-haul.html` from `template.html` and the numbers in `prototypes/haul/tables.json`, so the two prototypes always use the same tables. Open the file in a browser to play.

- **Mouse:** click a wheel to spin it; click Deliver.
- **Keyboard:** number keys spin wheels; Space delivers.

It leaves out rigs, stakes, unlocks, sound, and seeing the next inspection's rule ahead of time.
