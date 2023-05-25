with import <nixpkgs> {}; {
  qpidEnv = stdenvNoCC.mkDerivation {
    name = "YarnDesktopClient-environment";
    buildInputs = [
            gcc
            cmake
            gtk4 
            pkg-config
            curl
            libsecret
            rapidjson
    ];
  };
}
