# libpacp_repair_tool
Delete broken packets and duplicate file header in libpcap file. 

# Useage
Pull the .pcap file on to the .exe compiled from this project.

The program will create repaired .pacp file in the same folder with source .pcap file.

See "Rename origin .pcap file" about the output file names. 

# Rename origin .pcap file
The rename feature only tested on win10 x64.

If rename fuction is on, the program will add "_origin" after the origin name of input file.

And the name of output file will be the origin name of input file.

    Ex.  
    Input: 123.pcap
    Origin: 123_origin.pcap
    Repaired: 123.pcap


If rename fuction is off(not compiled by VS), the output file name will be the name which add "_integrated" affer the origin input file name.

And the name of origin file will not be changed.
    Ex.  
    Input: 123.pcap
    Origin: 123.pcap
    Repaired: 123_integrated.pcap

>Repaired: 123_integrated.pcap
