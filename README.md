# Hall Sensor Parking Simulator

**Arduino project** — Real-time vehicle parking occupancy detection.

## Hardware
- Hall effect sensors (x6) for vehicle detection
- LEDs for slot status indication
- Arduino Uno/Nano

## Code
`hall_parking.ino` — monitors 6 parking slots via Hall sensors, displays live occupancy via Serial, drives status LEDs.

## Wiring
```
Hall sensor 1 → Pin 2    LED 1 → Pin 8
Hall sensor 2 → Pin 3    LED 2 → Pin 9
Hall sensor 3 → Pin 4    LED 3 → Pin 10
Hall sensor 4 → Pin 5    LED 4 → Pin 11
Hall sensor 5 → Pin 6    LED 5 → Pin 12
Hall sensor 6 → Pin 7    LED 6 → Pin 13
```

## Skills
Sensor Integration · State Management · Serial Communication
