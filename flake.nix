{
  inputs = {
    nixpkgs = {
      url = "github:nixos/nixpkgs/nixos-unstable";
    };
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
  };
  outputs = { nixpkgs, flake-utils, ... }: flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs {
        inherit system;
      };
      yarndesktopclient = (with pkgs; stdenv.mkDerivation {
          pname = "yarndesktopclient";
          version = "0.0.0";
          src = fetchgit {
            url = "https://github.com/stig-atle/yarndesktopclient";
            #rev = "v3.3.1";
            sha256 = "i+uLiWcslsMMIwOlBU6ZPKRvtA0yvN7uw2XvG9aLQr8=";
            fetchSubmodules = false;
          };
          nativeBuildInputs = [
            gcc
            cmake
            gtk4 
            pkg-config
            curl
            libsecret
            rapidjson
          ];
          buildPhase = "make -j $NIX_BUILD_CORES";
          installPhase = ''
            mkdir -p $out/bin
            mv $TMP/yarndesktopclient $out/bin
          '';
        }
      );
    in rec {
      defaultApp = flake-utils.lib.mkApp {
        drv = defaultPackage;
      };
      defaultPackage = yarndesktopclient;
      devShell = pkgs.mkShell {
        buildInputs = [
          yarndesktopclient
        ];
      };
    }
  );
}
