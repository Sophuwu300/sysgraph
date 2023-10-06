This simple program allows you to monitor your system's resources in the terminal. It uses /proc and /sys to get the information and shows them in a graph in the terminal. The graph will automatically scale to the size of the window, even if you resize it while the program is running. It also has logging functionality, so you can save the data to a file.

This program was originally written in Go. However, it was too slow and would cause screen tearing at higher resolutions. So I made it in C++ instead to solve the performance issues.

<img src="https://skisiel.com/files/sysmon/img/sysmon.png">
