# To build the docker image:
#   docker build -t esl-development .
#
# To run the docker image (after building). '-v' mounts a local directory to a directory in de container
#   Linux (maybe mac?)
#   docker run -it --rm --privileged -v <FCB_SOFTWARE_LOCAL_LOCATION>:/home/user/<FCB_SOFTWARE_DESTINATION> -v /dev/bus/usb:/dev/bus/usb esl-development
#
#   Windows
#   TODO

FROM ubuntu:20.04

ARG USER_ID=1000
ARG GROUP_ID=1000

ARG DEBIAN_FRONTEND=noninteractive

# Install dependencies
RUN apt-get update && \
    apt-get install -y build-essential cmake python3 python2 \
    git wget vim cscope ctags sudo curl software-properties-common libsdl-dev

# Install pip for Python 2
RUN add-apt-repository universe && apt-get update
RUN curl "https://bootstrap.pypa.io/pip/2.7/get-pip.py" --output get-pip.py
RUN python2 get-pip.py && rm get-pip.py

# Install pyserial (to flash software on the FCB)
RUN python2 -m pip install pyserial

# Create a new user
RUN addgroup --gid $GROUP_ID user
#RUN useradd -u $USER_ID -g $GROUP_ID -ms /bin/bash user

# Make the user able to do sudo without a password
RUN adduser --uid $USER_ID --gid $GROUP_ID --disabled-password --gecos '' user

# Add new user docker to sudo group
RUN adduser user sudo

# Add the user to the dialout group (does not work for ub
#RUN usermod -a -G dialout user

# Ensure sudo group users are not
# asked for a password when using
# sudo command by ammending sudoers file
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

# Remove the sudo messasge
RUN touch /home/user/.sudo_as_admin_successful

# Change the working directory
WORKDIR /home/user

# Change the user
USER user
