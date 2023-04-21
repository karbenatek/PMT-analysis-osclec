#!/usr/bin/env python

import numpy as np
import serial
import time
import pandas as pd
import csv
from datetime import datetime
import matplotlib.pyplot as plt


"""TDC_calibration.py: - This software is necessary to calibrate the TDC of the Main Board
                       - Python3 is required
                       - It is mandatory, before running the script, to create a subdirectory named "TDC_calibration" in the same directory where there is the code. This subdirectory will contain all the output files of the software

ACQ_TDC(): this function do a 5 minutes acquisition just to acquire t_fine and tot_fine data and the relative channel. Data are stored in a .csv file


calibratedTDC(): this function read the output file of ACQ_TDC() and create a file for each channel with the calibrated values of the TDC both for the time fine and the t.o.t. fine

"""

__author__ = "Aurora Langella"
__credits__ = ["Aurora Langella", "Luigi Lavitola", "Alfonso Boiano"]
__version__ = "1.0.1"
__maintainer__ = "Aurora Langella"
__email__ = "alangella@na.infn.it"





folder = "TDC_calibration/"


def ACQ_TDC():


    
    CHECK_MSB = 0
    STATUS = 0
    ROW_NUMBER = 0

    data_s = ""
    data = 0

    single_event = []  # this array contains data in raw format, as read from the serial port. It is reset event by event.
    buffer_array = []   # this array contains all the information of the event (channel, energy, event time, acquisition time). It is reset event by event.
    buffer_matrix = [] # this matrix is filled with a maximum of 100 buffer_array,then it's written on a file and reset.

    port=input('Insert the serial port path: ')
    
    #### create the output file ####

    now = datetime.now()
    start_time = now.strftime("%H:%M:%S")
    print("\nAcquisition started at " + str(start_time) + "\n")
    file_name = folder + 'ACQ_TDC_calibration.csv'
    print("Name of the output file:   " + str(file_name) + "\n")
    outFile = open(file_name, 'w')
    writer = csv.writer(outFile)
    header = ["Channel", "Time fine (u=297ps)", "T.o.t. fine (u=297ps)", "Acquisition time"]
    writer.writerow(header)
    outFile.close()


    #### open serial port ####

    ser = serial.Serial(port, 921600) #115200)
    print("Connected to: " + ser.portstr+"\n")


    #### data acquisition  and output file writing ####

    t_end = time.time() + 60*5
    #print(ser)
    while time.time() < t_end:
        #print(time.time())
        for line in ser.read():

            #print(line)

            if line == 13:

                try:
                    data = int(data_s, 16)
                    # print(data)
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

                        if(len(single_event) == 8):

                            buffer_array.append(single_event[1])  # channel
                            buffer_array.append(int(((single_event[3] << 3) & 0x001f))+int(single_event[4] >> 11))  # time fine
                            buffer_array.append(single_event[4] & 0x001f)  # tot fine
                            t = time.localtime()
                            buffer_array.append(time.strftime("%H:%M:%S", t))

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

    return(print("Acquisition for TDC calibration completed"))


