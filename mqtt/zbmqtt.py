# -*- coding: utf-8 -*-
"""
Created on Tue Nov 20 21:27:00 2018
@author: yann brengel

#
# For the full copyright and license information, please view the LICENSE
# file that was distributed with this source code.

"""
#!/usr/bin/python

import paho.mqtt.publish as publish
import paho.mqtt.client  as paho
import json
import socket,select
import sys
import os
import logging
import csv
import datetime
import time
import argparse
from threading import Timer,Thread
from pydispatch import dispatcher
from version import __version__

# definition event
PUBLISH_SIGNAL   ='PUBLISH_SIGNAL'
PUBLISH_SENDER   ='PUBLISH_SENDER'
SUBSCRIBE_SIGNAL ='SUBSCRIBE_SIGNAL'
SUBSCRIBE_SENDER ='SUBSCRIBE_SENDER'
MESSAGE_DATABASE_GW  = ["dbp","state","type_mess","id","mac","adress","type_dev"]
MESSAGE_DATABASE_ST  = ["dbp","state","type_mess","id","mac","cmd"   ,"act"]


############################################################################################
#
## thread d'abonnement des messages mqtt
##
############################################################################################
class mqtt_subscribe(Thread):

   # dictionary
   dict_topics =  [  {'type' : 'cmd' , 'topic':   'reset'     },
                     {'type' : 'cmd' , 'topic':   'permit'    },
                     {'type' : 'proc', 'topic':   'chanmask'  },
                     {'type' : 'ctrl', 'topic':   'lmp'       },
                     {'type' : 'ctrl', 'topic':   'plug'      },
                     {'type' : 'cmd', 'topic':   'device'      },
                     ]                  

  

   def __init__(self,host,control_port,mqtt_port,addr_broker):
      Thread.__init__(self)
        #
        # default parameter can be changed, example transport='websocket'
        # Client(client_id="", clean_session=True, userdata=None, protocol=MQTTv311, transport="tcp")
        # client.username_pw_set("awpywfth ", "v62KH8onlF56")
      self.client            = paho.Client(transport="websockets")  
      self.client.on_message = self.on_message
      self.client.on_publish = self.on_publish
      self.client.on_connect = self.on_connect
      self.addr_broker       = addr_broker
      self.mqtt_port         = int(mqtt_port)
      self.control_port      = control_port
      self.host              = host
      self.list_database     = []
      dispatcher.connect(self.subscribe_dispatcher_receive, signal=SUBSCRIBE_SIGNAL, sender=SUBSCRIBE_SENDER)
      


    ## fonction ecriture des commandes json vers port 2001
    ##
   def writeControl(self,cmd,ip,port):
      s = None
      for res in socket.getaddrinfo(ip, port, socket.AF_UNSPEC, socket.SOCK_STREAM,0,socket.AI_PASSIVE):
         af, socktype, proto, canonname, sa = res
         try:
               s = socket.socket(af, socktype, proto)
               s.settimeout(5)
         except socket.timeout:
               logging.info( "I timed out" )
               s = None
               continue
         try:
               h = s.connect(sa)
         except socket.error as msg:
               s.close()
               s = None
               continue
         break
      if s is None:
         logging.info( 'could not open socket')
         sys.exit(1)
      # sock.connect((ip, port))
      try:
         # on broadcast le message
         s.sendall(str.encode(cmd))
      except Exception as e:
         logging.info( "Impossible de se connecter au client: {}".format(e))
      finally:
         s.close()
         return 0
      return 123
   
  
   # dispatcher receive
   def subscribe_dispatcher_receive(self,message):
      #logging.info("receive message {}".format(message)) 
      if message not in self.list_database: 
         self.list_database.append(message)

   # extraire l'adresse mac 
   def get_mac_device(self,id):
      mac = "" 
      #logging.info("database {}".format(self.list_database))  
      for msg_dbp in self.list_database:
         if id  == msg_dbp[MESSAGE_DATABASE_GW.index("id")]:
            mac = msg_dbp[MESSAGE_DATABASE_GW.index("mac")]
            break
      return mac
   
   def wr_json_message(self,json_string):
      cmd = json.dumps(json_string)
      self.writeControl(cmd[1:-1],self.host,self.control_port)

    # callback
   def on_message(self,mosq, obj, msg):
#      logging.info("message iphone {} ".format(msg.topic))
      topic = msg.topic
      #logging.info("topic decode {} ".format(topic))
      # si topic est dans le dico
      if msg.payload: 
         payload = msg.payload.decode()
         #logging.info("payload {} ".format(payload))
      if topic.count('/') == 1:
         type_mess,cmd = topic.split('/')
      elif topic.count('/') == 2:
