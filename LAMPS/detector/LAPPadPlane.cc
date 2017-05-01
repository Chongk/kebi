#include "LAPPadPlane.hh"

#include "TGraph.h"
#include "TMath.h"
#include "TVector2.h"
#include "TH2Poly.h"
#include "TCollection.h"
#include "TEllipse.h"
#include "TLine.h"
#include "TCanvas.h"

#include <iostream>
using namespace std;

ClassImp(LAPPadPlane)

LAPPadPlane::LAPPadPlane()
:KBPadPlane("LAMPS-TPC Prototype Pad Plane", "")
{
}

bool LAPPadPlane::Init()
{
  TString padMapFileName;
  fPar -> GetParString("padMap", padMapFileName);
  ifstream padMapData(padMapFileName.Data());

  fPar -> GetParDouble("ppRMin", fRMin);
  fPar -> GetParDouble("ppRMax", fRMax);

  fTanPi1o8 = TMath::Tan(TMath::Pi()*1./8.);
  fTanPi3o8 = TMath::Tan(TMath::Pi()*3./8.);
  fTanPi5o8 = TMath::Tan(TMath::Pi()*5./8.);
  fTanPi7o8 = TMath::Tan(TMath::Pi()*7./8.);

  fSectionAngle[0] = TMath::Pi()/2;
  fSectionAngle[1] = TMath::Pi()*3/2;
  fSectionAngle[2] = 0;
  fSectionAngle[3] = TMath::Pi();

  fDW[0] = 4;
  fDW[1] = 4;
  fDW[2] = 3;
  fDW[3] = 3;

  fDR[0] = 15;
  fDR[1] = 15;
  fDR[2] = 10;
  fDR[3] = 10;

  Int_t nRowsArrayTemp1[] = {17, 19, 23, 25, 27, 31, 33};
  Int_t nRowsArrayTemp2[] = {21, 23, 25, 27, 29, 33, 35, 37, 39, 43, 45};
  for (auto nRows : nRowsArrayTemp1) {
    fNRowsInLayer[0].push_back(nRows);
    fNRowsInLayer[1].push_back(nRows);
  }
  for (auto nRows : nRowsArrayTemp2) {
    fNRowsInLayer[2].push_back(nRows);
    fNRowsInLayer[3].push_back(nRows);
  }

  fRBase[0] = 105.94;
  fRBase[1] = 105.94;
  fRBase[2] = 105.94;
  fRBase[3] = 105.94;

  Int_t asadID, agetID, channelID, padID;

  for (auto section = 0; section < 2; section++) {
    Int_t nLayers = fNRowsInLayer[section].size();
    for (auto layer = 0; layer < nLayers; layer++) {
      Int_t nRowsHalf = (fNRowsInLayer[section][layer]-1)/2;
      for (auto row = nRowsHalf; row >= -nRowsHalf; row--) {

        padMapData >> asadID >> agetID >> channelID >> padID;
        auto pad = new KBPad();

        Double_t i = row * fDW[section];
        Double_t j = fRBase[section] + layer * fDR[section];
        TVector2 pos(i,j);
        TVector2 posRot = pos.Rotate(fSectionAngle[section]);

        pad -> SetPlaneID(fPlaneID);
        pad -> SetPadID(padID);
        pad -> SetAsAdID(asadID);
        pad -> SetAGETID(agetID);
        pad -> SetChannelID(channelID);

        pad -> SetPosition(posRot.X(), posRot.Y());
        pad -> SetSectionRowLayer(section, row, layer);

        std::vector<Int_t> key;
        key.push_back(section);
        key.push_back(row);
        key.push_back(layer);

        fPadMap.insert(std::pair<std::vector<Int_t>, Int_t>(key,padID));
        fChannelArray -> Add(pad);
        fNPadsTotal++;
      }
    }
  }

  return true;
}

