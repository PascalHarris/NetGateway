#!/bin/bash
if sudo -nv 2>/dev/null && sudo -v ; then
    rootuser=TRUE
else
    rootuser=FALSE
fi
if [ -f /etc/os-release ]; then
    # freedesktop.org and systemd
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
elif type lsb_release >/dev/null 2>&1; then
    # linuxbase.org
    OS=$(lsb_release -si)
    VER=$(lsb_release -sr)
elif [ -f /etc/lsb-release ]; then
    # For some versions of Debian/Ubuntu without lsb_release command
    . /etc/lsb-release
    OS=$DISTRIB_ID
    VER=$DISTRIB_RELEASE
elif [ -f /etc/debian_version ]; then
    # Older Debian/Ubuntu/etc.
    OS=Debian
    VER=$(cat /etc/debian_version)
elif [ -f /etc/SuSe-release ]; then
    # Older SuSE/etc.
    ...
elif [ -f /etc/redhat-release ]; then
    # Older Red Hat, CentOS, etc.
    ...
else
    # Fall back to uname, e.g. "Linux <version>", also works for BSD, etc.
    OS=$(uname -s)
    VER=$(uname -r)
    if [ "$OS" == "Darwin" ]; then
       echo "Running MacOS"
       
       alias pkgupd='brew update'
       alias pkgman='brew install'
    else
       echo "Quitting - I don't know how to install on $OS"
       exit
    fi
fi

if [ "$OS" == "Darwin" ]; then
   alias pkgupd='brew update'
   alias pkgman='brew install'
   temp=$(xcode-select -p)
   if [ "$temp" != "/Applications/Xcode.app/Contents/Developer" ]; then
     touch /tmp/.com.apple.dt.CommandLineTools.installondemand.in-progress;
     PROD=$(softwareupdate -l |
       grep "\*.*Command Line" |
       head -n 1 | awk -F"*" '{print $2}' |
       sed -e 's/^ *//' |
       tr -d '\n')
     softwareupdate -i "$PROD" --verbose;
   fi
#   temp=$(brew)
#   if [ "$temp" == "-bash: brew: command not found" ]; then
#     /usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
#   fi
elif [ "$OS" == "Debian" ] || [ "$OS" == "Ubuntu" ]; then
   [ "$UID" -eq 0 ] || { echo "This script must be run as root."; exit 1;}
   alias pkgupd='apt-get update'
   alias pkgman='apt-get install'
fi
