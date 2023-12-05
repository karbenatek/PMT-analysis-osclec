#!/usr/bin/env python

'''
simple scripts for handling spectra from INFN electronics
'''

import numpy as np
import pandas as pd
from matplotlib import pyplot as plt
from scipy.optimize import curve_fit
from scipy import stats

def plthist(Q ,hist, title = "", logscale = True, interval = (0,0)):
    plt.bar(Q, hist)
    if logscale:
        plt.yscale('log')
    if title:
        plt.title(title)
    if interval != (0,0):
        plt.xlim(interval)
    plt.xlabel("Charge [ADC channel]")
    plt.ylabel("Count")
    
def gauss(x, amplitude, mean, stddev):
    return amplitude * np.exp(-((x - mean) / stddev)**2 / 2)

def fitgauss(x, hist):
    ampl, mean =  np.max(hist), x[np.argmax(hist)]
    sigma = 4
    popt, pcov = curve_fit(gauss, x, hist, p0= [ampl, mean, sigma])
    # plt.plot(x,hist)
    hist_fit = gauss(x, *popt)
    plt.plot(x,hist_fit,"r")
    return popt


proj_dir = "/home/tono/HyperK/PMT-analysis-osclec/"
voltages = [880,940,1020,1100]
fname_blank = [proj_dir + "data/blank_HV-%i.csv" %V for V in voltages]
fname_laser = [proj_dir + "data/laser_HV-%i.csv" %V for V in voltages]
fit_region = [(23,38), (23,55), (40,93), (60,140)]



plt.figure(figsize=(14,14))

mean = []
for i,V in enumerate(voltages):
    blank   = np.array( pd.read_csv(fname_blank[i]).iloc[:,1])
    laser   = np.array( pd.read_csv(fname_laser[i]).iloc[:,1])
    Q       = np.arange(len(blank))
    blank_sub   = blank[fit_region[i][0]:fit_region[i][1]]
    laser_sub   = laser[fit_region[i][0]:fit_region[i][1]]
    Q_sub       = Q[fit_region[i][0]:fit_region[i][1]]


    plt.clf()
    plt.subplot(2,1,1)
    plthist(Q, laser)
    plthist(Q, blank)
    plthist(Q, laser - blank)
    plt.legend(["laser", "blank", "subtraction"])
    plt.subplot(2,1,2)
    plthist(Q_sub, laser_sub - blank_sub,title="subtraction fit in selected region" ,logscale=False)
    

    pars = fitgauss(Q_sub, laser_sub - blank_sub)
    mean.append(pars[1])

    plt.savefig(proj_dir + "img/HV%i.png" %V)

i_ref_voltage = 1

plt.figure(figsize=(14,8))

plt.clf()

plt.plot(voltages, mean/mean[i_ref_voltage])
plt.xlabel("Bias voltage [V]")
plt.ylabel("Gain [spe for %iV]" %voltages[i_ref_voltage])
plt.savefig(proj_dir + "img/gainplot.png")
    

