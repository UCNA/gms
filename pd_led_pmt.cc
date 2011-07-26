

#include <stdlib.h>
//#include "options/options.h"

#include <TFile.h>
#include <TTree.h>
#include <TString.h>
#include <TCut.h>
#include <TProfile.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <TGraphErrors.h>
#include <TH2F.h>
#include <TStyle.h>
#include <TApplication.h>
#include <TF1.h>
#include <math.h>

// g++ `root-config --cflags` `root-config --libs` led_scan.cc -o led_scan_analysis


/**
 * Authors: Kevin Peter Hickerson
 *          Michael Mendenhall
 * Date: Aug 2010
 * Modified: July 16, 2011
 */

TF1* FitPedestal(const char *name, TTree *tree, TCut* cut)
{
    char pedestal_name[1024];
    sprintf(pedestal_name, "pedestal_histogram_%s", name);
    char pedestal_draw[1024];
    sprintf(pedestal_draw, "%s >> %s", name, pedestal_name);
    TH1F* pedestal_histogram = new TH1F(pedestal_name, "Pedestal Events", 2000,0,2000);
    tree->Draw(pedestal_draw, *cut);
    int max_bin = pedestal_histogram->GetMaximumBin();
    float max_bin_x = pedestal_histogram->GetBinCenter(max_bin);
    TF1 *fit = new TF1("gauss_fit", "gaus", max_bin_x-12, max_bin_x+12);
    if (!pedestal_histogram->Fit(fit, "R"))
    {
        printf("Pedestal fit success: mu = %g, sigma = %g\n",fit->GetParameter(1),fit->GetParameter(2));
        return fit;
    } 
    else 
    {
        printf("Couldn't fit pedestal to %s.\n", name);
        return 0;
    }
}

