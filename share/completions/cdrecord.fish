#
# Completions for Cdrecord
#

complete -c cdrecord -o version -d (N_ "Display version and exit") 
complete -c cdrecord -s v -d (N_ "Increment the level of general verbosity by one")
complete -c cdrecord -s V -d (N_ "Increment the verbose level in respect of SCSI command transport by one")
complete -c cdrecord -a "debug={1,2,3,4,5}" -d (N_ "Set the misc debug value to #")
complete -c cdrecord -s d -d (N_ "Increment the misc debug level by one") -a "1 2 3 4 5"
complete -c cdrecord -s s -o silent -d (N_ "Do not print out a status report for failed SCSI commands")
complete -c cdrecord -o force -d (N_ "Force to continue on some errors")
complete -c cdrecord -o immed -d (N_ "Tell cdrecord to set the SCSI IMMED flag in certain commands")
complete -c cdrecord -a "minbuf={25,35,45,55,65,75,85,95}" -d (N_ "Defines the minimum drive buffer fill ratio for the experimental ATAPI wait mode intended to free the IDE bus to allow hard disk and CD/DVD writer on the same IDE cable")
complete -c cdrecord -o dummy -d (N_ "Complete CD/DVD-Recorder recording process with the laser turned off")
complete -c cdrecord -o clone -d (N_ "Tells  cdrecord to handle images created by readcd -clone")
complete -c cdrecord -o dao
complete -c cdrecord -o sao -d (N_ "Set SAO (Session At Once) mode, usually called Disk At Once mode")
complete -c cdrecord -o tao -d (N_ "Set  TAO  (Track At Once) writing mode")
complete -c cdrecord -o raw -d (N_ "Set RAW writing mode")
complete -c cdrecord -o raw96r -d (N_ "Select Set RAW writing, the preferred raw writing mode")
complete -c cdrecord -o raw96p -d (N_ "Select Set RAW writing, the less preferred raw writing mode")
complete -c cdrecord -o raw16 -d (N_ "Select Set RAW writing, the preferred raw writing mode if raw96r is not supported")
complete -c cdrecord -o multi -d (N_ "Allow  multi  session CDs to be made")
complete -c cdrecord -o msinfo -d (N_ "Retrieve multi session info in a form suitable for mkisofs-1.10 or later")
complete -c cdrecord -o toc -d (N_ "Retrieve and print out the table of content or PMA of a CD")
complete -c cdrecord -o atip -d (N_ "Retrieve and print out the ATIP (absolute Time in Pre-groove) info")
complete -c cdrecord -o fix -d (N_ "The disk will only be fixated")
complete -c cdrecord -o nofix -d (N_ "Do not fixate the disk after writing the tracks")
complete -c cdrecord -o waiti -d (N_ "Wait for input to become available on standard input before trying to open the SCSI driver")
complete -c cdrecord -o load -d (N_ "Load the media and exit")
complete -c cdrecord -o lock -d (N_ "Load  the media, lock the door and exit")
complete -c cdrecord -o eject -d (N_ "Eject disk after doing the work")
complete -c cdrecord -a "speed={0,150,172,1385}" -d (N_ "Set the speed factor of the writing process to #") 
complete -c cdrecord -a "blank={help,all,fast,track,unreserve,trtail,unclose,session}" -d (N_ "Blank a CD-RW and exit or blank a CD-RW before writing")
complete -c cdrecord -o format -d (N_ "Format a CD-RW/DVD-RW/DVD+RW disc")
complete -c cdrecord -a "fs=" -d (N_ "Set  the  FIFO  (ring  buffer) size to #")
complete -c cdrecord -a "ts=" -d (N_ "Set the maximum transfer size for a single SCSI command to #")
complete -c cdrecord -a "dev=" -d (N_ "Sets  the  SCSI  target  for  the CD/DVD-Recorder")
complete -c cdrecord -a "gracetime=" -d (N_ "Set the grace time before starting to write to ># seconds")
complete -c cdrecord -a "timeout=" -d (N_ "Set the default SCSI command timeout value to # seconds")
complete -c cdrecord -a "driver={help,mmc_cd,mmc_cd_dvd,mmc_cdr,mmc_cdr_sony,mmc_dvd,mmc_dvdplus,mmc_dvdplusr,mmc_dvdplusrw,cw_7501,kodak_pcd_600,philips_cdd521,philips_cdd521_old,philips_cdd522,philips_dumb,pioneer_dws114x,plasmon_rf4100,ricoh_ro1060c,ricoh_ro1420c,scsi2_cd,sony_cdu924,teac_cdr50,tyuden_ew50,yamaha_cdr100,cdr_simul,dvd_simul}" -d (N_ "Allows the user to manually select a driver for the device")

# TODO: This argument accepts a comma separated list of arguments

