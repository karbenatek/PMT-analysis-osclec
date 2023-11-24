#!/usr/bin/env python
import os
import numpy as np
import serial
import time
import pandas as pd
import csv
from datetime import datetime
import threading
import sys

"""
ACQ_MainBoard(): - This software read continuosly data coming from the Main Board of the mPMT and extracts from them information about the energy, time and t.o.t. of each event. All this information are  stored in a .csv file
                 - In the directory where there is the  code is mandatory to create a "/run" directory
                 - The script must be in the same directory where there is the TDC_calibration directory
                 - Python3 is required

"""

__author__ = "Aurora Langella"
__credits__ = ["Aurora Langella", "Luigi Lavitola", "Alfonso Boiano"]
__version__ = "1.0.1"
__maintainer__ = "Aurora Langella"
__email__ = "alangella@na.infn.it"


def ACQ_MainBoard():

    CHECK_MSB = 0
    STATUS = 0
    ROW_NUMBER = 0

    data_s = ""
    data = 0

    single_event = []  # this array contains data in raw format, as read from the serial port. It is reset event by event.
    buffer_array = []  # this array contains all the information of the event (channel, energy, event time, acquisition time). It is reset event by event.
    buffer_matrix = []  # this matrix is filled with a maximum of 100 buffer_array,then it's written on a file and reset.

    #### open serial port ####
     
    # port = input('Insert the serial port path: ')
    port = "/dev/itel_DAQ"
    ser = serial.Serial(port, 921600)
    print("\nConnected to: " + ser.portstr + "\n")    
    
    

    #### create the output file ####

    now = datetime.now()
    start_time = now.strftime("%H:%M:%S")
    print("\nAcquisition started at " + str(start_time) + "\n")
    print("[Type 'quit' and press enter to stop the acquisition]\n")

    folder = "run/"
    timestr = time.strftime("%Y_%m_%d-%H_%M_%S")
    file_name = folder + timestr + '_data.csv'
    print("Name of the output file:   " + str(file_name) + "\n")
    outFile = open(file_name, 'w')
    writer = csv.writer(outFile)
    header = ["Channel", "Time Coarse (u=ns)", "T.o.t. coarse (u=ns)", "Time fine (u=ns)",  "T.o.t. fine (u=ns)", "Event time (ns)", "Event t.o.t. (ns)",  "Energy (u= ADC channels)", "Acquisition time", "Date"]
    writer.writerow(header)
    outFile.close()


    #### import t_fine calibrated values ####
    # get script path
    script_dir = os.path.dirname(os.path.abspath(__file__))
    # get TDC_calibration oath
    TDC_path = os.path.abspath(os.path.join(script_dir, "../cfg/TDC_calibration"))
    # read calibration values
    time_fine0_cal = pd.read_csv(TDC_path + "/TDC_CalibratedOutputs_Channel0.csv")  # CHANGE PATH AS NEEDED
    time_fine1_cal = pd.read_csv(TDC_path + "/TDC_CalibratedOutputs_Channel1.csv")  # CHANGE PATH AS NEEDED
    tfine0_cal = time_fine0_cal["Time fine (u=ns)"]
    totfine0_cal = time_fine0_cal["T.o.t. fine (u=ns)"]
    tfine1_cal = time_fine1_cal["Time fine (u=ns)"]
    totfine1_cal = time_fine1_cal["T.o.t. fine (u=ns)"]

    tfine_cal = []
    tfine_cal.append(tfine0_cal)
    tfine_cal.append(tfine1_cal)
    tfine_cal = np.array(tfine_cal)

    totfine_cal = []
    totfine_cal.append(totfine0_cal)
    totfine_cal.append(totfine1_cal)
    totfine_cal = np.array(totfine_cal)


       

    
    #### data acquisition  and output file writing ####

    while True:
        
        try:
            for line in ser.read():
                print(line)
                if line == 13:
                    
                    try:
                        data = int(data_s, 16)
                        print(data)
                        ROW_NUMBER = ROW_NUMBER + 1
                            
                    except ValueError:
                        print("non-ASCII character\n")
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

                                buffer_array.append(single_event[1])  # channel
                                buffer_array.append((int((single_event[2] << 13))+int((single_event[3] >> 2)))*5)  # time coarse
                                buffer_array.append(((single_event[4] >> 5) & 0x003f)*5)  # tot coarse
                                #if((single_event[1]!=31) and ((single_event[4] & 0x001f)!=31)):
                                if(single_event[1] != 31):
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
                                t = time.localtime()
                                buffer_array.append(time.strftime("%H:%M:%S", t))
                                buffer_array.append(time.strftime("%Y-%m-%d"))
                                
                                
                                buffer_matrix.append(buffer_array.copy())

                                buffer_array.clear()
                                single_event.clear()

                                if len(buffer_matrix) == 100:

                                    outFile = open(file_name, 'a')
                                    writer = csv.writer(outFile)
                                    writer.writerows(buffer_matrix)
                                    outFile.close()

                                    buffer_matrix.clear()
                                    STATUS = 0

                                else:

                                    single_event.clear()
                                    STATUS = 0

                            else:
                                single_event.clear()
                                STATUS = 0

                else:
                    data_s = data_s + chr(line)
        except:
            pass









def main():


    main_thread = threading.Thread(name = "ACQ_MainBoard", target = ACQ_MainBoard, daemon = True)
    main_thread.start()

    if input().lower() == 'quit':
        print('Terminating program')
        sys.exit(0)
    

if __name__ == "__main__":

    main()
