import FWCore.ParameterSet.Config as cms
from FWCore.ParameterSet.VarParsing import VarParsing

options = VarParsing('analysis')
options.register('fileList', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "List of root files to process.")
options.register('fileNamePrefix', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "Prefix to add to input file names.")
options.register('tupleOutput', 'output_tuple.root', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "Event tuple file.")
options.register('lumiFile', '', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "JSON file with lumi mask.")
options.register('isData', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 "For data : True, for MC : False, for Embedded : True")
options.register('isSingleMuonData', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 "Needed to record track collection for NMSSM ananlysis")
options.register('isEmbedded', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 "Set to true if you run over Z->TauTau embedded samples")
options.register('isHiggsSignal', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 "Set to true if you run over higgs signal samples -> needed for STXS1p1 flags")
options.register('year', 2016, VarParsing.multiplicity.singleton, VarParsing.varType.int,
                 "Options: 2016, 2017, 2018")
options.register('period', 'UL2016', VarParsing.multiplicity.singleton, VarParsing.varType.string,
                 "Options: UL2016APV, UL2016, UL2017, UL2018")
options.register('RunTauSpinnerProducer', False, VarParsing.multiplicity.singleton, VarParsing.varType.bool,
                 "Only do this if you want to calculate tauspinner weights for a sample with two taus and flat tau polarisation")
options.parseArguments()

# Configurable options =======================================================================
isData = options.isData
isSingleMuonData = options.isSingleMuonData
isEmbedded = options.isEmbedded
isHiggsSignal = options.isHiggsSignal
year = options.year
period = options.period
RunTauSpinnerProducer = options.RunTauSpinnerProducer

# # ============================================================================================
if isEmbedded : isData = True
# ============================================================================================

# Define the CMSSW process
process = cms.Process("TreeProducer")

# Load the standard set of configuration modules
process.load('Configuration.StandardSequences.Services_cff')
process.load('Configuration.StandardSequences.GeometryDB_cff')
process.load('Configuration.StandardSequences.MagneticField_38T_cff')
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_cff')
process.load("TrackingTools/TransientTrack/TransientTrackBuilder_cfi")
process.load('Configuration.StandardSequences.FrontierConditions_GlobalTag_condDBv2_cff')

# Global tag (from : https://twiki.cern.ch/twiki/bin/viewauth/CMS/PdmVRun2LegacyAnalysis - version : 2021-09-03)
if isData or isEmbedded : process.GlobalTag.globaltag = '106X_dataRun2_v35'
else:
    if period == 'UL2016' :   process.GlobalTag.globaltag = '106X_mcRun2_asymptotic_v17'
    elif period == 'UL2016APV' : process.GlobalTag.globaltag = '106X_mcRun2_asymptotic_preVFP_v11'
    elif period == 'UL2017' : process.GlobalTag.globaltag = '106X_mc2017_realistic_v9'
    elif period == 'UL2018' : process.GlobalTag.globaltag = '106X_upgrade2018_realistic_v16_L1v1'
    else:
        raise Exception('Error: period does not exist, options: UL2016APV, UL2016, UL2017, UL2018')

print "\nGlobal Tag: " + str(process.GlobalTag.globaltag)

# Message Logger settings
process.load("FWCore.MessageService.MessageLogger_cfi")
process.MessageLogger.destinations = ['cout', 'cerr']
process.MessageLogger.cerr.threshold = cms.untracked.string('INFO')
process.MessageLogger.cerr.FwkReport.reportEvery = 1000
# Set the process options -- Display summary at the end, enable unscheduled execution
process.options = cms.untracked.PSet(
    allowUnscheduled = cms.untracked.bool(True),
    wantSummary = cms.untracked.bool(True)
)

# How many events to process
process.maxEvents = cms.untracked.PSet(
   input = cms.untracked.int32(-1)
)

if options.maxEvents > 0:
    process.maxEvents.input = options.maxEvents

# Define the input source
process.source = cms.Source('PoolSource', fileNames = cms.untracked.vstring(), skipEvents = cms.untracked.uint32(0))

from DesyTauAnalyses.Producer.readFileList import *
if len(options.fileList) > 0:
    readFileList(process.source.fileNames, options.fileList, options.fileNamePrefix)
elif len(options.inputFiles) > 0:
    addFilesToList(process.source.fileNames, options.inputFiles, options.fileNamePrefix)

if len(options.lumiFile) > 0:
    import FWCore.PythonUtilities.LumiList as LumiList
    process.source.lumisToProcess = LumiList.LumiList(filename = options.lumiFile).getVLuminosityBlockRange()

### JECs ==============================================================================================
# From :  https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookJetEnergyCorrections#CorrPatJets

from PhysicsTools.PatAlgos.tools.jetTools import updateJetCollection
updateJetCollection(
  process,
  jetSource = cms.InputTag('slimmedJets'),
  labelName = 'UpdatedJEC',
  jetCorrections = ('AK4PFchs', cms.vstring(['L1FastJet', 'L2Relative', 'L3Absolute', 'L2L3Residual']), 'None')  # Update: Safe to always add 'L2L3Residual' as MC contains dummy L2L3Residual corrections (always set to 1)
)
process.jecSequence = cms.Sequence(process.patJetCorrFactorsUpdatedJEC * process.updatedPatJetsUpdatedJEC)

updateJetCollection(
  process,
  jetSource = cms.InputTag('slimmedJetsPuppi'),
  labelName = 'UpdatedJECPuppi',
  jetCorrections = ('AK4PFPuppi', cms.vstring(['L1FastJet', 'L2Relative', 'L3Absolute', 'L2L3Residual']), 'None')  # Update: Safe to always add 'L2L3Residual' as MC contains dummy L2L3Residual corrections (always set to 1)
)
process.jecSequencepuppi = cms.Sequence(process.patJetCorrFactorsUpdatedJECPuppi * process.updatedPatJetsUpdatedJECPuppi)

### END JECs ==========================================================================================

### MET ===============================================================================================
# from : https://twiki.cern.ch/twiki/bin/view/CMS/MissingETUncertaintyPrescription#We_have_a_tool_to_help_you_to_ap

# PFMET
from PhysicsTools.PatUtils.tools.runMETCorrectionsAndUncertainties import runMetCorAndUncFromMiniAOD

# If you only want to re-correct and get the proper uncertainties
runMetCorAndUncFromMiniAOD(process,
                           isData=isData
                           #isEmbeddedSample=isEmbedded,
                           #fixEE2017 = bool(period=='2017'),
                           #fixEE2017Params = {'userawPt': True, 'ptThreshold':50.0, 'minEtaThreshold':2.65, 'maxEtaThreshold': 3.139} ,
                           #postfix = "ModifiedMET"
                           )

# PuppiMET
from PhysicsTools.PatAlgos.slimming.puppiForMET_cff import makePuppiesFromMiniAOD
makePuppiesFromMiniAOD( process, True );

# If you only want to re-correct and get the proper uncertainties
runMetCorAndUncFromMiniAOD(process,
                           isData=isData,
                           #isEmbeddedSample=isEmbedded,
                           metType="Puppi",
                           postfix="Puppi",
                           jetFlavor="AK4PFPuppi",
                           )

