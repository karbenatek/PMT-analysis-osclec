#!/usr/bin/env python
"""
File : RunCrtlV1.py
Author: SER
Data  : 03/11/20211

File usati:
    AppSerialPortV1.py
    ListaCMD.py
    AppStore.py
Modifiche di Luigi
Version : 1.0
"""
from ListaCMD import RunControl
#from AppStore import csvStore
import time

Cmd = RunControl()

#mCsv = csvStore()

Cmd.Info()
Cmd.LoopRunControll()





