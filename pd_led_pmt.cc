

#include <stdlib.h>
#include "options/options.h"

#include <TFile.h>
//#include <TTree.h>
#include <TChain.h>
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



/**
 * Authors: Kevin Peter Hickerson
 *          Michael Mendenhall
 * Date: Aug 2010
 * Modified: July 16, 2011
 * Makefile: g++ `root-config --cflags` `root-config --libs` led_scan.cc -o led_scan_analysis
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
    int run[argc];
    if (argc < 2)
    {
        printf("Usage: %s <daq run number> [<second run number>]\n", argv[0]);
        exit(1);
    }
    else for (int n = 1; n < argc; n++)
    {
        run[n] = atoi(argv[n]);
        if (run[n] == 0)
        {
            printf("Need a valid run number for argument.\n");
            printf("Usage: %s <daq run number> [<second run number>]\n", argv[0]);
            exit(1);
        }
    }

    // run this as a ROOT application
    TApplication app("PD LED GMS Scans", &argc, argv);

    // Plot options
    gStyle->SetOptStat(1);
    gStyle->SetOptFit(1);

    TChain h1("h1");
    for (int n = 1; n < argc; n++) {
        char filename[1024];
        sprintf(filename, "/data/ucnadata/2010/rootfiles/full%s.root", argv[n]);
        h1.Add(filename);
    }

/*
        // Open ntuple
        TFile* myfile = new TFile(filename);
        if (myfile->IsZombie())
        {
            printf("File %s not found.\n", filename);
            exit(1);
        }

        // extract tree
        TTree* h1 = (TTree*)myfile->Get("h1");
        if (h1 == NULL)
        {
            printf("TTree not found in file %s.\n", filename);
            exit(0);
        }
*/

    // Define cuts
    TCut *led_cut = new TCut("(int(Sis00) & 128) > 0");     // 129 if east-PMTs on, 161 if GMS-ref also on
    TCut *pedestal_cut = new TCut("!(int(Sis00)&1)");       // 129 if east-PMTs on, 161 if GMS-ref also on
    //TCut *lednumcut = new TCut("(Sis00 == 129) && (Number < 400000)");

    TH2F* his2D[8];
    TProfile* p[8];
    TCanvas* c[8];
    TGraph* g[8];
    TGraphErrors* resg[8];

    //TString H2Fname[8] = { "H2FE1", "H2FE2", "H2FE3", "H2FE4", "H2FW1", "H2FW2", "H2FW3", "H2FW4"};
    const char * H2Fname[8] = { "H2FE1", "H2FE2", "H2FE3", "H2FE4", "H2FW1", "H2FW2", "H2FW3", "H2FW4"};
    //TString Qadc[8] = { "Qadc0", "Qadc1", "Qadc2", "Qadc3", "Qadc4", "Qadc5", "Qadc6", "Qadc7"};
    const char * Qadc[8] = { "Qadc0", "Qadc1", "Qadc2", "Qadc3", "Qadc4", "Qadc5", "Qadc6", "Qadc7"};
    TString Pname[8] = { "PE1", "PE2", "PE3", "PE4", "PW1", "PW2", "PW3", "PW4"};
    TString Cname[8] = { "CE1", "CE2", "CE3", "CE4", "CW1", "CW2", "CW3", "CW4"};
    TString Dname[8] = { "DE1", "DE2", "DE3", "DE4", "DW1", "DW2", "DW3", "DW4"};
    TString title[8] = { "LED Scan E1", "LED Scan E2", "LED Scan E3", "LED Scan E4",
                         "LED Scan W1", "LED Scan W2", "LED Scan W3", "LED Scan W4" };

    gStyle->SetPalette(1);
    gStyle->SetOptStat("");

    //printf("how much wood can a wood chuck chuck: %i\n", h1->GetEntries());

    TF1 *pd_ped_fit = FitPedestal("Pdc36", &h1, pedestal_cut);
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
        TF1 *ped_fit = FitPedestal(Qadc[i], &h1, pedestal_cut);
        float pedestal = 0;
        if (ped_fit)
            pedestal = ped_fit->GetParameter(1);

        // Define histograms
        his2D[i] = new TH2F(H2Fname[i], title[i], 256, -pedestal, 4096-pedestal, 256, -pd_pedestal, 256-pd_pedestal);
        char draw_cmd[1024];
        //sprintf(draw_cmd, "%s-%f:S83028/1e6 >> %s", Qadc[i], pedestal, H2Fname[i]);
        //sprintf(draw_cmd, "(%s-%f):(Pdc36-%f) >> %s", Qadc[i], pedestal, pd_pedestal, H2Fname[i]);
        sprintf(draw_cmd, "(Pdc36-%f) : (%s-%f) >> %s", pd_pedestal, (char*)Qadc[i], pedestal, H2Fname[i]);

        h1.Draw(TString(draw_cmd), *led_cut);
        his2D[i]->SetTitle(title[i]);

        int nled = (int)his2D[i]->GetEntries();

        if(!nled) {
            printf("Empty histogram for %i; skipping.\n",i);
            continue;
        }
        printf("Found %i LED events.\n", nled);

        // build a profile...
        printf("Building TProfile...\n");	
        p[i] = his2D[i]->ProfileX(Pname[i]);
        p[i]->SetErrorOption("s");


        // find the number of photoelectrons and plot...
        printf("Building nPE plot...\n");	
        const int n = p[i]->GetNbinsX();
        g[i] = new TGraph(n);
        const float max_npe = 1000;
        for (unsigned j = 0; j < n; j++)
        {
            float x = p[i]->GetBinCenter(j);
            float w = p[i]->GetBinError(j);
            //float y = (w<1.0)?0:pow(p[i]->GetBinContent(j)/w,2);
            float y = (w<1.0)?0:pow(p[i]->GetBinContent(j)/w,2);
            y = (y>max_npe)? max_npe : y;
            g[i]->SetPoint(j, x, y);
        }
        

        // Find the approximate range of the fit
        float pmt_adc_min = 500;
        float pmt_adc_max = pmt_adc_min;
        float pd_adc_min = 20;
        float pd_adc_max = 140;

        //float range_max = 1000;
        //float slope = 0;

        // iterate through the bins and stop 
        // before exceeding pd_adc_max
        for (unsigned j = 0; j < n; j++ ) 
        {
            float _x = p[i]->GetBinContent(j);
            if (_x < pd_adc_max) 
                pmt_adc_max = p[i]->GetBinCenter(j);
            else
                break;
        }