# Re-calculation of Puppi weights only needed if the latest Tune (v11) was not applied on MiniAOD yet (see here: https://twiki.cern.ch/twiki/bin/view/CMS/PUPPI#PUPPI_Status_Release_notes)
process.puppiNoLep.useExistingWeights = True
process.puppi.useExistingWeights = True

### Electron ID, scale and smearing =======================================================================
# from https://twiki.cern.ch/twiki/bin/view/CMS/EgammaMiniAODV2#2017_MiniAOD_V2%20#https://twiki.cern.ch/twiki/bin/view/CMS/EgammaPostRecoR
from RecoEgamma.EgammaTools.EgammaPostRecoTools import setupEgammaPostRecoSeq

if period == 'UL2016APV' :
    labelEra = '2016preVFP-UL'
    rerunIDs = True
    rerunEnergyCorrections = True
elif period == 'UL2016' :
    labelEra = '2016postVFP-UL'
    rerunIDs = True
    rerunEnergyCorrections = True
elif period == 'UL2017' :
    labelEra = '2017-UL'
    rerunIDs = True
    rerunEnergyCorrections = True
elif period == 'UL2018' :
    labelEra = '2018-UL'
    rerunIDs = True
    rerunEnergyCorrections = True

setupEgammaPostRecoSeq(process,
                       runVID=rerunIDs,
                       runEnergyCorrections=rerunEnergyCorrections,
                       era=labelEra)


# prompt lepton MVA ID ===============================================================================================
from PhysicsTools.NanoAOD.muons_cff import slimmedMuonsUpdated
from PhysicsTools.NanoAOD.muons_cff import isoForMu
from PhysicsTools.NanoAOD.muons_cff import ptRatioRelForMu
from PhysicsTools.NanoAOD.muons_cff import slimmedMuonsWithUserData
from PhysicsTools.NanoAOD.muons_cff import muonMVATTH

ptRatioRelForMu.srcJet = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer")
process.slimmedMuonsUpdated = slimmedMuonsUpdated
process.isoForMu = isoForMu
process.ptRatioRelForMu = ptRatioRelForMu
process.slimmedMuonsWithUserData = slimmedMuonsWithUserData
process.muonMVATTH = muonMVATTH

process.slimmedMuonsWithUserData.src = cms.InputTag("slimmedMuonsUpdated")
process.slimmedMuonsWithUserData.userFloats = cms.PSet(
    miniIsoChg = cms.InputTag("isoForMu:miniIsoChg"),
    miniIsoAll = cms.InputTag("isoForMu:miniIsoAll"),
    ptRatio = cms.InputTag("ptRatioRelForMu:ptRatio"),
    ptRel = cms.InputTag("ptRatioRelForMu:ptRel"),
    jetNDauChargedMVASel = cms.InputTag("ptRatioRelForMu:jetNDauChargedMVASel"),
    )
process.slimmedMuonsWithUserData.userIntFromBools = cms.PSet() # Removed
process.slimmedMuonsWithUserData.userInts = cms.PSet() # Removed
process.muonMVATTH.src = cms.InputTag("slimmedMuonsWithUserData")

process.muonMVATTH2016 = muonMVATTH.clone(
    weightFile =  cms.FileInPath("PhysicsTools/NanoAOD/data/mu_BDTG_2016.weights.xml"),
    name = cms.string("muonMVATTH2016"),
)

process.slimmedMuonsWithMVA = cms.EDProducer("PATMuonUserDataEmbedder",
    src = cms.InputTag("slimmedMuonsWithUserData"),
    userFloats = cms.PSet(
        muonMVATTH = cms.InputTag("muonMVATTH"),
        muonMVATTH2016 = cms.InputTag("muonMVATTH2016"),
        )
    )

from PhysicsTools.NanoAOD.electrons_cff import isoForEle

if year == 2016 :
    isoForEle.EAFile_MiniIso = "RecoEgamma/ElectronIdentification/data/Spring15/effAreaElectrons_cone03_pfNeuHadronsAndPhotons_25ns.txt"
    isoForEle.EAFile_PFIso = "RecoEgamma/ElectronIdentification/data/Summer16/effAreaElectrons_cone03_pfNeuHadronsAndPhotons_80X.txt"

isoForEle.src = cms.InputTag("slimmedElectrons")
process.isoForEle = isoForEle

from PhysicsTools.NanoAOD.electrons_cff import ptRatioRelForEle
ptRatioRelForEle.srcJet = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer")
ptRatioRelForEle.srcLep = cms.InputTag("slimmedElectrons")
process.ptRatioRelForEle = ptRatioRelForEle


slimmedElectronsWithUserData = cms.EDProducer("PATElectronUserDataEmbedder",
    src = cms.InputTag("slimmedElectrons"),
    userFloats = cms.PSet(
        miniIsoChg = cms.InputTag("isoForEle:miniIsoChg"),
        miniIsoAll = cms.InputTag("isoForEle:miniIsoAll"),
        PFIsoChg = cms.InputTag("isoForEle:PFIsoChg"),
        PFIsoAll = cms.InputTag("isoForEle:PFIsoAll"),
        PFIsoAll04 = cms.InputTag("isoForEle:PFIsoAll04"),
        ptRatio = cms.InputTag("ptRatioRelForEle:ptRatio"),
        ptRel = cms.InputTag("ptRatioRelForEle:ptRel"),
        jetNDauChargedMVASel = cms.InputTag("ptRatioRelForEle:jetNDauChargedMVASel"),
    ),
    userCands = cms.PSet(
        jetForLepJetVar = cms.InputTag("ptRatioRelForEle:jetForLepJetVar") # warning: Ptr is null if no match is found
    ),
)
process.slimmedElectronsWithUserData = slimmedElectronsWithUserData

from PhysicsTools.NanoAOD.electrons_cff import electronMVATTH
import PhysicsTools.PatAlgos.producersLayer1.electronProducer_cfi

process.electronMVATTH = electronMVATTH

process.electronMVATTH.src = cms.InputTag("slimmedElectronsWithUserData")
process.electronMVATTH.variables.LepGood_mvaFall17V2noIso =cms.string("userFloat('ElectronMVAEstimatorRun2Fall17NoIsoV2Values')")

process.electronMVATTH2016 = electronMVATTH.clone(
    weightFile =  cms.FileInPath("PhysicsTools/NanoAOD/data/el_BDTG_2016.weights.xml"),
    name = cms.string("electronMVATTH2016"),
)

process.slimmedElectronsWithMVA = cms.EDProducer("PATElectronUserDataEmbedder",
    src = cms.InputTag("slimmedElectronsWithUserData"),
    userFloats = cms.PSet(
        electronMVATTH = cms.InputTag("electronMVATTH"),
        electronMVATTH2016 = cms.InputTag("electronMVATTH2016"),
        )
    )

process.promptleptonMVA_sequence = cms.Sequence(process.slimmedMuonsUpdated * process.isoForMu * process.ptRatioRelForMu * process.slimmedMuonsWithUserData * process.muonMVATTH * process.muonMVATTH2016 * process.slimmedMuonsWithMVA * process.isoForEle * process.ptRatioRelForEle * process.slimmedElectronsWithUserData * process.electronMVATTH * process.electronMVATTH2016 * process.slimmedElectronsWithMVA)
#prompt lepton MVA ID ending ===============================================================================================


