# -*- coding: utf-8 -*-
"""
Created on Wed Mar 20 22:05:34 2019

@author: ybren
"""

import pandas as pd
import matplotlib.pyplot as plt
import datetime

file_csv = "logging_temperature.csv"
dict_id = {'Desktop':3,'living room':4,'kitchen':5,'bathroom':6,'spare time':7}

names = ['id','id_temp','time','time_value','temp','temp_value']
df = pd.read_csv(file_csv,delimiter=';',header=None)
df.columns = names

fig = plt.figure()
fig.set_size_inches(15.5, 10.5)

filt_id= df.id_temp==dict_id['Desktop']
df_temp1 = df[filt_id]
x1 = df_temp1['time_value']
y1 = df_temp1['temp_value']
ax_plot1 = fig.add_subplot(5,1,1)

plt.xlabel('time')
plt.ylabel('temperature')
plt.title('temperature room Desktop')
ax_plot1.plot(x1,y1,'green')


filt_id= df.id_temp==dict_id['living room']
df_temp2 = df[filt_id]
x2 = df_temp2['time_value']
y2 = df_temp2['temp_value']
ax_plot2 = fig.add_subplot(5,1,2)
plt.xlabel('time')
plt.ylabel('temperature')
plt.title('temperature living room')
plt.plot(x2,y2) 

filt_id= df.id_temp==dict_id['kitchen']
df_temp3 = df[filt_id]
x3 = df_temp3['time_value']
y3 = df_temp3['temp_value']
ax_plot3 = fig.add_subplot(5,1,3)
plt.xlabel('time')
plt.ylabel('temperature')
plt.title('temperature kitchen')
plt.plot(x3,y3)

filt_id= df.id_temp==dict_id['bathroom']
df_temp4 = df[filt_id]
x4 = df_temp4['time_value']
y4 = df_temp4['temp_value']
ax_plot4 = fig.add_subplot(5,1,4)
plt.xlabel('time')
plt.ylabel('temperature')
plt.title('temperature bathroom')
plt.plot(x4,y4)

filt_id= df.id_temp==dict_id['spare time']
df_temp5 = df[filt_id]
x5 = df_temp5['time_value']
y5 = df_temp5['temp_value']
ax_plot5 = fig.add_subplot(5,1,5)
plt.xlabel('time')
plt.ylabel('temperature')
plt.title('temperature spare time')
plt.plot(x5,y5)

plt.tight_layout()

time_record = datetime.datetime.now().strftime('%Y-%m-%d')
plt.savefig("temperature_"+ time_record + ".png")
plt.show()