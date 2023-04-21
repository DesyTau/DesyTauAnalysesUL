#include "DesyTauAnalyses/BBHTT/interface/SynchNTupleProducer_all_Defs_Alexeis.h"

#define pi   3.14159265358979312
#define d2r  1.74532925199432955e-02
#define r2d  57.2957795130823229

#define electronMass 	 0.000511
#define muonMass 	 0.105658
#define tauMass 	 1.77682
#define pionMass 	 0.1396

#define expectedtauspinnerweights 5
double MassFromTString(TString sample);
void initializeGenTree(SynchGenTree *gentree);
void FillVertices(const AC1B * analysisTree,SynchTree *otree, const bool isData);
void FillGenTree(const AC1B * analysisTree, SynchGenTree *gentree);
float getEmbeddedWeight(const AC1B * analysisTree, RooWorkspace* WS);
float getEmbeddedWeightKIT(const AC1B * analysisTree, RooWorkspace* WS, int era);
float getZPtEmbeddedWeight(const AC1B * analysisTree, TH2D * hist);
void FillElMu(const AC1B *analysisTree, SynchTree *otree, int electronIndex, float dRisoElectron, int muonIndex, float dRIsoMuon, int era, bool isEmbedded, bool isMcCorrectPuppi);

void CheckEMu(const AC1B * analysisTree, SynchTree * tree);

void getHiggsPtWeight(const AC1B * analysisTree, SynchTree * tree, RooWorkspace * ws, double mass);

bool accessTriggerInfo(const AC1B * analysisTree, TString HLTFilterName, unsigned int &nHLTFilter)
{
   bool isHLTFilter = false;
   
   for (unsigned int i=0; i<analysisTree->run_hltfilters->size(); ++i) {
      TString HLTFilter(analysisTree->run_hltfilters->at(i));
      if (HLTFilter==HLTFilterName) {
         nHLTFilter = i;
         isHLTFilter = true;
      }
   }
   return isHLTFilter;
}

void selectMuonElePair(AC1B *analysisTree, vector<int> muons, vector<int> electrons, bool isMuonIsoR03, bool isElectronIsoR03, float dRleptonsCut, float ptMuonHighCut, float ptElectronHighCut, int &electronIndex, int &muonIndex, float &isoMuMin, float &isoEleMin, int era, bool isEmbedded){
         
   for (unsigned int im=0; im<muons.size(); ++im) {
      unsigned int mIndex  = muons.at(im);
      float neutralHadIsoMu = analysisTree->muon_neutralHadIso[mIndex];
      float photonIsoMu = analysisTree->muon_photonIso[mIndex];
      float chargedHadIsoMu = analysisTree->muon_chargedHadIso[mIndex];
      float puIsoMu = analysisTree->muon_puIso[mIndex];
      if (isMuonIsoR03) {
         neutralHadIsoMu = analysisTree->muon_r03_sumNeutralHadronEt[mIndex];
         photonIsoMu = analysisTree->muon_r03_sumPhotonEt[mIndex];
         chargedHadIsoMu = analysisTree->muon_r03_sumChargedHadronPt[mIndex];
         puIsoMu = analysisTree->muon_r03_sumPUPt[mIndex];
      }
      float neutralIsoMu = neutralHadIsoMu + photonIsoMu - 0.5*puIsoMu;
      neutralIsoMu = TMath::Max(float(0),neutralIsoMu);
      float absIsoMu = chargedHadIsoMu + neutralIsoMu;
      float relIsoMu = absIsoMu/analysisTree->muon_pt[mIndex];
      
      for (unsigned int ie=0; ie<electrons.size(); ++ie) {
         unsigned int eIndex = electrons.at(ie);
         float dR = deltaR(analysisTree->electron_eta[eIndex],analysisTree->electron_phi[eIndex],
                           analysisTree->muon_eta[mIndex],analysisTree->muon_phi[mIndex]);
         
         if (dR<dRleptonsCut) continue;
         
         double ele_sf = 1.0;
         if (isEmbedded) ele_sf= EmbedElectronES_SF(analysisTree, era, electronIndex );

         bool trigMatch =
            (analysisTree->muon_pt[mIndex]>ptMuonHighCut) ||
            (analysisTree->electron_pt[eIndex] * ele_sf >ptElectronHighCut);
         if (!trigMatch) continue;
         
         float absIsoEle; 
         float relIsoEle;
         float rhoNeutral = analysisTree->rho;
         float  eA = getEffectiveArea( fabs(analysisTree->electron_superclusterEta[eIndex]) );
         absIsoEle = analysisTree->electron_r03_sumChargedHadronPt[eIndex] +
            TMath::Max(0.0f,analysisTree->electron_r03_sumNeutralHadronEt[eIndex]+analysisTree->electron_r03_sumPhotonEt[eIndex]-eA*rhoNeutral);
         relIsoEle = absIsoEle/(analysisTree->electron_pt[eIndex] *ele_sf);
            //}
         if (int(mIndex)!=muonIndex) {
            if (relIsoMu==isoMuMin) {
               if (analysisTree->muon_pt[mIndex]>analysisTree->muon_pt[muonIndex]) {
                  isoMuMin  = relIsoMu;
                  muonIndex = int(mIndex);
                  isoEleMin = relIsoEle;
                  electronIndex = int(eIndex);
               }
            }
            else if (relIsoMu<isoMuMin) {
               isoMuMin  = relIsoMu;
               muonIndex = int(mIndex);
               isoEleMin = relIsoEle;
               electronIndex = int(eIndex);
            }
         }
         else {
            if (relIsoEle==isoEleMin) {
               if (analysisTree->electron_pt[eIndex]>analysisTree->electron_pt[electronIndex]) {
                  isoEleMin = relIsoEle;
                  electronIndex = int(eIndex);
               }
            }
            else if (relIsoEle<isoEleMin) {
               isoEleMin = relIsoEle;
               electronIndex = int(eIndex);
            }
         }
      }
   }
}

bool triggerMatching(AC1B * analysisTree, Float_t eta, Float_t phi, bool isFilter, unsigned int nFilter, float deltaRTrigMatch = 0.5)
{
   bool trigMatch = false;
   if (!isFilter) return trigMatch;
   for (unsigned int iT=0; iT<analysisTree->trigobject_count; ++iT) {
      float dRtrig = deltaR(eta,phi,analysisTree->trigobject_eta[iT],analysisTree->trigobject_phi[iT]);
      if (dRtrig> deltaRTrigMatch) continue;
      if (analysisTree->trigobject_filters[iT][nFilter]) trigMatch = true;
      
   }
   
   return trigMatch;
}

void GetPuppiMET(AC1B * analysisTree, SynchTree * otree); 

void GetPFMET(AC1B * analysisTree, SynchTree * otree);

// Synch ntuple producer in the e+mu channel

