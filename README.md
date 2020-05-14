# contec_linux_dio

## npm module

### build

```
npm install
```


### setup contec DIO module

please see the contec/cdio directory

We have to change access condition of /dev/cdiousb000 to read/write

So plase install systemctl unit files

```
sudo cp contec/run*.* /etc/systemd/system
sudo systemctl enable run-contec.path
sudo systemctl enable run-chmod.service
sudo systemctl restart run-contec.path
sudo systemctl restart run-chmod.service

```

then when you plug in the cable of USB for contec DIO module,
access condition of the /dev/cdiousb000 is chaned 666.


## COMMAND version setup 

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

## setting

```
cd contec/cdio/config
sudo ./config
sudo ./contec_dio_start.sh
```



