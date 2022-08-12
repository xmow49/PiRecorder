#!/bin/bash
sudo systemctl stop pi-recorder.service
sudo cp ./pi-recorder.service /etc/systemd/system
cp ./bin/ARM/Debug/PiRecorder.out /root/PiRecorder.out

sudo systemctl daemon-reload

sudo systemctl enable pi-recorder.service
sudo systemctl start pi-recorder.service
sudo systemctl status pi-recorder.service