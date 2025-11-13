#!/usr/bin/env python
import time
import serial


ser = serial.Serial(
        port='/dev/ttyAMA0', #Replace ttyS0 with ttyAM0 for Pi1,Pi2,Pi0
        baudrate = 9600,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS,
        timeout=1
)

counter = 1

time.sleep(1)

relOn = [0xA0,0x01,0x01,0xA2] # bytearray([160,1,1,162])
relOff = [0xA0,0x01,0x00,0xA1] # bytearray([160,1,0,161])

while 1: 
    ser.write(relOn) 
    time.sleep(5)
    ser.write(relOff)
    time.sleep(5)