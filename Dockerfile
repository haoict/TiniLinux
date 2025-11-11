FROM ubuntu:24.04

# Install necessary packages
RUN apt update && apt install -y sudo git build-essential libncurses-dev dosfstools parted mtools curl nano file wget zip unzip cpio rsync bc

# ubuntu user exists, create home directory and set password
RUN mkdir -p /home/ubuntu && \
    chown ubuntu:ubuntu /home/ubuntu && \
    echo "ubuntu:ubuntu" | chpasswd

# Add the user to the sudo group
RUN usermod -aG sudo ubuntu

# Set user and working directory
WORKDIR /home/ubuntu
USER ubuntu

CMD ["/bin/bash", "-c", "sleep infinity"]
