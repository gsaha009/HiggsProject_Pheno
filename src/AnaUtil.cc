#include <climits>
#include <cmath>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "TDirectory.h"
#include "TMath.h"
#include "TLorentzVector.h"

#include "AnaUtil.h"

using std::cout;
using std::cerr;
using std::endl;
using std::ios;
using std::setw;
using std::setprecision;

using std::string;
using std::vector;
using std::pair;
using std::map; 


namespace AnaUtil {
  void printP4(const TLorentzVector& lv, const string& tag, std::ostream& os) {
    os << setprecision(3);
    os << tag << " = (" 
       << setw(7) << lv.Pt()  << "," 
       << setw(7) << lv.Eta() << "," 
       << setw(7) << lv.Phi() << "," 
       << setw(7) << lv.Energy() << ")" 
       << endl;
  }


  double deltaPhi(double phia, double phib) {
    double dphi = phia - phib;
    while (dphi > TMath::Pi()) dphi -= 2 * TMath::Pi();
    while (dphi <= -TMath::Pi()) dphi += 2 * TMath::Pi();
    return dphi;
  }
  double deltaPhi(const TLorentzVector& a, const TLorentzVector& b) {
    return deltaPhi(a.Phi(), b.Phi());
  }
  double deltaR(const TLorentzVector& a, const TLorentzVector& b) {
    double dphi = deltaPhi(a,b);
    double deta = a.Eta() - b.Eta();
    return std::sqrt(dphi * dphi + deta * deta);
  }
  bool sameObject(const TLorentzVector& lv1, const TLorentzVector& lv2) {
    return (std::fabs(lv1.Pt() - lv2.Pt()) < 1.0e-08 && lv1.DeltaR(lv2) < 1.0e-06);
  }

  void buildList(const vector<string>& tokens, vector<string>& list) {
    for (vector<string>::const_iterator it  = tokens.begin()+1;
	 it != tokens.end(); ++it) {
      list.push_back(*it);
    }
  }

  double cutValue(const map<string, double>& m, const string& mkey) {
    if (m.find(mkey) == m.end()) {
      cerr << ">>> key: " << mkey << " not found in the map!" << endl;
      for (auto const& el: m)
        cerr << el.first << ": " << setw(7) << el.second << endl;
      return -999;
    }
    //assert(m.find(cname) != m.end());
    return m.find(mkey)->second;
  }

  const map<string, double>& cutMap(const map<string, map<string, double>>& hmap, const string& mkey) {
    assert(hmap.find(mkey) != hmap.end());
    return hmap.find(mkey)->second;
  }


