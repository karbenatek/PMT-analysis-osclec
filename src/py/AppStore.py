#!/usr/bin/python

"""
File : AppStore.py
Author: SER
Data  : 03/11/2021
Version : 1.0

Esempi.




"""
import csv
import json
import configparser

'''
    Configuration file:

'''
class StoreSerial():
    def __init__(self):
        super().__init__()

        self.InitStore()

    def InitStore(self):
        print("InitStore")
        self.config = configparser.ConfigParser()
        self.config['DEFAULT'] = {  "Port"    :"Com1", 
                                    "BaudRate":115200, 
                                    "DataBits":8,
                                    "StopBits":1,
                                    "Parity"  :"None",
                                    "FlowCrt" :"None"}

        self.config['Scheda 1'] = {}
        self.config['Scheda 1']['SCh1-User'] = 'pippo'

        self.config['Scheda2'] = {}
        topsecret = self.config['Scheda2']
        topsecret['Sch2-Port'] = '50022'     # mutates the parser
        topsecret['Sch2-ForwardX11'] = 'no'  # same here
        
        # Aggiunge nuova voce
        self.config['DEFAULT']['ForwardX11'] = 'yes'
        self.config['DEFAULT']['Port'] = 'Com2'
        
        with open("Serial.cfg","w") as cfgFile:
            self.config.write(cfgFile)

    def GetStore(self):
        print(" Get ")
        self.config.read("Serial.cfg")
        print("\n\rSettori----")
        print(self.config.sections())

        print("\n\rVoci Scheda1 ----")
        for key in self.config['Scheda2']:
            print(key)
        
        print("---------")
    def CfgStore(self,strCom,BRate):
        self.config[strCom] = {  "Port"    :"Com1", 
                            "BaudRate":115200, 
                            "DataBits":8,
                            "StopBits":1,
                            "Parity"  :"None",
                            "FlowCrt" :"None"}
        
        with open("Serial.cfg","w") as cfgFile:
            self.config.write(cfgFile)

        """
        
        
        """
    class csvStore():
        def __init__(self):
            super().__init__()

            self.SaveCsvStoreV1()

        my_dict = {'1': 'aaa', '2': 'bbb', '3': 'ccc'}

        def SaveCsvStore(self):
            with open('test.csv', 'w') as f:
                for key in self.my_dict.keys():
                    f.write("%s,%s\n"%(key,self.my_dict[key]))

        def SaveCsvStoreV1(self):
            csv_columns = ['No','Name','Country']
            dict_data = [
            {'No': 1, 'Name': 'Alex', 'Country': 'India'},
            {'No': 2, 'Name': 'Ben', 'Country': 'USA'},
            {'No': 3, 'Name': 'Shri Ram', 'Country': 'India'},
            {'No': 4, 'Name': 'Smith', 'Country': 'USA'},
            {'No': 5, 'Name': 'Yuva Raj', 'Country': 'India'},
            ]
            csv_file = "Names.csv"
            try:
                with open(csv_file, 'w') as csvfile:
                    writer = csv.DictWriter(csvfile, fieldnames=csv_columns)
                    writer.writeheader()
                    for data in dict_data:
                        writer.writerow(data)
            except IOError:
                print("I/O error")