# Tau ID ===============================================================================================
# https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuidePFTauID#Running_of_the_DNN_based_tau_ID

updatedTauName = "NewTauIDsEmbedded" #name of pat::Tau collection with new tau-Ids
import RecoTauTag.RecoTau.tools.runTauIdMVA as tauIdConfig
tauIdEmbedder = tauIdConfig.TauIDEmbedder(process, cms, debug = False,
                                          updatedTauName = updatedTauName,
                                          toKeep = [ "2017v2", "deepTau2017v2p1"]#,"MVADM_2017_v1"]
                                          )

tauIdEmbedder.runTauID()
# END Tau ID ===========================================================================================


# Vertex Refitting ===============================================================================================

#load vertex refitting excluding tau tracks
process.load('HiggsCPinTauDecays.TauRefit.AdvancedRefitVertexProducer_cfi')
process.load('HiggsCPinTauDecays.TauRefit.LeptonPreSelections_cfi')
process.load('HiggsCPinTauDecays.TauRefit.MiniAODRefitVertexProducer_cfi')
# END Vertex Refitting ===========================================================================================

# HTXS ========================================================================================================
if not isData and isHiggsSignal:
    process.load("SimGeneral.HepPDTESSource.pythiapdt_cfi")
    process.mergedGenParticles = cms.EDProducer("MergedGenParticleProducer",
                                                inputPruned = cms.InputTag("prunedGenParticles"),
                                                inputPacked = cms.InputTag("packedGenParticles"),
                                                )
    process.myGenerator = cms.EDProducer("GenParticles2HepMCConverter",
                                         genParticles = cms.InputTag("mergedGenParticles"),
                                         genEventInfo = cms.InputTag("generator"),
                                         signalParticlePdgIds = cms.vint32(25),
                                         )
    process.rivetProducerHTXS = cms.EDProducer('HTXSRivetProducer',
                                               HepMCCollection = cms.InputTag('myGenerator','unsmeared'),
                                               LHERunInfo = cms.InputTag('externalLHEProducer'),
                                               ProductionMode = cms.string('AUTO'),
                                               )
    process.htxsSequence = cms.Sequence(  process.mergedGenParticles * process.myGenerator * process.rivetProducerHTXS )
else :
    process.htxsSequence = cms.Sequence( )
# END HTXS ====================================================================================================


# Pre-firing weights ==========================================================================================
# https://twiki.cern.ch/twiki/bin/viewauth/CMS/L1PrefiringWeightRecipe#Recipe_details_10_6_X_X_26_or_12
from PhysicsTools.PatUtils.l1PrefiringWeightProducer_cfi import l1PrefiringWeightProducer
if period == 'UL2018':
    process.prefiringweight = l1PrefiringWeightProducer.clone(
    TheJets = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer"), #this should be the slimmedJets collection with up to date JECs !
    DataEraECAL = cms.string("None"),
    DataEraMuon = cms.string("20172018"),
    UseJetEMPt = cms.bool(False),
    PrefiringRateSystematicUnctyECAL = cms.double(0.2),
    PrefiringRateSystematicUnctyMuon = cms.double(0.2)
    )
elif period == 'UL2017':
    process.prefiringweight = l1PrefiringWeightProducer.clone(
    TheJets = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer"), #this should be the slimmedJets collection with up to date JECs !
    DataEraECAL = cms.string("UL2017BtoF"),
    DataEraMuon = cms.string("20172018"),
    UseJetEMPt = cms.bool(False),
    PrefiringRateSystematicUnctyECAL = cms.double(0.2),
    PrefiringRateSystematicUnctyMuon = cms.double(0.2)
    )
elif period == 'UL2016':
    process.prefiringweight = l1PrefiringWeightProducer.clone(
    TheJets = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer"), #this should be the slimmedJets collection with up to date JECs !
    DataEraECAL = cms.string("UL2016postVFP"),
    DataEraMuon = cms.string("2016postVFP"),
    UseJetEMPt = cms.bool(False),
    PrefiringRateSystematicUnctyECAL = cms.double(0.2),
    PrefiringRateSystematicUnctyMuon = cms.double(0.2)
    )
elif period == 'UL2016APV':
    process.prefiringweight = l1PrefiringWeightProducer.clone(
    TheJets = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer"), #this should be the slimmedJets collection with up to date JECs !
    DataEraECAL = cms.string("UL2016preVFP"),
    DataEraMuon = cms.string("2016preVFP"),
    UseJetEMPt = cms.bool(False),
    PrefiringRateSystematicUnctyECAL = cms.double(0.2),
    PrefiringRateSystematicUnctyMuon = cms.double(0.2)
    )
# END Pre-firing weights ======================================================================================

# Trigger list ================================================================================================
# !!!!! WARNING : in 2018 all tau trigger names changed in the middle of the year -> please add also other names -> more information can be found here: https://twiki.cern.ch/twiki/bin/viewauth/CMS/TauTrigger#Trigger_table_for_2018 !!!!
HLTlist = cms.untracked.vstring(
#SingleMuon
'HLT_IsoMu20_v',
'HLT_IsoMu24_v',
'HLT_IsoMu27_v',
'HLT_IsoTkMu24_v',
'HLT_IsoMu22_v',
'HLT_IsoMu22_eta2p1_v',
'HLT_IsoTkMu22_v',
'HLT_IsoTkMu22_eta2p1_v',
# Muon-Tau triggers
'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_SingleL1_v',
'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v',
'HLT_IsoMu24_eta2p1_LooseChargedIsoPFTau20_SingleL1_v',
'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_v',
# SingleElectron
'HLT_Ele35_WPTight_Gsf_v',
'HLT_Ele25_eta2p1_WPTight_Gsf_v',
'HLT_Ele27_eta2p1_WPTight_Gsf_v',
'HLT_Ele27_WPTight_Gsf_v',
# Electron-Tau triggers
'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v',
# Dilepton triggers
'HLT_DoubleEle24_eta2p1_WPTight_Gsf_v',
'HLT_DoubleEle24_22_eta2p1_WPLoose_Gsf_v',
'HLT_DoubleIsoMu24_eta2p1_v',
'HLT_Mu17_Mu8_SameSign_DZ_v',
'HLT_Mu18_Mu9_SameSign_v',
'HLT_Mu18_Mu9_SameSign_DZ_v',
'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v',
'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_v',
# Triple muon
'HLT_TripleMu_12_10_5_v',
# Muon+Electron triggers
'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v',
'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v',
# Ditau triggers
'HLT_DoubleMediumIsoPFTau35_Trk1_eta2p1_Reg_v', #2016
'HLT_DoubleMediumCombinedIsoPFTau35_Trk1_eta2p1_Reg_v', #2016
'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v',
'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v',
'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v',
'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v', #2018                                                                           
'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v', #2018                                                                                    
'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v', #2018                                                                            
'HLT_DoubleTightChargedIsoPFTauHPS40_Trk1_eta2p1_Reg_v', #2018                                                                                 
'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v',  #2018 embed                                                                     
'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v', #2018 embed                                                                     
'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v', #2018 embed                                                                              
'HLT_DoubleMediumChargedIsoPFTauHPS35_Trk1_eta2p1_Reg_v', #2018 embed                                                                          
'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v', #2017 embed                                                                      
'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v', #2017 embed                                                                     
'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v', #2017 embed                                                                              
'HLT_DoubleMediumIsoPFTau35_Trk1_eta2p1_Reg_v', #2016 embed                 

# Single tau triggers
'HLT_MediumChargedIsoPFTau180HighPtRelaxedIso_Trk50_eta2p1_v',
'HLT_MediumChargedIsoPFTau180HighPtRelaxedIso_Trk50_eta2p1_1pr_v',
# MET Triggers
'HLT_PFMETNoMu110_PFMHTNoMu110_IDTight_v',
'HLT_PFMETNoMu120_PFMHTNoMu120_IDTight_v',
)

