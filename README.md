# Container RH sensor firmware

## Setup socat server

```bash
PORT=9999
sudo ufw allow $PORT/tcp
sudo ufw reload
sudo cp ./forward_socket@.service /etc/systemd/system
sudo systemctl daemon-reload
sudo systemctl enable --now "forward_socket@$PORT.service"
```