int main(int argc, char * argv[]){
// first argument - config file for analysis
// second argument - file list (MUST BE IN THE SAME DIRECTORY OF THE EXECUTABLE)
// third argument - channel ("et" or "mt")
// third argument - index of first file to run on (optional, ignored if only one file is used)
// fourth argument - index of last file to run on (optional, ignored if only one file is used)

  using namespace std;
  gErrorIgnoreLevel = kFatal;
  string cmsswBase = (getenv("CMSSW_BASE"));
  
  // Load CrystalBallEfficiency class
  TString pathToCrystalLib = (TString) cmsswBase + "/src/HTT-utilities/CorrectionsWorkspace/CrystalBallEfficiency_cxx.so";
  int openSuccessful = gSystem->Load(pathToCrystalLib);
  if (openSuccessful != 0) {
    cout<<pathToCrystalLib<<" not found. Please create this file by running \"root -l -q CrystalBallEfficiency.cxx++\" in src/HTT-utilities/CorrectionsWorkspace/. "<<endl;
    exit(-1);
  }

  if(argc < 3){
    std::cout << "RUN ERROR: wrong number of arguments"<< std::endl;
    std::cout << "Please run the code in the following way:"<< std::endl;
    std::cout << "SynchNTupleProducer_em_Run2 NameOfTheConfigurationFile FileList" << std::endl;
    std::cout << "example: SynchNTupleProducer_Run2 analysisMacroSynch_em_2018.conf DATA_SingleMuon" << std::endl;
    exit(-1);
  }

  // **** configuration analysis  
  Config cfg(argv[1]);

  TString ch("em");

  // configuration process
  const string sample = argv[2];
  const bool isData = cfg.get<bool>("isData");
  const string infiles = argv[2];
  TString SampleName(argv[2]);
  bool isGGH = SampleName.Contains("SUSYGluGluToHToTauTau");
  double HiggsMass = 1000.;
  if (isGGH)
    HiggsMass = MassFromTString(SampleName);

  lumi_json json;
  if (isData){ 
    const string json_name = cfg.get<string>("JSON");
    read_json(TString(TString(cmsswBase) + "/src/" + TString(json_name)).Data(), json);
  }

  const string era_string = cfg.get<string>("era");
  TString Era(era_string);
  int era = 2016;
  if (Era.Contains("2017")) era = 2017;
  if (Era.Contains("2018")) era = 2018;

  const bool synch            = cfg.get<bool>("Synch");
  const bool ApplyPUweight    = cfg.get<bool>("ApplyPUweight"); 
  const bool ApplyLepSF       = cfg.get<bool>("ApplyLepSF"); 
  const bool ApplySVFit       = cfg.get<bool>("ApplySVFit");
  const bool ApplyFastMTT     = cfg.get<bool>("ApplyFastMTT");
  const bool ApplyBTagScaling = cfg.get<bool>("ApplyBTagScaling");
  const bool ApplyBTagReshape = cfg.get<bool>("ApplyBTagReshape");
  const bool ApplySystShift   = cfg.get<bool>("ApplySystShift");
  const bool ApplyMetFilters  = cfg.get<bool>("ApplyMetFilters");
  const bool usePuppiMET      = cfg.get<bool>("UsePuppiMET");

  //pileup distrib
  const string pileUpInDataFile = cfg.get<string>("pileUpInDataFile");
  const string pileUpInMCFile = cfg.get<string>("pileUpInMCFile");
  const string pileUpforMC = cfg.get<string>("pileUpforMC");

  //svfit
  const string svFitPtResFile = TString(TString(cmsswBase) + "/src/" + TString(cfg.get<string>("svFitPtResFile"))).Data();

  //zptweight file 
  const string ZptweightFile = cfg.get<string>("ZptweightFile");
  const string ZptNLOweightFile = cfg.get<string>("ZptNLOweightFile");
  const string ZptEmbweightFile = cfg.get<string>("ZptEmbweightFile");

  //b-tag scale factors
  const string BTagAlgorithm = cfg.get<string>("BTagAlgorithm");
  const string BtagSfFile = cmsswBase + "/src/" + cfg.get<string>("BtagSfFile");
  const string btagReshapeFileName = cmsswBase + "/src/" + cfg.get<string>("BTagReshapeFileName");
  if( ApplyBTagScaling && gSystem->AccessPathName( (TString) BtagSfFile) ){
    cout<<BtagSfFile<<" not found. Please check."<<endl;
    exit(-1);
  }
  cout<<"using "<<BTagAlgorithm<<endl;
  
  // JER
  const string m_resolution_filename = cfg.get<string>("JERResolutionFileName");
  const string m_resolution_sf_filename = cfg.get<string>("JERSFFileName");
  std::unique_ptr<JME::JetResolution> m_resolution_from_file;
  std::unique_ptr<JME::JetResolutionScaleFactor> m_scale_factor_from_file;
  m_resolution_from_file.reset(new JME::JetResolution(cmsswBase+"/src/"+m_resolution_filename));
  m_scale_factor_from_file.reset(new JME::JetResolutionScaleFactor(cmsswBase+"/src/"+m_resolution_sf_filename));

  JME::JetResolution resolution = *m_resolution_from_file;
  JME::JetResolutionScaleFactor resolution_sf = *m_scale_factor_from_file;

  // JES uncertainties
  const string jes_unc_filename = cfg.get<string>("JESUncertaintyFileName");
  string JESUncertaintyFileName = cmsswBase + "/src/" + jes_unc_filename;

  // BTag calibration
  BTagCalibration calib;
  BTagCalibrationReader reader_B;
  BTagCalibrationReader reader_C;
  BTagCalibrationReader reader_Light;
  if(ApplyBTagScaling){
    calib = BTagCalibration(BTagAlgorithm, BtagSfFile);
    reader_B = BTagCalibrationReader(BTagEntry::OP_MEDIUM, "central",{"up","down"});
    reader_C = BTagCalibrationReader(BTagEntry::OP_MEDIUM, "central",{"up","down"});
    reader_Light = BTagCalibrationReader(BTagEntry::OP_MEDIUM, "central",{"up","down"});
    reader_B.load(calib, BTagEntry::FLAV_B, "comb");
    reader_C.load(calib, BTagEntry::FLAV_C, "comb");
    reader_Light.load(calib, BTagEntry::FLAV_UDSG, "incl");
  }
    
  TString pathToTaggingEfficiencies = (TString) cmsswBase + "/src/" + cfg.get<string>("BtagMCeffFile");
  if (ApplyBTagScaling && gSystem->AccessPathName(pathToTaggingEfficiencies)){
    cout<<pathToTaggingEfficiencies<<" not found. Please check."<<endl;
    exit(-1);
  }
    
  TFile *fileTagging  = new TFile(pathToTaggingEfficiencies);
  TH2F  *tagEff_B     = 0;
  TH2F  *tagEff_C     = 0;
  TH2F  *tagEff_Light = 0;
  TRandom3 *rand = new TRandom3();

  if(ApplyBTagScaling){
    tagEff_B     = (TH2F*)fileTagging->Get("btag_eff_b");
    tagEff_C     = (TH2F*)fileTagging->Get("btag_eff_c");
    tagEff_Light = (TH2F*)fileTagging->Get("btag_eff_oth");
  }  
  BTagReshape * reshape = NULL;
  if (ApplyBTagReshape) reshape = new BTagReshape(btagReshapeFileName);
  //exit(-1);

  const struct btag_scaling_inputs inputs_btag_scaling_medium = {reader_B, reader_C, reader_Light, tagEff_B, tagEff_C, tagEff_Light, reshape, rand};

  // MET Recoil Corrections
  const bool isDY = (infiles.find("DYJets") != string::npos) || (infiles.find("DY1Jets") != string::npos) || (infiles.find("DY2Jets") != string::npos) || (infiles.find("DY3Jets") != string::npos) || (infiles.find("DY4Jets") != string::npos) || (infiles.find("EWKZ") != string::npos);//Corrections that should be applied on EWKZ are the same needed for DY
  const bool is_amcatnlo = (infiles.find("amcatnlo") != string::npos);
  const bool isDYamcatnlo = is_amcatnlo && isDY;
  const bool isWJets = (infiles.find("WJets") != string::npos) || (infiles.find("W1Jets") != string::npos) || (infiles.find("W2Jets") != string::npos) || (infiles.find("W3Jets") != string::npos) || (infiles.find("W4Jets") != string::npos) || (infiles.find("EWK") != string::npos);
  const bool isWGamma = infiles.find("WGToLNuG") != string::npos;
  const bool isWWTo1L = infiles.find("WWToLNuQQ") != string::npos;
  const bool isWZTo1L = infiles.find("WZTo1L") != string::npos;
  const bool isHToTauTau = infiles.find("HToTauTau") != string::npos;
  const bool isHToWW = infiles.find("HToWW") != string::npos;
  const bool isEWKZ =  infiles.find("EWKZ") != string::npos;
  const bool isMG = infiles.find("madgraph") != string::npos;
  const bool isMSSMsignal =  (infiles.find("SUSYGluGluToHToTauTau")!= string::npos) || (infiles.find("SUSYGluGluToBBHToTauTau")!= string::npos);
  const bool isTTbar = (infiles.find("TT_INCL") != string::npos) || (infiles.find("TTTo") != string::npos);
  const bool isYBYT  = infiles.find("_ybyt_M125") != string::npos;
  const bool isYB2   = infiles.find("_yb2_M125") != string::npos;
  const bool nonStandardQCDscale = isYBYT || isYB2;


  const bool isMcCorrectPuppi = 
    SampleName.Contains("TTTo2L2Nu") ||
    SampleName.Contains("ST_t-channel_antitop_4f") ||
    SampleName.Contains("ST_t-channel_top_4f") ||
    SampleName.Contains("ST_tW_antitop_5f") ||
    SampleName.Contains("VVTo2L2Nu") ||
    SampleName.Contains("WZTo2L2Q") ||
    SampleName.Contains("WZTo3LNu") ||
    SampleName.Contains("ZZTo2L2Q") ||
    SampleName.Contains("ZZTo4L");

  bool isHiggs = isHToTauTau || isHToWW; 

  const bool isEmbedded = infiles.find("Embed") != string::npos;

  const bool ApplyRecoilCorrections = cfg.get<bool>("ApplyRecoilCorrections") && !isEmbedded && !isData && (isDY || isWJets || isHiggs || isMSSMsignal || isWGamma || isWWTo1L || isWZTo1L);
  kit::RecoilCorrector PFMetRecoilCorrector(cfg.get<string>("PFMetRecoilFilePath"));
  kit::RecoilCorrector recoilCorrector(cfg.get<string>("RecoilFilePath"));
  kit::MEtSys MetSys(cfg.get<string>("RecoilSysFilePath"));


  // kinematic cuts on electrons
  const float ptElectronLowCut    = cfg.get<float>("ptElectronLowCut");
  const float ptElectronHighCut   = cfg.get<float>("ptElectronHighCut");
  const float etaElectronCut      = cfg.get<float>("etaElectronCut");
  const float dxyElectronCut      = cfg.get<float>("dxyElectronCut");
  const float dzElectronCut       = cfg.get<float>("dzElectronCut");
  const string lowPtLegElectron   = cfg.get<string>("LowPtLegElectron");
  const string highPtLegElectron  = cfg.get<string>("HighPtLegElectron");
  
  // veto electrons
  const float ptVetoElectronCut   = cfg.get<float>("ptVetoElectronCut");
  const float etaVetoElectronCut  = cfg.get<float>("etaVetoElectronCut");
  const float dxyVetoElectronCut  = cfg.get<float>("dxyVetoElectronCut");
  const float dzVetoElectronCut   = cfg.get<float>("dzVetoElectronCut");
  const float isoVetoElectronCut  = cfg.get<float>("isoVetoElectronCut");
  
  // kinematic cuts on muons
  const float ptMuonLowCut    = cfg.get<float>("ptMuonLowCut");
  const float ptMuonHighCut   = cfg.get<float>("ptMuonHighCut");
  const float etaMuonCut      = cfg.get<float>("etaMuonCut");
  const float dxyMuonCut      = cfg.get<float>("dxyMuonCut");
  const float dzMuonCut       = cfg.get<float>("dzMuonCut");
  const string lowPtLegMuon   = cfg.get<string>("LowPtLegMuon");
  const string highPtLegMuon  = cfg.get<string>("HighPtLegMuon");
   
  // veto muons
  const float ptVetoMuonCut   = cfg.get<float>("ptVetoMuonCut");
  const float etaVetoMuonCut  = cfg.get<float>("etaVetoMuonCut");
  const float dxyVetoMuonCut   = cfg.get<float>("dxyVetoMuonCut");
  const float dzVetoMuonCut   = cfg.get<float>("dzVetoMuonCut");
  const float isoVetoMuonCut   = cfg.get<float>("isoVetoMuonCut");
  
  // dR trigger, leptons
  const float dRTrigMatch = cfg.get<float>("dRTrigMatch");
  const float dRleptonsCut   = cfg.get<float>("dRleptonsCut");
  const bool isMuonIsoR03 = cfg.get<bool>("IsMuonIsoR03");
  const bool isElectronIsoR03 = cfg.get<bool>("IsElectronIsoR03");

  // dz filter
  const bool applyDzFilterMatch = cfg.get<bool>("ApplyDzFilterMatch");
  const string mu23ele12DzFilter = cfg.get<string>("Mu23Ele12DzFilter");
  const string mu8ele23DzFilter = cfg.get<string>("Mu8Ele23DzFilter");

  float dRIsoMuon = 0.4;
  if (isMuonIsoR03) dRIsoMuon = 0.3;
  float dRIsoElectron = 0.4;
  if (isElectronIsoR03) dRIsoElectron = 0.3;

  TString LowPtLegMuon(lowPtLegMuon);
  TString LowPtLegElectron(lowPtLegElectron);
  TString HighPtLegMuon(highPtLegMuon);
  TString HighPtLegElectron(highPtLegElectron);
  TString Mu23Ele12DzFilter(mu23ele12DzFilter);
  TString Mu8Ele23DzFilter(mu8ele23DzFilter);

  const float jetEtaCut = cfg.get<float>("JetEtaCut");
  const float jetPtLowCut = cfg.get<float>("JetPtLowCut");
  const float jetPtHighCut = cfg.get<float>("JetPtHighCut");
  const float dRJetLeptonCut = cfg.get<float>("dRJetLeptonCut");

  const float bJetEtaCut = cfg.get<float>("bJetEtaCut");
  const float btagCut = cfg.get<float>("btagCut");

  // correction workspace
  const string CorrectionWorkspaceFileName = cfg.get<string>("CorrectionWorkspaceFileName");
  const string CorrectionWorkspaceFileNameEmb = cfg.get<string>("CorrectionWorkspaceFileNameEmb");

  // Met correction in embedded sample
  double genMetScale = 1;
  if (era==2016)
    genMetScale = 0.992;
  if (era==2017)
    genMetScale = 0.955;
  if (era==2018)
    genMetScale = 0.957;

  bool triggerEmbed2017 = false;
  float ptTriggerEmbed2017 = 40;
  float etaTriggerEmbed2017 = 1.479;
  float dzFilterEff_data = 0.95;
  float dzFilterEff_mc = 0.95;
  if (era==2016) {
    dzFilterEff_data = 0.98;
    dzFilterEff_mc = 1.0;
  }

  if (era==2017&&isEmbedded)
    triggerEmbed2017 = true;

  // **** end of configuration analysis

  std::cout << "end of configuration " << std::endl;

  //file list creation
  int ifile = 0;
  int jfile = -1;

  if (argc > 3)
    ifile = atoi(argv[3]);
  if (argc > 4)
    jfile = atoi(argv[4]);
  
  // create input files list
  std::vector<std::string> fileList;  
  int NumberOfFiles = 0;
  if (infiles.find(".root") != std::string::npos){
    ifile = 0;
    jfile = 1;
    fileList.push_back(infiles);
  }
  else {
    ifstream input;
    std::string infile;  
    input.open(infiles);

    while(true){
      input>>infile;
      if(!input.eof()){
	if (infile.length() > 0){
	  fileList.push_back(infile);
	  NumberOfFiles += 1 ;
	}
      }
      else
	break;
    }
    
    if(jfile < 0)
      jfile = fileList.size();   
  }
  
  if(NumberOfFiles < jfile) jfile = NumberOfFiles;

  std::cout << "Number of files : " << jfile << std::endl;

  //  for (int iF = ifile; iF < jfile; ++iF) {
  //    std::cout<<fileList[iF]<<std::endl;
  //  }

  TString rootFileName(sample);
  std::string ntupleName("makeroottree/AC1B");
  std::string initNtupleName("initroottree/AC1B");

  // PU reweighting - initialization
  PileUp *PUofficial = new PileUp();
  if(ApplyPUweight){
    TFile *filePUdistribution_data = new TFile(TString(cmsswBase) + "/src/" + TString(pileUpInDataFile), "read");
    TFile *filePUdistribution_MC = new TFile (TString(cmsswBase) + "/src/" + TString(pileUpInMCFile), "read");
    TH1D *PU_data = (TH1D *)filePUdistribution_data->Get("pileup");    
    TH1D *PU_mc = (TH1D *)filePUdistribution_MC->Get(TString(pileUpforMC));
    if (PU_mc == NULL) {
      std::cout << "Histogram " << pileUpforMC << " is not present in pileup file" << std::endl;
      exit(-1);
    }
    PUofficial->set_h_data(PU_data);
    PUofficial->set_h_MC(PU_mc);
  }  

  std::cout << "PU loaded" << std::endl;

  // Workspace with corrections
  TString workspace_filename = TString(cmsswBase) + "/src/" + CorrectionWorkspaceFileName;
  TString workspace_filename_emb = TString(cmsswBase) + "/src/" + CorrectionWorkspaceFileNameEmb;
  cout << "Taking correction workspace for SFs from " << workspace_filename << endl;
  TFile *f_workspace = new TFile(workspace_filename, "read");
  if (f_workspace->IsZombie()) {
    std::cout << " workspace file " << workspace_filename << " not found. Please check. " << std::endl;
     exit(-1);
   }

  cout << "Taking correction workspace for embedded weights from " << workspace_filename_emb << endl;
  TFile *f_workspace_emb = new TFile(workspace_filename_emb, "read");
  if (f_workspace_emb->IsZombie()) {
    std::cout << " workspace file " << workspace_filename_emb << " not found. Please check. " << std::endl;
     exit(-1);
   }

  RooWorkspace *wSF  = (RooWorkspace*)f_workspace->Get("w");
  RooWorkspace *wEmb = (RooWorkspace*)f_workspace_emb->Get("w");

  //  TString ggHWeightsFile("higgs_pt_v0.root");
  /*
  TString ggHWeightsFile("higgs_pt_v2.root");
  if (era==2016)
    //    ggHWeightsFile = "higgs_pt_2016_v0.root";
    ggHWeightsFile = "higgs_pt_2016_v2.root";
  TFile * f_ggHWeights = new TFile(TString(cmsswBase) + "/src/DesyTauAnalyses/Common/data/" + ggHWeightsFile);
  if (f_ggHWeights->IsZombie()) {
    std::cout << "Cannot open file " << ggHWeightsFile << std::endl;
    exit(-1);
  }
  */
  //  RooWorkspace * higgsPt_ws = (RooWorkspace*)f_ggHWeights->Get("w");

  // Zpt reweighting for LO DY samples 
  TFile *f_zptweight = new TFile(TString(cmsswBase) + "/src/" + ZptweightFile, "read");
  if (f_zptweight==0 || f_zptweight->IsZombie()) {
    std::cout << "File " << TString(cmsswBase) << "/src/" << ZptweightFile << " not found" << std::endl;
    std::cout << "quitting.." << std::endl;
    exit(-1);
  }
  TH2D *h_zptweight = (TH2D*)f_zptweight->Get("zptmass_histo");
  if (h_zptweight==NULL) {
    std::cout << "histogram zptmass_histo is absent in file " 
	      << "File " << TString(cmsswBase) << "/src/" << ZptweightFile << std::endl;
    std::cout << "quitting..." << std::endl;
    exit(-1);
  }

  // Zpt reweighting for NLO DY samples
  TFile * f_zptNLOweight = new TFile(TString(cmsswBase) + "/src/" + ZptNLOweightFile, "read");
  if (f_zptNLOweight==0 || f_zptNLOweight->IsZombie()) {
    std::cout << "file " << TString(cmsswBase) << "/src/" << ZptNLOweightFile << " not found" << std::endl;
    std::cout << "quitting..." << std::endl;
    exit(-1);
  }
  TH2D * h_zptNLOweight = (TH2D*)f_zptNLOweight->Get("DY_NLO");
  TH2D * h_zptNLOweight_1btag = (TH2D*)f_zptNLOweight->Get("DY_Btag_NLO");
  TH2D * h_zptNLOweight_1bsys = (TH2D*)f_zptNLOweight->Get("DYJetscorr_NLO");
  //  TH2D * h_zptNLOweight_2btag = (TH2D*)f_zptNLOweight->Get("DYJetscorr_NLO_2btag");
  if (h_zptNLOweight == NULL ||
      h_zptNLOweight_1btag == NULL) {
    std::cout << "File " << TString(cmsswBase) << "/src/" << ZptNLOweightFile 
	      << " is empty. check content" << std::endl;
    exit(-1);
  }

  // Zpt reweighting for embedded
  TFile * f_zptweight_emb = new TFile(TString(cmsswBase) + "/src/" + ZptEmbweightFile, "read");

  TString era_shift("2016");
  if (era==2017) era_shift = "2017";
  if (era==2018) era_shift = "2018";
  TH2D * h_zptweight_emb = (TH2D*)f_zptweight_emb->Get("shifts_"+era_shift);
  if (h_zptweight_emb==NULL) {
    std::cout << "Problem opening file with embedded zpt weights" << std::endl;
    exit(-1);
  }

  std::cout << "ZPt weights" << std::endl;

  // load QCD =========================
  const string qcdFileName = cfg.get<string>("QCDFileName");
  TFile *fQCD = new TFile(TString(cmsswBase)+"/src/"+qcdFileName);

  TF1 *OS_SS_njetgt1 = (TF1*)fQCD->Get("OS_SS_transfer_factors_njetgt1");
  TF1 *OS_SS_njet1 = (TF1*)fQCD->Get("OS_SS_transfer_factors_njet1");
  TF1 *OS_SS_njet0 = (TF1*)fQCD->Get("OS_SS_transfer_factors_njet0");
  
  TGraph *OS_SS_njet0_Par0_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par0_njet0_UP");
  TGraph *OS_SS_njet0_Par0_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par0_njet0_DOWN");
  TGraph *OS_SS_njet0_Par1_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par1_njet0_UP");
  TGraph *OS_SS_njet0_Par1_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par1_njet0_DOWN");  
  TGraph *OS_SS_njet0_Par2_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par2_njet0_UP");
  TGraph *OS_SS_njet0_Par2_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par2_njet0_DOWN");  
  
  TGraph *OS_SS_njet1_Par0_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par0_njet1_UP");
  TGraph *OS_SS_njet1_Par0_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par0_njet1_DOWN");
  TGraph *OS_SS_njet1_Par1_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par1_njet1_UP");
  TGraph *OS_SS_njet1_Par1_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par1_njet1_DOWN");  
  TGraph *OS_SS_njet1_Par2_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par2_njet1_UP");
  TGraph *OS_SS_njet1_Par2_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par2_njet1_DOWN");  
  
  TGraph *OS_SS_njetgt1_Par0_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par0_njetgt1_UP");
  TGraph *OS_SS_njetgt1_Par0_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par0_njetgt1_DOWN");
  TGraph *OS_SS_njetgt1_Par1_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par1_njetgt1_UP");
  TGraph *OS_SS_njetgt1_Par1_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par1_njetgt1_DOWN");  
  TGraph *OS_SS_njetgt1_Par2_UP = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par2_njetgt1_UP");
  TGraph *OS_SS_njetgt1_Par2_DOWN = (TGraph*)fQCD->Get("OS_SS_transfer_factors_Par2_njetgt1_DOWN");  
   
  TH2F *hNonClosureCorrection = (TH2F*)fQCD->Get("NonClosureCorrection");
  TH2F *hIsolationCorrection = (TH2F*)fQCD->Get("IsolationCorrection");

  // scale factor class
  ScaleFactors * scaleFactors = new ScaleFactors(wSF,era,isEmbedded);

  // output fileName with histograms
  rootFileName += "_";
  rootFileName += ifile;
  rootFileName += "_em_Sync.root";
    
  std::cout <<rootFileName <<std::endl;  

  TFile *file = new TFile(rootFileName, "recreate");
  file->cd("");

  TH1D *inputEventsH = new TH1D("inputEventsH", "", 1, -0.5, 0.5);
  TH1D *nWeightedEventsH = new TH1D("nWeightedEvents", "", 1, -0.5, 0.5);
  
  TH1D *nWeightedEventsScaleCentralH = new TH1D("nWeightedEventsScaleCentral", "", 1, -0.5, 0.5);
  TH1D *nWeightedEventsScaleUpH      = new TH1D("nWeightedEventsScaleUp", "", 1, -0.5, 0.5);
  TH1D *nWeightedEventsScaleDownH    = new TH1D("nWeightedEventsScaleDown", "", 1, -0.5, 0.5);

  TH1D *SumScaleCentralH = new TH1D("SumScaleCentral", "", 1, -0.5, 0.5);
  TH1D *SumScaleUpH      = new TH1D("SumScaleUp", "", 1, -0.5, 0.5);
  TH1D *SumScaleDownH    = new TH1D("SumScaleDown", "", 1, -0.5, 0.5);

  TTree *tree = new TTree("TauCheck", "TauCheck");
  //  TTree *gtree = new TTree("GenTauCheck", "GenTauCheck");
  SynchTree *otree = new SynchTree(tree,ch,isGGH);
  //  SynchGenTree *gentree = new SynchGenTree(gtree);

  int nTotalFiles = 0;
  int nEvents = 0;
  int selEvents = 0;
  int nFiles = 0;
  
  //svFit
  TH1::AddDirectory(false);  
  TFile *inputFile_visPtResolution = new TFile(svFitPtResFile.data());

  std::cout << "inputFile_visPtResolution : " << std::endl;

  //Systematics init
  
  MuonScaleSys *muonScaleSys = 0;
  ElectronScaleSys *electronScaleSys = 0;

  ZPtWeightSys* zPtWeightSys = 0;
  TopPtWeightSys* topPtWeightSys = 0;
  BtagSys * btagSys = 0;
  BtagSys * mistagSys = 0;
  std::vector<JetEnergyScaleSys*> jetEnergyScaleSys;
  JESUncertainties * jecUncertainties = 0;

  std::vector<TString> metSysNames = {"CMS_scale_met_unclustered_13TeV"};
  std::vector<TString> recoilSysNames = {"CMS_htt_boson_reso_met_13TeV",
					 "CMS_htt_boson_scale_met_13TeV"};
  TString embeddedMetSystematics("CMS_scale_met_embedded_13TeV");

  std::vector<PFMETSys*> metSys;
  std::vector<PuppiMETSys*> puppiMetSys;

  if((!isData||isEmbedded) && ApplySystShift){

    //    muonScaleSys = new MuonScaleSys(otree);
    //    muonScaleSys->SetUseSVFit(false);
    //    muonScaleSys->SetUseFastMTT(false);
    //    muonScaleSys->SetSvFitVisPtResolution(inputFile_visPtResolution);
    //    muonScaleSys->SetUsePuppiMET(usePuppiMET);
    
    electronScaleSys = new ElectronScaleSys(otree);
    electronScaleSys->SetUseSVFit(ApplySVFit);
    electronScaleSys->SetUseFastMTT(ApplyFastMTT);
    electronScaleSys->SetSvFitVisPtResolution(inputFile_visPtResolution);
    electronScaleSys->SetUsePuppiMET(usePuppiMET);
    electronScaleSys->SetScaleFactors(scaleFactors);

    /*
    if (isEmbedded) {
      if (usePuppiMET) {
	PuppiMETSys * puppiEmbedMetSys = new PuppiMETSys(otree,embeddedMetSystematics);
	puppiMetSys.push_back(puppiEmbedMetSys);
      }
    }
    */

    // systematics only for MC
    if (!isEmbedded) {
      //      btagSys = new BtagSys(otree,TString("Btag"));
      //      btagSys->SetConfig(&cfg);
      //      btagSys->SetBtagScaling(&inputs_btag_scaling_medium);
      //      mistagSys = new BtagSys(otree,TString("Mistag"));
      //      mistagSys->SetConfig(&cfg);
      //      mistagSys->SetBtagScaling(&inputs_btag_scaling_medium);
      if (ApplyRecoilCorrections) {
	if (usePuppiMET) {
	  for (unsigned int i = 0; i < recoilSysNames.size(); ++i) {
	    PuppiMETSys * puppiMetRecoilSys = new PuppiMETSys(otree,recoilSysNames[i]);
	    puppiMetRecoilSys->SetMEtSys(&MetSys);
	    puppiMetSys.push_back(puppiMetRecoilSys);
	  }
	}
      }
      else {
	for (unsigned int i = 0; i<metSysNames.size(); ++i) {
	  if (usePuppiMET)
	    puppiMetSys.push_back(new PuppiMETSys(otree,metSysNames[i]));
	  else
	    metSys.push_back(new PFMETSys(otree,metSysNames[i]));
	}
      }
      if (cfg.get<bool>("splitJES")){
	JESUncertainties *jecUncertainties;
	jecUncertainties = new JESUncertainties(JESUncertaintyFileName);
	std::vector<std::string> JESnames = jecUncertainties->getUncertNames();
	for (unsigned int i = 0; i < JESnames.size(); i++) std::cout << "i: "<< i << ", JESnames.at(i) : " << JESnames.at(i) << std::endl;
	for (unsigned int i = 0; i < JESnames.size(); i++){
	  JetEnergyScaleSys *aJESobject = new JetEnergyScaleSys(otree, TString(JESnames.at(i)));
	  aJESobject->SetConfig(&cfg);
	  aJESobject->SetBtagScaling(&inputs_btag_scaling_medium);
	  aJESobject->SetJESUncertainties(jecUncertainties);
	  jetEnergyScaleSys.push_back(aJESobject);
	}	  
      }
      else { // use JEC uncertainty from analysis tree
	JetEnergyScaleSys *singleJES = new JetEnergyScaleSys(otree, TString("JES"));
	singleJES->SetConfig(&cfg);
	singleJES->SetBtagScaling(&inputs_btag_scaling_medium);
	singleJES->SetJESUncertainties(jecUncertainties);
	jetEnergyScaleSys.push_back(singleJES);
      }
      JetEnergyScaleSys * JERsys = new JetEnergyScaleSys(otree, TString("JER"));
      JERsys->SetConfig(&cfg);
      JERsys->SetBtagScaling(&inputs_btag_scaling_medium);
      JERsys->SetJESUncertainties(jecUncertainties);
      jetEnergyScaleSys.push_back(JERsys);
    }
    //    else {
    //      puppiMetSys.push_back(new PuppiMETSys(otree,embeddedMetSystematics));
    //    }
  }

  // list of met filters from config
  std::vector<TString> met_filters_list;
  for (unsigned int i = 1; i < (unsigned int) cfg.get<int>("num_met_filters") + 1; i++) {
    met_filters_list.push_back(cfg.get<string>("met_filter_" + std::to_string(i)));
  }

  ///////////////FILE LOOP///////////////

  for (int iF = ifile; iF < jfile; ++iF) {
    std::cout << "file " << iF + 1 << " out of " << fileList.size() << " filename : " << fileList[iF] << std::endl;
    
    TFile *file_ = TFile::Open(fileList[iF].data());

    if (file_==0)
      continue;

    if (file_->GetListOfKeys()->GetSize() == 0)
      continue; 

    if (file_->GetEND() > file_->GetSize())
      continue; 

    if (file_->GetSeekKeys()<=file_->GetEND()-file_->GetSize())
      continue;

    if (file_->IsZombie()) {
      cout << "cannot open file " << fileList[iF].data() << std::endl;
      continue;
    }
    TTree *_tree = (TTree*)file_->Get(TString(ntupleName));  
    if (_tree == NULL) {
      std::cout << "TTree " << ntupleName << " is absent" << std::endl;
      continue;
    }
    
    TH1D *histoInputEvents = NULL;
    histoInputEvents = (TH1D*)file_->Get("makeroottree/nEvents");
    if (histoInputEvents == NULL) continue;
    int NE = int(histoInputEvents->GetEntries());
    std::cout << "      number of input events    = " << NE << std::endl;
    for (int iE = 0; iE < NE; ++iE)
      inputEventsH->Fill(0.);

    TTree * _inittree = (TTree*)file_->Get(TString(initNtupleName));
    if (_inittree!=NULL) {
      Float_t genweight;
      if (!isData)
	_inittree->SetBranchAddress("genweight",&genweight);
      Long64_t numberOfEntriesInitTree = _inittree->GetEntries();
      std::cout << "      number of entries in Init Tree = " << numberOfEntriesInitTree << std::endl;
      for (Long64_t iEntry=0; iEntry<numberOfEntriesInitTree; iEntry++) {
	_inittree->GetEntry(iEntry);
	if (isData && !isEmbedded)
	  nWeightedEventsH->Fill(0.,1.);
	else {
	  double gen_weight = 1.0;
	  if (genweight<0) gen_weight = -1.0;
	  nWeightedEventsH->Fill(0.,gen_weight);
	}
      }
    }

    //    AC1B analysisTree(_tree, isData);
    AC1B analysisTree(_tree);
    // set AC1B for JES Btag and MET systematics
    if ( !isData && !isEmbedded && ApplySystShift) {
      //	btagSys->SetAC1B(&analysisTree);
      //	mistagSys->SetAC1B(&analysisTree);
      for (unsigned int i = 0; i < jetEnergyScaleSys.size(); i++)
      	(jetEnergyScaleSys.at(i))->SetAC1B(&analysisTree);
      for (unsigned int i = 0; i < metSys.size(); i++)
	(metSys.at(i))->SetAC1B(&analysisTree);
      for (unsigned int i = 0; i < puppiMetSys.size(); ++i)
	(puppiMetSys.at(i))->SetAC1B(&analysisTree);
    }
    
    ///////////////EVENT LOOP///////////////
    Long64_t numberOfEntries = analysisTree.GetEntries();
    std::cout << "      number of entries in Tree = " << numberOfEntries << std::endl;    

    for (Long64_t iEntry = 0; iEntry < numberOfEntries; iEntry++) {

      if (nEvents % 10000 == 0) 
      	cout << "      processed " << nEvents << " events" << endl; 

      analysisTree.GetEntry(iEntry);
      nEvents++;

      float qcdScaleUp = analysisTree.weightScale4;
      if (nonStandardQCDscale)
	qcdScaleUp = analysisTree.weightScale2;

      float qcdScaleDown = 1.0/qcdScaleUp;
      if (qcdScaleDown>5.) qcdScaleDown = 5.0;
      if (qcdScaleDown<0.01) qcdScaleDown = 0.01;

      double wghtUp      = qcdScaleUp * analysisTree.genweight;
      double wghtCentral = analysisTree.genweight;
      double wghtDown    = qcdScaleDown * analysisTree.genweight;

      nWeightedEventsScaleCentralH->Fill(0.,wghtCentral);
      nWeightedEventsScaleUpH->Fill(0.,wghtUp);
      nWeightedEventsScaleDownH->Fill(0.,wghtDown);

      SumScaleCentralH->Fill(0.,1.);
      SumScaleUpH->Fill(0.,qcdScaleUp);
      SumScaleDownH->Fill(0.,qcdScaleDown);

      // counting b-jets
      int nbjets = 0;
      if (!isData) {
	unsigned int njets = analysisTree.pfjet_count;
	for (unsigned int jet = 0 ; jet<njets ; ++jet) {
	  if (analysisTree.pfjet_flavour[jet]==5||
	      analysisTree.pfjet_flavour[jet]==-5)
	    nbjets++;
	}
      }
      otree->gen_nbjets_cut = nbjets;
      otree->gen_nbjets = analysisTree.genparticles_nbjets;
      //      if (otree->gen_nbjets_cut>0||otree->gen_nbjets>0) {
      //	std::cout << "gen_nbjets_cut = " << otree->gen_nbjets_cut << std::endl;
      //	std::cout << "gen_nbjets     = " << otree->gen_nbjets << std::endl;
      //	std::cout << std::endl;
      //      }
      // filling generator tree
      //      if (!isData){
      //	initializeGenTree(gentree);
      //      	FillGenTree(&analysisTree,gentree);
      //      	gentree->Fill();
      //      }

      //Skip events not passing the MET filters, if applied
      bool passed_all_met_filters = passedAllMetFilters(&analysisTree, met_filters_list);
      if (ApplyMetFilters && !synch && !passed_all_met_filters) continue;
      otree->passedAllMetFilters = passed_all_met_filters;
      
      // accessing trigger info ====

      bool isMu23Ele12DzFilter = false;
      bool isMu8Ele23DzFilter = false;
      unsigned int nMu23Ele12DzFilter = 0;
      unsigned int nMu8Ele23DzFilter = 0;

      unsigned int nLowPtLegElectron = 0;
      unsigned int nHighPtLegElectron = 0;
      unsigned int nLowPtLegMuon = 0;
      unsigned int nHighPtLegMuon = 0;
      bool isLowPtLegElectron = accessTriggerInfo(&analysisTree,LowPtLegElectron,nLowPtLegElectron);
      bool isHighPtLegElectron = accessTriggerInfo(&analysisTree,HighPtLegElectron,nHighPtLegElectron);
      bool isLowPtLegMuon = accessTriggerInfo(&analysisTree,LowPtLegMuon,nLowPtLegMuon);
      bool isHighPtLegMuon = accessTriggerInfo(&analysisTree,HighPtLegMuon,nHighPtLegMuon);
      if (applyDzFilterMatch){
	isMu23Ele12DzFilter = accessTriggerInfo(&analysisTree,Mu23Ele12DzFilter,nMu23Ele12DzFilter);
	isMu8Ele23DzFilter = accessTriggerInfo(&analysisTree,Mu8Ele23DzFilter,nMu8Ele23DzFilter);
      }
    
      otree->run  = analysisTree.event_run;
      otree->lumi = analysisTree.event_luminosityblock;
      otree->evt  = analysisTree.event_nr;
    
      // lumi filter
      if ((isData || isEmbedded) && !isGoodLumi(otree->run, otree->lumi, json))
      	continue;
        
      otree->npv = analysisTree.primvertex_count;
      otree->npu = analysisTree.numtruepileupinteractions;// numpileupinteractions;
      otree->rho = analysisTree.rho;

      // selecting electrons      
      float sf_eleES = 1.0;   
      vector<int> electrons; electrons.clear();
      for (unsigned int ie = 0; ie<analysisTree.electron_count; ++ie) {
	bool electronMvaId = analysisTree.electron_mva_wp90_noIso_Fall17_v2[ie];        
	if (isEmbedded) sf_eleES = EmbedElectronES_SF(&analysisTree, era, ie);          
	if (sf_eleES*analysisTree.electron_pt[ie] <= ptElectronLowCut) continue;
	if (fabs(analysisTree.electron_eta[ie]) >= etaElectronCut) continue;
	if (fabs(analysisTree.electron_dxy[ie]) >= dxyElectronCut) continue;
	if (fabs(analysisTree.electron_dz[ie]) >= dzElectronCut) continue;
	if (!electronMvaId) continue;
	if (!analysisTree.electron_pass_conversion[ie]) continue;
	if (analysisTree.electron_nmissinginnerhits[ie] > 1) continue;
	electrons.push_back(ie);
      }
    
      // selecting muons
      vector<int> muons; muons.clear();
      for (unsigned int im = 0; im < analysisTree.muon_count; ++im) {
	bool muonMediumId = isIdentifiedMediumMuon(im, &analysisTree, isData);	          
	if (analysisTree.muon_pt[im] <= ptMuonLowCut) continue;
	if (fabs(analysisTree.muon_eta[im]) >= etaMuonCut) continue;
	if (fabs(analysisTree.muon_dxy[im]) >= dxyMuonCut) continue;
	if (fabs(analysisTree.muon_dz[im]) >= dzMuonCut) continue;
	if (!muonMediumId) continue;
	muons.push_back(im);
      }
    
      //      std::cout << "muons = " << muons.size() << std::endl;
      //      std::cout << "electrons = " << electrons.size() << std::endl;

      if (muons.size() == 0) continue;
      if (electrons.size() == 0) continue;
    
      int electronIndex = -1;
      int muonIndex = -1;
      float isoMuMin = 1e+10;
      float isoEleMin = 1e+10;
      bool isMuonIsoR03 = false;
      bool isElectronR03 = true;
      selectMuonElePair(&analysisTree, muons, electrons, isMuonIsoR03, isElectronIsoR03, dRleptonsCut, ptMuonHighCut, ptElectronHighCut, electronIndex, muonIndex, isoMuMin, isoEleMin, era, isEmbedded);
      if (electronIndex<0) continue;
      if (muonIndex<0) continue;
      
      // Filling ntuple with the electron and muon information
      FillElMu(&analysisTree,otree,electronIndex,dRIsoElectron,muonIndex,dRIsoMuon,era,isEmbedded,isMcCorrectPuppi);

      //      std::cout << "fill emu " << std::endl;
      //      std::cout << "e : pt = " << otree->pt_1 << "  eta = " << otree->eta_1 << "  phi = " << otree->phi_1 << std::endl;
      //      std::cout << "m : pt = " << otree->pt_2 << "  eta = " << otree->eta_2 << "  phi = " << otree->phi_2 << std::endl;

      //all criterua passed, we fill vertices here;	
      //      FillVertices(&analysisTree, otree, isData);
      
      //      std::cout << "fill vertices" << std::endl;

      /////////////////////////////
      // Trigger matching
      /////////////////////////////
      bool isHighPtMuonMatch = triggerMatching(&analysisTree,otree->eta_2,otree->phi_2,isHighPtLegMuon,nHighPtLegMuon,dRTrigMatch);
      bool isLowPtMuonMatch  = triggerMatching(&analysisTree,otree->eta_2,otree->phi_2,isLowPtLegMuon,nLowPtLegMuon,dRTrigMatch);

      bool isHighPtElectronMatch = triggerMatching(&analysisTree,otree->eta_1,otree->phi_1,isHighPtLegElectron,nHighPtLegElectron,dRTrigMatch);;
      bool isLowPtElectronMatch  = triggerMatching(&analysisTree,otree->eta_1,otree->phi_1,isLowPtLegElectron,nLowPtLegElectron,dRTrigMatch);

      bool isHighPtMuonDZMatch = true;
      bool isLowPtMuonDZMatch = true;
      bool isHighPtElectronDZMatch = true;
      bool isLowPtElectronDZMatch = true;

      if (applyDzFilterMatch) {
	isLowPtMuonDZMatch  = triggerMatching(&analysisTree,otree->eta_2,otree->phi_2,isMu8Ele23DzFilter,nMu8Ele23DzFilter,dRTrigMatch);
	isHighPtMuonDZMatch = triggerMatching(&analysisTree,otree->eta_2,otree->phi_2,isMu23Ele12DzFilter,nMu23Ele12DzFilter,dRTrigMatch); 
	isLowPtElectronDZMatch = triggerMatching(&analysisTree,otree->eta_1,otree->phi_1,isMu23Ele12DzFilter,nMu23Ele12DzFilter,dRTrigMatch);
	isHighPtElectronDZMatch = triggerMatching(&analysisTree,otree->eta_1,otree->phi_1,isMu8Ele23DzFilter,nMu8Ele23DzFilter,dRTrigMatch);
      }

      otree->trg_singlemuon = false;
      otree->trg_singleelectron = false;
      otree->singleLepTrigger = false;

      // irrelevant for emu channel
      otree->trg_doubletau = false;
      otree->trg_mutaucross = false;
      otree->trg_mutaucross_mu = false;
      otree->trg_mutaucross_tau = false;
      otree->trg_etaucross = false;
      otree->trg_etaucross_e = false;
      otree->trg_etaucross_tau = false;

      // emu triggers (add them to tree)
      otree->trg_mulow = isLowPtMuonMatch;
      otree->trg_muhigh = isHighPtMuonMatch;
      otree->trg_elow = isLowPtElectronMatch;
      otree->trg_ehigh = isHighPtElectronMatch;
      otree->trg_muhigh_elow = isHighPtMuonMatch && isLowPtElectronMatch && isHighPtMuonDZMatch && isLowPtElectronDZMatch;
      otree->trg_ehigh_mulow = isHighPtElectronMatch && isLowPtMuonMatch && isLowPtMuonDZMatch && isHighPtElectronDZMatch;

      bool trigger_fired = otree->trg_ehigh_mulow || otree->trg_muhigh_elow;

      //extra lepton veto
      TString chE("et");
      TString chMu("mt");
      otree->extraelec_veto = extra_electron_veto(electronIndex, chE, &cfg, &analysisTree, era, isEmbedded);
      otree->extramuon_veto = extra_muon_veto(muonIndex, chMu, &cfg, &analysisTree, isData);

      //      CheckEMu(&analysisTree,otree);
      //      std::cout << "after trigger" << std::endl;

      jets::CreateUncorrectedJets(&analysisTree);

      // initialize JER (including data and embedded) 
      otree->apply_recoil = ApplyRecoilCorrections;
      jets::initializeJER(&analysisTree);

      //      std::cout << "initialising JER" << std::endl;
      //      std::cout << "event number = " << analysisTree.event_nr << std::endl;
      if (!isData && !isEmbedded) { // JER smearing
	jets::associateRecoAndGenJets(&analysisTree, resolution);
	jets::smear_jets(&analysisTree,resolution,resolution_sf,true);
      }

      //counting jet
      jets::counting_jets(&analysisTree, otree, &cfg, &inputs_btag_scaling_medium);

      //      std::cout << "counted jets" << std::endl;
  
      ////////////////////////////////////////////////////////////
      ////////////////////////  M E T ////////////////////////////
      ////////////////////////////////////////////////////////////
      TLorentzVector genV( 0., 0., 0., 0.);
      TLorentzVector genL( 0., 0., 0., 0.);

      // !!!!!!!!!!! include electron and jet !!!!!!!!!!!!!
      // !!!!!!!!!!! smearing corrections !!!!!!!!!!!!!!!!!
      GetPuppiMET(&analysisTree, otree);
      GetPFMET(&analysisTree, otree);

      otree->met_uncorr = otree->puppimet;
      otree->metphi_uncorr = otree->puppimetphi;
      otree->njetshad = otree->njets;
      if (isWJets) otree->njetshad += 1;

      if(ApplyRecoilCorrections){        
      	genV = genTools::genV(analysisTree);
      	genL = genTools::genL(analysisTree);

        genTools::KITRecoilCorrections( recoilCorrector, ApplyRecoilCorrections, // pass the value != 0 to apply corrections
          otree->puppimet, otree->puppimetphi,
          genV.Px(), genV.Py(),
          genL.Px(), genL.Py(),
          otree->njetshad,
          otree->met_rcmr, otree->metphi_rcmr
        );
        
        // overwriting with recoil-corrected values 
        otree->puppimet = otree->met_rcmr;
        otree->puppimetphi = otree->metphi_rcmr;   
	
        genTools::KITRecoilCorrections( PFMetRecoilCorrector, ApplyRecoilCorrections, // pass the value != 0 to apply corrections
          otree->met, otree->metphi,
          genV.Px(), genV.Py(),
          genL.Px(), genL.Py(),
          otree->njetshad,
          otree->met_rcmr, otree->metphi_rcmr
        );
	otree->met = otree->met_rcmr;
	otree->metphi = otree->metphi_rcmr;
      }
      
      ////////////////////////////////////////////////////////////
      // Filling variables (with corrected MET and electron pt)
      ////////////////////////////////////////////////////////////

      TLorentzVector muonLV; muonLV.SetPtEtaPhiM(otree->pt_2,
						 otree->eta_2,
						 otree->phi_2,
						 muonMass);

      TLorentzVector electronLV; electronLV.SetPtEtaPhiM(otree->pt_1,
							 otree->eta_1,
							 otree->phi_1,
							 electronMass);

      TLorentzVector metLV; metLV.SetXYZT(otree->met*TMath::Cos(otree->metphi),
					  otree->met*TMath::Sin(otree->metphi),
					  0.0,
					  otree->met);

      TLorentzVector puppimetLV; puppimetLV.SetXYZT(otree->puppimet*TMath::Cos(otree->puppimetphi),
						    otree->puppimet*TMath::Sin(otree->puppimetphi),
						    0.0,
						    otree->puppimet);

      TLorentzVector dileptonLV = muonLV + electronLV;
      otree->m_vis = dileptonLV.M();
    
      // opposite charge
      otree->os = (otree->q_1 * otree->q_2) < 0.;
    
      otree->mt_1 = mT(electronLV, metLV);
      otree->mt_2 = mT(muonLV, metLV);
      otree->puppimt_1 = mT(electronLV, puppimetLV);
      otree->puppimt_2 = mT(muonLV, puppimetLV);
    
      // bisector of lepton and tau transverse momenta
    
      otree->pzetavis  = calc::pzetavis(electronLV,muonLV);

      TLorentzVector metxLV = metLV;
      if (usePuppiMET) 
	metxLV = puppimetLV;

      otree->pt_tt = (dileptonLV+metxLV).Pt();   
      otree->mt_tot = calc::mTtot(electronLV,muonLV,metxLV);
      otree->pzetamiss = calc::pzetamiss(electronLV,muonLV,metxLV);
      otree->pzeta = calc::pzeta(electronLV,muonLV,metxLV);


      // Preselection cuts ->
      bool isSRevent = otree->iso_1<0.4&&otree->iso_2<0.4&&otree->extramuon_veto<0.5&&otree->extraelec_veto<0.5&&trigger_fired&&((otree->nbtag>0||otree->nbtag<3)||(otree->nbtag_raw>0||otree->nbtag_raw<3));

      if (!isSRevent) continue;

      ////////////////////////////////////////////////////////////
      // ID/Iso and Trigger Corrections
      ////////////////////////////////////////////////////////////

      // setting weights to 1
      otree->trkeffweight_1 = 1;
      otree->trkeffweight_2 = 1;
      otree->trigweight_1 = 1;
      otree->trigweight_2 = 1;
      otree->idisoweight_1 = 1;
      otree->idisoweight_antiiso_1 = 1;
      otree->idisoweight_2 = 1;
      otree->idisoweight_antiiso_2 = 1;

      otree->trigweight = 1;
      otree->trigweightSingle = 1;
      otree->trigweightEMu = 1;

      otree->effweight = 1;
      otree->effweightSingle = 1;
      otree->effweightEMu = 1;

      otree->weight = 1;
      otree->weightSingle = 1;
      otree->weightEMu = 1;

      otree->trigweight_l_lt = 1;
      otree->trigweight_t_lt = 1;
	
      float eff_data_trig_m = 1;
      float eff_mc_trig_m = 1;
      float sf_trig_m = 1;
      float eff_data_trig_e = 1;
      float eff_mc_trig_e = 1;
      float sf_trig_e = 1;
      
      float eff_data_trig_mhigh = 1;
      float eff_mc_trig_mhigh = 1;
      float eff_data_trig_mlow = 1;
      float eff_mc_trig_mlow = 1;

      float eff_data_trig_ehigh = 1;
      float eff_mc_trig_ehigh = 1;
      float eff_data_trig_elow = 1;
      float eff_mc_trig_elow = 1;
       
      //      std::cout << "before weights " << std::endl;

      if ((!isData || isEmbedded) && ApplyLepSF) {
      	TString suffix = "mc";
      	TString suffixRatio = "ratio";

	// IC scale factors --->
	scaleFactors->setLeptons(otree->pt_1,otree->eta_1,otree->iso_1,
				 otree->pt_2,otree->eta_2,otree->iso_2);

	otree->idisoweight_1 = scaleFactors->getIdIso1_SF();
	otree->idisoweight_2 = scaleFactors->getIdIso2_SF();
	otree->trkeffweight_1 = scaleFactors->getTrk1_SF();
	otree->trkeffweight_2 = scaleFactors->getTrk2_SF();
       

	otree->trigweight_1 = sf_trig_e;
	otree->trigweight_2 = sf_trig_m;
	
	float eff_single_data = 1.0 - (1.0-eff_data_trig_e)*(1.0-eff_data_trig_m);
	float eff_single_mc   = 1.0 - (1.0-eff_mc_trig_e)  *(1.0-eff_mc_trig_m);

	if (eff_single_mc<1e-3||eff_single_data<1e-3) 
	  otree->trigweightSingle = 0.0;
	else
	  otree->trigweightSingle = eff_single_data/eff_single_mc;

	// e-mu trigger
	otree->trigweightEMu = scaleFactors->getTrigger_SF();

	otree->trigweight = otree->trigweightEMu;
	
	/*
	std::cout << "pt_1 : " << otree->pt_1 << "  eta_1 = " << otree->eta_1 << "  iso_1 = " << otree->iso_1 << std::endl; 
	std::cout << "idiso_1 = " << otree->idisoweight_1 << std::endl;
	std::cout << "trkeff_1 = " << otree->trkeffweight_1 << std::endl; 
	std::cout << "pt_2 : " << otree->pt_2 << "  eta_2 = " << otree->eta_2 << "  iso_2 = " << otree->iso_2 << std::endl; 
	std::cout << "idiso_2 = " << otree->idisoweight_2 << std::endl;
	std::cout << "trkeff_2 = " << otree->trkeffweight_2 << std::endl;
	
	std::cout << "trigweight(single-lep) = " << otree->trigweightSingle << std::endl;
	std::cout << "trigweight(e+mu)       = " << otree->trigweightEMu << std::endl;

	std::cout << std::endl;
	*/

	float eff_emu_weight = otree->idisoweight_1 * otree->trkeffweight_1 * otree->idisoweight_2 * otree->trkeffweight_2;
	otree->effweight = eff_emu_weight * otree->trigweight;
	otree->effweightSingle = eff_emu_weight * otree->trigweightSingle;
	otree->effweightEMu = eff_emu_weight * otree->trigweightEMu;
	otree->weight = otree->effweight;
	otree->weightSingle = otree->effweightSingle;
	otree->weightEMu = otree->effweightEMu;
      }
      /*
      */
      // b-tagging weight 
      //      std::cout << "BTagSF = " << otree->btagSF << std::endl;
      if (!isData) {
	otree->weight *= otree->btagSF;
	otree->weightEMu *= otree->btagSF;
      }

      // embedded weight
      otree->embweight = 1.0;
      if (isEmbedded) {
	otree->embweight = getEmbeddedWeight(&analysisTree, wEmb);
	//	otree->embweight = getEmbeddedWeightKIT(&analysisTree, correctionWS, era);
	if (otree->embweight>10.0)
	  cout << "warning : embedding weight = " << otree->embweight << endl;
	otree->weight *= otree->embweight;
	otree->weightSingle *= otree->embweight;
	otree->weightEMu *= otree->embweight;
      }

      // zpt embedded weight
      otree->zptembweight = 1.0;
      if (isEmbedded) {
	otree->zptembweight = getZPtEmbeddedWeight(&analysisTree,h_zptweight_emb);
	otree->weight *= otree->zptembweight;
	otree->weightSingle *= otree->zptembweight;
	otree->weightEMu *= otree->zptembweight;
      }

      // PU weight
      otree->puweight = 1.0;
      if (ApplyPUweight) {
        otree->puweight = float(PUofficial->get_PUweight(double(analysisTree.numtruepileupinteractions)));
	otree->weight *= otree->puweight;
	otree->weightSingle *= otree->puweight;
	otree->weightEMu *= otree->puweight;
	//	std::cout << "puweight = " << otree->puweight << std::endl;
      }

      // generator weight
      otree->mcweight = 1.0;
      if(!isData || isEmbedded){
	otree->mcweight = 1.0;
	if (analysisTree.genweight<0.) otree->mcweight = -1.0;
        otree->gen_noutgoing = analysisTree.genparticles_noutgoing;
	if (is_amcatnlo)
	  otree->gen_noutgoing = analysisTree.genparticles_noutgoing_NLO;
	if (isEmbedded) {
	  otree->mcweight = analysisTree.genweight;
	  if (otree->mcweight>1.0)
	    otree->mcweight = 0.0;
	}
	otree->weight *= otree->mcweight;
	otree->weightSingle *= otree->mcweight;
	otree->weightEMu *= otree->mcweight;	
      }
      
      //Theory uncertainties 
      otree->weight_CMS_scale_gg_13TeVUp   = qcdScaleUp;
      otree->weight_CMS_scale_gg_13TeVDown = qcdScaleDown;

      otree->weight_CMS_QCDScale[0] = analysisTree.weightScale0;
      otree->weight_CMS_QCDScale[1] = analysisTree.weightScale1;
      otree->weight_CMS_QCDScale[2] = analysisTree.weightScale2;
      otree->weight_CMS_QCDScale[3] = analysisTree.weightScale3;
      otree->weight_CMS_QCDScale[4] = analysisTree.weightScale4;
      otree->weight_CMS_QCDScale[5] = analysisTree.weightScale5;
      otree->weight_CMS_QCDScale[6] = analysisTree.weightScale6;
      otree->weight_CMS_QCDScale[7] = analysisTree.weightScale7;
      otree->weight_CMS_QCDScale[8] = analysisTree.weightScale8;

      otree->weight_CMS_PS_FSR_ggH_13TeVDown = analysisTree.gen_pythiaweights[4]/analysisTree.gen_pythiaweights[0];
      otree->weight_CMS_PS_FSR_ggH_13TeVUp   = analysisTree.gen_pythiaweights[5]/analysisTree.gen_pythiaweights[0];

      otree->weight_CMS_PS_ISR_ggH_13TeVDown = analysisTree.gen_pythiaweights[26]/analysisTree.gen_pythiaweights[0];
      otree->weight_CMS_PS_ISR_ggH_13TeVUp   = analysisTree.gen_pythiaweights[27]/analysisTree.gen_pythiaweights[0];

      //Prefiring weights for CP analysis
      otree->prefiringweight     = analysisTree.prefiringweight;
      otree->prefiringweightUp   = analysisTree.prefiringweightup;
      otree->prefiringweightDown = analysisTree.prefiringweightdown;
      if (!isData && !isEmbedded) {
	if (era<2018) {
	  otree->weight *= otree->prefiringweight;
	  otree->weightSingle *= otree->prefiringweight;
	  otree->weightEMu *= otree->prefiringweight;
	}
      }
      // ************************
      // QCD background weights *
      // ************************
      otree->qcdweight_deltaR = 1.0;

      otree->qcdweight_deltaR_0jet_Par0_up = 1.0;
      otree->qcdweight_deltaR_0jet_Par0_down = 1.0;
      otree->qcdweight_deltaR_0jet_Par1_up = 1.0;
      otree->qcdweight_deltaR_0jet_Par1_down = 1.0;
      otree->qcdweight_deltaR_0jet_Par2_up = 1.0;
      otree->qcdweight_deltaR_0jet_Par2_down = 1.0;

      otree->qcdweight_deltaR_1jet_Par0_up = 1.0;
      otree->qcdweight_deltaR_1jet_Par0_down = 1.0;
      otree->qcdweight_deltaR_1jet_Par1_up = 1.0;
      otree->qcdweight_deltaR_1jet_Par1_down = 1.0;
      otree->qcdweight_deltaR_1jet_Par2_up = 1.0;
      otree->qcdweight_deltaR_1jet_Par2_down = 1.0;

      otree->qcdweight_deltaR_2jet_Par0_up = 1.0;
      otree->qcdweight_deltaR_2jet_Par0_down = 1.0;
      otree->qcdweight_deltaR_2jet_Par1_up = 1.0;
      otree->qcdweight_deltaR_2jet_Par1_down = 1.0;
      otree->qcdweight_deltaR_2jet_Par2_up = 1.0;
      otree->qcdweight_deltaR_2jet_Par2_down = 1.0;

      

      if(otree->njets==0){
	otree->qcdweight_deltaR =OS_SS_njet0->Eval(otree->dr_tt);
	otree->qcdweight_deltaR_0jet_Par0_up =  OS_SS_njet0_Par0_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_0jet_Par0_down =  OS_SS_njet0_Par0_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_0jet_Par1_up =  OS_SS_njet0_Par1_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_0jet_Par1_down =  OS_SS_njet0_Par1_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_0jet_Par2_up =  OS_SS_njet0_Par2_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_0jet_Par2_down =  OS_SS_njet0_Par2_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	/*
	std::cout << "qcdweight_deltaR_0jet_Par0_up   = " << otree->qcdweight_deltaR_0jet_Par0_up << std::endl;
	std::cout << "qcdweight_deltaR_0jet_Par0_down = " << otree->qcdweight_deltaR_0jet_Par0_down << std::endl;
	std::cout << "qcdweight_deltaR_0jet_Par1_up   = " << otree->qcdweight_deltaR_0jet_Par1_up << std::endl;
	std::cout << "qcdweight_deltaR_0jet_Par1_down = " << otree->qcdweight_deltaR_0jet_Par1_down << std::endl;
	std::cout << "qcdweight_deltaR_0jet_Par2_up   = " << otree->qcdweight_deltaR_0jet_Par2_up << std::endl;
	std::cout << "qcdweight_deltaR_0jet_Par2_down = " << otree->qcdweight_deltaR_0jet_Par2_down << std::endl;
	*/

      }
      else if(otree->njets ==1) {
	otree->qcdweight_deltaR = OS_SS_njet1->Eval(otree->dr_tt);
	otree->qcdweight_deltaR_1jet_Par0_up =  OS_SS_njet1_Par0_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_1jet_Par0_down =  OS_SS_njet1_Par0_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_1jet_Par1_up =  OS_SS_njet1_Par1_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_1jet_Par1_down =  OS_SS_njet1_Par1_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_1jet_Par2_up =  OS_SS_njet1_Par2_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_1jet_Par2_down =  OS_SS_njet1_Par2_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	/*
	std::cout << "qcdweight_deltaR_1jet_Par0_up   = " << otree->qcdweight_deltaR_1jet_Par0_up << std::endl;
	std::cout << "qcdweight_deltaR_1jet_Par0_down = " << otree->qcdweight_deltaR_1jet_Par0_down << std::endl;
	std::cout << "qcdweight_deltaR_1jet_Par1_up   = " << otree->qcdweight_deltaR_1jet_Par1_up << std::endl;
	std::cout << "qcdweight_deltaR_1jet_Par1_down = " << otree->qcdweight_deltaR_1jet_Par1_down << std::endl;
	std::cout << "qcdweight_deltaR_1jet_Par2_up   = " << otree->qcdweight_deltaR_1jet_Par2_up << std::endl;
	std::cout << "qcdweight_deltaR_1jet_Par2_down = " << otree->qcdweight_deltaR_1jet_Par2_down << std::endl;
	*/
      }
      else {

	otree->qcdweight_deltaR = OS_SS_njetgt1->Eval(otree->dr_tt);
	otree->qcdweight_deltaR_2jet_Par0_up =  OS_SS_njetgt1_Par0_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_2jet_Par0_down =  OS_SS_njetgt1_Par0_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_2jet_Par1_up =  OS_SS_njetgt1_Par1_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_2jet_Par1_down =  OS_SS_njetgt1_Par1_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_2jet_Par2_up =  OS_SS_njetgt1_Par2_UP->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
	otree->qcdweight_deltaR_2jet_Par2_down =  OS_SS_njetgt1_Par2_DOWN->Eval(otree->dr_tt)/otree->qcdweight_deltaR;
        /*
	std::cout << "qcdweight_deltaR_2jet_Par0_up   = " << otree->qcdweight_deltaR_2jet_Par0_up << std::endl;
	std::cout << "qcdweight_deltaR_2jet_Par0_down = " << otree->qcdweight_deltaR_2jet_Par0_down << std::endl;
	std::cout << "qcdweight_deltaR_2jet_Par1_up   = " << otree->qcdweight_deltaR_2jet_Par1_up << std::endl;
	std::cout << "qcdweight_deltaR_2jet_Par1_down = " << otree->qcdweight_deltaR_2jet_Par1_down << std::endl;
	std::cout << "qcdweight_deltaR_2jet_Par2_up   = " << otree->qcdweight_deltaR_2jet_Par2_up << std::endl;
	std::cout << "qcdweight_deltaR_2jet_Par2_down = " << otree->qcdweight_deltaR_2jet_Par2_down << std::endl;
	*/
      }

      float pt1 = otree->pt_1;
      float pt2 = otree->pt_2;
      if (pt1>149.) pt1 = 149.0;
      if (pt2>149.) pt2 = 149.0;

      otree->qcdweight_nonclosure = hNonClosureCorrection->GetBinContent(hNonClosureCorrection->GetXaxis()->FindBin(pt2),hNonClosureCorrection->GetYaxis()->FindBin(pt1));
      otree->qcdweight_isolationcorrection = hIsolationCorrection->GetBinContent(hIsolationCorrection->GetXaxis()->FindBin(pt2),hIsolationCorrection->GetYaxis()->FindBin(pt1));
        
      otree->qcdweight=otree->qcdweight_deltaR*otree->qcdweight_nonclosure*otree->qcdweight_isolationcorrection;

      ////////////////////////////////////////////////////////////
      // Z pt weight
      ////////////////////////////////////////////////////////////
      

      otree->zptweight = 1.;
      if (!isData && isDY && !isDYamcatnlo){
        genV = genTools::genV(analysisTree); // gen Z boson 
      	float bosonMass = genV.M();
      	float bosonPt = genV.Pt();

        double massxmin = h_zptweight->GetXaxis()->GetXmin();
        double massxmax = h_zptweight->GetXaxis()->GetXmax();

        double ptxmin = h_zptweight->GetYaxis()->GetXmin();
        double ptxmax = h_zptweight->GetYaxis()->GetXmax();

      	Float_t zptmassweight = 1;
      	if (bosonMass > 50.0) {
          float bosonMassX = bosonMass;
          float bosonPtX = bosonPt;
          if (bosonMassX > massxmax) bosonMassX = massxmax - h_zptweight->GetXaxis()->GetBinWidth(h_zptweight->GetYaxis()->GetNbins())*0.5;
          if (bosonPtX < ptxmin)     bosonPtX = ptxmin + h_zptweight->GetYaxis()->GetBinWidth(1)*0.5;
          if (bosonPtX > ptxmax)     bosonPtX = ptxmax - h_zptweight->GetYaxis()->GetBinWidth(h_zptweight->GetYaxis()->GetNbins())*0.5;
          zptmassweight = h_zptweight->GetBinContent(h_zptweight->GetXaxis()->FindBin(bosonMassX), h_zptweight->GetYaxis()->FindBin(bosonPtX));
	}	
	//	std::cout << "madgraph ->" << std::endl;
	//	std::cout << "Z Mass = " << bosonMass << "  pt = " << bosonPt << "  weight = " << zptmassweight << std::endl;
	otree->zptweight = zptmassweight;
      }
      if (!isData && isDY && isDYamcatnlo) {
	genV = genTools::genV(analysisTree); // gen Z boson ?
        float bosonMass = genV.M();
        float bosonPt = genV.Pt();
	TH2D * histZPt = h_zptNLOweight;
	if (otree->nbtag>=1) {
	  histZPt = h_zptNLOweight_1btag;
	}
	int massBins = histZPt->GetNbinsY();
	int ptBins = histZPt->GetNbinsX();
	float MassMin = histZPt->GetYaxis()->GetBinLowEdge(1);
	float MassMax = histZPt->GetYaxis()->GetBinLowEdge(massBins+1);
	float ptMin = histZPt->GetXaxis()->GetBinLowEdge(1);
	float ptMax = histZPt->GetXaxis()->GetBinLowEdge(ptBins+1);
	float zptmassweight = 1;
	if (bosonMass<MassMin) bosonMass = MassMin+0.5;
	if (bosonMass>MassMax) bosonMass = MassMax-0.5;
	if (bosonPt<ptMin) bosonPt = ptMin+0.5;
	if (bosonPt>ptMax) bosonPt = ptMax-0.5;
	zptmassweight = histZPt->GetBinContent(histZPt->FindBin(bosonPt,bosonMass));
	//	std::cout << "amcatnlo ->" << std::endl;
	//	std::cout << "Z Mass = " << bosonMass << "  pt = " << bosonPt << "  weight = " << zptmassweight << std::endl;
	otree->zptweight = zptmassweight;
	// Pascal's corretion ->
	if (otree->nbtag>=1) {
	  histZPt = h_zptNLOweight_1bsys;
	  bosonMass = genV.M();
	  bosonPt = genV.Pt();
	  massBins = histZPt->GetNbinsY();
	  ptBins = histZPt->GetNbinsX();
	  MassMin = histZPt->GetYaxis()->GetBinLowEdge(1);
	  MassMax = histZPt->GetYaxis()->GetBinLowEdge(massBins+1);
	  ptMin = histZPt->GetXaxis()->GetBinLowEdge(1);
	  ptMax = histZPt->GetXaxis()->GetBinLowEdge(ptBins+1);	  
	  if (bosonMass<MassMin) bosonMass = MassMin+0.5;
	  if (bosonMass>MassMax) bosonMass = MassMax-0.5;
	  if (bosonPt<ptMin) bosonPt = ptMin+0.5;
	  if (bosonPt>ptMax) bosonPt = ptMax-0.5;
	  otree->zptweight_1btag = histZPt->GetBinContent(histZPt->FindBin(bosonPt,bosonMass));
	}
	else {
	  otree->zptweight_0btag = zptmassweight;
	}

      }
      otree->weight *= otree->zptweight;
      otree->weightSingle *= otree->zptweight;
      otree->weightEMu *= otree->zptweight;
      
      ////////////////////////////////////////////////////////////
      // Top pt weight
      ////////////////////////////////////////////////////////////

      otree->topptweight = 1.;
      if(!isData && isTTbar){
        float a_topPtWeight = cfg.get<float>("a_topPtWeight");
        float b_topPtWeight = cfg.get<float>("b_topPtWeight");
        float c_topPtWeight = cfg.get<float>("c_topPtWeight");
        float max_pt_topPtWeight = cfg.get<float>("max_pt_topPtWeight");
        otree->topptweight = genTools::topPtWeight_Run2(analysisTree, a_topPtWeight, b_topPtWeight, c_topPtWeight, max_pt_topPtWeight);
        // otree->topptweight = genTools::topPtWeight(analysisTree, 1); // 1 is for Run1 - use this reweighting as recommended by HTT 17
	otree->weight *= otree->topptweight;
	otree->weightSingle *= otree->topptweight;
	otree->weightEMu *= otree->topptweight;
      }

      // ************************
      // Data has weight 1.0
      // ************************
      if (!isEmbedded && isData) {
	otree->weight = 1.0;
	otree->weightSingle = 1.0;
	otree->weightEMu = 1.0;
      }

      //      if (isGGH) {
      //	getHiggsPtWeight(&analysisTree,otree,higgsPt_ws,HiggsMass);
      //      }



      //boolean used to compute SVFit variables only on SR events, it is set to true when running Synchronization to run SVFit on all events

      // initialize svfit and fastMTT variables
      otree->m_sv   = -10;
      otree->pt_sv  = -10;
      otree->eta_sv = -10;
      otree->phi_sv = -10;
      otree->met_sv = -10;
      otree->mt_sv = -10;
      otree->m_fast = -10;
      otree->mt_fast = -10;
      otree->pt_fast = -10;
      otree->phi_fast = -10;
      otree->eta_fast = -10;
      if ( (ApplySVFit||ApplyFastMTT) && isSRevent) {
	svfit_variables("em", &analysisTree, otree, &cfg, inputFile_visPtResolution);
      }
      // +++++++++++++++++++++++++++++++++++++++++++++++++
      // ++++++++ Systematic uncertainties +++++++++++++++
      // +++++++++++++++++++++++++++++++++++++++++++++++++
      
      /*      
      std::cout << "before systematics -> " << std::endl;
      std::cout << "pt_1      = " << otree->pt_1 << std::endl;
      std::cout << "pt_2      = " << otree->pt_2 << std::endl;
      std::cout << "m_sv      = " << otree->m_sv << std::endl;
      std::cout << "weightEMu     = " << otree->weightEMu << std::endl;
      std::cout << "embedweight   = " << otree->embweight << std::endl;
      std::cout << "mcweight      = " << otree->mcweight << std::endl;
      std::cout << "idisoweight_1 = " << otree->idisoweight_1 << std::endl;
      std::cout << "idisoweight_2 = " << otree->idisoweight_2 << std::endl;
      std::cout << "trigweightEMu = " << otree->trigweightEMu << std::endl;
      std::cout << "puweight      = " << otree->puweight << std::endl;
      std::cout << "btagweight    = " << otree->btagweight << std::endl;
      std::cout << std::endl;
      */

      // evaluate systematics for MC 
      if( !isData && !isEmbedded && ApplySystShift){
	//	  btagSys->Eval();
	//	  mistagSys->Eval();
	for(unsigned int i = 0; i < jetEnergyScaleSys.size(); i++) {
	  //	  cout << endl;
	  //	  cout << "+++++++++++++++++++++++++++++++++++++++++++++++++" << endl;
	  //	  cout << endl;	  
	  (jetEnergyScaleSys.at(i))->Eval(); 
	  //	  cout << endl;
	  //	  cout << "++++++++++++++++++++++++++++++++++++++++++++++++++++++" << endl; 
	  //	  cout << endl;
	}
      }

      //      if (isEmbedded && ApplySystShift) {
      //	for(unsigned int i = 0; i < puppiMetSys.size(); ++i)
      //	  (puppiMetSys.at(i))->Eval();	
      //      }

      if ((!isData||isEmbedded) && ApplySystShift) {

	if (usePuppiMET) {
	  for(unsigned int i = 0; i < puppiMetSys.size(); ++i)
	    (puppiMetSys.at(i))->Eval();
	}
	else {
	  for(unsigned int i = 0; i < metSys.size(); ++i) 
	    (metSys.at(i))->Eval();
	}

	//	muonScaleSys->Eval(utils::EMU);
		electronScaleSys->SetElectronIndex(electronIndex);
		electronScaleSys->SetIsEmbedded(isEmbedded);
		electronScaleSys->SetAC1B(&analysisTree);
		electronScaleSys->SetConfig(&cfg);
		electronScaleSys->Eval(utils::EMU);
	//	std::cout << std::endl;
      }

      selEvents++;      
      //      cout << "Filling tuple -> m_sv = " << otree->m_sv << "    puppimet = " << otree->puppimet << endl;
      otree->Fill();
    } // event loop
    
    nFiles++;
    delete _tree;
    file_->Close();
    delete file_;
  } // file loop
   

  std::cout << std::endl;
  std::cout << "Total number of input events    = " << int(inputEventsH->GetEntries()) << std::endl;
  std::cout << "Total number of events in Tree  = " << nEvents << std::endl;
  std::cout << "Total number of selected events = " << selEvents << std::endl;
  std::cout << std::endl;
  
  file->cd("");
  file->Write();

  // delete systematics objects

  //  if (muonScaleSys != 0) {
  //    muonScaleSys->Write("",TObject::kOverwrite);
  //    delete muonScaleSys;
  //  }
  
  if (electronScaleSys != 0) {
    electronScaleSys->Write("",TObject::kOverwrite);
    delete electronScaleSys;
  }

  //  if (btagSys != 0) {
  //    btagSys->Write("",TObject::kOverwrite);
  //    delete btagSys;
  //  }

  //  if (mistagSys != 0) {
  //    mistagSys->Write("",TObject::kOverwrite);
  //    delete mistagSys;
  //  }

  if(jetEnergyScaleSys.size() > 0){
    for (unsigned int i = 0; i < jetEnergyScaleSys.size(); i++){
      (jetEnergyScaleSys.at(i))->Write("",TObject::kOverwrite);
      delete jetEnergyScaleSys.at(i);
    }
  }

  if (metSys.size() > 0){
    for (unsigned int i = 0; i < metSys.size(); i++ ) {
      (metSys.at(i))->Write("",TObject::kOverwrite);
      delete metSys.at(i);
    }
  }

  if (puppiMetSys.size() > 0){
    for (unsigned int i = 0; i < puppiMetSys.size(); i++ ) {
      (puppiMetSys.at(i))->Write("",TObject::kOverwrite);
      delete puppiMetSys.at(i);
    }
  }

  delete scaleFactors;
  file->Close();
  delete file;

}