if isData:
    HLTlist += cms.untracked.vstring(
        # Single-Jet Triggers
        'HLT_PFJet60_v',
        'HLT_PFJet80_v',
        'HLT_PFJet140_v',
        'HLT_PFJet200_v',
        'HLT_PFJet260_v',
        'HLT_PFJet320_v',
        'HLT_PFJet400_v',
        'HLT_PFJet450_v',
        'HLT_PFJet500_v',
)

HLTlist_2016 = cms.untracked.vstring(
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v',
    'HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v',
)
HLTlist_2017 = cms.untracked.vstring(
    #SingleElectron
    'HLT_Ele32_WPTight_Gsf_v',
    'HLT_Ele35_WPTight_Gsf_v',
    'HLT_Ele32_WPTight_Gsf_L1DoubleEG_v',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v',
    )
HLTlist_2018 = cms.untracked.vstring(
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_CrossL1_v',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_TightID_CrossL1_v',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_CrossL1_v',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_TightID_CrossL1_v',
    'HLT_Ele32_WPTight_Gsf_v',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v',
    )

if year == 2016   : HLTlist += HLTlist_2016
elif year == 2017 : HLTlist += HLTlist_2017
elif year == 2018 : HLTlist += HLTlist_2018

print "\nTriggers that will be recorded:"
print HLTlist

# END Trigger list ============================================================================================

# Filter list =================================================================================================
muon_hlt_filters = cms.untracked.vstring(
    'HLT_IsoMu22_v.*:hltL3crIsoL1sMu20L1f0L2f10QL3f22QL3trkIsoFiltered0p09',
    'HLT_IsoMu22_eta2p1_v.*:hltL3crIsoL1sSingleMu20erL1f0L2f10QL3f22QL3trkIsoFiltered0p09',
    'HLT_IsoTkMu22_v.*:hltL3fL1sMu20L1f0Tkf22QL3trkIsoFiltered0p09',
    'HLT_IsoTkMu22_eta2p1_v.*:hltL3fL1sMu20erL1f0Tkf22QL3trkIsoFiltered0p09',
    'HLT_IsoMu20_v.*:hltL3crIsoL1sMu18L1f0L2f10QL3f20QL3trkIsoFiltered0p07,hltL3crIsoL1sMu18L1f0L2f10QL3f20QL3trkIsoFiltered0p09',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_SingleL1_v.*:hltL1sSingleMu18erIorSingleMu20er',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_SingleL1_v.*:hltL3crIsoL1sSingleMu18erIorSingleMu20erL1f0L2f10QL3f19QL3trkIsoFiltered0p09',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_SingleL1_v.*:hltOverlapFilterSingleIsoMu19LooseIsoPFTau20',
    'HLT_IsoMu27_v.*:hltL3crIsoL1sMu22Or25L1f0L2f10QL3f27QL3trkIsoFiltered0p07,hltL3crIsoL1sMu22Or25L1f0L2f10QL3f27QL3trkIsoFiltered0p09',
    'HLT_IsoMu24_v.*:hltL3crIsoL1sSingleMu22L1f0L2f10QL3f24QL3trkIsoFiltered0p07,hltL3crIsoL1sMu22L1f0L2f10QL3f24QL3trkIsoFiltered0p09',
    'HLT_IsoTkMu24_v.*:hltL3fL1sMu22L1f0Tkf24QL3trkIsoFiltered0p09',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltL1sMu18erTau24erIorMu20erTau24er,hltL1sBigORMu18erTauXXer2p1',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltL3crIsoBigORMu18erTauXXer2p1L1f0L2f10QL3f20QL3trkIsoFiltered0p07,hltL3crIsoL1sMu18erTau24erIorMu20erTau24erL1f0L2f10QL3f20QL3trkIsoFiltered0p07',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_CrossL1_v.*:hltL3crIsoBigORMu18erTauXXer2p1L1f0L2f10QL3f20QL3trkIsoFiltered0p07,hltL3crIsoL1sMu18erTau24erIorMu20erTau24erL1f0L2f10QL3f20QL3trkIsoFiltered0p07',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_CrossL1_v.*:hltHpsOverlapFilterIsoMu20LooseChargedIsoPFTau27L1Seeded',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_TightID_CrossL1_v.*:hltHpsOverlapFilterIsoMu20LooseChargedIsoTightOOSCPhotonsPFTau27L1Seeded',
    'HLT_IsoMu24_eta2p1_LooseChargedIsoPFTau20_SingleL1_v.*:hltL1sSingleMu22er',
    'HLT_IsoMu24_eta2p1_LooseChargedIsoPFTau20_SingleL1_v.*:hltL3crIsoL1sSingleMu22erL1f0L2f10QL3f24QL3trkIsoFiltered0p07',
    'HLT_IsoMu24_eta2p1_LooseChargedIsoPFTau20_SingleL1_v.*:hltOverlapFilterIsoMu24LooseChargedIsoPFTau20',
    'HLT_DoubleIsoMu24_eta2p1_v.*:hltL3crIsoL1sDoubleMu22erL1f0L2f10QL3f24QL3trkIsoFiltered0p07',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu23TrkIsoVVLEle12CaloIdLTrackIdLIsoVLMuonlegL3IsoFiltered23',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu23TrkIsoVVLEle12CaloIdLTrackIdLIsoVLDZFilter',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu8TrkIsoVVLEle23CaloIdLTrackIdLIsoVLDZFilter',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu8TrkIsoVVLEle23CaloIdLTrackIdLIsoVLMuonlegL3IsoFiltered8,hltL3fL1sMu7EG23f0Filtered8',# second filter only needed for 2017 and 2018
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v.*:hltL1sMu5EG20IorMu5IsoEG18,hltL1sMu5EG20IorMu5IsoEG18IorMu5IsoEG20IorMu5EG23',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v.*:hltMu8TrkIsoVVLEle23CaloIdLTrackIdLIsoVLMuonlegL3IsoFiltered8',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v.*:hltL1sMu20EG10,hltL1sMu20EG10IorMu23EG10',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v.*:hltMu23TrkIsoVVLEle12CaloIdLTrackIdLIsoVLMuonlegL3IsoFiltered23',
    'HLT_Mu17_Mu8_SameSign_DZ_v.*:hltL3pfL1sDoubleMu114ORDoubleMu125L1f0L2pf0L3PreFiltered8,hltL3pfL1sDoubleMu114L1f0L2pf0L3PreFiltered8',
    'HLT_Mu17_Mu8_SameSign_DZ_v.*:hltL3fL1sDoubleMu114L1f0L2f10OneMuL3Filtered17',
    'HLT_Mu17_Mu8_SameSign_DZ_v.*:hltDiMuonGlb17Glb8DzFiltered0p2',
    'HLT_Mu17_Mu8_SameSign_DZ_v.*:hltDiMuonGlb17Glb8DzFiltered0p2SameSign',
    'HLT_Mu18_Mu9_SameSign_DZ_v.*:hltL1sDoubleMu125to157',
    'HLT_Mu18_Mu9_SameSign_DZ_v.*:hltL3fL1DoubleMu157fFiltered9',
    'HLT_Mu18_Mu9_SameSign_DZ_v.*:hltL3fL1DoubleMu157fFiltered18',
    'HLT_Mu18_Mu9_SameSign_DZ_v.*:hltDiMuon189SameSignFiltered',
    'HLT_Mu18_Mu9_SameSign_DZ_v.*:hltDiMuon189SameSignDzFiltered0p2',
    'HLT_TripleMu_12_10_5_v.*:hltL1sTripleMu0IorTripleMu553',
    'HLT_TripleMu_12_10_5_v.*:hltL3fL1TripleMu553f0PreFiltered555,hltL1TripleMu553L2TriMuFiltered3L3TriMuFiltered5',
    'HLT_TripleMu_12_10_5_v.*:hltL3fL1TripleMu553f0Filtered10105,hltL1TripleMu553L2TriMuFiltered3L3TriMuFiltered10105',
    'HLT_TripleMu_12_10_5_v.*:hltL3fL1TripleMu553f0Filtered12105,hltL1TripleMu553L2TriMuFiltered3L3TriMuFiltered12105',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_v.*:hltL3crIsoL1sMu18erTauJet20erL1f0L2f10QL3f19QL3trkIsoFiltered0p09',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_v.*:hltOverlapFilterIsoMu19LooseIsoPFTau20',	
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_TightID_CrossL1_v.*:hltL3crIsoBigORMu18erTauXXer2p1L1f0L2f10QL3f20QL3trkIsoFiltered0p07,hltL3crIsoL1sMu18erTau24erIorMu20erTau24erL1f0L2f10QL3f20QL3trkIsoFiltered0p07',
    )
