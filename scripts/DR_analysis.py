#!/usr/bin/env python

import numpy as np
import math
from matplotlib import pyplot as plt
import matplotlib.mlab as mlab
import pandas as pd 
import sys
import sklearn
from pylab import *
from scipy.optimize import curve_fit
from scipy.stats import norm

def gauss(x,mu,sigma,A):
    return A*exp(-(x-mu)**2/2/sigma**2)



### to run the script --> python3 DR_hist.py infile_name.csv 

inFile= sys.argv[1]
data = pd.read_csv(inFile)
dr_ch0 = data['Freq_CH0']
dr_ch1 = data['Freq_CH1']

### find expected parameters

mu_0, sigma_0 = norm.fit(dr_ch0)
mu_1, sigma_1 = norm.fit(dr_ch1)

ax0=np.linspace(1,len(dr_ch0),len(dr_ch0))
ax1=np.linspace(1,len(dr_ch1),len(dr_ch1))

fig,ax = plt.subplots(2,2, figsize=(8, 8))

### DR hist CH0

ax[0,0].set_xlim([min(dr_ch0)-5, max(dr_ch0)+5])
n0,bins0,patches0 = ax[0,0].hist(dr_ch0, bins='auto', alpha=0.5, label = 'data')
bins0=(bins0[1:]+bins0[:-1])/2 # for len(x)==len(y)

expected=(mu_0, sigma_0,1000)
params,cov =curve_fit(gauss,bins0,n0,expected)
sigma=sqrt(diag(cov))
ax[0,0].plot(bins0,gauss(bins0,*params),color='red',lw=1,label='model')

ax[0,0].set_title('Dark Rate CH0')
ax[0,0].set_xlabel('Frequency (Hz)')
ax[0,0].set_ylabel('counts')

print('\nCH0:\n\u03BC', '= ', int(params[0]),'\u00B1', int(sigma[0]),', \u03C3','= ', int(params[1]), '\u00B1', int(sigma[1]), ', N', '= ', int(params[2]),'\u00B1', int(sigma[2]),'\n')


### DR hist CH1

ax[0,1].set_xlim([min(dr_ch1)-5, max(dr_ch1)+5])
n1,bins1,patches1 = ax[0,1].hist(dr_ch1, bins='auto', alpha=0.5, label = 'data')
bins1=(bins1[1:]+bins1[:-1])/2 # for len(x)==len(y)

expected=(mu_1, sigma_1,1000)
params,cov =curve_fit(gauss,bins1,n1,expected)
sigma=sqrt(diag(cov))
ax[0,1].plot(bins1,gauss(bins1,*params),color='red',lw=1,label='model')

ax[0,1].set_title('Dark Rate CH1')
ax[0,1].set_xlabel('Frequency (Hz)')
ax[0,1].set_ylabel('counts')

print('\nCH1:\n\u03BC', '= ', int(params[0]),'\u00B1', int(sigma[0]),', \u03C3','= ', int(params[1]), '\u00B1', int(sigma[1]), ', N', '= ', int(params[2]),'\u00B1', int(sigma[2]),'\n')

# TREND DR CH0

ax[1,0].plot(ax0,dr_ch0)
#ax[1,0].ylim(0,1000)
ax[1,0].set_title('Dark Rate Trend CH0')
ax[1,0].set_xlabel('Counts')
ax[1,0].set_ylabel('Dark rate(Hz)')


# TREND DR CH1

ax[1,1].plot(ax1,dr_ch1)
ax[1,1].set_title('Dark Rate Trend CH1')
ax[1,1].set_xlabel('Counts')
ax[1,1].set_ylabel('Dark rate(Hz)')


plt.show()
