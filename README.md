# Nymble Task
Calculating baud for UART communication with STM32L476RG with W25Q64FW (16 MB Flash)

The Firmware uses two buffer system, while one buffer is being filled with UART data via the DMA (without the invlovement of processor), the other buffer would be dumped onto the external EEPROM.  
The Buffer points are then switched and the process continues.

There are Firmware utilizes RTOS and semaphores for task sheduling and task syncronization.  
UART DMA callback also helped with syncronization.

Additional Driver code had to written for interacting with W25Q64 EEPROM


## Demo 
![sample](https://github.com/user-attachments/assets/6493db0e-0530-4b22-80a9-7ae40e50eeb1)

The MAGENTA text shows the Auto selected COM port and the selected baud rate  
The CYAN text shows the data that is flowing from PC to MCU  
The GREEN text shows the data that is flowing from MCU to PC  
The YELLOW text highlights the calculated data from PC  


## Implementation

First the PC accepts a input string from user, after that the program divides the complete string into smaller chuncks called packets and sends it to the MCU.  

But before sending any packet the program sends the length of the packet to the MCU, the MCU first receives the size of the packet, prepares its buffer and then accepts the packet content.

There are two buffers, while one buffer is being filled with UART data from DMA, the other buffer is being dumped on the external EEPROM using SPI

Baud calculation on MCU side is the average of the calculated baud of all the packets, baud Calculation on MCU side is much accurate.  
Since PC side baud calculation does not take 'size' transfer into account.

If the MCU receives 'size' as 0, it stops receiving the packets and prepares end transmission, which is reading from eeprom (packet wise) and sending it back to PC

### Merts 

The code is highly scalable, the input string could be as large the entire storge of the external eeprom 
The baud rate could be 1 bits / second to 115200 bits / second

Task Syncronization is taken very seriously here. Hence the variable baud does not effect the firmware.

Baud calculation is event based, the timer waits for the entire buffer to be received, but once entire packet is received the uart interput would stop the timmer.  
Giving us an accurate timing calculation 


### Pin Mapping 

![image](https://github.com/user-attachments/assets/e0c9424c-35fc-41e5-831a-2d9e40bf6bf7)

