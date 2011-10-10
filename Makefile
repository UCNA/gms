all: Bi LED PD

Bi: Bi_pulser.cc
	g++ `root-config --cflags` `root-config --libs` -g Bi_pulser.cc -o Bi_pulser_analysis

LED: led_scan.cc    
	g++ `root-config --cflags` `root-config --libs` -g led_scan.cc -o led_scan_analysis

PD: pd_led_pmt.cc    
	g++ `root-config --cflags` `root-config --libs` -g pd_led_pmt.cc -o pd_led_pmt_analysis