def calibrationTDC():

    TDC_STEP = 20

    #### open the output file of ACQ_TDC() ####

    inFile = folder + 'ACQ_TDC_calibration.csv'
    data = pd.read_csv(inFile)
    channel = data['Channel']
    t_fine = data["Time fine (u=297ps)"]
    tot_fine = data["T.o.t. fine (u=297ps)"]
    t_fine = np.array(t_fine)
    tot_fine = np.array(tot_fine)
    channel_0 = np.where((channel == 0) & (t_fine != 31) & (tot_fine != 31))  # index of events in channel 0 and t_fine and tot fine are different than 31
    channel_1 = np.where((channel == 1) & (t_fine != 31) & (tot_fine != 31))  # index of events in channel 1 and t_fine and tot fine are different than 31

    #### get the calibrated values ####

    if(len(channel_0) != 0):

        tfine_correct_0 = []
        tot_fine_correct_0 = []
        correct_outputs_0 = []


        #### tFine channel 0 ####

        #tf_n_0, tf_bins_0, tf_patches_0 = plt.hist(t_fine[channel_0], bins=np.arange(TDC_STEP+1), alpha=0.5, label = 'data', color='red', ec='red')   # plot the histogram

        tf_hist_0, tf_binedges_0 = np.histogram(t_fine[channel_0], bins=np.arange(TDC_STEP+1))
        #print(hist0)
        tf_mean_freq_0 = np.mean(tf_hist_0)
        #print(freq_med0)

        for i in range(1, len(tf_hist_0)+1):

            tfine_correct_0.append(tf_hist_0[:i].sum()*5/TDC_STEP/tf_mean_freq_0)

        #print(tfine_correct)


        #### totFine channel 0 ####

        #totf_n_0, totf_bins_0, totf_patches_0 = plt.hist(tot_fine[channel_0], bins=np.arange(TDC_STEP+1), alpha=0.5, label = 'data', color='skyblue', ec='skyblue')  # plot the histogram

        totf_hist_0, totf_binedges_0 = np.histogram(tot_fine[channel_0], bins=np.arange(TDC_STEP+1))
        #print(totf_hist_0)
        totf_mean_freq_0 = np.mean(totf_hist_0)
        #print(totf_mean_freq_0)

        for i in range(1, len(totf_hist_0)+1):

            tot_fine_correct_0.append(totf_hist_0[:i].sum()*5/TDC_STEP/totf_mean_freq_0)


        #### writing calibrated values in a .csv file ####

        file_name0 = folder + 'TDC_CalibratedOutputs_Channel0.csv'
        outFile0 = open(file_name0, "w")
        writer0 = csv.writer(outFile0)
        header = ["Time fine (u=ns)", "T.o.t. fine (u=ns)"]
        writer0.writerow(header)
        correct_outputs_0.append(tfine_correct_0)
        correct_outputs_0.append(tot_fine_correct_0)
        correct_outputs_0 = np.array(correct_outputs_0)
        correct_outputs_0 = correct_outputs_0.T
        writer0.writerows(correct_outputs_0)
        outFile0.close()

    if(len(channel_1) != 0):

        tfine_correct_1 = []
        tot_fine_correct_1 = []
        correct_outputs_1 = []


        #### tFine channel 1 ####

        #tf_n_1, tf_bins_1, tf_patches_1 = plt.hist(t_fine[channel_1], bins=np.arange(TDC_STEP+1), alpha=0.5, label = 'data', color='skyblue', ec='skyblue')   # plot the histogram

        tf_hist_1, tf_binedges_1 = np.histogram(t_fine[channel_1], bins=np.arange(TDC_STEP+1))
        #print(tf_hist_1)
        tf_mean_freq_1 = np.mean(tf_hist_1)
        #print(tf_mean_freq_1)

        for i in range(1, len(tf_hist_1)+1):

            tfine_correct_1.append(tf_hist_1[:i].sum()*5/TDC_STEP/tf_mean_freq_1)

        #print(tfine_correct)


        #### totFine channel 1 #####

        #totf_n_1, totf_bins_1, totf_patches_1 = plt.hist(tot_fine[channel_1], bins=np.arange(TDC_STEP+1), alpha=0.5, label = 'data', color='skyblue', ec='skyblue')   # plot the histogram

        totf_hist_1, totf_binedges_1 = np.histogram(tot_fine[channel_1], bins=np.arange(TDC_STEP+1))
        #print(totf_hist1)
        totf_mean_freq_1 = np.mean(totf_hist_1)
        #print(totf_mean_med_1)

        for i in range(1, len(totf_hist_1)+1):

            tot_fine_correct_1.append(totf_hist_1[:i].sum()*5/TDC_STEP/totf_mean_freq_1)


        #### writing calibrated values in a .csv file ####

        file_name1 = folder + 'TDC_CalibratedOutputs_Channel1.csv'
        outFile1 = open(file_name1, "w")
        writer1 = csv.writer(outFile1)
        header = ["Time fine (u=ns)", "T.o.t. fine (u=ns)"]
        writer1.writerow(header)
        correct_outputs_1.append(tfine_correct_1)
        correct_outputs_1.append(tot_fine_correct_1)
        correct_outputs_1 = np.array(correct_outputs_1)
        correct_outputs_1 = correct_outputs_1.T
        writer1.writerows(correct_outputs_1)
        outFile1.close()

    #plt.show()



def main():

    ACQ_TDC()
    calibrationTDC()


if __name__ == "__main__":

    main()
