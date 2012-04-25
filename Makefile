
all: pulser scan pmt

pulser:
	g++ `root-config --cflags` `root-config --libs` Bi_pulser.cc -o Bi_pulser_analysis

scan:
	g++ `root-config --cflags` `root-config --libs` led_scan.cc -o led_scan_analysis

pmt:	
	g++ `root-config --cflags` `root-config --libs` pd_led_pmt.cc -o pd_led_pmt_analysis

clean:
	rm Bi_pulser_analysis
	rm led_scan_analysis
	rm pd_led_pmt_analysis
	rm *.o

