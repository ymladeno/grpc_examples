from enum import Enum


class PowerState(Enum):
    ACTIVE = 1
    TRANSITION_TO_ACTIVE = 2
    LPM = 3
    TRANSITION_TO_LPM = 4


class BatteryLevel(Enum):
    FULL = 0
    WARNING = 1
    LOW = 2
    CRITICAL = 3

class BatteryStatus(Enum):
    IDLE = 0
    CHARGING = 1
    DISCHARGING = 2
    NOT_PRESENT = 3
    FAULT = 4

class ButtonType(Enum):
    UNKNOWN = 0
    PANIC = 1
    GENERAL_PURPOSE = 2

class ButtonEvent(Enum):
    UNKNOWN = 0
    SHORT_PRESS = 1
    LONG_PRESS = 2
    ULTRA_LONG_PRESS = 3
    DOUBLE_PRESS = 4
    PRESS = 5
    RELEASE = 6