void CheckEMu(const AC1B *analysisTree, SynchTree *otree) {

  bool check = otree->iso_1<0.15 && otree->iso_2<0.2;// && ((otree->q_1 * otree->q_2)>0.);
  bool genmatch_em = otree->gen_match_1==1&&otree->gen_match_2==2;
  bool genmatch_mm = otree->gen_match_1==2&&otree->gen_match_2==2;

  bool Xprint =  check && genmatch_mm;
  if (!Xprint) return;

  std::cout << "e : pT = " << otree->pt_1 
	    << "  eta = " << otree->eta_1 
	    << "  phi = " << otree->phi_1
	    << "  q = " << otree->q_1 
	    << "  gen_match = " << otree->gen_match_1 << std::endl;

  std::cout << "m : pT = " << otree->pt_2 
	    << "  eta = " << otree->eta_2 
	    << "  phi = " << otree->phi_2
	    << "  q = " << otree->q_2 
	    << "  gen_match = " << otree->gen_match_2 << std::endl;
  for (unsigned int igen = 0; igen < analysisTree->genparticles_count; ++igen) {
    if (analysisTree->genparticles_isLastCopy[igen]||analysisTree->genparticles_isFirstCopy[igen]) {
      if ( analysisTree->genparticles_pdgid[igen] == 11 || 
	   analysisTree->genparticles_pdgid[igen] == -11 ||
	   analysisTree->genparticles_pdgid[igen] == -13 ||
	   analysisTree->genparticles_pdgid[igen] == 13) {
	TLorentzVector genLV; genLV.SetXYZT(analysisTree->genparticles_px[igen],
					    analysisTree->genparticles_py[igen],
					    analysisTree->genparticles_pz[igen],
					    analysisTree->genparticles_e[igen]);
	std::cout << analysisTree->genparticles_pdgid[igen] << " : "
		  << "  pT = " << genLV.Pt()
		  << "  eta = " << genLV.Eta()
		  << "  phi = " << genLV.Phi(); 
	if (analysisTree->genparticles_isFirstCopy[igen]) std::cout << " * ";
	std::cout << std::endl;
      }
    }
  }
  std::cout << std::endl;

}

