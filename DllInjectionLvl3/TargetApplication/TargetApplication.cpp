#include <Windows.h>
#include <iostream>

int main()
{
    int iteration = 1;

    std::cout << "Start\n";
    while (1) {
        printf("Loop iteration %d\n", iteration);
        SleepEx(1 * 1000, NULL);
        iteration += 1;
    }
    std::cout << "Done!";

    system("pause");
}