if year != 2016 and not isEmbedded:
    muon_hlt_filters += cms.untracked.vstring(
        'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltOverlapFilterIsoMu20LooseChargedIsoPFTau27L1Seeded',
    )
if year == 2016 :
    muon_hlt_filters += cms.untracked.vstring(
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltL1sMu20EG10IorMu23EG10',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltL1sMu5EG20IorMu5IsoEG18IorMu5IsoEG20IorMu5EG23',
    'HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v.*:hltDiMuonGlb17Trk8RelTrkIsoFiltered0p4DzFiltered0p2',
    'HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v.*:hltL3fL1sDoubleMu114L1f0L2f10L3Filtered17',
    'HLT_Mu17_TrkIsoVVL_TkMu8_TrkIsoVVL_DZ_v.*:hltDiMuonGlbFiltered17TrkFiltered8',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v.*:hltDiMuonGlb17Glb8RelTrkIsoFiltered0p4DzFiltered0p2',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v.*:hltL3fL1sDoubleMu114L1f0L2f10OneMuL3Filtered17',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_v.*:hltL3pfL1sDoubleMu114L1f0L2pf0L3PreFiltered8,hltL3pfL1sDoubleMu114ORDoubleMu125L1f0L2pf0L3PreFiltered8',
    )
if year == 2017 :
    muon_hlt_filters += cms.untracked.vstring(
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltDiMuon178Mass8Filtered',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltDiMuon178RelTrkIsoFiltered0p4DzFiltered0p2',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltL3fL1DoubleMu155fPreFiltered8',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltL3fL1DoubleMu155fFiltered17',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltL3fL1DoubleMu155fFiltered17',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltL3fL1DoubleMu155fPreFiltered8',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltDiMuon178RelTrkIsoFiltered0p4DzFiltered0p2',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltDiMuon178Mass3p8Filtered',
    )
if year == 2018 :
    muon_hlt_filters += cms.untracked.vstring(
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltDiMuon178Mass8Filtered',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltDiMuon178RelTrkIsoFiltered0p4DzFiltered0p2',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltL3fL1DoubleMu155fPreFiltered8',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass8_v.*:hltL3fL1DoubleMu155fFiltered17',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltL3fL1DoubleMu155fFiltered17',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltL3fL1DoubleMu155fPreFiltered8',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltDiMuon178RelTrkIsoFiltered0p4DzFiltered0p2',
    'HLT_Mu17_TrkIsoVVL_Mu8_TrkIsoVVL_DZ_Mass3p8_v.*:hltDiMuon178Mass3p8Filtered',
    )
if year == 2018 and isEmbedded:
    muon_hlt_filters += cms.untracked.vstring(
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltL1sBigORMu18erTauXXer2p1',
    )

electron_hlt_filters = cms.untracked.vstring(
    'HLT_Ele27_eta2p1_WPTight_Gsf_v.*:hltEle27erWPTightGsfTrackIsoFilter',
    'HLT_Ele27_WPTight_Gsf_v.*:hltEle27WPTightGsfTrackIsoFilter',
    'HLT_Ele32_WPTight_Gsf_v.*:hltEle32WPTightGsfTrackIsoFilter',
    'HLT_Ele32_WPTight_Gsf_DoubleL1EG_v.*:hltEle32L1DoubleEGWPTightGsfTrackIsoFilter',
    'HLT_Ele32_WPTight_Gsf_DoubleL1EG_v.*:hltEGL1SingleEGOrFilter',
    'HLT_Ele32_WPTight_Gsf_L1DoubleEG_v.*:hltEle32L1DoubleEGWPTightGsfTrackIsoFilter',
    'HLT_Ele35_WPTight_Gsf_v.*:hltEle35noerWPTightGsfTrackIsoFilter',
    'HLT_Ele25_eta2p1_WPTight_Gsf_v.*:hltEle25erWPTightGsfTrackIsoFilter',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltEle24erWPTightGsfTrackIsoFilterForTau',
    'HLT_DoubleEle24_eta2p1_WPTight_Gsf_v.*:hltDoubleEle24erWPTightGsfTrackIsoFilterForTau',
    'HLT_DoubleEle24_22_eta2p1_WPLoose_Gsf_v.*:hltEle24Ele22WPLooseGsfleg1TrackIsoFilter',
    'HLT_DoubleEle24_22_eta2p1_WPLoose_Gsf_v.*:hltEle24Ele22WPLooseGsfleg2TrackIsoFilter',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu23TrkIsoVVLEle12CaloIdLTrackIdLIsoVLElectronlegTrackIsoFilter',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu23TrkIsoVVLEle12CaloIdLTrackIdLIsoVLDZFilter',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v.*:hltL1sMu20EG10,hltL1sMu20EG10IorMu23EG10',
    'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_v.*:hltMu23TrkIsoVVLEle12CaloIdLTrackIdLIsoVLElectronlegTrackIsoFilter',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu8TrkIsoVVLEle23CaloIdLTrackIdLIsoVLElectronlegTrackIsoFilter',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltMu8TrkIsoVVLEle23CaloIdLTrackIdLIsoVLDZFilter',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v.*:hltL1sMu5EG20IorMu5IsoEG18,hltL1sMu5EG20IorMu5IsoEG18IorMu5IsoEG20IorMu5EG23',
    'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_v.*:hltMu8TrkIsoVVLEle23CaloIdLTrackIdLIsoVLElectronlegTrackIsoFilter',
    'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg1Filter',
    'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg2Filter',
    'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltEle23Ele12CaloIdLTrackIdLIsoVLDZFilter',
    'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_v.*:hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg1Filter',
    'HLT_Ele23_Ele12_CaloIdL_TrackIdL_IsoVL_v.*:hltEle23Ele12CaloIdLTrackIdLIsoVLTrackIsoLeg2Filter',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_CrossL1_v.*:hltHpsSelectedPFTau30LooseChargedIsolationL1HLTMatched',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_CrossL1_v.*:hltHpsOverlapFilterIsoEle24WPTightGsfLooseIsoPFTau30',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_TightID_CrossL1_v.*:hltHpsSelectedPFTau30LooseChargedIsolationTightOOSCPhotonsL1HLTMatched',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_TightID_CrossL1_v.*:hltHpsOverlapFilterIsoEle24WPTightGsfLooseIsoTightOOSCPhotonsPFTau30',
    )