//// FILLING FUNCTIONS //////

void FillVertices(const AC1B *analysisTree, SynchTree *otree, const bool isData){

  otree->RecoVertexX = analysisTree->primvertex_x;
  otree->RecoVertexY = analysisTree->primvertex_y;
  otree->RecoVertexZ = analysisTree->primvertex_z;  

  otree->pvx_bs = analysisTree->primvertexwithbs_x;
  otree->pvy_bs = analysisTree->primvertexwithbs_y;
  otree->pvz_bs = analysisTree->primvertexwithbs_z;

  if(!isData){
    for (unsigned int igen = 0; igen < analysisTree->genparticles_count; ++igen) {

  //here fill the generator vertices to have the gen information present in tree PER GOOD RECO EVENT
  //Note: we may want to add constraint that the W and Z are prompt. If we remove these, may get in trouble with a DY or W MC sample..

      if ( analysisTree->genparticles_pdgid[igen] == 23 || 
	   analysisTree->genparticles_pdgid[igen] == 24 ||
	   analysisTree->genparticles_pdgid[igen] == -24 ||
	   analysisTree->genparticles_pdgid[igen] == 25 || 
	   analysisTree->genparticles_pdgid[igen] == 35 || 
	   analysisTree->genparticles_pdgid[igen] == 36 ||
	   analysisTree->genparticles_pdgid[igen] == 6 ||
	   analysisTree->genparticles_pdgid[igen] == -6 ) {
        otree->GenVertexX = analysisTree->genparticles_vx[igen];
        otree->GenVertexY = analysisTree->genparticles_vy[igen];
        otree->GenVertexZ = analysisTree->genparticles_vz[igen];
        break;
      }
    }
  }
  else {//if it is data, fill with something recognisable nonsensible
    otree->GenVertexX = 0;
    otree->GenVertexY = 0;
    otree->GenVertexZ = 0;
  }
}

