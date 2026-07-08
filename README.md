# RPM Sensor Kernel Driver
This is a kernel module that makes a virtual RPM sensor as a character device , with a python client for user space interaction with this kernal space module

What it does
* Registers a misc Character device using miscdevices
* Generaates a simulated reaading aat aa fixed interval using a kernel timer
* uses a wait queue which enaables event driven and not polling basically blocks the read()
* A custom IOCTL interface to get and set interval in milliseconds
* Uses Mutex to protect shared space
