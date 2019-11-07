# -*- coding: utf-8 -*-
"""
Created on Sun Mar 24 12:19:42 2019

@author: ybren
"""

import pysftp

#nopts = pysftp.CnOpts()
#cnopts.hostkeys.load('known_hosts')
local_path = "logging_temperature.csv"
remote_path = "/home/pi/Documents/logging_temperature.csv"
cnopts = pysftp.CnOpts()
cnopts.hostkeys = None   
with pysftp.Connection(host='192.168.0.12',username='pi',password='yourpassword', cnopts=cnopts) as sftp:
    sftp.get(remote_path,local_path)