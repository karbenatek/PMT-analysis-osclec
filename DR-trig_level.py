#!/usr/bin/env python

from hvmodbus import HVModbus
from AppSerialPortV1 import COMPort
from serial import Serial, STOPBITS_ONE
from time import sleep
from numpy import linspace, array
from matplotlib import pyplot as plt

class RC():
    def __init__(self, SerialPort):
        self.SerialPort = SerialPort
        try:
            self.Port = Serial(  SerialPort, 
                                        #baudrate=921600, 
                                        baudrate= 115200,
                                        bytesize= 8, 
                                        timeout= 2, 
                                        stopbits= STOPBITS_ONE
                                        )
        except :
            print("Connection error")

    def Close(self):
        self.Port.close()
    def setCh0(self, run = False):
        if run:
            self.Send_CMD(2, 0xfe)
        else:
            self.Send_CMD(2, 0x01)
    
    def getCh0Rate(self):
        sleep(1.005)

        f = self.Send_CMD(9,-1)
        return f

    def Send_CMD (self,addr, dato = -1 ): 
        if dato != -1 : # WR
            Str_to_send = (format(addr, 'X')+ " " + format(dato, 'X')+ "\n").encode()
        else :  #RD
            Str_to_send = (format(addr, 'X')+"\n").encode()

        self.Port.write(Str_to_send)
        line = self.Port.read(5)

        try:
            line = int(line,16)
        except ValueError as verr:
                print(">",verr)
                line = "Error"
                pass # do job to handle: s does not contain anything convertible to int
        except Exception as ex:
                print(">",ex)
                line = "Error"
                pass # do job to handle: Exception occurred while converting to int
        
        return line


def scanTHR(hv, rc, THR, n = 10): #scans dark rates for thresholds
    DR = []
    rc.setCh0(True)
    for thr in THR:
        hv.setThreshold(thr)
        sleep(0.5)
        dr = 0
        for i in range(n):
            dr += rc.getCh0Rate()
        dr = dr/n
        DR.append(dr)
        print("%i mV: %i Hz" %(thr, dr))
    plt.ylabel("Dark rate [Hz]")
    plt.xlabel("Threshold [mV]")
    plt.plot(THR, DR, "-x")
    plt.show()
    return DR


if __name__ == '__main__':
    hv_SerialPort = "/dev/ttyUSB3" 
    hv_Address = 6
    rc_SerialPort = "/dev/ttyUSB2" #run controll
    
    hv = HVModbus()
    hv.open(hv_SerialPort,hv_Address)
    # hv.reset()
    # hv.powerOn()
    print(hv.readMonRegisters())

    rc = RC(rc_SerialPort)
    # rc.setCh0(True)
    THR = []
    for i in range(5,15):
        if i >= 9 and i < 11:
            for j in range(10):
                THR.append(i*10 + j)
        else:
            THR.append(i*10)
            THR.append(i*10 + 5)

    
    print("THR =",THR)

    # DR = scanTHR(hv, rc, THR, 10)
    print(DR)
    rc.Close()
