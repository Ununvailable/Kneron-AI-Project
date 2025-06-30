#!/bin/bash

# example list to copy
files=(
    "install_driver_windows"
    "scan_devices"
    "kl720_update_kdp2_to_kdp2_flash_boot"
    "generic_inference"
    "generic_command"
)

function _kbuild ()
{
    build_name=$1;
    if [[ -z $build_name ]]; then
        build_name=build;
    fi;
    \rm -rf $build_name;
    \mkdir $build_name;
    cd $build_name;
    cmake -DCMAKE_COLOR_DIAGNOSTICS=1 .. -G"MSYS Makefiles";
    make -O -j ${files[@]};
    cd -
}

# Variable initialization
pack_folder_name="mptool2_v$(cat VERSION)"
firmware_path="res/firmware/KL720/"
model_path="res/models/KL720/YoloV5s_640_640_3/"
exec_path="res/bin/"
image_path="res/images/"

# build PLUS if "-b|--build" is specified
if [[ -z $1 ]];then
    :
elif [[ "$1" = "--build" ]] || [[ "$1" = "-b" ]];then
    saved_pwd=$(pwd)
    cd ../../ > /dev/null
    _kbuild
    cd $saved_pwd
else
    echo "Error! uncognized parameter"
    exit 1
fi

echo " Packing mptool2 ...."
mkdir $pack_folder_name
cd $pack_folder_name > /dev/null


# cp exectuables to mptool
mkdir -p "$exec_path"

for file in "${files[@]}"; do
    cp -f ../../../build/bin/$file ./$exec_path/
done

dlls=(
    "libkplus.dll"
    "libstdc++-6.dll"
    "libusb-1.0.dll"
    "libwdi.dll"
    "libwinpthread-1.dll"
)

for dll in "${dlls[@]}"; do
    cp -f ../../../build/bin/$dll ./$exec_path/
done


# cp firmare
mkdir -p "$firmware_path"
cp -f ../../../$firmware_path/fw*.bin ./$firmware_path/

# cp test model
mkdir -p "$model_path"
cp -f ../../../$model_path/models_720.nef ./$model_path/

# cp test image
mkdir -p "$image_path"
cp -f ../../../$image_path/car_park_barrier_608x608.bmp $image_path/

# cp readme
# append version in REMDME.md
cp ../README.md ./
echo "# Note" >> README.md
echo -e "connecting Kneron PLUS version: $(cat ../VERSION)" >> README.md

# cp VERSION
cp -f ../VERSION ./

# cp golden.txt
cp -f ../golden.txt ./res/

# cp mptool2.bat
cp -f ../mptool2.bat ./

# cp thirdparty (zadig)
cp -rf ../thirdparty ./

cd - > /dev/null

# zip the package
zip -q ${pack_folder_name}.zip ./$pack_folder_name
if [[ $? != 0 ]];then
    echo "ERROR! fail on creating zip file"
    exit 1
fi

# print summary
echo
echo
echo " --- Packing Summary ---"
tree --dirsfirst --sort name ./$pack_folder_name/

echo
echo "[Done] ${pack_folder_name}.zip is prepared"

echo
echo "!! Remember to check if VERSION is aligned with PLUS version !!"
