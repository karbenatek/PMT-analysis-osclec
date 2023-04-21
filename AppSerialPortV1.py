#!/usr/bin/python

"""
File : AppSerialPortv1.py
Author: SER
Data  : 03/11/2021
Version : 1.0

Installare pyserial 

Esempi.

import serial.tools.list_ports
comlist = serial.tools.list_ports.comports()
connected = []
for element in comlist:
    connected.append(element.device)
print("Connected COM ports: " + str(connected))



import serial.tools.list_ports
ports = serial.tools.list_ports.comports()

for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))

Parameters:	

    port            – Device name or None.
    baudrate (int)  – Baud rate such as 9600 or 115200 etc.
    bytesize        – Number of data bits. Possible values: FIVEBITS, SIXBITS, SEVENBITS, EIGHTBITS
    parity          – Enable parity checking. Possible values: PARITY_NONE, PARITY_EVEN, PARITY_ODD PARITY_MARK, PARITY_SPACE
    stopbits        – Number of stop bits. Possible values: STOPBITS_ONE, STOPBITS_ONE_POINT_FIVE, STOPBITS_TWO
    timeout (float) – Set a read timeout value.
    xonxoff (bool)  – Enable software flow control.
    rtscts (bool)   – Enable hardware (RTS/CTS) flow control.
    dsrdtr (bool)   – Enable hardware (DSR/DTR) flow control.

    write_timeout (float)       – Set a write timeout value.
    inter_byte_timeout (float) – Inter-character timeout, None to disable (default).

Raises:	

    ValueError – Will be raised when parameter are out of range, e.g. baud rate, data bits.
    SerialException – In case the device can not be found or can not be configured.


"""

import sys

import glob
import serial
import serial.tools.list_ports as port_list
from AppStore import StoreSerial

import numpy as np
import math
import time

# ---------------------
class COMPort(StoreSerial):
    def __init__(self):
        self.serialString = ""  
        print("\n\n>Available serial ports ")
        self.PortDisponibili()
        self.Config()
    
    mCom = ""
    mBaundRate = 115200


    def Config(self):
        self.mCom = input(">Select serial port : ")
      #  self.CfgStore(self.mCom,self.mBaundRate)
        try:
            self.Port = serial.Serial(  self.mCom, 
                                        #baudrate=921600, 
                                        baudrate=115200,
                                        bytesize=8, 
                                        timeout=2, 
                                        stopbits=serial.STOPBITS_ONE
                                        )
            print(">Periferica ",self.mCom," OK")
            
           # self.CfgStore(self.mCom,self,self.mBaundRate)
        except :
            print(">Seriale non TROVATA ")
            quit()
    
    """
    
    """
    def PortDisponibili(self):
        ports = list(port_list.comports())
        for p in ports:
            print (">",p)

    def Put(self,dati):
        if self.Port.is_open:
            #self.Port.write(b"str")
            self.Port.write(str.encode(dati))
        else:
            print("Port close")

    def Get(self):
        if  self.Port.in_waiting > 0:
            self.serialString = self.Port.readline()
        else:
            print("Nessun dato disponibile")
        try:
            print(self.serialString.decode("Ascii"))
        except:
            pass
        return self.Port.in_waiting
    
    def PortClose(self):
        self.Port.close()
    
    ## Ritona il valore letto
    def Send_CMD (self,addr, dato = -1 ): 

        if dato != -1 : # WR
            Str_to_send = (format(addr, 'X')+ " " + format(dato, 'X')+ "\n").encode()
        else :  #RD
            Str_to_send = (format(addr, 'X')+"\n").encode()
            #print("----------",Str_to_send )
        
        # Invia comando
        self.Port.write(Str_to_send)
        #print("   \tTX>",Str_to_send,end="")

        #ser.write(b"020\r")
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

       

