#!/usr/bin/env python

import csv, sys, os
import pandas as pd
from hvmodbus import HVModbus
from AppSerialPortV1 import COMPort
from serial import Serial, STOPBITS_ONE
from time import sleep, strftime, localtime
from datetime import datetime
import numpy as np
from numpy import linspace, array, histogram, zeros
from matplotlib import pyplot as plt
from threading import Thread, Lock
from queue import Queue, Empty
import math
from os import mkdir

time_format = "-%d-%m-%Y-%H-%M-%S"
# pd.DataFrame(calib).to_csv("calibration/Tenzo_calibration-%s.csv" %datetime.now().strftime("%d-%m-%Y-%H-%M-%S"),index_label="i", header=["Voltage [V]","Force [N]"])

def getTimeStamp():
    return  datetime.now().strftime("%Y_%m_%d-%H_%M_%S")

# Run Controll class
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

        self.PowerDisable =0xff

    def Close(self):
        self.Port.close()

    def setCh0(self, run = False):
        if run:
            self.PowerDisable =  self.PowerDisable & 0xfe
        else:
            self.PowerDisable =  self.PowerDisable | 0x01
        self.Send_CMD(2, self.PowerDisable)
        
    
    def getCh0Rate(self):
        sleep(1.001)

        f = self.Send_CMD(9,-1)
        return f

    def Send_CMD (self, addr, dato = -1 ): 
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