#         logging.info("topic ctrl pluf ") 
         type_mess,cmd,id_device = topic.split('/') 
      else: 
         logging.info(" loupe    ") 
      try:           
         topic_search = next(item for item in self.dict_topics if item["topic"] == cmd)
      except KeyError:
         logging.info ("error topic not exist ")
      if topic_search:
#         logging.info("topic search {}".format(topic_search))
         if topic_search["topic"] == "device" and payload == "True": # si demande d'envoi de la liste des device       
               dispatcher.send(message="request list device",signal=PUBLISH_SIGNAL, sender=PUBLISH_SENDER)
               logging.info("dispatcher send list device")
         elif topic_search["topic"] == "reset" and payload == "0":   # reset factory
               dispatcher.send(message="request clear device",signal=PUBLISH_SIGNAL, sender=PUBLISH_SENDER)
         elif topic_search["topic"] == "permit" and payload == "True":          
            json_string = { topic_search['type'] :  { "setpermit":"1", "duration":180 }}
         elif topic_search["topic"]  == "plug" or topic_search["topic"]  == "lmp":
            mac = self.get_mac_device(id_device)
            json_string = {  topic_search['type'] : { "mac":mac,"cmd":payload}}        
         else:
            json_string = { topic_search['type']  : { topic_search['topic']: int(msg.payload) }}
         #logging.info("json string send to controller interface {}".format(json_string))
         self.wr_json_message(json_string)                  
     # else:
#         logging.info(" ah bon") 
       # send message to controller interface    
   
         
    # callback
   def on_publish(self, mqttc, obj, mid):
      pass

    # callback
   def on_subscribe(self, mqttc, obj, mid, granted_qos):
      pass

    # callback
   def on_connect(self, client, obj, flags, rc):
#      logging.info("MQTT connected with result code {}".format(rc))
      self.client.subscribe("cmd/#", 0)
      self.client.subscribe("proc/#", 0)
      self.client.subscribe("ctrl/#", 0)

    # callback
   def on_log(self, mqttc, obj, level, string):
      print(string)

   def run(self):
      logging.info("start thread subscribe ")
      self.client.connect(self.addr_broker, self.mqtt_port, 60)      
      self.client.loop_forever()
     



