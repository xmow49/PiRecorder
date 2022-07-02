# PiRecorder

A RaspberryPi software to record audio from a microphone when a button is pressed

## Install libraries

```
wget http://www.airspayce.com/mikem/bcm2835/bcm2835-1.71.tar.gz
tar zxvf bcm2835-1.71.tar.gz
cd bcm2835-1.71/
./configure
make
sudo make check
sudo make install
cd
curl -sL https://github.com/gavinlyonsrepo/SSD1306_OLED_RPI/archive/1.2.tar.gz | tar xz
cd SSD1306_OLED_RPI-1.2
sudo make
```
**Please enable I2C in raspi-config**

## Download Pi Recorder

```
cd
git clone https://github.com/xmow49/PiRecorder
cd PiRecorder
```

## Compile sources

```
g++ main.cpp -o PiRecorder -lpthread -lbcm2835 -lSSD1306_OLED_RPI
```

## Setup service
```
nano service.sh
```
Edit paths

```
chmod +x service.sh
./service.sh
```

And Voilà