Int_t LAPPadPlane::FindPadID(Int_t section, Int_t row, Int_t layer)
{
  if (section < 0 || section > 4)
    return -1;

  Int_t nLayers = fNRowsInLayer[section].size();
  if (layer < 0 || layer >= nLayers)
    return -1;

  if (std::abs(row) > (fNRowsInLayer[section][layer]-1)/2)
    return -1;

  std::vector<Int_t> key;
  key.push_back(section);
  key.push_back(row);
  key.push_back(layer);

  Int_t id = fPadMap[key];

  return id;
}

Int_t LAPPadPlane::FindPadID(Double_t i, Double_t j)
{
  auto section = FindSection(i, j);

  //cout << "section: " << section << endl;
  if (section == -1)
    return -1;

  Double_t r = i;
  Double_t w = j;
  if (section > 1) {
    r = j;
    w = i;
  }
  r = abs(r);

  Int_t layer = Int_t((r-fRBase[section]+fDR[section]/2.)/fDR[section]);
  //cout << r << " - " << fRBase[section]+fDR[section]/2. << " / " << fDR[section] << endl;
  Int_t nLayers = fNRowsInLayer[section].size();
  //cout << "layer: " << layer << endl;
  if (layer < 0 || layer >= nLayers)
    return -1;

  Int_t nRowsHalf = (fNRowsInLayer[section][layer]-1)/2;
  Double_t x0 = (w-fDR[section]/2.);
  Int_t row;
  if (x0 > 0)
    row = Int_t(x0/fDW[section]);
  else
    row = Int_t((-x0)/fDW[section])-1;

  //cout << "row: " << row << endl;
  if (std::abs(row) > nRowsHalf)
    return -1;

  std::vector<Int_t> key;
  key.push_back(section);
  key.push_back(row);
  key.push_back(layer);

  Int_t id = fPadMap[key];

  return id;
}

bool LAPPadPlane::IsInBoundary(Double_t i, Double_t j)
{
  if (TMath::Sqrt(i*i+j*j) < 300.)
    return true;

  return false;
}

TH2* LAPPadPlane::GetHist(Option_t *option)
{
  if (fH2Plane != nullptr)
    return fH2Plane; 

  TH2Poly *h2 = new TH2Poly();

  Double_t iPoints[5] = {0};
  Double_t jPoints[5] = {0};
  Double_t iSigns[5] = {-1, -1, 1, 1, -1};
  Double_t jSigns[5] = {-1, 1, 1, -1, -1};
  Double_t offset = 0.25;

  KBPad *pad;
  TIter iterPads(fChannelArray);
  while ((pad = (KBPad *) iterPads.Next())) {
    Int_t section = pad -> GetSection();
    if (section > 1)
      continue;

    Double_t dr = fDR[section]/2. - offset;
    Double_t dw = fDW[section]/2. - offset;
    Double_t di, dj;

    if (section == 0 || section == 1) {
      di = dr;
      dj = dw;
    }
    else {
      di = dw;
      dj = dr;
    }

    Double_t i = pad -> GetI();
    Double_t j = pad -> GetJ();

    for (auto iPoint = 0; iPoint < 5; iPoint++) {
      iPoints[iPoint] = i + iSigns[iPoint]*di;
      jPoints[iPoint] = j + jSigns[iPoint]*dj;

      pad -> AddPadCorner(iPoints[iPoint], jPoints[iPoint]);
    }

    h2 -> AddBin(5, iPoints, jPoints);
  }

  fH2Plane = (TH2 *) h2;
  fH2Plane -> SetStats(0);
  fH2Plane -> SetTitle(";x (mm); z (mm)");
  fH2Plane -> GetXaxis() -> CenterTitle();
  fH2Plane -> GetYaxis() -> CenterTitle();

  return fH2Plane;
}