/*
        // fit the range 
        printf("Finding range... ");
        float rough_delta = 4000;
        float prev_p_i = -1000;
        unsigned jumps = 20;
        for (unsigned j = 0; j < n; j += 10)
        {
            float _p_i_content = p[i]->GetBinContent(j);
            if ( _p_i_content > max_pd_channel/2 )
                jumps /= 4;
            if ( _p_i_content > max_pd_channel ) {
                range_max = j - jumps;
                printf("done.\n");
                break;
            }
        }
        printf("found range %f.\n", range_max); 
*/


        // fit a more accurate line
        printf("Fitting...");	
        //p[i]->Rebin(4);
        //TF1 *fit = new TF1("polyfit", "pol1", scan_start_time*60, (scan_time + scan_start_time)*60);
        //TF1 *fit = new TF1("polyfit", "pol1", scan_start_time, (scan_time + scan_start_time));
        //TF1 *fit = new TF1("polyfit", "[0]*x + [1]*x**2", 0, range_max);
        //TF1 *fit = new TF1("polyfit", "pol2", 0, range_max);
        //TF1 *fit = new TF1("polyfit", "[0] + [1]*(x/500)*exp((x/500)*[2])", 0, range_max);
        //TF1 *fit = new TF1("polyfit", "[0] + [1]*x/300 + [2]*x/300", 0, range_max);
        //TF1 *fit = new TF1("polyfit", "pol3", 0, range_max);
        TF1 *fit = new TF1("polyfit", "[0]*x + [1]*x*x", 0, pmt_adc_max);
        if (p[i]->Fit(fit, "R"))
            continue;
        printf("done.\n");	


        printf("Plotting LED intensity...");	
        p[i]->SetMarkerColor(2);
        p[i]->SetLineColor(2);
        p[i]->SetMarkerStyle(8);
        p[i]->SetMarkerSize(0.75);
        //his1.Fit("gaus");
        printf("done.\n");	


        // Draw what we fit
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
    TString _filename = "/data/kevinh/gms/images/pd_led_pmt_";
    _filename += argv[1];
    ew_canvas->SaveAs(_filename + ".gif");
    ew_canvas->SaveAs(_filename + ".pdf");

    app.Run();

    return 0;
}
