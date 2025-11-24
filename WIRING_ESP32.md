# ESP32 Wiring Guide - Quick Reference

## Standard Wiring (Default Pins)

### Connection Table

| LD1125H Pin | Wire Color | ESP32 Pin | Function |
|-------------|------------|-----------|----------|
| Pin 1 (VCC) | Red | 5V or 3.3V | Power Supply |
| Pin 2 (GND) | Black | GND | Ground |
| Pin 3 (URX) | Green | GPIO 17 | Radar RX ← ESP32 TX |
| Pin 4 (UTX) | Yellow | GPIO 16 | Radar TX → ESP32 RX |

## Visual Wiring Diagram

```
LD1125H Radar Sensor              ESP32 Dev Kit V1
(Front View)                      (Top View)

     ┌─────────┐
  ●──┤1 2 3 4 │                   ┌───────────────┐
     └─────────┘                  │               │
       │ │ │ │                    │   ┌─────┐     │
       │ │ │ │                    │   │ USB │     │
       │ │ │ │                    │   └─────┘     │
       │ │ │ └───────┐            │               │
       │ │ │         │            │  GND ●────────┼─── GND (2)
       │ │ └─────┐   │            │  3.3V●        │
       │ │       │   │            │  5V  ●────────┼─── VCC (1)
       │ └───┐   │   │            │ ....          │
       │     │   │   │            │               │
       └─┐   │   │   │            │ GPIO16●───────┼─── UTX (4)
         │   │   │   │            │ GPIO17●───────┼─── URX (3)
         │   │   │   │            │               │
        VCC GND TX  RX            │               │
        (1) (2) (3) (4)           └───────────────┘
```

## Pin Mapping

### LD1125H Pins (2mm pitch, 4-pin connector)
```
 [1]  [2]  [3]  [4]
 VCC  GND  URX  UTX
```

**Pin Functions:**
- **Pin 1 (VCC)**: 5V Power Input (3.3V-5V accepted)
- **Pin 2 (GND)**: Ground
- **Pin 3 (URX)**: Radar Serial Receive (TTL 3.3V)
- **Pin 4 (UTX)**: Radar Serial Transmit (TTL 3.3V)

### ESP32 Dev Kit V1 Pins
```
Left Side:              Right Side:
3V3                     GND
EN                      GPIO23
GPIO36                  GPIO22
GPIO39                  GPIO1 (TX0)
GPIO34                  GPIO3 (RX0)
GPIO35                  GPIO21
GPIO32                  GND
GPIO33                  GPIO19
GPIO25                  GPIO18
GPIO26                  GPIO5
GPIO27                  GPIO17 ◄── Radar URX (Pin 3)
GPIO14                  GPIO16 ◄── Radar UTX (Pin 4)
GPIO12                  GPIO4
GND ◄── Radar GND       GPIO0
GPIO13                  GPIO2
GPIO9                   GPIO15
GPIO10                  GND
GPIO11                  5V  ◄── Radar VCC (Pin 1)
5V
```

## Step-by-Step Wiring

### Step 1: Power Connections
1. Connect **LD1125H Pin 1 (VCC)** to **ESP32 5V** pin (or 3.3V if preferred)
2. Connect **LD1125H Pin 2 (GND)** to **ESP32 GND** pin

### Step 2: Serial Connections (CRITICAL!)
3. Connect **LD1125H Pin 3 (URX)** to **ESP32 GPIO 17** (ESP32 TX)
4. Connect **LD1125H Pin 4 (UTX)** to **ESP32 GPIO 16** (ESP32 RX)

**Remember**: RX and TX are CROSSED
- Radar **U**RX ← ESP32 **T**X
- Radar **U**TX → ESP32 **R**X

### Step 3: Verify
- Double-check all connections
- Ensure no short circuits
- Verify power polarity (VCC = +, GND = -)

## Power Options

### Option 1: USB Power (Recommended for Development)
- Connect ESP32 to computer via USB cable
- ESP32 provides 5V to radar from USB power
- Good for development and testing

### Option 2: External 5V Supply
- Use 5V regulated power supply (500mA minimum)
- Connect to ESP32 5V and GND pins
- More stable for production use

### Option 3: 3.3V Power
- Can power radar from ESP32 3.3V pin
- Works fine, radar accepts 3.3-5V
- Less power available (max 600mA from ESP32)

## Custom Pin Configuration

If GPIO 16/17 are not available, you can use other pins:

### Recommended Alternative Pins
- **RX options**: GPIO 25, 26, 27, 32, 33
- **TX options**: GPIO 25, 26, 27, 32, 33, 18, 19, 21, 22, 23

