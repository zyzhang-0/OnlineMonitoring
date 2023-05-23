#include "TOFtrayHistogramGroup.h"

#include <iostream>
#include <sstream>
#include <stdlib.h>

#include "TVirtualPad.h"
#include "TLine.h"
#include "TLatex.h"
#include "TStyle.h"
#include <TEnv.h>

#ifndef NEW_DAQ_READER
#  include <evpReader.hh>
#  include "daqFormats.h"
#  include "cfgutil.h"
#else
#  include "DAQ_READER/daqReader.h"
#  include "DAQ_TOF/tofReader.h"
#  include "DAQ_READER/cfgutil.h"
#  include "StEvent/StTriggerData.h"
#  include "TriggerData.h"
#endif
#include "TMapFile.h"
#include "EvpUtil.h"
#include "HistoHandler.h"

using namespace std;
char*  TOFtrayHistogramGroup::mTrayList = EvpUtil::cat(gEnv->GetValue("Online.tofConfigPath","."),"TOF_TrayNotInRun.txt");

ClassImp(TOFtrayHistogramGroup) ;

TOFtrayHistogramGroup::TOFtrayHistogramGroup() {
  // For ROOT I/O
  memset( TOF_Tray_LEhitmap, 0, sizeof(TOF_Tray_LEhitmap));
  memset( TOF_Tray_TEhitmap, 0, sizeof(TOF_Tray_TEhitmap));
}

TOFtrayHistogramGroup::TOFtrayHistogramGroup(unsigned int ipart,const char* group, const char* subGroup, const char* trigger, const char* detector)
  : HistogramGroup(group,subGroup,trigger,detector),mPart(ipart),mNtray(MAXTRAYS) {
 
  for(int i=0;i<MAXTRAYS;i++){
    actualTrayNum[i]=i+mPart*mNtray+1;
  }

  char tmpchr[200];
  char tmpchr1[200];
  for(int itray=0;itray<mNtray;itray++){
    sprintf(tmpchr,"TOF_Tray%03d_LE_hitmap",actualTrayNum[itray]);
    sprintf(tmpchr1,"Tray %03d Leading hitmap",actualTrayNum[itray]);
    TOF_Tray_LEhitmap[itray]=new TH1F(tmpchr,tmpchr1,192,-0.5,191.5);

    sprintf(tmpchr,"TOF_Tray%03d_TE_hitmap",actualTrayNum[itray]);
    sprintf(tmpchr1,"Tray %03d trailing hitmap",actualTrayNum[itray]);
    TOF_Tray_TEhitmap[itray]=new TH1F(tmpchr,tmpchr1,192,-0.5,191.5);
  }
  ReadTrayList(); 

}


TOFtrayHistogramGroup::~TOFtrayHistogramGroup() {
  for (int i = 0; i < mNtray; ++i)delete TOF_Tray_LEhitmap[i];
  for (int i = 0; i < mNtray; ++i)delete TOF_Tray_TEhitmap[i];
}


void TOFtrayHistogramGroup::reset() {
  for (int i = 0; i < mNtray; ++i)TOF_Tray_LEhitmap[i]->Reset();
  for (int i = 0; i < mNtray; ++i)TOF_Tray_TEhitmap[i]->Reset();
  ReadTrayList();

}


void TOFtrayHistogramGroup::draw(TCanvas* cc) {

  TLatex label;
  label.SetTextAlign(23);  // center, top
  label.SetTextSize(0.06);
  //label.SetTextColor(4);

  TLine  line;
  line.SetLineColor(4);
  line.SetLineWidth(2);
  //
  gStyle->SetPalette(1);
  gStyle->SetLabelSize(0.39,"y");
  gStyle->SetLabelSize(0.09,"x");
  gStyle->SetLabelSize(0.06,"xyz");
  gStyle->SetLabelSize(0.06,"y");
  gStyle->SetLabelSize(0.08,"x");
  gStyle->SetLabelOffset(0.01,"x");
  gStyle->SetLabelOffset(0.01,"y");

  gStyle->SetNdivisions(505,"xyz");

  gStyle->SetOptTitle(0);
  gStyle->SetTitleX(0.1); gStyle->SetTitleY(1.);
  gStyle->SetTitleW(0.8); gStyle->SetTitleH(0.086);
  //gStyle->SetTitleSize(0.06);
 
  gStyle->SetOptStat(0);
  gStyle->SetStatX(0.95); gStyle->SetStatY(0.92);
  gStyle->SetStatW(0.42); gStyle->SetStatH(0.20);

 //gStyle->SetStatFontSize(0.14);
  //gStyle->SetOptStat(1);
  char tmpchr[200];
  cc->cd(); cc->SetFillColor(0);
  cc->Clear();
  cc->Divide(5,6,0.00005,0.00005);
  //cc->Divide(5,6);


  for(int i=0;i<mNtray;i++){
    cc->cd(i+1);
    gPad->SetGridx(0);
    gPad->SetGridy(0);

    TOF_Tray_LEhitmap[i]->GetYaxis()->SetLabelSize(0.07);
    TOF_Tray_LEhitmap[i]->GetXaxis()->SetLabelSize(0.05);

    TOF_Tray_LEhitmap[i]->SetXTitle("Chan #");
    TOF_Tray_LEhitmap[i]->SetYTitle("Counts");

    TOF_Tray_LEhitmap[i]->SetLineColor(2);
    TOF_Tray_TEhitmap[i]->SetLineColor(4);
    TOF_Tray_LEhitmap[i]->Draw();
    TOF_Tray_TEhitmap[i]->Draw("same");

    label.SetTextSize(0.11);

    label.SetTextColor(45);
    float hmax=TOF_Tray_LEhitmap[i]->GetMaximum();
    if(hmax<1) hmax=1;
    sprintf(tmpchr,"%d",actualTrayNum[i]);
    label.DrawLatex(  20., 0.95*hmax, tmpchr);

    if(NotActiveTray[actualTrayNum[i]]) { 
      sprintf(tmpchr,"Not Active");
      label.SetTextColor(54);
      label.SetTextSize(0.1);
      label.DrawLatex(90., 0.6*hmax, tmpchr);
    } 

  }

  cc->Update();

} 