class ITEL():
    def __init__(self, hv_SerialPort, hv_Address, rc_SerialPort, daq_SerialPort):
        print("Initializing connection...")
        self.hv = HVModbus()
        self.hv.open(hv_SerialPort, hv_Address)
        self.hv.reset()
        self.rc = RC(rc_SerialPort)
        self.daq = Serial(daq_SerialPort, 921600, timeout= 0.5)
        self.initDAQ()
        self.limits = (250, 400)
        self.EventFile = None
        self.HistFile = None


        print(self.hv.readMonRegisters())

    def rampDown(self):
        self.hv.reset()
        if self.hv.getStatus() == 0 and self.hv.getVoltage() > 200:
            print("Ramping down!")
            self.hv.powerOff()
            while self.hv.getStatus() == 3: # 3 is ramping down state
                sleep(2)
                print("HV = %4.3f V" %self.hv.getVoltage(), end="\r")

    def rampUp(self):
        """
        Set HV on and block terminal until desired voltage is reached
        """
        self.hv.reset()
        if self.hv.getStatus() in [0, 1] and self.hv.getVoltage() < 200:
            print("Ramping up!")
            self.hv.powerOn()
        self.waitHVready()

    def waitHVready(self):
        while self.hv.getStatus() == 2: # 2 is ramping up state
            sleep(2)
            print("HV = %4.3f V" %self.hv.getVoltage(), end="\r")
        print("Waiting 4 seconds to stabilize...")            
        for i in range(2):
            sleep(2)
            print("HV = %4.3f V" %self.hv.getVoltage(), end="\r")

    def scanTHRrates(self, THR = [60, 80, 100, 120, 140], n = 10): #scans dark rates for thresholds in THR
        # THR = []
        # for i in range(4,10):
        #     THR.append(i*10)
        # for i in range(91,120):
        #     THR.append(i)
        # for i in range(12,17):
        #     THR.append(i*10)
        # print("Scanning for thresholds:", THR)
        print("Threshold values to scan: ")
        print(THR)
        DR = []
        self.rampUp()

        self.rc.setCh0(True)
        for thr in THR:
            self.hv.setThreshold(thr)
            sleep(0.5)
            dr = 0
            for i in range(n):
                dr += self.rc.getCh0Rate()
            dr = dr/n
            DR.append(dr)
            print("%i mV: %i Hz" %(thr, dr))
        self.rc.setCh0(False)
        
        plt.ylabel("Dark rate [Hz]")
        plt.xlabel("Threshold [mV]")
        plt.plot(THR, DR, "-x")
        plt.savefig("DRxTHR.png")
        return DR
    
    def initDAQ(self):
        self.DAQrunning = "idle" # this is a DAQ thread controll varialbe! can be "idle"/"run"/"kill"/"done" or int (this number refers to how many events remain to be acquired) 
        self.EventsInQueue = 0

        self.DAQthread = Thread(name = "ACQ_MainBoard", target = self.ACQ_MainBoard, daemon = True)
        self.DAQlock = Lock()
        self.DAQlock.acquire()
        self.EventWriterThread = Thread(name = "EventWriter", target = self.EventWriter, daemon = True)
        self.DAQqueue = Queue(4096)
        self.DAQthread.start()
        self.EventWriterThread.start()
 
    def startDAQ(self):
        if self.DAQlock.locked():
            self.DAQrunning = "run"
            self.rc.setCh0(True)
            self.DAQlock.release()
        else:
            print("DAQ thread is aleready running")

    def stopDAQ(self):
        if not self.DAQlock.locked():
            self.DAQrunning = "idle"
        else:
            print("DAQ thread is already stopped.")

    def killDAQ(self):
        self.DAQrunning = "kill"
        self.rc.setCh0(False)
        if self.DAQlock.locked():
            self.DAQlock.release()
        self.DAQthread.join()

    def getNevents(self, N):
        if self.DAQlock.locked():
            print("Starting acqusition of", N, "events.")
            self.DAQrunning = int(N)
            self.rc.setCh0(True)
            self.DAQlock.release()

            while self.DAQrunning != "done":
                sleep(0.2)
        else:
            print("Other process is running on DAQ thread.")
    
    def waitDAQfinish(self):
        if isinstance(self.DAQrunning, int):
            self.DAQlock.acquire()
            sleep(1)
            # self.DAQlock.release()
        else:
            self.stopDAQ()


    def ACQ_MainBoard(self):
        CHECK_MSB = 0
        STATUS = 0
        ROW_NUMBER = 0

        data_s = ""
        data = 0

        single_event = []  # this array contains data in raw format, as read from the serial port. It is reset event by event.
        buffer_array = []  # this array contains all the information of the event (channel, energy, event time, acquisition time). It is reset event by event.
        buffer_matrix = []  # this matrix is filled with a maximum of 100 buffer_array,then it's written on a file and reset.

        #### open serial port ####        
        
        print("\nConnected to: " + self.daq.portstr + "\n")

        #### create the output file ####

        now = datetime.now()
        start_time = now.strftime("%H:%M:%S")
        # print("\nAcquisition started at " + str(start_time) + "\n")
        # print("[Type 'quit' and press enter to stop the acquisition]\n")

        # folder = "run/"
        # timestr = strftime("%Y_%m_%d-%H_%M_%S")
        # file_name = folder + timestr + '_data.csv'
        # print("Name of the output file:   " + str(file_name) + "\n")
        # outFile = open(file_name, 'w')
        # writer = csv.writer(outFile)
        # header = ["Channel", "Time Coarse (u=ns)", "T.o.t. coarse (u=ns)", "Time fine (u=ns)",  "T.o.t. fine (u=ns)", "Event time (ns)", "Event t.o.t. (ns)",  "Energy (u= ADC channels)", "Acquisition time", "Date"]
        # writer.writerow(header)
        # outFile.close()

        #### import t_fine calibrated values ####
        # get script path
        script_dir = os.path.dirname(os.path.abspath(__file__))
        # get TDC_calibration oath
        TDC_path = os.path.abspath(os.path.join(script_dir, "../cfg/TDC_calibration"))
        # read calibration values
        time_fine0_cal = pd.read_csv(TDC_path + "/TDC_CalibratedOutputs_Channel0.csv")
        time_fine1_cal = pd.read_csv(TDC_path + "/TDC_CalibratedOutputs_Channel1.csv")
        tfine0_cal = time_fine0_cal["Time fine (u=ns)"]
        totfine0_cal = time_fine0_cal["T.o.t. fine (u=ns)"]
        tfine1_cal = time_fine1_cal["Time fine (u=ns)"]
        totfine1_cal = time_fine1_cal["T.o.t. fine (u=ns)"]

        tfine_cal = []
        tfine_cal.append(tfine0_cal)
        tfine_cal.append(tfine1_cal)
        tfine_cal = array(tfine_cal)

        totfine_cal = []
        totfine_cal.append(totfine0_cal)
        totfine_cal.append(totfine1_cal)
        totfine_cal = array(totfine_cal)


        

        
        #### data acquisition  and output file writing ####

        while self.DAQrunning != "kill":
            
            if isinstance(self.DAQrunning, int):
                if self.DAQrunning == 0:
                    self.DAQrunning = "done"
                    
                    # if self.DAQlock.locked():
                    #     self.DAQlock.release()
                        # sleep(0.2)

            if isinstance(self.DAQrunning, str):
                if self.DAQrunning in ["idle", "done"]:
                    self.rc.setCh0(False)
                    # print("DAQ paused")
                    self.DAQlock.acquire()
                    # print("DAQ resumed")


            for line in self.daq.read():
                
                if line == 13: # might be symbol data packet start..?
                    
                    try:
                        data = int(data_s, 16)
                        #print(data)
                        ROW_NUMBER = ROW_NUMBER + 1
                            
                    except ValueError:
                        # print("non-ASCII character\n")
                        STATUS = 0
                        single_event.clear()
                        pass

                    data_s = ""

                    if (data == 47787):  # Head 47787=BAAB
                        
                        ROW_NUMBER = 0
                        STATUS = 1
                        single_event.append(data)

                    elif (STATUS == 1):  # Channel

                        CHECK_MSB = data >> 15

                        if CHECK_MSB == 0:

                            single_event.append(data)
                            STATUS = 2

                        else:

                            single_event.clear()
                            STATUS = 0

                    elif ((STATUS == 2) and (ROW_NUMBER == 2)):  # TCoarse
                        
                        CHECK_MSB = data >> 15

                        if CHECK_MSB == 0:

                            single_event.append(data)

                        else:

                            single_event.clear()
                            STATUS = 0

                    elif ((STATUS == 2) and (ROW_NUMBER == 3)):  # TCoarse+Tfine

                        CHECK_MSB = data >> 15

                        if CHECK_MSB == 0:
                            single_event.append(data)

                        else:

                            single_event.clear()
                            STATUS = 0

                    elif ((STATUS == 2) and (ROW_NUMBER == 4)):  # TFine+ Width Coarse + Width Fine

                        CHECK_MSB = data >> 15

                        if CHECK_MSB == 0:
                            
                            single_event.append(data)

                        else:

                            single_event.clear()
                            STATUS = 0

                    elif ((STATUS == 2) and (ROW_NUMBER == 5)):  # energy

                        CHECK_MSB = data >> 15

                        if CHECK_MSB == 0:

                            single_event.append(data)

                        else:

                            single_event.clear()
                            STATUS = 0

                    elif ((STATUS == 2) and (ROW_NUMBER == 6) and (data != 65263) and (data != 47787)):  # crc

                        CHECK_MSB = data >> 15

                        if (CHECK_MSB == 0):

                            single_event.append(data)

                        else:

                            single_event.clear()
                            STATUS = 0

                    elif((STATUS == 2) and (ROW_NUMBER == 7)):  # Tail--->FEEF=65263

                        if(data == 65263):

                            single_event.append(data)

                            if(len(single_event) == 8 and ((single_event[4] & 0x001f) != 31)):
                            #if(len(single_event)==8):
                                # this is the event case
                                

                                buffer_array.append(single_event[1])  # channel
                                buffer_array.append((int((single_event[2] << 13))+int((single_event[3] >> 2)))*5)  # time coarse
                                buffer_array.append(((single_event[4] >> 5) & 0x003f)*5)  # tot coarse
                                #if((single_event[1]!=31) and ((single_event[4] & 0x001f)!=31)):
                                if(single_event[1] != 31 and (int(((single_event[3] << 3) & 0x001f))+int(single_event[4] >> 11)) != 31):
                                    # print((int(((single_event[3] << 3) & 0x001f))+int(single_event[4] >> 11)), single_event[1])
                                    buffer_array.append(tfine_cal[single_event[1], (int(((single_event[3] << 3) & 0x001f))+int(single_event[4] >> 11))])  # time fine calibrated
                                    buffer_array.append(totfine_cal[single_event[1], single_event[4] & 0x001f])  # tot fine calibrated
                                    buffer_array.append((int((single_event[2] << 13))+int((single_event[3] >> 2)))*5-tfine_cal[single_event[1], (int(((single_event[3] << 3) & 0x001f))+int(single_event[4] >> 11))])  # event time = t_coarse- t_fine
                                    buffer_array.append(int((single_event[4] >> 5) & 0x003f)*5-totfine_cal[single_event[1], single_event[4] & 0x001f]+tfine_cal[single_event[1], (int(((single_event[3] << 3) & 0x001f))+int(single_event[4] >> 11))])  # event time width = tot_coarse - tot_fine_cal + t_fine_cal

                                else:  # pps channel (31)
                                    buffer_array.append(int((single_event[3] << 3) & 0x001f)+int(single_event[4] >> 11))  # time fine
                                    buffer_array.append(single_event[4] & 0x001f)  # tot fine
                                    buffer_array.append((int((single_event[2] << 13))+int((single_event[3] >> 2)))*5)  # event time = time coarse
                                    buffer_array.append(int((single_event[4] >> 5) & 0x003f)*5-int(single_event[4] & 0x001f)+int((single_event[3] << 3) & 0x001f)+int(single_event[4] >> 11))  # event time width = tot_coarse - tot_fine + t_fine

                                buffer_array.append(single_event[5])  # energy
                                t = localtime()
                                buffer_array.append(strftime("%H:%M:%S", t))
                                buffer_array.append(strftime("%Y-%m-%d"))
                                
                                
                                # buffer_matrix.append(buffer_array.copy())
                                
                                print("#######")
                                print(buffer_array)
                                print("")
                                self.EventsInQueue += 1
                                self.DAQqueue.put(buffer_array.copy())
                                # print("DAQ status:", self.DAQrunning)

                                # case when DAQ is set to obtain certain amount of events
                                if isinstance(self.DAQrunning, int):
                                    if self.DAQrunning > 0:
                                        if self.DAQrunning % 250 == 0:
                                            print("%i remaining" %self.DAQrunning, end='\r')
                                        self.DAQrunning -= 1
                                    # else:
                                    #     self.DAQrunning = "idle"
                                buffer_array.clear()
                                single_event.clear()
                                
                                STATUS = 0

                                # if self.EventsInQueue >= 100:
                                #     self.EventWriterThread.start()
                                #     # self.flushQueue()

                            else:
                                single_event.clear()
                                STATUS = 0

                else:
                    data_s = data_s + chr(line)

    def scanTHRspectra(self, THR = [20, 40, 60, 80, 100, 120, 140], meas_time = 30):
        self.rampUp()
        directory = "data/scanTHRspectra_" + getTimeStamp()
        mkdir(directory)
        for thr in THR:
            self.hv.setThreshold(thr)
            sleep(0.5)
            self.startDAQ()
            sleep(meas_time)
            self.stopDAQ()
            buffer = array([])
            n = 0
            nLeft = 0
            nRight = 0

            while not self.DAQqueue.empty():
                n = n+1

                energy = int(self.DAQqueue.get()[7])
                if energy < self.limits[0]:
                    nLeft += 1
                elif energy > self.limits[1]:
                    nRight += 1
                else:
                # print(energy)
                    buffer.append(energy)

            print(n, "events")
            print(nLeft, "on the bellow,", nRight,"above limits")
            # buffer = array(buffer)
            # hist = histogram(buffer)
            self.histogram = plt.hist(buffer, bins=self.limits[1] - self.limits[0], range=self.limits, alpha=0.5, label = 'data', color='skyblue', ec='skyblue')[0]
            plt.xlabel("Energy [ADC channels]",fontsize=10)
            plt.ylabel("Counts", fontsize=10)
            plt.title("Charge spectrum for threshold %i mV" %thr)
            # plt.show()
            plt.savefig(directory + "/%imV.png" %thr)
            pd.DataFrame(np.array(self.histogram)).to_csv(directory + "/%imV.csv" %thr ,index_label="i",header=["counts"])
            plt.cla()
        # itel.rampDown()

    # init event file
    def openOutputFile(self, EventFileName = None):
        self.EventFile = open(EventFileName, "w")
        self.EventFileWriter = csv.writer(self.EventFile)
        header = ["Channel", "Time Coarse (u=ns)", "T.o.t. coarse (u=ns)", "Time fine (u=ns)",  "T.o.t. fine (u=ns)", "Event time (ns)", "Event t.o.t. (ns)",  "Energy (u= ADC channels)", "Acquisition time", "Date"]
        self.EventFileWriter.writerow(header)

    # dequeue all events and write them into event file
    def EventWriter(self):
        while self.DAQrunning != "kill":
            try:
                Event = self.DAQqueue.get(timeout = 10)
                # self.EventFileWriter.writerow(self.DAQqueue.get(timeout = 10))
            except Empty:
                if self.DAQrunning not in ["idle", "stop", "done"]:
                    print("Timeout 10 s passed in DAQ - stoping DAQ ...")
                    self.stopDAQ()
                    self.closeOuputFile()
                    exit(1)
                    
            else:
                self.EventFileWriter.writerow(Event)

    def closeOuputFile(self):
        self.EventFile.close()



    def getSpectrum(self, n_Events = None, ACQ_time = None, OutputFileName = None , HV = 940, THR = 60, plot = True):
        # n_Events or ACQ_time has to be defined!
        if (n_Events is None and ACQ_time is None) or (n_Events is not None and ACQ_time is not None):
            print("Either n_Events or ACQ_time has to be defined.")
            return 1
        
        # set working parameters
        self.hv.setThreshold(THR)
        self.hv.setVoltageSet(HV)
        sleep(0.5)
        self.rampUp()

        #init output event file
        self.openOutputFile(OutputFileName + "_events.csv")

        if n_Events is not None:
            self.getNevents(n_Events)

        if ACQ_time is not None:
            self.startDAQ()
            sleep(ACQ_time)
            self.stopDAQ()
        
        # get remaining elements from DAQqueue
        while not self.DAQqueue.empty():        
            sleep(2)

        self.closeOuputFile()

        # while not self.DAQqueue.empty():
        #     # TODO: get other elements from DAQqueue and write them to output event file
        #     # this should be done inside DAQ cycle :) -> write only when N events are buffered -> make function to write all events to file
            
        #     # NOTE: this buffer contains energies, that are later on flushed into energy histogram
        #     buffer.append(int(self.DAQqueue.get()[7])) # do this in "flush" function... 
        #     # BOTH OUPUTS CAN BE USEFULL... however the histogram is redundant :)

        # self.histogram = plt.hist(buffer, bins=self.limits[1] - self.limits[0], range=self.limits, alpha=0.5, label = 'data', color='skyblue', ec='skyblue')[0]
        # print(self.histogram[0])
        # plt.xlabel("Energy [ADC channels]",fontsize=10)
        # plt.ylabel("Counts", fontsize=10)
        # if OutFile is not None:
        #     plt.savefig("%s.png" %OutFile)
        #     pd.DataFrame(np.array(self.histogram)).to_csv("%s.csv" %OutFile ,index_label="i",header=["counts"])
        # if plot:
        #     plt.show()


if __name__ == '__main__':
    itel = ITEL("/dev/itel_HV", 6, "/dev/itel_RC", "/dev/itel_DAQ")
    # itel.getSpectrum(ACQ_time= 120, OutFile= "data/spe", THR = 40)
    itel.getSpectrum(ACQ_time = 5, OutputFileName= "/tmp/bleh", HV = 880, THR = 30, plot = True)
    # itel.getSpectrum(n_Events = 15, OutputFileName= "/tmp/bleh", HV = 880, THR = 30, plot = True)

    # sleep(1)


    # for HV in [880, 940, 1020, 1100, 1180]:
    #     itel.getSpectrum(ACQ_time= 240, OutFile= "data/laser_HV-%i" %HV, HV = HV, THR = 30, plot = False)

    # itel.getSpectrum(n_Events=10000, OutFile= "mess/itel-test")
    # itel.scanTHRspectra([80],10)
    # itel.hv.setVoltageSet()
    # itel.scanTHRrates([26,27,28,29,30], n = 5)
    # itel.hv.setVoltageSet(940)
    # itel.rampUp()
    # itel.rampDown()
    # itel.scanTHRrates()
    # itel.scanTHRspectra()
    # itel.startDAQ()
    itel.killDAQ()