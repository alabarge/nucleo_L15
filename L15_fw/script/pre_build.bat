..\utils\fw_ver.exe ..\L15_fw\build.inc ..\L15_fw\build.h ..\.git
echo f|xcopy ..\L15_fw\build.h ..\L15_fw\share\build.h /F /Y /R
echo f|xcopy ..\L15_fw\cp_srv\cp_msg.h ..\L15_fw\share\cp_msg.h /F /Y /R
echo f|xcopy ..\L15_fw\daq_srv\daq_msg.h ..\L15_fw\share\daq_msg.h /F /Y /R
