from authcode import code
from bitstring import BitArray
import time, serial, csv, sys, os

with serial.Serial('/dev/ttyUSB1', 250000) as ser:
    config = BitArray(hex='0000')
    while (True):
        if (ser.inWaiting() > 0):
            line = ser.readline().decode("ascii").strip()
            if ',' in line:
                arr = line.split(',')
                print(line)
                if(arr[0] == 'A094005'):
                    data = bytes.fromhex(arr[3])
                    stage = data[0]
                    data = data[1:3].hex()
                    # print(str(stage) + ' ' + data)
                    if(stage == 0):
                        resp = code(BitArray(hex=data), stage, config)
                    if(stage == 1):
                        config = code(BitArray(hex=data), 0x01)
                        resp = BitArray(hex=data)
                    if(stage > 1):
                        resp = BitArray(hex=data)
                    # print(resp.hex)
                    response = 'A114000,00,01,'+str(stage).zfill(2)+resp.hex+'\n'
                    ser.write(bytes(response, "ascii"))
        time.sleep(0.01)