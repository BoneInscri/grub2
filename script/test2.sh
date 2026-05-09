sudo bash install_hvisor.sh
cd examples
sudo bash daemon.sh test/virtio_cfg2.json
sleep 3
cd start_zone
sudo bash start_zone2.sh 