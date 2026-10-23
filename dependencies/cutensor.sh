#!/usr/bin/env bash
wget https://developer.download.nvidia.com/compute/cutensor/2.8.1/local_installers/cutensor-local-repo-ubuntu2604-2.8.1_2.8.1-1_amd64.deb
sudo dpkg -i cutensor-local-repo-ubuntu2604-2.8.1_2.8.1-1_amd64.deb
sudo cp /var/cutensor-local-repo-ubuntu2604-2.8.1/cutensor-*-keyring.gpg /usr/share/keyrings/
sudo apt-get update
sudo apt-get -y install cutensor
sudo apt-get -y install cutensor-cuda-13