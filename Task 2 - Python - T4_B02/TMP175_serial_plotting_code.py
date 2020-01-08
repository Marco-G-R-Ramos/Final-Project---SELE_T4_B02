import serial  # import of the necessary modules/libraries
import numpy as np
import matplotlib.pyplot as plt

ser = serial.Serial() # this 4 lines of code will setup the 
ser.baudrate = 9600   # serial communication(UART) with the
ser.port = 'COM5'     # arduino/atmega328 that reads values
ser.open()            # of temperature from the TMP175 Sensor

def decode(A1): # function to decode from hexacimal values to decimal
    a=A1.decode("utf-8") # it will have 4 digits, with the first 
    a1=a[0]+a[1]         # 2 being for the integers, so we just add them
    a11 = int(a1, 16)   
    a33 = int(a[2], 16)
    return a11+(a33*0.0625) # here we add the integers with the decimals which come in units of 0,0625 ºC
                            
def plotsetup(M): # function to setup the first plot  with M points
    plt.ion() # update enabled plot
    plt.figure() # opens popup figure
    Ttotal=(0.25*M) # Total time in seconds
    print('Waiting time will be '+str(int(Ttotal))+' seconds')
    x=list(np.linspace(0,int(Ttotal),M)) # creates that time scale with M intervals
    y=[] # list that will store the temperature values
    for i in range(M): # cicle for the first plot
        A=ser.read(4) # reads 4 bits of hexadecimal from the arduino/atmega328 that receives it from the sensor
        y.append(decode(A)) # will add to the list of temperature values the decoded hexadecimal to decimal temperature values
    return x,y # return the both lists of time and temperature

def plotupdate(M,N):
    x,y = plotsetup(M) # first this setups the plot with M points
    while True: # this will loop and update
        lista=[y[N:]] # takes out the first N values of the y list
        x1=[] # makes x1 a empty list for the adition of N values
        for i in range(N): # reads N new values to update the plot with
            x2 = ser.read(4)
            x1.append(decode(x2))
        for j in range(N): # will add N new values to the list
            lista[0].append(x1[j])
        plt.ylabel('Temperature (ºC)') #enables the y axis label
        plt.xlabel('Time (seconds)')   #enables the x axis label
        plt.plot(x,lista[0]) # puts up the updated plot
        plt.pause(0.01) # it needs this pause or the plot will crash
        plt.cla() # deletes the plot and enables it to update in the next loop
        y=lista[0] # makes y now an updated list without the first N values and new N values
        
plotupdate(200,5) # delete the first "#" to make this code automatic.
#ser.close() 
