// TargetApplication.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <iostream>


int main()
{
    std::cout << "Start!\n";
    SleepEx(1000 * 60, true);
    std::cout << "Done\n";
    system("pause");
}