if not isEmbedded:
    electron_hlt_filters +=cms.untracked.vstring(
        'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltL1sBigORLooseIsoEGXXerIsoTauYYerdRMin0p3,hltL1sIsoEG22erIsoTau26erdEtaMin0p2',
        'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltOverlapFilterIsoEle24WPTightGsfLooseIsoPFTau30',
    )
else:
    electron_hlt_filters +=cms.untracked.vstring(
        'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltL1sBigORLooseIsoEGXXerIsoTauYYerdRMin0p3',
    )
if year == 2016 :
    electron_hlt_filters +=cms.untracked.vstring(
        'HLT_Mu23_TrkIsoVVL_Ele12_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltL1sMu20EG10IorMu23EG10',
        'HLT_Mu8_TrkIsoVVL_Ele23_CaloIdL_TrackIdL_IsoVL_DZ_v.*:hltL1sMu5EG20IorMu5IsoEG18IorMu5IsoEG20IorMu5EG23',
    )
if year == 2017:
    electron_hlt_filters +=cms.untracked.vstring(
        'HLT_Ele35_WPTight_Gsf_v.*:hltEGL1SingleEGOrFilter',
    )


tau_hlt_filters = cms.untracked.vstring(
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_SingleL1_v.*:hltPFTau20TrackLooseIsoAgainstMuon',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_SingleL1_v.*:hltOverlapFilterSingleIsoMu19LooseIsoPFTau20',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltL1sMu18erTau24erIorMu20erTau24er,hltL1sBigORMu18erTauXXer2p1',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltPFTau27TrackLooseChargedIsoAgainstMuon',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltOverlapFilterIsoMu20LooseChargedIsoPFTau27L1Seeded',
    'HLT_IsoMu24_eta2p1_LooseChargedIsoPFTau20_SingleL1_v.*:hltPFTau20TrackLooseChargedIsoAgainstMuon',
    'HLT_IsoMu24_eta2p1_LooseChargedIsoPFTau20_SingleL1_v.*:hltOverlapFilterIsoMu24LooseChargedIsoPFTau20',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltPFTau30TrackLooseChargedIso,hltSelectedPFTau30LooseChargedIsolationL1HLTMatched',
    'HLT_DoubleMediumIsoPFTau35_Trk1_eta2p1_Reg_v.*:hltDoublePFTau35TrackPt1MediumIsolationDz02Reg',
    'HLT_DoubleMediumCombinedIsoPFTau35_Trk1_eta2p1_Reg_v.*:hltDoublePFTau35TrackPt1MediumCombinedIsolationDz02Reg',
    'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v.*:hltDoublePFTau35TrackPt1TightChargedIsolationAndTightOOSCPhotonsDz02Reg',
    'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v.*:hltDoublePFTau40TrackPt1MediumChargedIsolationAndTightOOSCPhotonsDz02Reg',
    'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v.*:hltDoublePFTau40TrackPt1TightChargedIsolationDz02Reg',
    'HLT_MediumChargedIsoPFTau180HighPtRelaxedIso_Trk50_eta2p1_v.*:hltPFTau180TrackPt50LooseAbsOrRelMediumHighPtRelaxedIsoIso',
    'HLT_MediumChargedIsoPFTau180HighPtRelaxedIso_Trk50_eta2p1_v.*:hltSelectedPFTau180MediumChargedIsolationL1HLTMatched',
    'HLT_MediumChargedIsoPFTau180HighPtRelaxedIso_Trk50_eta2p1_1pr_v.*:hltPFTau180TrackPt50LooseAbsOrRelMediumHighPtRelaxedIso1Prong,hltSelectedPFTau180MediumChargedIsolationL1HLTMatched1Prong',    
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_v.*:hltOverlapFilterIsoMu19LooseIsoPFTau20',
    'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_v.*:hltPFTau20TrackLooseIsoAgainstMuon',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_CrossL1_v.*:hltHpsSelectedPFTau27LooseChargedIsolationAgainstMuonL1HLTMatched',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_CrossL1_v.*:hltHpsOverlapFilterIsoMu20LooseChargedIsoPFTau27L1Seeded',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_TightID_CrossL1_v.*:hltHpsOverlapFilterIsoMu20LooseChargedIsoTightOOSCPhotonsPFTau27L1Seeded',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_TightID_CrossL1_v.*:hltHpsSelectedPFTau27LooseChargedIsolationTightOOSCPhotonsAgainstMuonL1HLTMatched',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_CrossL1_v.*:hltEle24erWPTightGsfTrackIsoFilterForTau',
    'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTauHPS30_eta2p1_TightID_CrossL1_v.*:hltEle24erWPTightGsfTrackIsoFilterForTau',
    'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v.*:hltDoublePFTau40TrackPt1MediumChargedIsolationAndTightOOSCPhotonsDz02Reg',#2018                                                                                                                                         
    'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v.*:hltDoublePFTau40TrackPt1TightChargedIsolationDz02Reg', #2018                    
    'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v.*:hltDoublePFTau35TrackPt1TightChargedIsolationAndTightOOSCPhotonsDz02Reg',#2018                                                                                                                                           
    'HLT_DoubleTightChargedIsoPFTauHPS40_Trk1_eta2p1_Reg_v.*:hltHpsDoublePFTau40TrackPt1TightChargedIsolationDz02Reg', #2018                   
    'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2018 embed                                      
    'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2018 embed                                     
    'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2018 embed                                              
    'HLT_DoubleMediumChargedIsoPFTauHPS35_Trk1_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2018 embed                                          
    'HLT_DoubleTightChargedIsoPFTau35_Trk1_TightID_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2017 embed                                      
    'HLT_DoubleMediumChargedIsoPFTau40_Trk1_TightID_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2017 embed                                     
    'HLT_DoubleTightChargedIsoPFTau40_Trk1_eta2p1_Reg_v.*:hltDoubleL2IsoTau26eta2p2', #2017 embed                                              
    'HLT_DoubleMediumIsoPFTau35_Trk1_eta2p1_Reg_v.*:hltDoublePFTau35Reg', #2016 embed           
    )