### Update Code for Custom Pins
Edit these lines in the code:
```cpp
#define RADAR_RX 25  // Change to your chosen RX pin
#define RADAR_TX 26  // Change to your chosen TX pin
```

### Example Alternative Wiring
```cpp
// Using GPIO 25 and 26 instead of 16 and 17
#define RADAR_RX 25
#define RADAR_TX 26
```

Then connect:
- LD1125H Pin 3 (URX) → ESP32 GPIO 26
- LD1125H Pin 4 (UTX) → ESP32 GPIO 25

## Testing Connections

### 1. Power Test
After connecting power:
- ESP32 power LED should light up
- LD1125H may have a small LED indicator

### 2. Serial Communication Test
Upload the code and open Serial Monitor (115200 baud):
```
[INIT] Radar serial initialized at 115200 baud (RX=GPIO16, TX=GPIO17)
[RADAR] Raw data: occ, dis=1.50  ← Should see this!
```

If you see `[RADAR] Raw data:` messages, wiring is correct!

### 3. Movement Test
Wave your hand in front of the radar:
```
[RADAR] Raw data: mov, dis=2.30
[DETECT] MOVING at 2.3m
```

## Common Wiring Mistakes

### ❌ Mistake 1: RX/TX Not Crossed
```
Wrong:
Radar URX → ESP32 RX (GPIO16) ✗
Radar UTX → ESP32 TX (GPIO17) ✗

Correct:
Radar URX → ESP32 TX (GPIO17) ✓
Radar UTX → ESP32 RX (GPIO16) ✓
```

### ❌ Mistake 2: Wrong Power Voltage
- Don't connect to GPIO pins for power
- Use VCC (5V or 3.3V) rail pins only
- Check polarity: Red = VCC, Black = GND

### ❌ Mistake 3: No Common Ground
- ESP32 GND must connect to Radar GND
- Required for serial communication
- Use same ground for power supply

## Breadboard Wiring Example

```
                    Breadboard
    ┌───────────────────────────────────┐
    │                                   │
    │  [LD1125H Radar]                  │
    │   1   2   3   4                   │
    │   │   │   │   │                   │
  ──┼───┼───┼───┼───┼──  5V Power Rail
    │   │   │   │   │                   │
    │   └───┼───┘   │                   │
  ──┼───────┼───────┼──  GND Rail
    │       │       │                   │
    │      ┌┴┐     ┌┴┐                  │
    │      │G│     │G│                  │
    │      │P│     │P│                  │
    │      │I│     │I│                  │
    │      │O│     │O│                  │
    │      │1│     │1│                  │
    │      │7│     │6│                  │
    │      └─┘     └─┘                  │
    │       │       │                   │
    │   ┌───┴───────┴───┐               │
    │   │   ESP32        │               │
    │   │   Dev Kit V1   │               │
    │   └────────────────┘               │
    └───────────────────────────────────┘
```

## Troubleshooting Wiring Issues

### No Radar Data Received

**Check List:**
1. ✓ Is radar powered? (Check VCC and GND)
2. ✓ Are RX/TX crossed correctly?
3. ✓ Is common ground connected?
4. ✓ Are you using GPIO 16 and 17?
5. ✓ Is radar working? (Test with USB-Serial adapter)

### Intermittent Data

**Possible Causes:**
- Loose connections → Re-seat all wires
- Bad breadboard contacts → Try different holes
- Interference → Shorten wires, add decoupling capacitor

### ESP32 Keeps Resetting

**Power Issue:**
- Insufficient power supply → Use better USB cable
- Voltage drop → Add 100µF capacitor near ESP32
- Current spike → Use external 5V power supply

## Additional Components (Optional)

### Decoupling Capacitor
Add 100µF capacitor between VCC and GND near radar:
```
        100µF
VCC ────||──── GND
```
Benefits:
- Reduces noise
- Stabilizes power
- Prevents brownouts

### Pull-up Resistors (Usually Not Needed)
If communication is unreliable, try 4.7kΩ pull-ups:
```
        4.7kΩ          4.7kΩ
VCC ─────/\/\/\─── TX   ─────/\/\/\─── RX
              │                    │
```

## Summary

✅ **4 connections required**: VCC, GND, RX, TX
✅ **Cross RX/TX**: Radar URX → ESP32 TX17, Radar UTX → ESP32 RX16
✅ **Common ground** is essential
✅ **5V or 3.3V** power both work
✅ **Test with Serial Monitor** to verify

Your wiring is correct when you see:
```
[RADAR] Raw data: mov, dis=2.50
[DETECT] MOVING at 2.5m
```

Happy building! 🚀
