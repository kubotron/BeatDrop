#diskutil mount /dev/disk2s1
cd Release
../utils/build/pre_build.sh
make
../utils/build/post_build.sh
#todo select vario from a list of usb devices
cp SKYDROP.FW /Volumes/NO\ NAME/
echo "+++++++DONE+++++++++"
echo "will unmount /Volumes/NO\ NAME/"
diskutil unmount /Volumes/NO\ NAME/