############################################################################################
##
## thread de publication des messages mqtt
##
############################################################################################
#
class mqtt_publish(Thread):

   # voir definition zigbee device
   SMART_PLUG                = "81"
   SENSOR_TEMPERATURE        = "24321"
   LAMP_ONOFF                = "0"
  
   MESSAGE_DATABASE_SENSOR   = ["dbp","state","type_mess","time","mac","value","parent_mac"]

   def __init__(self, hote,port,broker):
      Thread.__init__(self)
      self.hote = hote
      self.port = port
      self.ip   = broker
      self.dict_gw = {}
      self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)      
      self.socket.connect((self.hote, self.port))
      self.inout = [self.socket]
      self.dict_count = {}
      self.dict_temp = {}
      self.dict_hum = {}
      self.count = 0
      self.request_list_device = False
      self.list_device = []
      
      dispatcher.connect(self.publish_dispatcher_receive,  signal=PUBLISH_SIGNAL, sender=PUBLISH_SENDER)

   # dispatcher receive, on recupere la commande de la liste des devices
   def publish_dispatcher_receive(self,message):
      if message == "request list device":
        self.request_list_device = True
      elif message == "request clear device":
        self.list_device = []
             
   #  write log power in csv
   def write_energy(self,id_smartplug,time,energy):
       with open("/home/pi/Documents/logging_energy.csv", "a") as log:
            log.write("id;{0};time;{1};energy;{2}\n".format(id_smartplug,str(time),str(energy)))

    #  write log temperature in csv
   def write_temp(self,id_temp,time,temp):
       with open("/home/pi/Documents/logging_temperature.csv", "a") as log:
            log.write("id;{0};time;{1};temp;{2}\n".format(id_temp,str(time),str(temp))) 
          
   # compute delta time
   def compute_deltatime(self,time_acquire):
      time_acquire = int(time_acquire,16)
      time_value = time.ctime(time_acquire)
      value =datetime.datetime.strptime(time_value, "%a %b %d %H:%M:%S %Y")
      return (datetime.timedelta(hours=value.hour,minutes=value.minute,seconds=value.second))

   # extraction des messages stocke dans la socket
   def extract_message(self,message):
      list_dbp = []
      mess_dbp = message.decode('utf-8').split(" ")
      for i in range(len(mess_dbp)):
        if 'dbp' in mess_dbp[i]:
            if mess_dbp[i+2] == "gw" or  mess_dbp[i+2] == "bat" or mess_dbp[i+2] == "tmp" or  mess_dbp[i+2] == "hum" or mess_dbp[i+2] == "st":
               list_dbp.append(mess_dbp[i:i+7])
      return list_dbp
      
   # envoi liste device sur requete
   def send_list_device(self):
      mess_mqtt = ""
      for dbp,state,type_mess,id,mac,adress,type_dev in self.list_device:
         if type_dev == self.SENSOR_TEMPERATURE:
            mess_mqtt = mess_mqtt +" SENSOR_TEMPERATURE"+id+","
         elif type_dev == self.SMART_PLUG:
            mess_mqtt = mess_mqtt +" SMART_PLUG"+id+","
         elif type_dev == self.LAMP_ONOFF:
            mess_mqtt = mess_mqtt +" LAMP_ONOFF"+id+","
      if mess_mqtt != "":
         logging.info("publish mess gateway mqtt")
         publish.single(topic="gateway/device", payload=mess_mqtt[:-1], hostname=self.ip,transport="websockets")      
      self.request_list_device = False
        
   def run(self):
      logging.info("start thread publish ")
      while 1:
        if self.request_list_device == True:
           self.send_list_device()        
        new_data = self.socket.recv(2048)
        if len(new_data) != 0:
           #logging.info("message socket = {}".format(new_data))
           list_mess_dbp = self.extract_message(new_data)
           for mess_dbp in list_mess_dbp:
             # logging.info("message dbp = {}".format(mess_dbp))
              if mess_dbp[MESSAGE_DATABASE_GW.index("type_mess")] == "gw":  # gateway discovery
                    type_dev = mess_dbp[MESSAGE_DATABASE_GW.index("type_dev")]
                    #logging.info("type_dev  = {}".format(type_dev)) 
                    dispatcher.send(message=mess_dbp,signal=SUBSCRIBE_SIGNAL, sender=SUBSCRIBE_SENDER)                                            
                    if type_dev == self.SENSOR_TEMPERATURE:
                       id    = mess_dbp[MESSAGE_DATABASE_GW.index("id")]
                       if id not in self.dict_count:
                          self.dict_count[id] = 0  
                    elif type_dev == self.SMART_PLUG:
                       id    = mess_dbp[MESSAGE_DATABASE_GW.index("id")]
                    elif type_dev == self.LAMP_ONOFF:
                       id    = mess_dbp[MESSAGE_DATABASE_GW.index("id")]
                    else:
                       id = 10000
                    if mess_dbp[MESSAGE_DATABASE_GW.index("mac")] not in self.dict_gw:
                       self.dict_gw[mess_dbp[MESSAGE_DATABASE_GW.index("mac")]] = id                       
                    if mess_dbp not in self.list_device:
                       self.list_device.append(mess_dbp)                        
              elif mess_dbp[MESSAGE_DATABASE_ST.index("type_mess")] == "st":  # state device
                   state = mess_dbp[MESSAGE_DATABASE_ST.index("cmd")]
                   #logging.info("state  = {}".format(state))   
                   mac   = mess_dbp[MESSAGE_DATABASE_ST.index("mac")]                       
                   if mac in self.dict_gw.keys():
                      id_state = self.dict_gw[mac]
                      topic_state = "sensor/state" + "/" + str(id_state)                                                 
                      #logging.info("id state  = {}".format(id_state))                          
                      publish.single(topic=topic_state, payload=state, hostname=self.ip,transport="websockets")                      
              elif mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("type_mess")] == "tmp":  # temperature xiaomi
                 value =  mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("value")]
                 time  =  mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("time")]
                 delta_time = self.compute_deltatime(time)
                # logging.info("delta = {}".format(delta_time))
                 # recuperation id suivant adresse mac
                 mac = mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("mac")]
                 if mac in self.dict_gw.keys():                         
                     id_temp = self.dict_gw[mac]
                     conversion = float.fromhex(value)
                     temperature = conversion/100
                     self.dict_temp[id_temp]=temperature
                     # self.write_temp(id_temp,delta_time,temperature)