void LAPPadPlane::DrawFrame(Option_t *option)
{
  Color_t lineColor = kBlack;

  TEllipse *outerCircle = new TEllipse(0, 0, fRMax, fRMax);
            outerCircle -> SetFillStyle(0);
            outerCircle -> SetLineColor(lineColor);
            outerCircle -> Draw("samel");

  TEllipse *innerCircle = new TEllipse(0, 0, fRMin, fRMin);
            innerCircle -> SetFillStyle(0);
            innerCircle -> SetLineColor(lineColor);
            innerCircle -> Draw("samel");

  TLine* line1 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*1/8),  fRMax*TMath::Sin(TMath::Pi()*1/8),
                fRMin*TMath::Cos(TMath::Pi()*1/8),  fRMin*TMath::Sin(TMath::Pi()*1/8));
  TLine* line2 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*3/8),  fRMax*TMath::Sin(TMath::Pi()*3/8),
                fRMin*TMath::Cos(TMath::Pi()*3/8),  fRMin*TMath::Sin(TMath::Pi()*3/8));
  TLine* line3 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*5/8),  fRMax*TMath::Sin(TMath::Pi()*5/8),
                fRMin*TMath::Cos(TMath::Pi()*5/8),  fRMin*TMath::Sin(TMath::Pi()*5/8));
  TLine* line4 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*7/8),  fRMax*TMath::Sin(TMath::Pi()*7/8),
                fRMin*TMath::Cos(TMath::Pi()*7/8),  fRMin*TMath::Sin(TMath::Pi()*7/8));
  TLine* line5 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*9/8),  fRMax*TMath::Sin(TMath::Pi()*9/8),
                fRMin*TMath::Cos(TMath::Pi()*9/8),  fRMin*TMath::Sin(TMath::Pi()*9/8));
  TLine* line6 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*11/8), fRMax*TMath::Sin(TMath::Pi()*11/8),
                fRMin*TMath::Cos(TMath::Pi()*11/8), fRMin*TMath::Sin(TMath::Pi()*11/8));
  TLine* line7 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*13/8), fRMax*TMath::Sin(TMath::Pi()*13/8),
                fRMin*TMath::Cos(TMath::Pi()*13/8), fRMin*TMath::Sin(TMath::Pi()*13/8));
  TLine* line8 
    = new TLine(fRMax*TMath::Cos(TMath::Pi()*15/8), fRMax*TMath::Sin(TMath::Pi()*15/8),
                fRMin*TMath::Cos(TMath::Pi()*15/8), fRMin*TMath::Sin(TMath::Pi()*15/8));

  line1 -> SetLineColor(lineColor);
  line2 -> SetLineColor(lineColor);
  line3 -> SetLineColor(lineColor);
  line4 -> SetLineColor(lineColor);
  line5 -> SetLineColor(lineColor);
  line6 -> SetLineColor(lineColor);
  line7 -> SetLineColor(lineColor);
  line8 -> SetLineColor(lineColor);

  line1 -> Draw("samel");
  line2 -> Draw("samel");
  line3 -> Draw("samel");
  line4 -> Draw("samel");
  line5 -> Draw("samel");
  line6 -> Draw("samel");
  line7 -> Draw("samel");
  line8 -> Draw("samel");
}

Int_t LAPPadPlane::FindSection(Double_t i, Double_t j)
{
  if (j > fTanPi3o8*i) {
    if (j > fTanPi1o8*i) {
      if (j > fTanPi7o8*i) {
        if (j > fTanPi5o8*i) {
          return 2;
        } else return -1;
      } else return 0;
    } else return -1;
  }
  else
  {
    if (j < fTanPi1o8*i) {
      if (j < fTanPi7o8*i) {
        if (j < fTanPi5o8*i) {
          return 3;
        } else return -1;
      } else return 1;
    } else return -1;
  }
}

TCanvas *LAPPadPlane::GetCanvas(Option_t *option)
{
  if (fCanvas == nullptr)
    fCanvas = new TCanvas(fName+Form("%d",fPlaneID),fName,1300,800);

  return fCanvas;
}