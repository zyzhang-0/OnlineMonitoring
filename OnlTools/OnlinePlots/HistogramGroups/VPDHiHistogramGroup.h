#ifndef VPDHiHistogramGroup_h
#define VPDHiHistogramGroup_h


#include "Rtypes.h"
#include "HistogramGroup.h"

class VPDHiHistogramGroup : public HistogramGroup {

public:
  VPDHiHistogramGroup();
  VPDHiHistogramGroup(const char* group, const char* subGroup="vpdHi", const char* trigger="any", const char* detector="vpd");
  ~VPDHiHistogramGroup();

  virtual void reset();
  virtual bool fill(evpReader* evp, char* datap);
  virtual void draw(TCanvas* cc); 

private:

  TH2* h_vpd_cdb[4];                // ADC spectra of the 4 CDBs

  TH2* h_vpd_tac_east_vs_tac_west;
  TH2* h_vpd_vertex_vs_l3_vertex;

  TH2* h_vpd_earliestTAC_vs_eastchan;
  TH2* h_vpd_earliestTAC_vs_westchan;

  ClassDef(VPDHiHistogramGroup,1) ;
};


#endif
