#!/usr/bin/env python

import numpy as np
import math
from matplotlib import pyplot as plt
import pandas as pd
from scipy.stats import norm
import sys
from scipy.optimize import curve_fit
import os
import matplotlib.colors as colors
from pylab import *

def gauss(x,mu,sigma,A):
    return A*exp(-(x-mu)**2/2/sigma**2)

def multimodal(x,mu1,sigma1,A1,mu2,sigma2,A2,mu3,sigma3,A3,mu4,sigma4,A4):
    return gauss(x,mu1,sigma1,A1)+gauss(x,mu2,sigma2,A2) + gauss(x,mu3,sigma3,A3) + gauss(x,mu4,sigma4,A4)

def bimodal(x,mu1,sigma1,A1,mu2,sigma2,A2):
    return gauss(x,mu1,sigma1,A1)+gauss(x,mu2,sigma2,A2)

def get_sub(x):
    normal = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+-=()"
    sub_s = "ₐ₈CDₑբGₕᵢⱼₖₗₘₙₒₚQᵣₛₜᵤᵥwₓᵧZₐ♭꜀ᑯₑբ₉ₕᵢⱼₖₗₘₙₒₚ૧ᵣₛₜᵤᵥwₓᵧ₂₀₁₂₃₄₅₆₇₈₉₊₋₌₍₎"
    res = x.maketrans(''.join(normal), ''.join(sub_s))
    return x.translate(res)
  


inFile = sys.argv[1]

# fit0 = input("Do you want to fit ch0? (yes/no) ")
# fit1 = input("Do you want to fit ch1? (yes/no) ")
fit0 = "no"
fit1 = "no"

data = pd.read_csv(inFile)
# print(data)
    
fig,ax = plt.subplots(2, figsize=(8, 8))
energy=data['Energy (u= ADC channels)'].to_numpy()
channel = data['Channel']

channel_0=np.where(channel==0) #index of events in channel 0
channel_1=np.where(channel==1) #index of events in channel 1
# print("bla", channel_0[0])
# print(energy)
    
if(len(channel_0[0])>1):        
    bins0 = np.linspace(math.ceil(min(energy[channel_0])), math.floor(max(energy[channel_0])), (max(energy[channel_0])- min(energy[channel_0])))    
    n0, bins0, patches0 = ax[0].hist(energy[channel_0], bins=bins0, alpha=0.5, label = 'data', color='skyblue', ec='skyblue')
    ax[0].set_xlabel("Energy (u= ADC channels)",fontsize=10)
    ax[0].set_ylabel("counts", fontsize=11)
    ax[0].set_xlim([min(energy[channel_0])-50, max(energy[channel_0])+50])    
    ax[0].set_title("Charge Spectrum Channel 0 (JC)", fontsize = 10)

    bins0=(bins0[1:]+bins0[:-1])/2 # for len(x)==len(y)
    if(fit0 == "yes"):
        #bigaussian
    
        expected0=(275,3,7000,330,3,7000) #mean, sigma, counts for each
        params0,cov0=curve_fit(bimodal,bins0,n0,expected0)
        sigma0=sqrt(diag(cov0))
        
        ax[0].plot(bins0,bimodal(bins0,*params0),color='red',lw=1,label='model')
        
        #gaussian
        """expected_g0=(278,3,1000)
        params0,cov0 =curve_fit(gauss,bins0,n0,expected_g0)
        sigma0=sqrt(diag(cov0))
        plot(bins0,gauss(bins0,*params0),color='red',lw=1,label='model')"""

        

        print('\nCH0:\n\nped: \u03BC', '= ', int(params0[0]),'\u00B1', int(sigma0[0]),', \u03C3','= ', int(params0[1]), '\u00B1', int(sigma0[1]), ', N', '= ', int(params0[2]),'\u00B1', int(sigma0[2]), '\n1pe: \u03BC', '= ', int(params0[3]),'\u00B1', int(sigma0[3]),', \u03C3','= ', int(params0[4]), '\u00B1', int(sigma0[4]), ', N', '= ', int(params0[5]),'\u00B1', int(sigma0[5]) )
    
        
        
    
if(len(channel_1[0])>1):
    
    bins1 = np.linspace(math.ceil(min(energy[channel_1])), math.floor(max(energy[channel_1])), (max(energy[channel_1])- min(energy[channel_1])))
    n1, bins1, patches1 = ax[1].hist(energy[channel_1], bins=bins1, alpha=0.5, label = 'data', color='skyblue', ec='skyblue')
    ax[1].set_xlabel("Energy (u= ADC channels)",fontsize=10)
    ax[1].set_ylabel("counts", fontsize=11)
    ax[1].set_xlim([min(energy[channel_1])-50, max(energy[channel_1])+50])
    ax[1].set_title("Charge Spectrum Channel 1 (JD)", fontsize=10)

    bins1=(bins1[1:]+bins1[:-1])/2 # for len(x)==len(y)
    if(fit1 == "yes"):
        #bigaussian
        expected1=(280,3,7000,325,3,7000) #mean, sigma, counts for each
        params1,cov1=curve_fit(bimodal,bins1,n1,expected1)
        sigma1=sqrt(diag(cov1))
        ax[1].plot(bins1,bimodal(bins1,*params1),color='red',lw=1,label='model')
    
        #gaussian
        """expected_g1=(278,3,1000)
        params1,cov1 =curve_fit(gauss,bins1,n1,expected_g1)
        sigma1=sqrt(diag(cov1))
        plot(bins1,gauss(bins1,*params1),color='red',lw=1,label='model')"""

    
        print('\nCH1:\n\nped: \u03BC', '= ', int(params1[0]),'\u00B1', int(sigma1[0]),', \u03C3','= ', int(params1[1]), '\u00B1', int(sigma1[1]), ', N', '= ', int(params1[2]),'\u00B1', int(sigma1[2]), '\n1pe: \u03BC', '= ', int(params1[3]),'\u00B1', int(sigma1[3]),', \u03C3','= ', int(params1[4]), '\u00B1', int(sigma1[4]), ', N', '= ', int(params1[5]),'\u00B1', int(sigma1[5]),'\n')


# plot = input("Do you want to show the plots? (yes/no) ")
plot = "yes"
if(plot=="yes"):
    plt.show()





      