  void tokenize(const string& str, vector<string>& tokens, const string& delimiters) {
    // Skip delimiters at beginning. 
    string::size_type lastPos = str.find_first_not_of(delimiters, 0);
    // Find first "non-delimiter".
    string::size_type pos = str.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)  {
      // Found a token, add it to the vector.
      tokens.push_back(str.substr(lastPos, pos - lastPos));
      // Skip delimiters.  Note the "not_of"  
      lastPos = str.find_first_not_of(delimiters, pos);
      // Find next "non-delimiter"
      pos = str.find_first_of(delimiters, lastPos);
    }
  }
  void buildMap(const vector<string>& tokens, map<string, int>& hmap) {
    string key = tokens.at(1) + "-" + tokens.at(2) + "-" + tokens.at(3);
    hmap.insert({key, 1});
  }

  void storeCuts(const vector<string>& tokens, map<string, map<string, double>>& hmap) {
    const string& key = tokens.at(0);
    map<string, double> m;
    auto pos = hmap.find(key);
    if (pos != hmap.end()) m = pos->second;
    for (auto it = tokens.begin()+1; it != tokens.end(); ++it) {
      // Split the line into words                                                                                                                                      
      vector<string> cutstr;
      tokenize(*it, cutstr, "=");
      if (cutstr.size() < 2) continue;
      m.insert({ cutstr[0], std::stod(cutstr[1]) });
    }
    if (!m.empty()) hmap[key] = m;
  }
  void showCuts(const map<string, map<string, double>>& hmap, ostream& os) {
    for (auto const& el: hmap) {
      os << ">>> " << el.first << endl;
      auto const& cutm = el.second;
      os << std::fixed << std::setprecision(2);
      for (auto const& il: cutm)
        os << setw(16) << il.first << ": "
           << setw(7)  << il.second << endl;
    }
  }
  // ------------------------------------------------------------------------
  // Convenience routine for filling 1D histograms. We rely on root to keep 
  // track of all the histograms that are booked all over so that we do not 
  // have to use any global variables to save the histogram pointers. Instead, 
  // we use the name of the histograms and gROOT to retrieve them from the 
  // Root object pool whenever necessary. This is the closest one can go to 
  // hbook and ID based histogramming
  // -------------------------------------------------------------------------
  TH1* getHist1D(const char* hname) {
    TObject *obj = gDirectory->GetList()->FindObject(hname); 
    if (obj == nullptr) {
      /*      cerr << "**** getHist1D: Histogram for <" << hname 
	   << "> not found! (" 
	   << __FILE__ << ":" << __LINE__ << ")" 
	   << endl;*/
      return nullptr;
    }
    TH1* h = nullptr;
    if (obj->InheritsFrom("TH1D"))
      h = dynamic_cast<TH1D*>(obj);
    else if (obj->InheritsFrom("TH1C"))
      h = dynamic_cast<TH1C*>(obj);
    else if (obj->InheritsFrom("TH1K"))
      h = dynamic_cast<TH1K*>(obj);
    else if (obj->InheritsFrom("TH1S"))
      h = dynamic_cast<TH1S*>(obj);
    else if (obj->InheritsFrom("TH1I"))
      h = dynamic_cast<TH1I*>(obj);
    else
      h = dynamic_cast<TH1F*>(obj);
    
    if (h == nullptr) {
      cerr << "**** getHist1D: <" << hname 
  	 << "> may not be a 1D Histogram! (" 
  	 << __FILE__ << ":" << __LINE__ << ")" 
  	 << endl;
    }
    return h;
  }
  TH1* getHist1D(const string& hname) {
    return getHist1D(hname.c_str());
  }
  
  // ---------------------------------------------
  // Convenience routine for filling 2D histograms
  // ---------------------------------------------
  TH2* getHist2D(const char* hname) {
    TObject *obj = gDirectory->GetList()->FindObject(hname); 
    if (obj == nullptr) {
      cerr << "**** getHist2D: Histogram for <" << hname 
  	 << "> not found! (" 
  	 << __FILE__ << ":" << __LINE__ << ")" 
  	 << endl;
      return nullptr;
    }
    
    TH2* h = nullptr;
    if (obj->InheritsFrom("TH2D"))
      h = dynamic_cast<TH2D*>(obj);
    else if (obj->InheritsFrom("TH2C"))
      h = dynamic_cast<TH2C*>(obj);
    else if (obj->InheritsFrom("TH2S"))
      h = dynamic_cast<TH2S*>(obj);
    else if (obj->InheritsFrom("TH2I"))
      h = dynamic_cast<TH2I*>(obj);
    else
      h = dynamic_cast<TH2F*>(obj);
    
    if (h == nullptr) {
      cerr << "**** getHist2D: <" << hname 
  	 << "> may not be a 2D Histogram! (" 
  	 << __FILE__ << ":" << __LINE__ << ")" 
  	 << endl;
    }
    return h;
  }
  TH2* getHist2D(const string& hname) {
    return getHist2D(hname.c_str());
  }
  // ---------------------------------------------
  // Convenience routine for filling 3D histograms
  // ---------------------------------------------
  TH3* getHist3D(const char* hname) {
    TObject *obj = gDirectory->GetList()->FindObject(hname); 
    if (obj == nullptr) {
      cerr << "**** getHist3D: Histogram for <" << hname 
  	 << "> not found! (" 
  	 << __FILE__ << ":" << __LINE__ << ")" 
  	 << endl;
      return nullptr;
    }
    
    TH3* h = nullptr;
    if (obj->InheritsFrom("TH3D"))
      h = dynamic_cast<TH3D*>(obj);
    else if (obj->InheritsFrom("TH3C"))
      h = dynamic_cast<TH3C*>(obj);
    else if (obj->InheritsFrom("TH3S"))
      h = dynamic_cast<TH3S*>(obj);
    else if (obj->InheritsFrom("TH3I"))
      h = dynamic_cast<TH3I*>(obj);
    else
      h = dynamic_cast<TH3F*>(obj);
    
    if (h == nullptr) {
      cerr << "**** getHist3D: <" << hname 
  	 << "> may not be a 3D Histogram! (" 
  	 << __FILE__ << ":" << __LINE__ << ")" 
  	 << endl;
    }
    return h;
  }
  TH3* getHist3D(const string& hname) {
    return getHist3D(hname.c_str());
  }
  
  // --------------------------------------------------
  // Convenience routine for filling profile histograms
  // --------------------------------------------------
  TProfile* getProfile(const char* hname) {
    TProfile *h = dynamic_cast<TProfile*>(gDirectory->GetList()->FindObject(hname));
    if (h == nullptr) {
      cerr << "**** getProfile: Profile Histogram for <" << hname 
  	 << "> not found! (" 
  	 << __FILE__ << ":" << __LINE__ << ")" 
  	 << endl;
    }
    return h;
  }
  TProfile* getProfile(const string& hname) {
    return getProfile(hname.c_str());
  }
  
  bool fillProfile(const char *hname, float xvalue, float yvalue, double w) {
    TProfile *h = getProfile(hname);
    if (h == nullptr) return false;
    
    h->Fill(xvalue, yvalue, w);
    return true;
  }
  bool fillProfile(const string& hname, float xvalue, float yvalue, double w) {
    return fillProfile(hname.c_str(), xvalue, yvalue, w);
  }
  void showEfficiency(const string& hname, 
		      const std::vector<std::string>& slist, 
		      const string& header, 
		      const string& tag, 
		      std::ostream& os) 
  {
    os << ">>> " << header << " Efficiency" << endl;
    TH1 *h = AnaUtil::getHist1D(hname);
    if (h != nullptr) {
      os << setw(64) << "CutFlow"
  	   << setw(13) << tag
  	   << setw(10) << "AbsEff"
	 << setw(10) << "AbsEffErr"
  	   << setw(10) << "RelEff"
	 << setw(10) << "RelEffErr"
  	   << endl;
      os.precision(3);
      int nbins = h->GetNbinsX();
      for (int i = 1; i <= nbins; ++i) {
        double cont  = static_cast<double>(h->GetBinContent(1));
        double conti = static_cast<double>(h->GetBinContent(i));
        double contj = static_cast<double>(h->GetBinContent(i-1));
  	os << setw(64) << slist[i-1]
	   << setprecision(0) 
	   << setw(13) << conti
	   << setprecision(5) 
	   << setw(10) << ((conti > 0) ? conti/cont : 0.0)
	   << setw(10) << (1/cont)*TMath::Sqrt(conti*(1-(conti/cont)))
	   << setw(10) << ( i == 1 ? 1.0 :(contj > 0) ? conti/contj : 0.0)
	   << setw(10) << ((contj > 0) ? (1/contj)*TMath::Sqrt(conti*(1-(conti/contj))) : 0.0)
	   << endl;
      }
    }
  }

  void scaleHistogram(const string& hname, double fac) {
    TH1 *h = AnaUtil::getHist1D(hname);
    if (h != nullptr) {
#if 0
      int nbins = h->GetNbinsX();
      for (int i = 1; i <= nbins; ++i) {
	//	cout<<"Bin no: "<<"\t"<<i<<endl;
	//	cout<<"before scl: "<< h->GetBinContent(i)<<endl;
        double cont = static_cast<double>(h->GetBinContent(i) * fac);
	//	cout<<i<<") Cut: "<<cont<<endl;
        double err  = static_cast<double>(h->GetBinError(i) * fac);
	cout<<i<<") Cut: "<<cont<<"\t"<<"Bin Error: "<<err<<endl;
	h->SetBinContent(i, cont);
        h->SetBinError(i, err);
      }
#endif     
      h->Scale(fac);
    }
  }
  void showCount(const string& hname, 
		 const std::vector<std::string>& slist, 
		 const std::string& tag, 
		 int prec, 
		 std::ostream& os) 
  {
    os << ">>> " << tag << endl;
    TH1 *h = AnaUtil::getHist1D(hname);
    if (h == nullptr) return;

    os.precision(prec);
    os << setw(16) << " Total"
       << setw(10) << h->Integral()
       << endl;
    int nbins = h->GetNbinsX();
    os.precision(prec);
    for (int i = 1; i <= nbins; ++i)
      os << setw(16) << slist[i-1]
	 << setw(10) << h->GetBinContent(i)
	 << endl;
  }

}