float getZPtEmbeddedWeight(const AC1B *analysisTree, TH2D * hist) {

  std::vector<TLorentzVector> taus; taus.clear();
  float emWeight = 1;
  for (unsigned int igentau = 0; igentau < analysisTree->gentau_count; ++igentau) {
    TLorentzVector tauLV; tauLV.SetXYZT(analysisTree->gentau_px[igentau], 
					analysisTree->gentau_py[igentau],
					analysisTree->gentau_pz[igentau],
					analysisTree->gentau_e[igentau]);
    if (analysisTree->gentau_isPrompt[igentau]&&analysisTree->gentau_isFirstCopy[igentau]) {
      taus.push_back(tauLV);
    }
  }

  //  std::cout << "n taus = " << taus.size() << std::endl;

  if (taus.size() == 2) {
    double mass = (taus[0]+taus[1]).M();
    double pT = (taus[0]+taus[1]).Pt();
    if (mass<1000.&&pT<1000.) {
      emWeight = hist->GetBinContent(hist->FindBin(mass,pT));
      //      std::cout << "M = " << mass << "  pT = " << pT << "  weight = " << emWeight << std::endl;
    }
  }
  return emWeight; 
}
float getEmbeddedWeight(const AC1B *analysisTree, RooWorkspace * wEm) {

  std::vector<TLorentzVector> taus; taus.clear();
  float emWeight = 1;
  for (unsigned int igentau = 0; igentau < analysisTree->gentau_count; ++igentau) {
    TLorentzVector tauLV; tauLV.SetXYZT(analysisTree->gentau_px[igentau], 
					analysisTree->gentau_py[igentau],
					analysisTree->gentau_pz[igentau],
					analysisTree->gentau_e[igentau]);
    if (analysisTree->gentau_isPrompt[igentau]&&analysisTree->gentau_isFirstCopy[igentau]) {
      taus.push_back(tauLV);
    }
  }

  //  std::cout << "n taus = " << taus.size() << "  :  wEm = " << wEm << std::endl;

  if (taus.size() == 2) {
    double gt1_pt  = taus[0].Pt();
    double gt1_eta = taus[0].Eta();
    double gt2_pt  = taus[1].Pt();
    double gt2_eta = taus[1].Eta();
    wEm->var("gt_pt")->setVal(gt1_pt);
    wEm->var("gt_eta")->setVal(gt1_eta);
    double id1_embed = wEm->function("m_sel_id_ic_ratio")->getVal();
    wEm->var("gt_pt")->setVal(gt2_pt);
    wEm->var("gt_eta")->setVal(gt2_eta);
    double id2_embed = wEm->function("m_sel_id_ic_ratio")->getVal();
    wEm->var("gt1_pt")->setVal(gt1_pt);
    wEm->var("gt2_pt")->setVal(gt2_pt);
    wEm->var("gt1_eta")->setVal(gt1_eta);
    wEm->var("gt2_eta")->setVal(gt2_eta);
    double trg_emb = wEm->function("m_sel_trg_ic_ratio")->getVal();
    emWeight = id1_embed * id2_embed * trg_emb;
  }
  //  std::cout << "IC : EmbWeight = " << emWeight << std::endl;

  return emWeight;

}