#                         self.logger.info(" id;"+id_temp+" time;"+str(delta_time)+" temp;"+str(temperature))
                     if id_temp in self.dict_count:
                       self.count =  self.dict_count[id_temp]                         
                       self.count = self.count + 1
                       self.dict_count[id_temp] = self.count                    
                     #logging.info("temperature =   {}".format(temperature))
                     topic_temp  = "sensor/tmp" + "/" + id_temp
                     topic_count = "sensor/count" + "/" + id_temp
                     publish.single(topic=topic_temp,  payload=temperature, hostname=self.ip,retain=True,transport="websockets")
                     publish.single(topic=topic_count, payload=self.dict_count[id_temp], hostname=self.ip,retain=True,transport="websockets")
                    
              elif mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("type_mess")] == "hum": # humidite xiaomi
                    value =  mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("value")]                        
                    mac = mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("mac")]
                    if mac in self.dict_gw.keys():       
                        id_hum = self.dict_gw[mac]
                        conversion = float.fromhex(value)
                        humidity=conversion/100
                        self.dict_hum[id_hum] = humidity
                        #logging.info("humidity =  {}".format(humidity))
                        topic_hum = "sensor/hum" + "/" + id_hum
                        publish.single(topic=topic_hum, payload=humidity, hostname=self.ip,retain=True,transport="websockets")
              elif mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("type_mess")] == "bat": # conso smart plug xiaomi
                    value =  mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("value")]
                    mac   = mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("mac")]
                    time  =  mess_dbp[self.MESSAGE_DATABASE_SENSOR.index("time")]
                    delta_time = self.compute_deltatime(time)
                    
                    if mac in self.dict_gw.keys():       
                        id_bat = self.dict_gw[mac]
                        conversion = float.fromhex(value)
                        # self.write_energy(id_bat,delta_time,conversion)
                        value_conso = (float)(conversion - 16519)/ 9.96;
                        if value_conso <0:
                           value_conso = 0.0
                        #logging.info("conso smart plug = {}".format(value_conso))
                        topic_bat = "sensor/power" + "/" + id_bat 
                        publish.single(topic=topic_bat, payload=value_conso, hostname=self.ip,retain=True,transport="websockets")

      self.socket.close()
      logging.info("close ")    


# thread periodique pour heartbeat
class timer_message_mqtt(Thread):

    def __init__(self, duree, fonction, args=[], kwargs={}):
        Thread.__init__(self)
        self.duree = duree
        self.fonction = fonction
        self.args = args
        self.kwargs = kwargs
        self.encore = True  # pour permettre l'arret a la demande

    def run(self):
        while self.encore:
            self.timer = Timer(self.duree, self.fonction, self.args, self.kwargs)
            self.timer.setDaemon(True)
            self.timer.start()
            self.timer.join()

    def stop(self):
        self.encore = False  # pour empecher un nouveau lancement de Timer et terminer le thread
        if self.timer.isAlive():
            self.timer.cancel()  # pour terminer une eventuelle attente en cours de Timer


# monitoring thread and reboot system if problem occur 
def heartbeat(hearbeat,ip,thread_publish,thread_subscribe):
   #logging.info("heartbeat")
   if thread_publish.is_alive() and thread_subscribe.is_alive():
      #logging.info("heartbeat")      
      jsonstr = json.dumps( {"status": 1 })
      publish.single(topic="gateway/heartbeat", payload=jsonstr, hostname=ip,transport="websockets")
   else:
      logging.info("thread is dead, system must reboot")
      os.system("sudo reboot")

if __name__ == '__main__':
    
   logger    = logging.getLogger()
   handler   = logging.StreamHandler()
   formatter = logging.Formatter('%(asctime)s %(name)-12s %(levelname)-8s %(message)s')
   handler.setFormatter(formatter)
   logger.addHandler(handler)
   logger.setLevel(logging.DEBUG)

# Creation des threads

   parser = argparse.ArgumentParser()  
   parser.add_argument('--mqtt_host', help='MQTT host', default='192.168.0.17')
   parser.add_argument('--mqtt_port', help='MQTT port', default='1883')   
   args = parser.parse_args()
   time_out = 60.0
   CONTROL_PORT = 2001
   SERVER_PORT  = 2002   
   file_csv     = "logging_temperature.csv"
   logging.info(__version__)
  

   thread_1 =  mqtt_publish("localhost",SERVER_PORT,args.mqtt_host)
   thread_1.start()

   thread_2 =  mqtt_subscribe("localhost",CONTROL_PORT,args.mqtt_port,args.mqtt_host)
   thread_2.start()
      
   thread_3 = timer_message_mqtt(time_out,heartbeat,["heartbeat"], {'ip': args.mqtt_host,'thread_publish': thread_1,'thread_subscribe': thread_2})
   thread_3.start()
   