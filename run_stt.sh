#!/bin/sh

if [ ! -d "onsem/stt/models/${1}" ]
then
    unzip onsem/stt/models/${1}.zip -d onsem/stt/models/
fi

./onsem/stt/write_asr_result_in_a_file.py -m onsem/stt/models/${1} -f build/voicebotgui/out_asr.txt
