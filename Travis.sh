# this script is intended to be run by Travis
# see https://travis-ci.org/avilleret/Gem

if [[ "$TRAVIS_OS_NAME" = "linux" ]]; then
  #sudo apt-get install autopoint dh-autoreconf libdv4-dev libiec61883-dev \
  #  libmagick++-dev libmagick++5 libmagickwand-dev libpopt-dev libquicktime-dev libquicktime2 libunicap2 libunicap2-dev \
  #  libftgl-dev libfreetype6-dev libgl1-mesa-dev libglu1-mesa-dev \
  #  libdc1394-dev
  sudo apt-get update -qq
  sudo apt-get build-dep gem -qq
  sudo apt-get install libglfw-dev libglfw3-dev libsdl2-devs -qq

  # wget http://msp.ucsd.edu/Software/pd-0.46-2.src.tar.gz
  # tar -xvf pd-0.46-2.src.tar.gz

  ./autogen.sh && ./configure --without-ftgl --with-pd=$(pwd)/pd-0.46-2/src && make
else
  brew update
  brew install pkg-config gettext
  brew install fribidi --universal
  brew link gettext --force
  brew install imagemagick ftgl
  brew install sdl homebrew/versions/glfw2 homebrew/versions/glfw3

  wget http://msp.ucsd.edu/Software/pd-0.46-2-64bit.mac.tar.gz
  tar -xvf pd-0.46-2-64bit.mac.tar.gz

  ./autogen.sh
  ./configure --with-pd=$(pwd)/Pd-0.46-2-64bit.app/Contents/Resources/  --without-ftgl --without-QuickTime-framework --without-Carbon-framework && make
fi