float getEmbeddedWeightKIT(const AC1B *analysisTree, RooWorkspace * wEm, int era) {

  std::vector<TLorentzVector> taus; taus.clear();
  float emWeight = 1;
  for (unsigned int igentau = 0; igentau < analysisTree->gentau_count; ++igentau) {
    TLorentzVector tauLV; tauLV.SetXYZT(analysisTree->gentau_px[igentau], 
					analysisTree->gentau_py[igentau],
					analysisTree->gentau_pz[igentau],
					analysisTree->gentau_e[igentau]);
    if (analysisTree->gentau_isPrompt[igentau]&&analysisTree->gentau_isFirstCopy[igentau]) {
      taus.push_back(tauLV);
    }
  }

  //  std::cout << "n taus = " << taus.size() << "  :  wEm = " << wEm << std::endl;

  if (taus.size() == 2) {
    double gt1_pt  = taus[0].Pt();
    double gt1_eta = taus[0].Eta();
    double gt2_pt  = taus[1].Pt();
    double gt2_eta = taus[1].Eta();
    wEm->var("gt_pt")->setVal(gt1_pt);
    wEm->var("gt_eta")->setVal(gt1_eta);
    double id1_embed = wEm->function("m_sel_idEmb_ratio")->getVal();
    wEm->var("gt_pt")->setVal(gt2_pt);
    wEm->var("gt_eta")->setVal(gt2_eta);
    double id2_embed = wEm->function("m_sel_idEmb_ratio")->getVal();
    wEm->var("gt1_pt")->setVal(gt1_pt);
    wEm->var("gt2_pt")->setVal(gt2_pt);
    wEm->var("gt1_eta")->setVal(gt1_eta);
    wEm->var("gt2_eta")->setVal(gt2_eta);
    double trg_emb = 1.0;
    if (era==2016)
      trg_emb = wEm->function("m_sel_trg_kit_ratio")->getVal();
    else
      trg_emb = wEm->function("m_sel_trg_ratio")->getVal();
    emWeight = id1_embed * id2_embed * trg_emb;
  }
  //  std::cout << "KIT : EmbWeight = " << emWeight << std::endl;

  return emWeight;

}

