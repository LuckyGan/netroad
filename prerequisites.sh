#! /bin/bash

# For Ubuntu 14.04.5 LTS

# Upgrade gcc

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update
sudo apt-get install -y gcc-4.9 g++-4.9
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.9 60 \
	--slave /usr/bin/g++ g++ /usr/bin/g++-4.9

# Prerequisites

sudo apt-get install -y \
	python python-dev python-pip \
	mercurial python-setuptools git \
	qt5-default \
	python-pygraphviz python-kiwi python-pygoocanvas \
	libgoocanvas-dev ipython \
	openmpi-bin openmpi-common openmpi-doc libopenmpi-dev \
	autoconf cvs bzr unrar \
	gdb valgrind \
	uncrustify \
	doxygen graphviz imagemagick \
	texlive texlive-extra-utils texlive-latex-extra \
	texlive-font-utils texlive-lang-portuguese dvipng \
	python-sphinx dia \
	gsl-bin libgsl0-dev libgsl0ldbl \
	flex bison libfl-dev \
	tcpdump \
	sqlite sqlite3 libsqlite3-dev \
	libxml2 libxml2-dev \
	cmake libc6-dev libc6-dev-i386 libclang-dev \
	libgtk2.0-0 libgtk2.0-dev \
	vtun lxc \
	libboost-signals-dev libboost-filesystem-dev

sudo pip install cxxfilt

sudo apt-get install -y \
	libncurses-dev libssl-dev libfs-dev libdb-dev \
	libpcap-dev libsysfs-dev
