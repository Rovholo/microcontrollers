import RPi.GPIO as GPIO # Import Raspberry Pi GPIO library
import time

gpio = 16

GPIO.setwarnings(False) # Ignore warning for now
GPIO.setmode(GPIO.BCM) # Use physical pin numbering
GPIO.setup(gpio, GPIO.IN, pull_up_down=GPIO.PUD_UP)

button_state = GPIO.input(gpio)
prev_button_state = button_state;

while True: # Run forever
    button_state = GPIO.input(gpio)

    # Check if the button state has changed (press or release event)
    if button_state != prev_button_state:
        if button_state == GPIO.LOW:  # Button is pressed
            print("The button is pressed!")
        else:  # Button is released
            print("The button is released!")

        # Update the previous button state
        prev_button_state = button_state

    # Small delay to avoid unnecessary reading
    time.sleep(0.1)

    # if GPIO.input(gpio) == GPIO.HIGH:
        # print("Button was pushed!")