bool TOFtrayHistogramGroup::fill(evpReader* evp, char* datap) { 
  
  int ret=tofReader(datap);

  if(ret <= 0)   {
    fprintf(stderr,"TOF: problems in data (%d) - continuing...",ret);
    return false;
  }
  // 

  int halftrayid=-1;
  int trayid=-1;
  //leadinghits.clear();
  //trailinghits.clear();
  int lowtraynum= actualTrayNum[0];
  int hightraynum= actualTrayNum[MAXTRAYS-1];

  for(int ifib=0;ifib<4;ifib++){
    int ndataword = tof.ddl_words[ifib];    // 
    //cout<<"TOF:: ifib="<<ifib<<" ndataword="<< ndataword<<endl;
    if(ndataword<=0) continue;
    for(int iword=0;iword<ndataword;iword++){
      int dataword=tof.ddl[ifib][iword];
      //cout<<"TOF :: dataword=0x"<<hex<<dataword<<dec<<" "<<iword<<"/"<<dec<<ndataword<<" ifiber="<<ifib<<endl;
      if( (dataword&0xF0000000)>>28 == 0x2) continue;  
      if( (dataword&0xF0000000)>>28 == 0xD) continue;  
      if( (dataword&0xF0000000)>>28 == 0xE) continue;  
      if( (dataword&0xF0000000)>>28 == 0xA) {  // header trigger data flag
	// do nothing at this moment.
	continue;
      }
      // geographical data words for tray number.
      if( (dataword&0xF0000000)>>28 == 0xC) {
         halftrayid = dataword&0x01;    
         trayid     = (dataword&0x0FE)>>1;
         continue;
      }

      if(trayid < lowtraynum || trayid > hightraynum) continue;  

      if( (dataword&0xF0000000)>>28 == 0x6) {continue;}
      //
      int edgeid =int( (dataword & 0xf0000000)>>28 );
      if((edgeid !=4) && (edgeid!=5)) continue;


      int tdcid=(dataword & 0x0F000000)>>24;  // 0-15
      //cout<<"tdcid="<<tdcid<<" halftrayid="<<halftrayid<<endl;
      int tdigboardid=tdcid/4;   // 0-3 for half tray.
      int  tdcchan=(dataword&0x00E00000)>>21;          // tdcchan is 0-7 here.
      int globaltdcchan=tdcchan + (tdcid%4)*8+tdigboardid*24+96*halftrayid; // 0-191 for tray

      // fill hitmap.
      int histogramnum = (trayid-1)%MAXTRAYS;
      if(edgeid==4)TOF_Tray_LEhitmap[histogramnum]->Fill(globaltdcchan);
      if(edgeid==5)TOF_Tray_TEhitmap[histogramnum]->Fill(globaltdcchan);


    }  // end loop nword
  }  // end loop fiber
  
  return true;

}

void TOFtrayHistogramGroup::ReadTrayList(){
  
  //cout<<"TOFtrayHistogramGroup::TrayList config file:"<<mTrayList<<endl;

  TString buffer;

  ifstream filein(mTrayList);
  for(int i=0;i<128;i++){NotActiveTray[i]=false;}
  if(filein){ 
    while(!filein.eof()) {
      buffer.ReadLine(filein);
      if(buffer.BeginsWith("/")) continue;
      if(buffer.BeginsWith("#")) continue;
      int trayid = atoi(buffer.Data());
      if(trayid<1 || trayid>127) continue;
      NotActiveTray[trayid]=true;
    }   
  } else {cout<<"TOFtrayHistogramGroup::Can not open file:"<<mTrayList<<endl;}
  filein.close();

}
