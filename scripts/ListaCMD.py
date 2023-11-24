#!/usr/bin/env python

"""
File : ListaCMD.py
Author: SER
Data  : 03/11/2021
Version : 1.0
"""


from typing import Match
from AppSerialPortV1 import COMPort

import sys
import time
from pytimedinput import timedKey


# ---------------------
class RunControl():
    def __init__(self):
        self.Port = COMPort()

        self.Config()
        self.Info()

    # - Ogetti usati


    def Config(self):
        # Variabile per il menu di stampa
        self.Pll = "Error" # Locked
        self.Fifo = "Full" # Empty
        self.FreCh0 = 20
        self.FreCh1 = 30
        self.TempoMorto = 0
        self.CH0 = "Enable" # Disable
        self.CH1 = "Enable" # Disable
        self.CS0 = "Enable" # Disable
        self.CS1 = "Enable" # Disable
        self.EventSec = "Enable" # Disable
        self.Pulser = 300
        self.tbuio= 2400
        self.Tpicco = 7

        # Variabili che contengono lo stato
        # localmente.
        self.PowerDisable =0xff
        self.CsStato =0x00
        self.VariClock = 0x0000
        self.Port.Send_CMD(6,0xff)
        
        #-- Elenco indirizzi di letture
        self.addrElenco = { "PwrDis": 2,    # read bit0 = Ch0 e bit1 = ch1
                            "Tbuio": 4,     # 10ns X step
                            "CS": 6,        # read bit0 = CsCh0 e bit1 = Csch1
                            "Tpicco": 5,    # 5ns X step
                            "FreqCh0": 9,   # Hz se ffff = canale sotto soglia
                            "FreqCh1": 10,  # Hz se ffff = canale sotto soglia
                            "Pulser": 0x34, # mS se 0 = Off
                            "Vari": 8,       # bit2  En = evento secondi, bit12 Fifo HF, bit14 PLL locked
                            "TempoMorto": 0x1C # Tempo Morto ACQ
                            }
        #-- Elenco letture  non usata                 
        self.DatiLetture =self.addrElenco.copy()
    """ Lettura parametri:
        Seguenza di invio dei comandi.
        Per aggiornare la schermata di visualizzazione
    """
    def LoopRead(self):
        for x in range(0,len(self.addrElenco)):
            v = list(self.DatiLetture.items())[x][1]        # Valore del comando
            Comando = list(self.DatiLetture.items())[x][0]  # Nome del comando
            #print(">",Comando, end="")

            dati = self.Port.Send_CMD(v,-1)
            print(" --> Rx>",dati)

            self.EstrazioneParametri(Comando,dati)

    """
    Visualizza:
        Tutti i comandi disponibili con i ralativi 
        valori
    """
    def Info(self):
        self.LoopRead() #Lettura di tutti i parametri
        print(f"\n\
****  Parametri Run Control  *****\n\
* 1)Ch0              {self.CH0!s}\n\
* 2)Ch1              {self.CH1!s}\n\
* 3)Pulser           {self.Pulser!r} Hz\n\
* 4)Event seconds    {self.EventSec!r}\n\
* 5)Settling time    {self.tbuio!r} ns\n\
* 6)Time to peak     {self.Tpicco!r} ns\n\
* s)Store monitoring parameters\n\
* q)Exit\n\
* ---------------------------------\n\
* Ratemater Ch0  {self.FreCh0!s} Hz\n\
* Ratemater Ch1  {self.FreCh1!s} Hz\n\
* Dead time      {self.TempoMorto} %\n\
* Fifo Full      {self.Fifo!r} \n\
* PLL200MHz      {self.Pll!r}\n\
            ")
    """
        Gestione Tastiera:
        Ciclo di attesa comandi dalla tastiera
    """
    def LoopRunControll(self):
        key =""
        while key !="q":
            key = input("\n> ")
        
            #--- Selezione CMD 1
            if key == '1':
                print("\nCh0 E = Enable D = Disable")
                dato = input(">").upper()
                if dato  == "E":
                    self.PowerDisable =  self.PowerDisable & 0xfe
                elif dato  == "D":
                    self.PowerDisable =  self.PowerDisable | 0x01
                print("Rx>",self.Port.Send_CMD(2,self.PowerDisable))
                self.Info()
            #--- Selezione CMD 2
            elif key == '2':
                print("\nCh1 E = Enable D = Disable")
                dato = input(">").upper()
                if dato  == "E":
                    self.PowerDisable =  self.PowerDisable & 0xfd
                elif dato  == "D":
                    self.PowerDisable =  self.PowerDisable | 0x02

                print("Rx>",self.Port.Send_CMD(2,self.PowerDisable))
                self.Info()
            
            #--- Selezione CMD 3 Pulser
            elif key == '3':
                dato = input(">Pulser(0 = Off) Hz = ") 
                
                try:
                    fdato = float(dato)
                except ValueError:
                    fdato = 0.0
                    pass

                if (fdato <= 1000) and (fdato>0.007):
                    self.Pulser = int(1000/fdato)
                    print("Rx>",self.Port.Send_CMD(0x34,self.Pulser))
                elif (fdato == 0):   
                    self.Pulser = 0
                    print("Rx>",self.Port.Send_CMD(0x34,self.Pulser))
                else:
                    print("Errone valore")
                self.Info()
            #--- Selezione CMD 4 Evento secondi
            elif key == '4':
                print("\nEvent seconds  E = Enable D = Disable")
                dato = input(">").upper()
                if dato  == "E":
                    self.VariClock = self.VariClock | 0x0004
                    print("Rx>",self.Port.Send_CMD(8,self.VariClock))
                elif dato  == "D":
                    self.VariClock = self.VariClock & 0x00fb
                    print("Rx>",self.Port.Send_CMD(8,self.VariClock))
                self.Info()
            #---- Selezione CMD 5 Tempo Buio
            elif key == '5':
                dato = input(">Settling time (ns), min. 800 ns = ") 
                try:
                    fdato = float(dato)
                except ValueError:
                    fdato = 800.0
                    pass
                
                if fdato >= 800:
                    self.tbuio = int(fdato/10)
                    print("Rx>",self.Port.Send_CMD(0x4,self.tbuio))
                else:
                    self.tbuio = 80
                    print("Rx>",self.Port.Send_CMD(0x4,self.tbuio))
                self.Info()
            #---- Selezione CMD 6 Tempo Picco
            elif key == '6':
                dato = input(">Time to peak ADC (ns), min. 20 ns = ") 
                try:
                    fdato = float(dato)
                except ValueError:
                    fdato = 35.0
                    pass
                
                if fdato >= 20:
                    self.Tpicco = int(fdato/5)
                    print("Rx>",self.Port.Send_CMD(0x4,self.Tpicco))
                else:
                    self.Tpicco = 35
                    print("Rx>",self.Port.Send_CMD(0x4,self.Tpicco))
                self.Info()

            # Salvataggio dati
            elif key == 's':
                    Save_Rate = input("Storage rate (sec) >")
                    folder = "log/"
                    timestr = time.strftime("%Y_%m_%d-%H_%M_%S")
                    Save_Nome_File = input("File name >")
                    if Save_Nome_File.find(".") == -1 :
                        Save_Nome_File = folder + Save_Nome_File + "_" + timestr + ".csv"
                    else :
                        Save_Nome_File = folder + Save_Nome_File

                    f = open(Save_Nome_File, "w")
                    #Header = ["Time", "Freq_CH0", "Freq_CH1"]
                    f.write("Time,Freq_CH0,Freq_CH1,DeadTime,FifoFull\n")
                    f.close()
                    timeout = time.time() + 2
                    key2=""
                    userText = ""
                    while(userText == ""):
                        userText, timedOut = timedKey(timeout=1, allowCharacters="x")
                        
                        if time.time() > timeout:
                            self.Info()
                            timeout = time.time() + int(Save_Rate)
                            f = open(Save_Nome_File, "a")
                            f.write(str(time.strftime("%Y_%m_%d-%H_%M_%S ")) +"," + \
                                str(self.FreCh0) +","+ str(self.FreCh1) +","+ \
                                str(self.TempoMorto) +"," + str(self.Fifo)+"\n")
                            f.close()
                            print("To stop press x")

                    self.Info()

            
            elif key == 'q':
                print("\n\n  bye .")  
                self.Port.PortClose()
                
            #--- Selezione CMD Invio
            else:
                self.Info()
    """
        Estrae le informazioni dai valori letti
        con la seriale
    
    """
    def EstrazioneParametri(self,Cmd,Valore):
         # - codifica cmd PwrDis (Ch0, Ch1) ricevuto
        if list(self.DatiLetture.items())[0][0] == Cmd : 
            intNum = int(Valore)
            binNum = bin(intNum)[2:] 
            #print("\t\t",binNum)
            # print(binNum[6])
            # print(type(binNum[7]))
            # Canale 2
            if(len(binNum) >7):
                if binNum[7] == "0":
                    self.CH0 = "Enable"
                elif binNum[7] == "1":
                    self.CH0 = "Disable"
                else:
                    pass

                if binNum[6] == "0":
                    self.CH1 = "Enable"
                elif binNum[6] == "1":
                    self.CH1 = "Disable"
                else:
                    pass
            else:
                pass
        
        # - codifica cmd Tbuio ricevuto
        elif list(self.DatiLetture.items())[1][0] == Cmd :
            intNum = int(Valore)
            self.tbuio = intNum*10
    
        # - codifica cmd ACQ (CS0, CS1) ricevuto
        elif list(self.DatiLetture.items())[2][0] == Cmd : 
            intNum = int(Valore)
           # binNum = (bin(intNum)[2:]).format(8)
            binNum ='{0:08b}'.format(intNum)
            #print("\t\t",binNum)

            if binNum[7] == "1":
                self.CS0 = "Enable"
            elif binNum[7] == "0":
                self.CS0 = "Disable"
            else:
                pass

            if binNum[6] == "1":
                self.CS1 = "Enable"
            elif binNum[6] == "0":
                self.CS1 = "Disable"
            else:
                pass

        # - codifica cmd Tpicco ricevuto
        elif list(self.DatiLetture.items())[3][0] == Cmd : 
            intNum = int(Valore)
            self.Tpicco = intNum*5
        
        # - codifica cmd FREQ_CH0 ricevuto
        elif list(self.DatiLetture.items())[4][0] == Cmd : 
            intNum = int(Valore)
            if intNum <0XFFFE:
                self.FreCh0 = intNum
            elif intNum == 0XFFFE :
                 self.FreCh0 = "ALERT! Overflow"
            else:
                self.FreCh0 = "ALERT! threshold too low"

        # - codifica cmd FREQ_CH1 ricevuto
        elif list(self.DatiLetture.items())[5][0] == Cmd : #
            intNum = int(Valore)
            if intNum <0XFFFE:
                self.FreCh1 = intNum
            elif intNum == 0XFFFE :
                 self.FreCh1 = "ALERT! Overflow"
            else:
                self.FreCh1 = "ALERT! threshold too low"
        
        # - codifica cmd Pulser ricevuto
        elif list(self.DatiLetture.items())[6][0] == Cmd : 
            intNum = float(Valore)
            #print(intNum)
            if intNum > 0:
                self.Pulser = 1000.00/intNum
            else:
                self.Pulser = "OFF"
        
        # - codifica cmd Vari ricevuto
        elif list(self.DatiLetture.items())[7][0] == Cmd : 
            intNum = int(Valore)
            binNum ='{0:16b}'.format(intNum)
            #print("\t\t",binNum)
            if binNum[13] == "1":
                self.EventSec = "Enable"
            elif binNum[13] == "0":
                self.EventSec = "Disable"
            else:
                pass  
            
            if binNum[1] == "1":
                self.Pll = "Locked"
            else:
                self.PLL = "PLL ERRORE! Not Locked"

            if binNum[3] == '1':
                self.Fifo = "Fifo Full"
            else:
                self.Fifo = "Fifo OK"
        
        # - codifica cmd TempoMorto ricevuto
        elif list(self.DatiLetture.items())[8][0] == Cmd : 
            intNum = int(Valore)
            a = (100 - (100*(intNum/48829)))
            self.TempoMorto  ='{:.2f}'.format(a)
            #self.TempoMorto = (100 - (100/48829)*intNum)

         # - altrimenti (se non è negativo)
        else:
            pass


