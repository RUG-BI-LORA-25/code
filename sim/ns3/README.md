# Environment

## Installation
Run `setup.sh` and pray to god you got all the dependencies installed. Kidding:

(For Ubuntu/Debian)

```bash
sudo apt install g++ python3 cmake ninja-build git ccache
```

Foro ther OSes, figure it tf out.

## Usage
Once you've build ns3 (by running `setup.sh`), you can run simulations like so:

```bash
./ns-3-dev/ns3 run <script-name>
``` 

To test, do `./ns-3-dev/test.py` (that shit is 3000 lines!)