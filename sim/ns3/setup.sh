#!/bin/bash
set -e

git clone https://gitlab.com/nsnam/ns-3-dev.git && cd ns-3-dev &&
git clone https://github.com/signetlabdei/lorawan src/lorawan &&
tag=$(< src/lorawan/NS3-VERSION) && tag=${tag#release } && git checkout $tag -b $tag

# Configure and build
echo "Configuring NS-3..."
./ns3 configure --enable-examples --enable-tests --enable-modules lorawan

echo "Building NS-3 (this may take a while)..."
./ns3 build

echo "✓ Setup complete! Run './ns3 run <simulation>' to execute"
