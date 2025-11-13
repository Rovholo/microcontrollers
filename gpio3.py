import RPi.GPIO as GPIO
from time import time


pin = 12

GPIO.setmode(GPIO.BOARD)

GPIO.setup(pin, GPIO.IN, pull_up_down=GPIO.PUD_DOWN)

def binary_acquire(pin, duration):
    # acquires data as quickly as possible
    t0 = time() # time is in seconds here
    results = []
    while (time() - t0) < duration:
        results.append(GPIO.input(pin))
    return results

print("Acquiring data for 1 second")
# acquire data for 1 second
results = binary_acquire(pin, 1.0)
print("Done!")
print(",".join([str(result) for result in results]))
GPIO.cleanup()