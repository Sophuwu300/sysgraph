# sysgraph
Really simple thingy that shows a graph of your cpu and ram usage on most linux distros.
### dependencies
depends on "libboost-dev" "g++" "make"
Debian/Ubuntu install dependencies:
```sh
sudo apt install libboost-dev build-essential
```
### compile
```sh
make
```
### install
```sh
sudo install ./build/sysgraph /usr/local/bin/sysgraph
```
### Usage
Run `sysgraph` in a terminal to see the graph. Press `ctrl`+`c` to exit.
The graph will update every second. The graph shows in an alternate terminal buffer.
On exit with `ctrl`+`c` the program will exit safely and 
the terminal will be restored to its previous state.

### Screenshot
<img src="https://cdn.sophuwu.site/img/sysgraph.png" alt="sysgraph in a terminal"></img>

### Note

Temperature monitoring does not work on ARM processors. All AMD Ryezen and most Intel I series will work though.