if isEmbedded:
    tau_hlt_filters +=cms.untracked.vstring(
        'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltL1sBigORLooseIsoEGXXerIsoTauYYerdRMin0p3',
    )
else:
    tau_hlt_filters +=cms.untracked.vstring(
        'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltL1sBigORLooseIsoEGXXerIsoTauYYerdRMin0p3,hltL1sIsoEG22erIsoTau26erdEtaMin0p2',
        'HLT_Ele24_eta2p1_WPTight_Gsf_LooseChargedIsoPFTau30_eta2p1_CrossL1_v.*:hltOverlapFilterIsoEle24WPTightGsfLooseIsoPFTau30',
    )
if isEmbedded and year == 2016:
    tau_hlt_filters +=cms.untracked.vstring(
	'HLT_IsoMu19_eta2p1_LooseIsoPFTau20_v.*:hltL1sMu18erTau20er',
    )
if isData and year == 2018:
    tau_hlt_filters +=cms.untracked.vstring(
        'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltSelectedPFTau27LooseChargedIsolationAgainstMuonL1HLTMatched',
    )
if year == 2017:
    tau_hlt_filters +=cms.untracked.vstring(
        'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltSelectedPFTau27LooseChargedIsolationAgainstMuonL1HLTMatched',
    )
if isEmbedded and year == 2018:
    tau_hlt_filters +=cms.untracked.vstring(
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTau27_eta2p1_CrossL1_v.*:hltL1sBigORMu18erTauXXer2p1',
    'HLT_IsoMu20_eta2p1_LooseChargedIsoPFTauHPS27_eta2p1_CrossL1_v.*:hltL1sBigORMu18erTauXXer2p1',

    )
jet_hlt_filters = cms.untracked.vstring(
    'HLT_PFJet60_v.*:hltSinglePFJet60',
    'HLT_PFJet80_v.*:hltSinglePFJet80',
    'HLT_PFJet140_v.*:hltSinglePFJet140',
    'HLT_PFJet200_v.*:hltSinglePFJet200',
    'HLT_PFJet260_v.*:hltSinglePFJet260',
    'HLT_PFJet320_v.*:hltSinglePFJet320',
    'HLT_PFJet400_v.*:hltSinglePFJet400',
    'HLT_PFJet450_v.*:hltSinglePFJet450',
    'HLT_PFJet500_v.*:hltSinglePFJet500',
    )
# END Filter list =============================================================================================

# NTuple Maker =========================================================================================
storeGenParticles = False
if isEmbedded or not isData :
    storeGenParticles = True
if isEmbedded :
    triggerLabel = "SIMembedding"
    minrecmuonpt = 5.
else :
    triggerLabel = "HLT"
    minrecmuonpt = 2.

process.initroottree = cms.EDAnalyzer("InitAnalyzer",
IsData = cms.untracked.bool(isData),
#IsData = cms.untracked.bool(False),
GenParticles = cms.untracked.bool(storeGenParticles),
GenJets = cms.untracked.bool(not isData)
)
process.makeroottree = cms.EDAnalyzer("NTupleMaker",
# data, year, period, skim
IsData = cms.untracked.bool(isData),
IsEmbedded = cms.untracked.bool(isEmbedded),
Year = cms.untracked.uint32(year),
Period = cms.untracked.string(period),
Skim = cms.untracked.uint32(0),
# switches of collections
GenParticles = cms.untracked.bool(storeGenParticles),
GenJets = cms.untracked.bool(not isData),
SusyInfo = cms.untracked.bool(not isData),
Trigger = cms.untracked.bool(True),
RecPrimVertex = cms.untracked.bool(True),
RecPrimVertexWithBS = cms.untracked.bool(True),
RefittedVertex = cms.untracked.bool(True),
RefittedVertexWithBS = cms.untracked.bool(True),
ApplyTauSpinner = cms.untracked.bool(RunTauSpinnerProducer),
RecBeamSpot = cms.untracked.bool(True),
RecTrack = cms.untracked.bool(not isData or isSingleMuonData),
RecPFMet = cms.untracked.bool(False),
RecPFMetCorr = cms.untracked.bool(True),
RecPuppiMet = cms.untracked.bool(True),
RecMvaMet = cms.untracked.bool(False),
RecMuon = cms.untracked.bool(True),
RecPhoton = cms.untracked.bool(False),
RecElectron = cms.untracked.bool(True),
RecTau = cms.untracked.bool(True),
L1Objects = cms.untracked.bool(True),
RecJet = cms.untracked.bool(True),
RecJetPuppi= cms.untracked.bool(True),
RecHTXS = cms.untracked.bool(isHiggsSignal),
# collections
MuonCollectionTag = cms.InputTag("slimmedMuonsWithMVA"), #added prompt muon MVA
ElectronCollectionTag = cms.InputTag("slimmedElectronsWithMVA"), #added prompt electron MVA
applyElectronESShift = cms.untracked.bool(not isData or isEmbedded),
TauCollectionTag = cms.InputTag("NewTauIDsEmbedded"),
L1MuonCollectionTag = cms.InputTag("gmtStage2Digis:Muon"),
L1EGammaCollectionTag = cms.InputTag("caloStage2Digis:EGamma"),
L1TauCollectionTag = cms.InputTag("caloStage2Digis:Tau"),
L1JetCollectionTag = cms.InputTag("caloStage2Digis:Jet"),
#JetCollectionTag = cms.InputTag("slimmedJets"),
JetCollectionTag = cms.InputTag("updatedPatJetsUpdatedJEC::TreeProducer"),
PuppiJetCollectionTag = cms.InputTag("updatedPatJetsUpdatedJECPuppi::TreeProducer"),
MetCollectionTag = cms.InputTag("slimmedMETs::@skipCurrentProcess"),
MetCorrCollectionTag = cms.InputTag("slimmedMETs::TreeProducer"),
PuppiMetCollectionTag = cms.InputTag("slimmedMETsPuppi::TreeProducer"),
MvaMetCollectionsTag = cms.VInputTag(cms.InputTag("MVAMET","MVAMET","TreeProducer")),
TrackCollectionTag = cms.InputTag("generalTracks"),
GenParticleCollectionTag = cms.InputTag("prunedGenParticles"),
GenJetCollectionTag = cms.InputTag("slimmedGenJets"),
TriggerObjectCollectionTag = cms.InputTag("slimmedPatTrigger"),
BeamSpotCollectionTag =  cms.InputTag("offlineBeamSpot"),
PVCollectionTag = cms.InputTag("offlineSlimmedPrimaryVertices"),
PVwithBSCollectionTag =  cms.InputTag("MiniAODRefitVertexBSProducer"),
RefittedPVCollectionTag =  cms.InputTag("AdvancedRefitVertexNoBSProducer"),
RefittedwithBSPVCollectionTag =  cms.InputTag("AdvancedRefitVertexBSProducer"),
LHEEventProductTag = cms.InputTag("externalLHEProducer"),
SusyMotherMassTag = cms.InputTag("susyInfo","SusyMotherMass"),
SusyLSPMassTag = cms.InputTag("susyInfo","SusyLSPMass"),
htxsInfo = cms.InputTag("rivetProducerHTXS", "HiggsClassification"),

# TRIGGER INFO  =========================================================================================
HLTriggerPaths = HLTlist,
TriggerProcess = cms.untracked.string(triggerLabel),
Flags = cms.untracked.vstring(
'Flag_goodVertices',
'Flag_globalSuperTightHalo2016Filter',
'Flag_HBHENoiseFilter',
'Flag_HBHENoiseIsoFilter',
'Flag_EcalDeadCellTriggerPrimitiveFilter',
'Flag_BadPFMuonFilter',
'Flag_BadPFMuonDzFilter',
'Flag_hfNoisyHitsFilter',
'Flag_BadChargedCandidateFilter',
'Flag_eeBadScFilter',
'Flag_ecalBadCalibFilter',
'Flag_METFilters',
'allMetFilterPaths',
),
FlagsProcesses = cms.untracked.vstring("RECO","PAT"),
BadChargedCandidateFilter =  cms.InputTag("BadChargedCandidateFilter"),
BadPFMuonFilter = cms.InputTag("BadPFMuonFilter"),
BadGlobalMuons    = cms.InputTag("badGlobalMuonTagger","bad","TreeProducer"),
BadDuplicateMuons = cms.InputTag("cloneGlobalMuonTagger","bad","TreeProducer"),
# tracks
RecTrackPtMin = cms.untracked.double(1.0),
RecTrackEtaMax = cms.untracked.double(2.4),
RecTrackDxyMax = cms.untracked.double(1.0),
RecTrackDzMax = cms.untracked.double(1.0),
RecTrackNum = cms.untracked.int32(0),
# muons
RecMuonPtMin = cms.untracked.double(minrecmuonpt),
RecMuonEtaMax = cms.untracked.double(2.5),
RecMuonHLTriggerMatching = muon_hlt_filters,
RecMuonNum = cms.untracked.int32(0),
# photons
RecPhotonPtMin = cms.untracked.double(20.),
RecPhotonEtaMax = cms.untracked.double(10000.),
RecPhotonHLTriggerMatching = cms.untracked.vstring(),
RecPhotonNum = cms.untracked.int32(0),
# electrons
RecElectronPtMin = cms.untracked.double(8.),
RecElectronEtaMax = cms.untracked.double(2.6),
RecElectronHLTriggerMatching = electron_hlt_filters,
RecElectronNum = cms.untracked.int32(0),
# taus
RecTauPtMin = cms.untracked.double(15),
RecTauEtaMax = cms.untracked.double(2.5),
RecTauHLTriggerMatching = tau_hlt_filters,
RecTauFloatDiscriminators = cms.untracked.vstring(),
RecTauBinaryDiscriminators = cms.untracked.vstring(),
RecTauNum = cms.untracked.int32(0),
# jets
RecJetPtMin = cms.untracked.double(18.),
RecJetEtaMax = cms.untracked.double(5.2),
RecJetHLTriggerMatching = jet_hlt_filters,
#from https://twiki.cern.ch/twiki/bin/view/CMS/DeepJet
RecJetBtagDiscriminators = cms.untracked.vstring(
'pfCombinedInclusiveSecondaryVertexV2BJetTags',
'pfDeepCSVJetTags:probb',
'pfDeepCSVJetTags:probbb',
'pfDeepFlavourJetTags:probb',
'pfDeepFlavourJetTags:probbb',
'pfDeepFlavourJetTags:problepb',
'pfDeepFlavourJetTags:probc',
'pfDeepFlavourJetTags:probuds',
'pfDeepFlavourJetTags:probg'
),
RecJetNum = cms.untracked.int32(0),
SampleName = cms.untracked.string("Data"),
#TauSpinnerinput = cms.InputTag("prunedGenParticles"),
TauSpinnerAngles = cms.string("0,0.25,0.5,-0.25,0.375")
)
# END NTuple Maker ======================================================================================


