from machine import Pin
import time

# List of all ESP32-C3 GPIO pins
esp32_c3_gpio = [
    0, 1, 2, 3, 4, 5,
    6, 7, 8, 9, 10, 11,
    12, 13, 14, 15, 16, 17,
    18, 19, 20, 21
]

# Loop through each GPIO pin
for pin_num in esp32_c3_gpio:
    try:
        led = Pin(pin_num, Pin.IN)   # assign pin as output
        print("Testing GPIO", pin_num)

        # led.value(1)   # turn LED on
        # print("GPIO", pin_num, "ON")
        print("GPIO", pin_num, led.value())
        
        time.sleep(3) # keep it on for 10 seconds

        # led.value(0)   # turn LED off
        print("GPIO", pin_num, led.value())
        time.sleep(2)  # short pause before next pin

    except Exception as e:
        print("GPIO", pin_num, "not usable:", e)