int main (int argc, char **argv)
{
    /*
       const char * optv[] = {
       "c:count <number>",
       "s?str   <string>",
       "x|xmode",
       NULL
       } ;

       Options  opts(*argv, optv);
       OptArgvIter  iter(--argc, ++argv);
       const char *optarg, *str = NULL;
       int  errors = 0, xflag = 0, count = 1;

       while( char optchar = opts(iter, optarg) ) {
       switch (optchar) {
       case 's' :
       str = optarg; break;
       case 'x' :
       ++xflag; break;
       case 'c' :
       if (optarg == NULL)  ++errors;
       else  count = (int) atol(optarg);
       break;
       default :  ++errors; break;
       } //switch
       }
     */

    if (argc < 2)
    {
        printf("Usage: %s <daq run number> [scan start time in min] [scan time in min]\n", argv[0]);
        exit(1);
    }
    int run = atoi(argv[1]);
    if (run == 0)
    {
        printf("Need a valid run number.\n");
        printf("Usage: %s <daq run number> [scan start time in min] [scan time in min]\n", argv[0]);
        exit(1);
    }

    float scan_time = 15.0; // in minutes
    float scan_start_time = 1.0; // in minutes
    if (argc > 2) 
    {
        scan_start_time = atof(argv[2]);
    }

    if (argc > 3) 
    {
        scan_time = atof(argv[3]);
    }

    char filename[1024];
    //sprintf(filename, "/home/data_analyzed/2010/rootfiles/full%s.root", argv[1]);
    sprintf(filename, "/data/ucnadata/2010/rootfiles/full%s.root", argv[1]);
    // example: /data/ucnadata/2010/rootfiles/full16290.root

    // run this as a ROOT application
    TApplication app("LED Scans", &argc, argv);

    // Plot options
    gStyle->SetOptStat(1);
    gStyle->SetOptFit(1);

    // Open ntuple
    TFile* myfile = new TFile(filename);
    if (myfile->IsZombie())
    {
        printf("File %s not found.\n", filename);
        exit(1);
    }

    TTree* h1 = (TTree*)myfile->Get("h1");
    if (h1 == NULL)
    {
        printf("TTree not found in file %s.\n", filename);
        exit(0);
    }

    // Define cuts
    TCut *led_cut = new TCut("(int(Sis00) & 128) > 0");           // 129 if east-PMTs on, 161 if GMS-ref also on
    TCut *pedestal_cut = new TCut("!(int(Sis00)&1)");           // 129 if east-PMTs on, 161 if GMS-ref also on
    //TCut *lednumcut = new TCut("(Sis00 == 129) && (Number < 400000)");

    TH2F* his2D[8];
    TProfile* p[8];
    TCanvas* c[8];
    TGraph* g[8];
    TGraphErrors* resg[8];

    TString H2Fname[8] = { "H2FE1", "H2FE2", "H2FE3", "H2FE4", "H2FW1", "H2FW2", "H2FW3", "H2FW4"};
    TString Qadc[8] = { "Qadc0", "Qadc1", "Qadc2", "Qadc3", "Qadc4", "Qadc5", "Qadc6", "Qadc7"};
    TString Pname[8] = { "PE1", "PE2", "PE3", "PE4", "PW1", "PW2", "PW3", "PW4"};
    TString Cname[8] = { "CE1", "CE2", "CE3", "CE4", "CW1", "CW2", "CW3", "CW4"};
    TString Dname[8] = { "DE1", "DE2", "DE3", "DE4", "DW1", "DW2", "DW3", "DW4"};
    TString title[8] = {  "LED Scan E1", "LED Scan E2", "LED Scan E3", "LED Scan E4",
        "LED Scan W1", "LED Scan W2", "LED Scan W3", "LED Scan W4"};

    gStyle->SetPalette(1);
    gStyle->SetOptStat("");

    printf("how much wood can a wood chuck chuck: %i\n", h1->GetEntries());

    TF1 *pd_ped_fit = FitPedestal("Pdc36", h1, pedestal_cut);
    float pd_pedestal = 0;
    if (pd_ped_fit)
        pd_pedestal = pd_ped_fit->GetParameter(1);
    else
        printf("couldn't fit pedestal to Pdc36\n");
    printf("pd_pedistal=%f\n", pd_pedestal);

    for (unsigned i = 0; i < 8; i++) 
    {
        c[i] = new TCanvas(Cname[i], title[i]);
        c[i]->Divide(2,1);
        c[i]->cd(1);

        // find Pedestal
        TF1 *ped_fit = FitPedestal(Qadc[i], h1, pedestal_cut);
        float pedestal = 0;
        if (ped_fit)
            pedestal = ped_fit->GetParameter(1);

        // Define histograms
        his2D[i] = new TH2F(H2Fname[i], "", 
                //(int)(1.2*(scan_time+scan_start_time)*6), 0, 1.2*(scan_time+scan_start_time)*60, 
                512, 0, 2047/4,
                1<<8, -pedestal, 4096-pedestal);
        //char draw_cmd[1024];
        //sprintf(draw_cmd, "%s-%f:S83028/1e6 >> %s", Qadc[i], pedestal, H2Fname[i]);
        //sprintf(draw_cmd, "(%s-%f):(Pdc36-%f) >> %s", Qadc[i], pedestal, pd_pedestal, H2Fname[i]);
        TString draw_cmd = "(";
        draw_cmd+= Qadc[i] ;
        draw_cmd+= "-" ;
        draw_cmd+= pedestal ;
        draw_cmd+= "):(Pdc36-" ;
        draw_cmd+= pd_pedestal  ;
        draw_cmd+= ") >> " ;
        draw_cmd+= H2Fname[i]; 
        h1->Draw(draw_cmd, *led_cut);
        his2D[i]->SetTitle(title[i]);

        int nled = (int)his2D[i]->GetEntries();

        if(!nled) {
            printf("Empty histogram for %i; skipping.\n",i);
            continue;
        }
        printf("Found %i LED events.\n", nled);

        printf("Building TProfile...\n");	
        p[i] = his2D[i]->ProfileX(Pname[i]);
        p[i]->SetErrorOption("s");


        // find the number of photoelectrons and plot
        printf("Building nPE plot...\n");	
        const int n = p[i]->GetNbinsX();
        g[i] = new TGraph(n);
        const float max_npe = 1000;
        for (unsigned j = 0; j < n; j++)
        {
            float x = p[i]->GetBinCenter(j);
            float w = p[i]->GetBinError(j);
            float y = (w<1.0)?0:pow(p[i]->GetBinContent(j)/w,2);
            y = (y>max_npe)?max_npe:y;
            g[i]->SetPoint(j, x, y);
        }

        // Find if the approximate range of the fit
        printf("Finding range...\n");
        float rough_delta = 0;
        float prev_p_i = 0;
        unsigned range_max = 275;
        for (unsigned j = 0; j < n; j += n/20)
        {
            float _p_i = p[i]->GetBinContent(j);
            float _rough_delta = _p_i - prev_p_i;
            if (_rough_delta > 0.5 * rough_delta)
            {
                range_max = j - 20;
                printf("found range for p[i] %i\n", range_max); 
                break;
            }
            prev_p_i = _p_i;
            rough_delta = _rough_delta;
        }

        // fit a more accurate line
        printf("Fitting...\n");	
        //p[i]->Rebin(4);
        //TF1 *fit = new TF1("polyfit", "pol1", scan_start_time*60, (scan_time + scan_start_time)*60);
        //TF1 *fit = new TF1("polyfit", "pol1", scan_start_time, (scan_time + scan_start_time));
        TF1 *fit = new TF1("polyfit", "pol1", 0, range_max);
        if (p[i]->Fit(fit, "R"))
            continue;
        printf("Plotting LED intensity...\n");	
        p[i]->SetMarkerColor(2);
        p[i]->SetLineColor(2);
        p[i]->SetMarkerStyle(8);
        p[i]->SetMarkerSize(0.75);
        //his1.Fit("gaus");

        his2D[i]->Draw("colz");
        p[i]->Draw("Same");
        p[i]->SetErrorOption(); // actual fit errors, not Poisson error bars for residual plot
        resg[i] = new TGraphErrors(p[i]->GetNbinsX());
        for (unsigned j = 0; j < p[i]->GetNbinsX(); j++)
        {
            float x = p[i]->GetBinCenter(j);

            float b = p[i]->GetBinContent(j);
            float y=0;
            if (b != 0)
                y = (b-fit->Eval(x))/b;
            if (y > 0.1)
                y = 0.1;
            if (y < -0.1)
                y = -0.1;

            //if(60*scan_start_time <= x) { // && x <= 60*(scan_start_time+scan_time)) {	
            if(x >= 0) { // && x <= 60*(scan_start_time+scan_time)) {	
                resg[i]->SetPoint(j, x, y);
                if(fabs(b) > 10) 
                    resg[i]->SetPointError(j, 0, p[i]->GetBinError(j)/b);
            } else {
                resg[i]->SetPoint(j,0,0);
            }
        }
        p[i]->SetErrorOption("s");
        c[i]->GetPad(2)->Divide(1,2);

        printf("Plotting nPE...\n");	
        c[i]->GetPad(2)->cd(1);
        if(g[i]->Fit(fit,"R"))
            continue;
        g[i]->SetTitle("Number of Photoelectrons");
        g[i]->Draw("AL");

        printf("Plotting residuals...\n");	
        c[i]->GetPad(2)->cd(2);
        resg[i]->SetTitle("PMT Linearity Residual");
        resg[i]->SetMinimum(-0.1);
        resg[i]->SetMaximum(0.1);
        resg[i]->Draw("AP");
        g[i]->SetLineColor(4);
    }

    TCanvas *ew_canvas = new TCanvas("PMT_linerarity_canvas", "PMT linearity scans for all tubes", 1280, 720);
    ew_canvas->Divide(2,1);
    for (int ew = 0; ew < 2; ew++) {
        ew_canvas->GetPad(ew+1)->Divide(2,2);
        for (int tx = 0; tx < 2; tx++) {
            for (int ty = 0; ty < 2; ty++) {
                ew_canvas->GetPad(ew+1)->cd(tx+2*ty+1);
                p[4*ew+tx+2*ty]->Draw("");
            }
        }
    }
    TString gif_filename = "/data/kevinh/gms/images/pd_led_pmt_";
    gif_filename += argv[1];
    gif_filename += ".gif";
    ew_canvas->SaveAs(gif_filename);

    //app.Run();

    return 0;
}
