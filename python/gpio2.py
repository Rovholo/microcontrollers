import RPi.GPIO as GPIO
import time

led1 = 16

GPIO.setmode(GPIO.BCM)
GPIO.setup(led1, GPIO.IN)

while True :
    input_value = GPIO.input(led1)
    time.sleep(2)
    if not input_value:
        print("" + str(input_value))
    else : print("" + str(input_value))