void initializeGenTree(SynchGenTree *gentree){
  gentree->Higgs_pt=-9999;
  gentree->Higgs_eta=-9999;
  gentree->Higgs_phi=-9999;
  gentree->Higgs_mass=-9999;
  gentree->pt_1=-9999;
  gentree->eta_1=-9999;
  gentree->phi_1=-9999;
  gentree->pt_2=-9999;
  gentree->eta_2=-9999;
  gentree->phi_2=-9999;

  gentree->VertexX=-9999;
  gentree->VertexY=-9999;
  gentree->VertexZ=-99999;


}

void FillGenTree(const AC1B *analysisTree, SynchGenTree *gentree){
  int ntaus=analysisTree->gentau_count;
  int npart=analysisTree->genparticles_count;
  int leptonid=15;
  TLorentzVector Tau1,Tau2,Tau;
  TLorentzVector Lepton;
  TLorentzVector lvector;
  int tauHIndex=-1;
  int tauLIndex=-1;
  int LeadingtauIndex=-1;
  int TrailingtauIndex=-1;
  double taumaxpt=-1;
  
  for(int itau=0;itau<ntaus;itau++){
    if(analysisTree->gentau_isLastCopy[itau]==1&&analysisTree->gentau_isPrompt[itau]==1){
      if(analysisTree->gentau_visible_pt[itau]>=taumaxpt) {
	LeadingtauIndex=itau; 
	taumaxpt=analysisTree->gentau_visible_pt[itau];
      }
    }
  }

  taumaxpt=-1; 
  for(int itau=0;itau<ntaus;itau++){
    if(analysisTree->gentau_isLastCopy[itau]==1&&analysisTree->gentau_isPrompt[itau]==1&&itau!=LeadingtauIndex){
      if(analysisTree->gentau_visible_pt[itau]>=taumaxpt) {
	TrailingtauIndex=itau; 
	taumaxpt=analysisTree->gentau_visible_pt[itau];
      }
    }
  }

  TLorentzVector genTauVis1; genTauVis1.SetXYZT(0,0,0,0);
  TLorentzVector genTauVis2; genTauVis2.SetXYZT(0,0,0,0);
  gentree->decaymode_1 = -1;
  gentree->decaymode_2 = -1;
  if (LeadingtauIndex>-1) {
    genTauVis1.SetXYZT(analysisTree->gentau_visible_px[LeadingtauIndex],
		       analysisTree->gentau_visible_py[LeadingtauIndex],
		       analysisTree->gentau_visible_pz[LeadingtauIndex],
		       analysisTree->gentau_visible_e[LeadingtauIndex]);
    gentree->decaymode_1 = analysisTree->gentau_decayMode[LeadingtauIndex];
  }
  if (TrailingtauIndex>-1) {
    genTauVis2.SetXYZT(analysisTree->gentau_visible_px[TrailingtauIndex],
		       analysisTree->gentau_visible_py[TrailingtauIndex],
		       analysisTree->gentau_visible_pz[TrailingtauIndex],
		       analysisTree->gentau_visible_e[TrailingtauIndex]);
    gentree->decaymode_2 = analysisTree->gentau_decayMode[TrailingtauIndex];
  }
  gentree->pt_1 = genTauVis1.Pt();
  gentree->eta_1 = genTauVis1.Eta();
  gentree->phi_1 = genTauVis1.Phi();

  gentree->pt_2 = genTauVis2.Pt();
  gentree->eta_2 = genTauVis2.Eta();
  gentree->phi_2 = genTauVis2.Phi();

  double dR;
  const double dRcut=0.3;
  for(int ipart=0;ipart<npart;ipart++){
    if((abs(analysisTree->genparticles_pdgid[ipart])==25||
	abs(analysisTree->genparticles_pdgid[ipart])==35||
	abs(analysisTree->genparticles_pdgid[ipart])==36)&&
       analysisTree->genparticles_isLastCopy[ipart]==1){
      TLorentzVector Higgs;
      Higgs.SetPxPyPzE(analysisTree->genparticles_px[ipart],
		       analysisTree->genparticles_py[ipart],
		       analysisTree->genparticles_pz[ipart],
		       analysisTree->genparticles_e[ipart]);
      gentree->Higgs_pt=Higgs.Pt();
      gentree->Higgs_eta=Higgs.Eta();
      gentree->Higgs_phi=Higgs.Phi();
      gentree->Higgs_mass=Higgs.M();
    }
  }

  
  if (LeadingtauIndex>-1&&TrailingtauIndex>-1)
    gen_acott(analysisTree,gentree,LeadingtauIndex,TrailingtauIndex);

  gentree->a1polarization_1=gen_A1Polarization(analysisTree,LeadingtauIndex);
  gentree->a1polarization_2=gen_A1Polarization(analysisTree,TrailingtauIndex);

//here fill the generator vertices to have the information present in tree
//Note: we may want to add constraint that the W and Z are prompt. If we remove these, may get in trouble with a DY or W MC sample..


  for (unsigned int igen=0; igen<analysisTree->genparticles_count; ++igen) {
    if ((analysisTree->genparticles_pdgid[igen]==23||analysisTree->genparticles_pdgid[igen]==24||
	analysisTree->genparticles_pdgid[igen]==25||analysisTree->genparticles_pdgid[igen]==35||analysisTree->genparticles_pdgid[igen]==36)&&analysisTree->genparticles_isLastCopy[igen]==1&&analysisTree->genparticles_isPrompt[igen]==1) {
      gentree->VertexX=analysisTree->genparticles_vx[igen];
      gentree->VertexY=analysisTree->genparticles_vy[igen];
      gentree->VertexZ=analysisTree->genparticles_vz[igen];
      break;
    }
  }
}

