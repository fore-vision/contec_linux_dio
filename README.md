# contec_linux_dio


## setup 

```
cd contec/cdio
make
sudo make install
cd cofnig
sudo ./contec_dio_start.sh

mkdir build
cd build
cmake ..
make
sudo make install
sudo chmod u+s /usr/local/bin/contecdio 
sudo cp contec/cdio/config/rc.local /etc
```