# Trigger filtering =====================================================================================
# From : https://github.com/cms-sw/cmssw/blob/CMSSW_10_2_X/HLTrigger/HLTfilters/plugins/HLTHighLevel.cc
# See also here: https://twiki.cern.ch/twiki/bin/view/CMS/TriggerResultsFilter

HLTlist_for_filtering = [i+"*" for i in HLTlist]

process.triggerSelection = cms.EDFilter("HLTHighLevel",
                                        TriggerResultsTag = cms.InputTag("TriggerResults","","HLT"),
                                        HLTPaths = cms.vstring(HLTlist_for_filtering),
                                        andOr = cms.bool(True),   # multiple triggers: True (OR) accept if ANY is true, False (AND) accept if ALL are true
                                        throw = cms.bool(False)   # throw exception on unknown path names
                                        )
# END Trigger filtering =================================================================================

#DEPRECATED old TauSpinner implementation, currently kept for possible cross checks
#define tauspinner producer. Here we define the mixing angles
#process.icTauSpinnerProducer = cms.EDProducer("GetTauSpinnerweights",
#  branch                  = cms.string("tauspinner"),
#  input                   = cms.InputTag("prunedGenParticles"),
#  theta                   = cms.string("0,0.25,0.5,-0.25,0.375")#if specify more than 5 angles, FIRST addapt NrAnglestoStore in ICTauSpinnerProducer and recompile!
#)
#process.icTauSpinnerSequence = cms.Sequence(process.icTauSpinnerProducer)

process.p = cms.Path(
  process.initroottree *
  process.triggerSelection * # trigger filtering
  process.jecSequence *  # New JECs
  process.jecSequencepuppi *  # New JECs
  process.fullPatMetSequence *  # Re-correcting PFMET
  process.egmPhotonIDSequence * # Puppi MET
  process.puppiMETSequence *  # Puppi MET
  #process.fullPatMetSequencePuppi *  # Re-correcting Puppi MET
  #process.fullPatMetSequence *  # Re-correcting PFMET
  process.fullPatMetSequencePuppi *  # Re-correcting Puppi MET
  process.egammaPostRecoSeq *               # electron energy corrections and Ids
  process.rerunMvaIsolationSequence *  # Tau IDs
  getattr(process,updatedTauName) *  # Tau IDs
  process.AdvancedRefitVertexNoBSBSSequence * # Vertex refit w/o BS
  process.AdvancedRefitVertexBSSequence * # Vertex refit w/ BS
  process.MiniAODRefitVertexBS * # PV with BS constraint
  process.htxsSequence * # HTXS
  process.prefiringweight * # prefiring-weights
  process.promptleptonMVA_sequence *
  process.makeroottree
)

#if RunTauSpinnerProducer: process.p *=process.icTauSpinnerSequence

process.TFileService = cms.Service("TFileService",
                                   fileName = cms.string(options.tupleOutput)
                                 )

process.output = cms.OutputModule("PoolOutputModule",
                                  fileName = cms.untracked.string(options.tupleOutput),
                                  outputCommands = cms.untracked.vstring(
                                    'keep *_*_bad_TreeProducer'#,
                                    #'drop patJets*_*_*_*'
                                    #'keep *_slimmedMuons_*_*',
                                    #'drop *_selectedPatJetsForMetT1T2Corr_*_*',
                                    #'drop patJets_*_*_*'
                                  ),
                                  SelectEvents = cms.untracked.PSet( SelectEvents = cms.vstring('p'))
)
