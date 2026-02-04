{
  description = "LoRaWAN development environment with HackRF gateway support";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    
    # gr-lora_sdr source
    gr-lora-sdr = {
      url = "github:tapparelj/gr-lora_sdr";
      flake = false;  # It's not a flake, just fetch the source
    };
  };

  outputs = { self, nixpkgs, flake-utils, gr-lora-sdr }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { 
          inherit system; 
          config.allowUnfree = true;
        };

        # Build gr-lora_sdr as a package
        gr-lora-sdr-pkg = pkgs.stdenv.mkDerivation {
          pname = "gr-lora_sdr";
          version = "0.0-git";
          src = gr-lora-sdr;

          nativeBuildInputs = with pkgs; [
            cmake
            pkg-config
            swig4
          ];

          buildInputs = with pkgs; [
            gnuradio
            boost
            volk
            spdlog
            libsndfile
            python3
            python3Packages.numpy
            python3Packages.pybind11
          ];

          cmakeFlags = [
            "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
            "-DGnuradio_DIR=${pkgs.gnuradio}/lib/cmake/gnuradio"
          ];

          # Set up Python path
          postInstall = ''
            mkdir -p $out/share/gnuradio/grc/blocks
          '';
        };

        # Python environment with all needed packages
        pythonEnv = pkgs.python3.withPackages (ps: with ps; [
          numpy
          pybind11
          # For the ChirpStack bridge script
          requests
        ]);

      in {
        # Development shell - enter with `nix develop`
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            # Node/embedded development
            platformio
            gcc-arm-embedded
            stlink
            
            # HackRF + GNU Radio
            hackrf
            gnuradio
            gr-lora-sdr-pkg
            
            # Python for gateway scripts
            pythonEnv
            
            # Useful tools
            minicom
            screen
          ];

          shellHook = ''
            echo "🚀 LoRaWAN Dev Environment"
            echo ""
            echo "Node development:"
            echo "  cd node && pio run"
            echo ""
            echo "HackRF gateway:"
            echo "  hackrf_info           # Check HackRF is connected"
            echo "  gnuradio-companion    # Open GNU Radio GUI"
            echo "  python sdr-gateway/gateway.py  # Run gateway"
            echo ""
            
            # Add gr-lora_sdr to paths
            export PYTHONPATH="${gr-lora-sdr-pkg}/lib/python${pkgs.python3.pythonVersion}/site-packages:$PYTHONPATH"
            export GRC_BLOCKS_PATH="${gr-lora-sdr-pkg}/share/gnuradio/grc/blocks:$GRC_BLOCKS_PATH"
          '';
        };

        # Also provide the old shell.nix behavior for backwards compat
        devShells.node = pkgs.mkShell {
          buildInputs = with pkgs; [
            platformio
            gcc-arm-embedded
            stlink
          ];
        };

        # Packages that can be built
        packages = {
          gr-lora-sdr = gr-lora-sdr-pkg;
          default = gr-lora-sdr-pkg;
        };
      }
    );
}
