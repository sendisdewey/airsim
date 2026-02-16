# SIMPLE FLIGHT SIMULATOR

Top-down military flight simulator. Buy planes, destroy tanks and AA, earn money, and survive.

---

## FEATURES

- **Huge World** - 16000x16000 pixel map
- **4 Aircraft** - SU-27, SU-30, SU-34, SU-57
- **Ground Units** - T-72B3 tanks and Pantsir-S1 AA
- **Weapons** - Machine gun and guided missiles
- **Shop** - Buy new planes with in-game cash
- **Economy** - Destroy targets, earn money, save for SU-57
- **Environment** - Forests, clouds, seamless grass
- **Navigation** - Arrow pointing to airfield

---

## CONTROLS

| Key | Action |
|-----|--------|
| W | Throttle up |
| S | Throttle down / Brake |
| A | Turn left |
| D | Turn right |
| Space | Drop bomb |
| F | Machine gun (hold) |
| R | Fire guided missile (cost: $10) |
| M | Open shop (on ground) |
| ESC | Exit game |

---

## GAMEPLAY MECHANICS

### Money System
- Destroy tank: +$20
- Destroy AA (Pantsir-S1): +$30
- Landing at airfield: +$50
- Missile cost: -$10

### Aircraft Shop

| Plane | Price | Speed Bonus | Armor Bonus | Bomb Bonus |
|-------|-------|-------------|-------------|------------|
| SU-27 | $0 | +0 | +0 | +0 |
| SU-30 | $2000 | +2 | +10 | +0 |
| SU-34 | $3000 | +3 | +15 | +40 |
| SU-57 | $5000 | +5 | +20 | +60 |

### Weapons

- **Bombs** - Area damage, destroy everything in radius
- **Machine Gun** - Rapid fire, great for light targets
- **Guided Missiles** - Lock onto nearest target, good against AA

### Airfield
- Land to refuel, reload bombs, and repair armor
- Landing gives +$50 bonus
- Shop available only when on ground

### AA (Pantsir-S1)
- Shoots at you when you're close
- Tracer rounds are yellow
- Takes 3 hits to destroy

---

## COMPILATION

### Requirements
- Qt5 development libraries
- C++ compiler (g++)

### Build (linux)
```bash
g++ main.cpp -o airsim $(pkg-config --cflags --libs Qt5Widgets)
```

## Build (windows)

### Option 1: Using MinGW (Recommended)

1. Install **MSYS2** from https://www.msys2.org
2. Open **MSYS2 UCRT64** terminal and install:
```bash
pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-qt5
```
