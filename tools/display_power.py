# -*- coding: utf-8 -*-
"""
Created on Wed Mar 20 22:05:34 2019

@author: ybren
"""

import pandas as pd
import matplotlib.pyplot as plt
import time

#compute number of seconds
def convtime(time_value):
    return sum(x * int(t) for x, t in zip([3600, 60, 1], time_value.split(":")))


file_csv = "logging_energy_frigo.csv"
#file_csv = "logging_energy_aspirateur.csv"
# file_csv = "logging_energy_1000_2m30.csv"
#file_csv = "logging_energy_300_30s.csv"
#file_csv = "logging_energy_1000_30s.csv"
dict_id = {'Desktop':2,'living room':1}

names = ['id','id_smart','time','time_value','energy','energy_value']
df = pd.read_csv(file_csv,delimiter=';',header=None)
df.columns = names

#filt_id= df.id_temp==dict_id['spare time']
filt_id= df.id_smart==dict_id['Desktop']
df_energy = df[filt_id]

print (df_energy)

x = df_energy['time_value']
y = df_energy['energy_value']

#print("power max = {}".format(y.idxmax()))

index_max = y.idxmax()
time_max = x[index_max]
value_max = y[index_max]
time0 = x[0]
print("power max = {}".format(value_max))

delta = convtime(time_max)- convtime(time0)
print ("delta  {}".format(delta))
print ("time = {}".format(time_max))
value_min = y.min()
print("power min = {}".format(y.min()))


#df_measure = df_energy.iloc[:index_max+1]
#print(df_measure)
#val= df_measure['energy_value'].sum()
#print(val/delta)


pente=(value_max-value_min)/delta
print ("puissance =  {}".format(pente))

plt.xlabel('time')
plt.ylabel('energy')
plt.title('energy room')
plt.plot(x,y)
plt.savefig("energy.png")
plt.show()
#