complete -c cdrecord -a "driveropts=" -d (N_ "Set driver specific options")
complete -c cdrecord -o setdropts -d (N_ "Set the driveropts specified by driveropts=option list, the speed of the drive and the dummy flag and exit")
complete -c cdrecord -o checkdrive -d (N_ "Checks if a driver for the current drive is present and exit")
complete -c cdrecord -o prcap -d (N_ "Print the drive capabilities for SCSI-3/mmc compliant drives as obtained from mode page 0x2A")
complete -c cdrecord -o inq -d (N_ "Do an inquiry for the drive, print the inquiry info and exit")
complete -c cdrecord -o scanbus -d (N_ "Scan all SCSI devices on all SCSI busses and print the inquiry strings")
complete -c cdrecord -o reset -d (N_ "Try to reset the SCSI bus where the CD recorder is located")
complete -c cdrecord -o abort -d (N_ "Try to send an abort sequence to the drive")
complete -c cdrecord -o overburn -d (N_ "Allow cdrecord to write more than the official size of a medium")
complete -c cdrecord -o ignsize -d (N_ "Ignore the known size of the medium, use for debugging only")
complete -c cdrecord -o useinfo -d (N_ "Use *.inf files to overwrite audio options")
complete -c cdrecord -a "defpregap=" -d (N_ "Set the  default pre-gap size for all tracks except track nr 1")
complete -c cdrecord -o packet -d (N_ "Set Packet writing mode (experimental interface)")
complete -c cdrecord -a "pktsize=" -d (N_ "Set the packet size to #, forces fixed packet mode (experimental)")
complete -c cdrecord -o noclose -d (N_ "Do not close the current track, only when in packet writing mode (experimental)")
complete -c cdrecord -a "mcn=" -d (N_ "Set the Media Catalog Number of the CD")
complete -c cdrecord -o text -d (N_ "Write CD-Text info based on info taken from a file that contains ascii info for the text strings")
complete -c cdrecord -a "textfile=" -d (N_ "Write CD-Text based on info found in the binary file filename")
complete -c cdrecord -a "cuefile=" -d (N_ "Take all recording related info from a CDRWIN compliant CUE sheet file")

# These completions are TRACK OPTIONS

complete -c cdrecord -a "isrc=" -d (N_ "Set the International Standard Recording Number for the next track")
complete -c cdrecord -a "index=" -d (N_ "Sets an index list for the next track")
complete -c cdrecord -o audio -d (N_ "All subsequent tracks are written in CD-DA audio format")
complete -c cdrecord -o swab -d (N_ "Audio data is assumed to be in  byte-swapped (little-endian) order")
complete -c cdrecord -o data -d (N_ "All subsequent tracks are written in CD-ROM mode 1 (Yellow Book) format")
complete -c cdrecord -o mode2 -d (N_ "All subsequent tracks are written in CD-ROM mode 2 format")
complete -c cdrecord -o xa -d (N_ "All subsequent tracks are written in CD-ROM XA mode 2 form 1 format")
complete -c cdrecord -o xa1 -d (N_ "All subsequent tracks are written in CD-ROM XA mode 2 form 1 format")
complete -c cdrecord -o xa2 -d (N_ "All subsequent tracks are written in CD-ROM XA mode 2 form 2 format")
complete -c cdrecord -o xamix -d (N_ "All subsequent tracks are written in a way that allows a mix of CD-ROM XA mode 2 form 1/2 format")
complete -c cdrecord -o cdi -d (N_ "The TOC type for the disk is set to CDI, with XA only")
complete -c cdrecord -o isosize -d (N_ "Use  the  ISO-9660  file system size as the size of the next track")
complete -c cdrecord -o pad -d (N_ "15 sectors of zeroed data will be added to the end of this and each subsequent data track")
complete -c cdrecord -a "padsize=" -d (N_ "Set the amount of data to be appended as padding to the next track")
complete -c cdrecord -o nopad -d (N_ "Do not pad the following tracks - the default")
complete -c cdrecord -o shorttrack -d (N_ "Allow all subsequent tracks to violate the Red Book track length standard (min 4 s)")
complete -c cdrecord -o noshorttrack -d (N_ "Re-enforce the Red Book track length standard (min 4 s)")
complete -c cdrecord -a "pregap=" -d (N_ "Set the  pre-gap size for the next track")
complete -c cdrecord -o preemp -d (N_ "All TOC entries for subsequent audio tracks will indicate that the audio data has been sampled with 50/15 microsec pre-emphasis")
complete -c cdrecord -o nopreemp -d (N_ "All TOC entries for subsequent audio tracks will indicate that the audio data has been mastered with linear data")
complete -c cdrecord -o copy -d (N_ "All TOC entries for subsequent audio tracks of the resulting CD will indicate that the audio data has permission to be copied without limit")
complete -c cdrecord -o nocopy -d (N_ "All TOC entries for subsequent audio tracks of the resulting CD will indicate that the audio data has permission to be copied only once for personal use")
complete -c cdrecord -o scms -d (N_ "All TOC entries for subsequent audio tracks of the resulting CD will indicate that the audio data has no permission to be copied")
complete -c cdrecord -a "tsize=" -d (N_ "If the master image for the next track has been stored on a raw disk, use this option to specify the valid amount of data on this disk")


