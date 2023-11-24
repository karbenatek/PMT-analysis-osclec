#!/usr/bin/env python

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import csv
import sys
from pylab import *
import glob
import os

list_of_files = glob.glob('run/*.csv') # * means all if need specific format then *.csv
latest_file = max(list_of_files, key=os.path.getctime)
print("Plotting data from the latest run, which is  " + str(latest_file))

#inFile= sys.argv[1]
data = pd.read_csv(latest_file)


#plt.style.use('fivethirtyeight')

#fig, axs = plt.subplots(nrows=1, ncols=2, constrained_layout=True)

fig,ax = plt.subplots(2, figsize=(8, 8))

channel = []
energy = []
channel_index=[]

def animate(i):
    
    data = pd.read_csv(latest_file)
    channel = data['Channel']
    energy = data['Energy (u= ADC channels)']
    energy=np.array(energy)
    channel_0=np.where(channel==0) #index of events in channel 0
    channel_1=np.where(channel==1) #index of events in channel 1 
    #self.ax[0].autoscale(False)
    ax[0].cla()
    ax[1].cla()
    if(len(channel_0[0])>1):
        
        bins0 = np.linspace(math.ceil(min(energy[channel_0])), 
                   math.floor(max(energy[channel_0])),
                   (max(energy[channel_0])- min(energy[channel_0])))

        n0, bins0, patches0 = ax[0].hist(energy[channel_0], bins=bins0, alpha=0.5, label = 'data', color='skyblue', ec='skyblue')
        array = np.array(energy[channel_0])
        ax[0].set_xlabel("Energy (u= ADC channels)",fontsize=10)
        ax[0].set_ylabel("counts", fontsize=11)
        ax[0].set_xlim([min(energy[channel_0])-50, max(energy[channel_0])+50])    
        ax[0].set_title("Charge Spectrum Channel 0 (JC)", fontsize = 10)
        
        
    
    if(len(channel_1[0])>1):
       
        bins1 = np.linspace(math.ceil(min(energy[channel_1])), 
                   math.floor(max(energy[channel_1])),
                   (max(energy[channel_1])- min(energy[channel_1])))

        n1, bins1, patches1 = ax[1].hist(energy[channel_1], bins=bins1, alpha=0.5, label = 'data', color='skyblue', ec='skyblue')
   
        ax[1].set_xlabel("Energy (u= ADC channels)",fontsize=10)
        ax[1].set_ylabel("counts", fontsize=11)
        ax[1].set_xlim([min(energy[channel_1])-50, max(energy[channel_1])+50])
        ax[1].set_title("Charge Spectrum Channel 1 (JD)", fontsize=10)   

def onclick(event):
    ax = event.inaxes
    if ax is None:
        pass
    elif event.button == 1:  # left click
        ax.set_autoscale_on(False)
    elif event.button == 3:  # right click
        ax.set_autoscale_on(True)
    else:
        pass

cid = fig.canvas.mpl_connect('button_press_event', onclick)


   

ani = FuncAnimation(fig, animate, interval=5000)



#plt.tight_layout()
plt.show()
    









        
       
