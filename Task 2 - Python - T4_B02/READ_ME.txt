Task 2 - Using Python for GUI in temperature readings

After setting up the Arduino to receive temperature readings, we wanted to receive these temperature bytes on the computer through a USB connection with the Arduino. We tried early on to use PuTTY, but this software does not have friendly user interfaces so we switched to Python. This was made with the library "Serial" from Python, to setup the serial communication, also we used plot and numerical libraries like "Matplotlib" and "Numpy". 

----------------------------
ATTENTION:This 3 libraries we talked about, will probably need to be installed in your IDLE or Python interperter!
----------------------------

The code we did for this, had in the first place a section to start and open the serial connection with the Arduino, throught UART(9600 baud rate), using "Serial". After this part, we have a decoding function (decode(A1), A1 is the 4 bytes we read from the Arduino), because the data came as hexadecimal, we need to convert the temperature bytes to decimal values. The third part of the code is a function (plotsetup(M,N)), where we load values into a list, to make the first plot. After loading all of these, we are ready to update the plot, that's where the fourth part of the code is needed, this last function is the function (plotupdate(M,N)), which updates the plot with N new values on a plot with M points. 

In resume, for a temperature read to start, we just need to put values of M and N into the last line of code, ''plotupdate(M,N)'' and run the code. This will result, on a updating plot with a total time range of about 0.25*M seconds, for example, if we choose M=200 we will get 50 seconds of time range and with a N=5, we will update every 5 new points, which will be every 1.25 seconds. 
---------------------------
Also, the decrease in the N value will make the plot time less precise, this is due to the time it will take to refresh each plot (about 3 ms) and its recommend to use and N equal or higher than 5.
---------------------------
Note : The first plot will only show after 0.25*M seconds after you start the code. This is because it waits till it has M points to start updating the plot!
