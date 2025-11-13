import RPi.GPIO as GPIO
import time

led1 = 12

GPIO.setmode(GPIO.BCM)
GPIO.setup(led1, GPIO.OUT)

while True :
    GPIO.output(led1, True)
    time.sleep(2)
    GPIO.output(led1,False)
    time.sleep(2)