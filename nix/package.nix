{
  # wrapQtAppsHook,
  # qtbase,
  # qtconnectivity,
  kdePackages,
  cmake,
  stdenv,
  src,
}:
stdenv.mkDerivation {
  pname = "airpods-handoff";
  version = "1.0.0";

  inherit src;

  nativeBuildInputs = [
    cmake
    kdePackages.wrapQtAppsHook
  ];

  buildInputs = [
    kdePackages.qtbase
    kdePackages.qtconnectivity
  ];
}
