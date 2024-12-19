#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Update the system
sudo apt update && sudo apt upgrade -y

# Install Java Runtime Environment (JRE)
echo "Installing JRE..."
sudo apt install -y default-jre

# Verify JRE installation
java -version || { echo "Error during JRE installation"; exit 1; }

# Install MariaDB
echo "Installing MariaDB..."
sudo apt install -y mariadb-server mariadb-client

# Enable and start MariaDB
sudo systemctl enable mariadb
sudo systemctl start mariadb

# Configure MariaDB (you can customize root password and permissions here)
echo "Configuring MariaDB..."
sudo mysql_secure_installation <<EOF
n
root
root
y
y
y
y
EOF

# Verify MariaDB installation
sudo systemctl status mariadb | grep "active (running)" || { echo "Error during MariaDB installation"; exit 1; }

# Add Grafana repository
echo "Adding Grafana repository..."
GPG_KEY_URL="https://packages.grafana.com/gpg.key"
REPO_FILE="/etc/apt/sources.list.d/grafana.list"
curl -fsSL $GPG_KEY_URL | sudo gpg --dearmor -o /usr/share/keyrings/grafana-archive-keyring.gpg
echo "deb [signed-by=/usr/share/keyrings/grafana-archive-keyring.gpg] https://packages.grafana.com/oss/deb stable main" | sudo tee $REPO_FILE

# Update repositories and install Grafana
sudo apt update
sudo apt install -y grafana

# Enable and start Grafana
sudo systemctl enable grafana-server
sudo systemctl start grafana-server

# Verify Grafana installation
sudo systemctl status grafana-server | grep "active (running)" || { echo "Error during Grafana installation"; exit 1; }

# Final message
echo "Installation completed!"
echo " - JRE installed successfully."
echo " - MariaDB installed and configured successfully."
echo " - Grafana installed and running at http://<RASPBERRY_IP>:3000"
echo "Use admin/admin to log in initially to Grafana."