//fill the otree with the electron/muon variables in channel emu
void FillElMu(const AC1B *analysisTree, SynchTree *otree, int electronIndex, float dRIsoElectron, int muonIndex, float dRIsoMuon, int era, bool isEmbedded, bool isMcCorrectPuppi){
  
  float sf_eleES = 1.;
  if (isEmbedded) sf_eleES = EmbedElectronES_SF(analysisTree, era, electronIndex);  

  otree->pt_1  = 	sf_eleES*analysisTree->electron_pt[electronIndex];
  otree->pt_uncorr_1 =  analysisTree->electron_pt[electronIndex];
  if (isMcCorrectPuppi) 
    otree->pt_uncorr_1 = analysisTree->electron_pt[electronIndex]/analysisTree->electron_corr[electronIndex];
  otree->eta_1 = 	analysisTree->electron_eta[electronIndex];
  otree->phi_1 = 	analysisTree->electron_phi[electronIndex];
  otree->m_1 = 		electronMass;
  otree->q_1 = -1;
  if (analysisTree->electron_charge[electronIndex]>0)
    otree->q_1 = 1;
  otree->gen_match_1 = analysisTree->electron_genmatch[electronIndex];

  otree->iso_1 =  abs_Iso_et(electronIndex, analysisTree, dRIsoElectron) / (sf_eleES*analysisTree->electron_pt[electronIndex]);

  otree->d0_1 = analysisTree->electron_dxy[electronIndex];
  otree->dZ_1 = analysisTree->electron_dz[electronIndex];

  otree->pt_2  = 	analysisTree->muon_pt[muonIndex];
  otree->eta_2 = 	analysisTree->muon_eta[muonIndex];
  otree->phi_2 = 	analysisTree->muon_phi[muonIndex];
  otree->m_2   =  muonMass;
  otree->q_2   =  analysisTree->muon_charge[muonIndex];
  otree->gen_match_2 = analysisTree->muon_genmatch[muonIndex];
  otree->iso_2 = abs_Iso_mt(muonIndex, analysisTree, dRIsoMuon) / analysisTree->muon_pt[muonIndex];
  otree->d0_2 = analysisTree->muon_dxy[muonIndex];
  otree->dZ_2 = analysisTree->muon_dz[muonIndex];
  otree->q_2 = -1;
  if (analysisTree->muon_charge[muonIndex]>0)
    otree->q_2 = 1;

  otree->dr_tt = deltaR(otree->eta_1,otree->phi_1,otree->eta_2,otree->phi_2);

  if (otree->gen_match_1==0||otree->gen_match_1>5) {
    float dRMin = 0.4;
    otree->gen_match_1 = 6;
    for (unsigned int jet =0; jet<analysisTree->pfjet_count; ++jet) {
      float dR = deltaR(analysisTree->pfjet_eta[jet], analysisTree->pfjet_phi[jet], otree->eta_1, otree->phi_1);
      if (dR<dRMin) {
	dRMin = dR;
	int flavor = analysisTree->pfjet_flavour[jet];
	if (flavor==4) otree->gen_match_1 = 7;
	if (flavor==5) otree->gen_match_1 = 8;
      }
    }
  }

  if (otree->gen_match_2==0||otree->gen_match_2>5) {
    float dRMin = 0.4;
    otree->gen_match_2 = 6;
    for (unsigned int jet =0; jet<analysisTree->pfjet_count; ++jet) {
      float dR = deltaR(analysisTree->pfjet_eta[jet], analysisTree->pfjet_phi[jet], otree->eta_2, otree->phi_2);
      if (dR<dRMin) {
	dRMin = dR;
	int flavor = analysisTree->pfjet_flavour[jet];
	if (flavor==4) otree->gen_match_2 = 7;
	if (flavor==5) otree->gen_match_2 = 8;
      }
    }
  }

}

double MassFromTString(TString sample) {
  double mass = 1000.;
  std::vector<int> masses_int = {
    60,80,90,95,100,110,120,125,130,140,160,180,200,
    250,300,350,400,450,500,600,700,800,900,1000,
    1200,1400,1500,1600,1800,2000,2300,2600,2900,3200,3500
  };
  
  for (auto mass_int : masses_int) {    
    TString substring = "M"+TString(to_string(mass_int))+"_";
    if (sample.Contains(substring)) {
      mass = double(mass_int);
      break;
    }
  }
  
  return mass;
  
}

void GetPFMET(AC1B * analysisTree, SynchTree * otree) {

  otree->met = TMath::Sqrt(analysisTree->pfmetcorr_ex*analysisTree->pfmetcorr_ex +
			   analysisTree->pfmetcorr_ey*analysisTree->pfmetcorr_ey);

  otree->metphi = TMath::ATan2(analysisTree->pfmetcorr_ey,analysisTree->pfmetcorr_ex);
  otree->metcov00 = analysisTree->pfmetcorr_sigxx;
  otree->metcov01 = analysisTree->pfmetcorr_sigxy;
  otree->metcov10 = analysisTree->pfmetcorr_sigyx;
  otree->metcov11 = analysisTree->pfmetcorr_sigyy;

}

void GetPuppiMET(AC1B * analysisTree, SynchTree * otree) {

  /*
  bool is2017 = false;

  double metUncorr = TMath::Sqrt(analysisTree->puppimet_ex*analysisTree->puppimet_ex + analysisTree->puppimet_ey*analysisTree->puppimet_ey);
  double puppimet_uncorr = metUncorr;
  double puppimetphi_uncorr = TMath::ATan2(analysisTree->puppimet_ey,analysisTree->puppimet_ex);

  double metUncorrPx = analysisTree->puppimet_ex;
  double metUncorrPy = analysisTree->puppimet_ey;

  otree->puppimet_rcmr = puppimet_uncorr;
  otree->puppimetphi_rcmr = puppimetphi_uncorr;

  double shiftX_jets = 0;
  double shiftY_jets = 0;

  std::cout << "Event number = " << analysisTree->event_nr << std::endl;

  double shiftX_ele = 0;
  double shiftY_ele = 0;
  
  if (isEmbedded||isMcCorrectPuppi) { 
     double px_ele_uncorr = otree->pt_uncorr_1*TMath::Cos(otree->phi_1);
     double py_ele_uncorr = otree->pt_uncorr_1*TMath::Sin(otree->phi_1);
     double px_ele = otree->pt_1*TMath::Cos(otree->phi_1);
     double py_ele = otree->pt_1*TMath::Sin(otree->phi_1);
     shiftX_ele = shiftX_ele + px_ele_uncorr - px_ele;
     shiftY_ele = shiftY_ele + py_ele_uncorr - py_ele;
  }
  
  double shiftX = shiftX_ele;
  double shiftY = shiftY_ele;

  double metCorrPx = metUncorrPx + shiftX;
  double metCorrPy = metUncorrPy + shiftY;

  double metCorrPx = metUncorrPx;
  double metCorrPy = metUncorrPy;

  otree->puppimet_ex_UnclusteredEnUp = analysisTree->puppimet_ex_UnclusteredEnUp + shiftX;
  otree->puppimet_ex_UnclusteredEnDown = analysisTree->puppimet_ex_UnclusteredEnDown + shiftX;

  otree->puppimet_ey_UnclusteredEnUp = analysisTree->puppimet_ey_UnclusteredEnUp + shiftY;
  otree->puppimet_ey_UnclusteredEnDown = analysisTree->puppimet_ey_UnclusteredEnDown + shiftY;
  */

  double puppimet_ex = analysisTree->puppimet_ex;
  double puppimet_ey = analysisTree->puppimet_ey;
  otree->puppimet = TMath::Sqrt(puppimet_ex*puppimet_ex+
				puppimet_ey*puppimet_ey);
  otree->puppimetphi = TMath::ATan2(puppimet_ey,puppimet_ex);
  otree->puppimet_ex_UnclusteredEnUp = analysisTree->puppimet_ex_UnclusteredEnUp;
  otree->puppimet_ex_UnclusteredEnDown = analysisTree->puppimet_ex_UnclusteredEnDown;
  otree->puppimet_ey_UnclusteredEnUp = analysisTree->puppimet_ey_UnclusteredEnUp;
  otree->puppimet_ey_UnclusteredEnDown = analysisTree->puppimet_ey_UnclusteredEnDown;

  //  std::cout << "puppimet  : pt = " << otree->puppimet << "  phi = " << otree->puppimetphi << std::endl;
  //  printf("met_x : central = %6.1f  down = %6.1f  up = %6.1f\n",
  //	 puppimet_ex,otree->puppimet_ex_UnclusteredEnDown,otree->puppimet_ex_UnclusteredEnUp);
  //  printf("met_y : central = %6.1f  down = %6.1f  up = %6.1f\n",
  //	 puppimet_ey,otree->puppimet_ey_UnclusteredEnDown,otree->puppimet_ey_UnclusteredEnUp);
  //  std::cout << std::endl;

  otree->puppimetcov00 = analysisTree->puppimet_sigxx;
  otree->puppimetcov01 = analysisTree->puppimet_sigxy;
  otree->puppimetcov10 = analysisTree->puppimet_sigyx;
  otree->puppimetcov11 = analysisTree->puppimet_sigyy;

}

void getHiggsPtWeight(const AC1B * analysisTree, SynchTree * otree, RooWorkspace * ws, double mass) {

  for (unsigned int i=0; i<30; ++i)
    otree->ggHWeights[i] = 1.0;

  int HiggsIndex = -1;
  for (unsigned int i=0; i<analysisTree->genparticles_count; ++i) {
    int pdgId = analysisTree->genparticles_pdgid[i];
    if (pdgId==25||pdgId==35||pdgId==36) {
      HiggsIndex = i;
      TLorentzVector HiggsLV; HiggsLV.SetXYZT(analysisTree->genparticles_px[HiggsIndex],
					      analysisTree->genparticles_py[HiggsIndex],
					      analysisTree->genparticles_pz[HiggsIndex],
					      analysisTree->genparticles_e[HiggsIndex]);

      //      std::cout << "pdgid = " << pdgId << "  pt = " << HiggsLV.Pt() << std::endl;
    }
  }
  if (HiggsIndex>=0) {
    TLorentzVector HiggsLV; HiggsLV.SetXYZT(analysisTree->genparticles_px[HiggsIndex],
					    analysisTree->genparticles_py[HiggsIndex],
					    analysisTree->genparticles_pz[HiggsIndex],
					    analysisTree->genparticles_e[HiggsIndex]);
    ws->var("h_pt")->setVal(HiggsLV.Pt());
    ws->var("h_mass")->setVal(mass);

    //    std::cout << "Higgs mass = " << mass << "  pT = " << HiggsLV.Pt() << std::endl;
    for (unsigned int i=0; i<30; ++i) {
      otree->ggHWeights[i] = ws->function(otree->ggHWeights_name[i].c_str())->getVal();
      //      std::cout << otree->ggHWeights_name[i] << " : " << otree->ggHWeights[i] << std::endl;
    }
  }
  //  std::cout << std::endl;

}
