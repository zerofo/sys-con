#pragma once

//This lock is a global lock for the whole application
//Everytime you call an api usbHsXXXX, you have to create this lock.
//For unknown reason, usbHsxxxx APIs cannot be called in parallel, they have to be called one by one.
//Make sure to create this object everytime you call usbHsxxx API

class SwitchUSBLock
{
public:
    SwitchUSBLock(bool scoped = true);
    ~SwitchUSBLock();
	
	void lock();
	void unlock();

private:
	bool m_scoped;
};