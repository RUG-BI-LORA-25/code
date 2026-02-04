{
  description = "LoRaWAN development environment with HackRF gateway support";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    
    gr-lora-sdr = {
      url = "github:tapparelj/gr-lora_sdr";
      flake = false; 
    };
  };

  outputs = { self, nixpkgs, flake-utils, gr-lora-sdr }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { 
          inherit system; 
          config.allowUnfree = true;
        };

        gnuradioWrapped = pkgs.gnuradio.override {
          extraPackages = with pkgs.gnuradioPackages; [
            osmosdr
          ];
          extraPythonPackages = with pkgs.python3Packages; [
            numpy
            requests
            pyzmq
          ];
        };

        gr-lora-sdr-pkg = pkgs.stdenv.mkDerivation {
          pname = "gr-lora_sdr";
          version = "0.0-git";
          src = gr-lora-sdr;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
            swig
          ];

          buildInputs = with pkgs; [
            gnuradioWrapped
            gnuradioWrapped.python
            gnuradioWrapped.python.pkgs.numpy
            gnuradioWrapped.python.pkgs.pybind11
            boost
            volk
            spdlog
            libsndfile
            gmp
            mpfr
          ];

          cmakeFlags = [
            "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
            "-DGnuradio_DIR=${gnuradioWrapped}/lib/cmake/gnuradio"
            "-DGMP_INCLUDE_DIR=${pkgs.gmp.dev}/include"
            "-DGMP_LIBRARY=${pkgs.gmp}/lib/libgmp.so"
            "-DGMPXX_LIBRARY=${pkgs.gmp}/lib/libgmpxx.so"
          ];

          postInstall = ''
            mkdir -p $out/share/gnuradio/grc/blocks
          '';
        };

      in {
        devShells.default = pkgs.mkShell {
          buildInputs = [
            # Node/embedded development
            pkgs.platformio
            pkgs.gcc-arm-embedded
            pkgs.stlink
            
            # HackRF + GNU Radio (with Python bindings included)
            pkgs.hackrf
            gnuradioWrapped
            gnuradioWrapped.pythonEnv
            gr-lora-sdr-pkg
          ];

          shellHook = ''
            echo "Node development:"
            echo "  cd node && pio run"
            echo ""
            echo "HackRF gateway:"
            echo "  hackrf_info           # Check HackRF is connected"
            echo "  gnuradio-companion    # Open GNU Radio GUI"
            echo "  python3 sdr-gateway/lora_rx.py  # Run LoRa receiver"
            echo ""
            
            export PYTHONPATH="${gr-lora-sdr-pkg}/lib/python3.13/site-packages:$PYTHONPATH"
            export GRC_BLOCKS_PATH="${gr-lora-sdr-pkg}/share/gnuradio/grc/blocks:$GRC_BLOCKS_PATH"
            
            # Increase GNU Radio buffer size for SF12 (needs 16384+ samples)
            export GR_CONF_DEFAULT_BUFFER_SIZE=65536
          '';
        };

        devShells.node = pkgs.mkShell {
          buildInputs = with pkgs; [
            platformio
            gcc-arm-embedded
            stlink
          ];
        };

        packages = {
          gr-lora-sdr = gr-lora-sdr-pkg;
          default = gr-lora-sdr-pkg;
        };
      }
    );
}