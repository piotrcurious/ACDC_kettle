# ACDC_kettle
Arduino device to switch kettle to DC power from solar panels if available. Coded by Bard AI
idea is to use DC line from one set of solar panels on the roof, shared my many apartments in apartment block. 
So there are just enough solar panels to allow one person to boil the water at a time. 
Voltage of the line indicates if there is enough sun and if no one else is using it. 
If some other device will be using the DC line to utilize the surplus power, it must optimize it's use to not cause too high voltage drop (most mppt inverters do exactly that)

If someone will turn on the kettle, voltage will drop, indicating line is in use. 
Kettle will boil with varying speed depending on amount of solar power available.

If there is not enough power for too long time, Arduino will merely switch the kettle to AC grid. 

There is also priority knob allowing to set up how badly we need the hot water. set to low if You can wait for the sun or are not in hurry and kettle will prefer to use solar DC power and wait for it longer. boiling water might take longer time in this case.
set the knob to high priority to use AC power just after 5 seconds of not enough solar power available, allowing water to boil just as quickly as on AC alone. 

use safety features like RCD fuses and remember no one will feel responsible for any damages or injuries because you built some random device designed by experimental AI. 
